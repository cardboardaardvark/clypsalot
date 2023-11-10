/* Copyright 2023 Tyler Riddle
 *
 * This file is part of Clypsalot. Clypsalot is free software: you can redistribute it and/or modify it under the terms
 * of the GNU Lesser General Public License as published by the Free Software Foundation, either version 3 of the License,
 * or (at your option) any later version. Clypsalot is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU Lesser
 * General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License along with Clypsalot. If not, see
 * <https://www.gnu.org/licenses/>.
 */

#include <cassert>

#include <clypsalot/error.hxx>
#include <clypsalot/logger.hxx>
#include <clypsalot/macros.hxx>
#include <clypsalot/thread.hxx>

/// @file
namespace Clypsalot
{
    static ThreadQueue* threadQueueSingleton = nullptr;
    static Mutex threadQueueSingletonMutex;
    thread_local bool ThreadQueue::m_insideQueueFlag = false;

    DebugMutex::DebugMutex() :
        m_recurseOk(false)
    { }

    DebugMutex::DebugMutex(const bool recurseOk) :
        m_recurseOk(recurseOk)
    { }

    DebugMutex::~DebugMutex()
    {
        if (_locked()) FATAL_ERROR("Mutex was locked when it was destroyed");
    }

    bool DebugMutex::_locked() const
    {
        return std::thread::id() != m_lockedBy;
    }

    /**
     * @brief Identify if the mutex is locked by any thread.
     * @return true if the mutex is locked otherwise false.
     */
    bool DebugMutex::locked() const
    {
        std::scoped_lock lock(m_metaMutex);
        return _locked();
    }

    bool DebugMutex::_haveLock() const
    {
        return std::this_thread::get_id() == m_lockedBy;
    }

    /**
     * @brief Identify if the calling thread has the mutex locked.
     * @return True if the mutex is locked by the calling thread otherwise false.
     */
    bool DebugMutex::haveLock() const
    {
        std::scoped_lock lock(m_metaMutex);
        return _haveLock();
    }

    size_t DebugMutex::lockCount() const
    {
        std::scoped_lock lock(m_metaMutex);
        return m_lockCounter;
    }

    /**
     * @brief Lock the mutex.
     *
     * If a deadlock condition or other problem is detected this method will print out
     * diagnostic information to the console then call abort().
     */
    void DebugMutex::lock()
    {
        std::unique_lock lock(m_metaMutex);

        if (_haveLock() && ! m_recurseOk) throw MutexLockError("Recursive lock attempt on mutex");

        lock.unlock();
        m_guardedMutex.lock();
        lock.lock();

        if (! m_recurseOk) assert(! _locked());

        m_lockedBy = std::this_thread::get_id();

        if (m_recurseOk) m_lockCounter++;
    }

    /**
     * @brief Unlock the mutex.
     *
     * If an attempt is made to unlock the mutex by a thread that does not have the mutex
     * locked this method will print out diagnostic information to the console then call abort().
     */
    void DebugMutex::unlock()
    {
        std::scoped_lock lock(m_metaMutex);

        if (! _haveLock()) throw MutexUnlockError("Mutex was not locked by the calling thread");

        assert(_locked());
        m_guardedMutex.unlock();

        if (m_recurseOk)
        {
            if (--m_lockCounter == 0)
            {
                m_lockedBy = std::thread::id();
            }
        }
        else
        {
            m_lockedBy = std::thread::id();
        }
    }

    /**
     * @brief Attempt to lock the mutex with out blocking.
     *
     * If a consistency problem is detected with the mutex lock state this method will print
     * out diagnostic information to the console then call abort().
     *
     * @return True if the mutex is now locked or false if the lock could not be aquired.
     */
    bool DebugMutex::tryLock()
    {
        std::scoped_lock lock(m_metaMutex);

        if (_haveLock()) throw MutexLockError("tryLock() would result in recursive locking of mutex");

        auto lockResult = m_guardedMutex.try_lock();

        if (! lockResult)
        {
            return false;
        }

        assert(! _locked());
        m_lockedBy = std::this_thread::get_id();
        return lockResult;
    }

    Lockable::Lockable(const bool recurseOk) :
        m_mutex(recurseOk)
    { }

#ifndef NDEBUG
    bool Lockable::haveLock() const
    {
        return m_mutex.haveLock();
    }
#endif

    /**
     * @brief Compatibility with the Lockable named requirement
     * @return The return value from \ref DebugMutex::tryLock() "tryLock()".
     */
    bool DebugMutex::try_lock()
    {
        return tryLock();
    }

    void Lockable::lock() const
    {
        m_mutex.lock();
    }

    void Lockable::unlock() const
    {
        m_mutex.unlock();
    }

    bool Lockable::tryLock() const
    {
        return m_mutex.try_lock();
    }

    bool Lockable::try_lock() const
    {
        return tryLock();
    }

    RecursiveDebugMutex::RecursiveDebugMutex() :
        DebugMutex(true)
    { }

    RecursiveLockable::RecursiveLockable() :
        Lockable(true)
    { }

    bool SharedDebugMutex::_locked() const
    {
        return std::thread::id() != m_lockedBy;
    }

    /// @brief Returns true if any thread holds an exclusive lock on the mutex
    bool SharedDebugMutex::locked() const
    {
        std::scoped_lock lock(m_metaMutex);
        return _locked();
    }

    bool SharedDebugMutex::_haveLock() const
    {
        return std::this_thread::get_id() == m_lockedBy;
    }

    /// @brief Returns true if the calling thread holds an exclusive lock
    bool SharedDebugMutex::haveLock() const
    {
        std::scoped_lock lock(m_metaMutex);
        return _haveLock();
    }

    /// @brief Obtain an exclusive lock on the shared mutex.
    void SharedDebugMutex::lock()
    {
        std::unique_lock lock(m_metaMutex);

        if (_haveLock())
        {
            FATAL_ERROR("Recursive exclusive lock attempt on mutex");
        }

        if (_haveSharedLock())
        {
            FATAL_ERROR("Recursive exclusive lock of mutex by thread that has a shared lock");
        }

        lock.unlock();
        m_guardedMutex.lock();
        lock.lock();

        assert(! _locked());
        assert(m_sharedBy.size() == 0);
        m_lockedBy = std::this_thread::get_id();
    }

    /**
     * @brief Try to obtain an exclusive lock with out blocking.
     * @return True if an exclusive lock could be obtained or false otherwise.
     */
    bool SharedDebugMutex::tryLock()
    {
        std::scoped_lock lock(m_metaMutex);

        if (_haveLock())
        {
            FATAL_ERROR("Recursive lock attempt of mutex");
        }

        if (_haveSharedLock())
        {
            FATAL_ERROR("Recursive lock attempt of mutex by thread that has a shared lock");
        }

        if (! m_guardedMutex.try_lock())
        {
            return false;
        }

        assert(! _locked());
        assert(! _sharedLocked());
        m_lockedBy = std::this_thread::get_id();

        return true;
    }

    /// @brief Compatibility function for the C++ SharedLocakble named requirement.
    bool SharedDebugMutex::try_lock()
    {
        return tryLock();
    }

    /// @brief Release an exclusive lock held on the shared mutex.
    void SharedDebugMutex::unlock()
    {
        std::scoped_lock lock(m_metaMutex);

        if (std::this_thread::get_id() != m_lockedBy)
        {
            FATAL_ERROR("Attempt to unlock mutex by thread that does not hold the lock");
        }

        m_guardedMutex.unlock();
        m_lockedBy = std::thread::id();
    }

    bool SharedDebugMutex::_sharedLocked() const
    {
        return m_sharedBy.size() != 0;
    }

    /// @brief Returns true if any thread holds a shared lock on the mutex.
    bool SharedDebugMutex::sharedLocked() const
    {
        std::scoped_lock lock(m_metaMutex);
        return _sharedLocked();
    }

    bool SharedDebugMutex::_haveSharedLock() const
    {
        const auto currentThread = std::this_thread::get_id();

        if (currentThread == m_lockedBy)
        {
            return true;
        }

        if (m_sharedBy.contains(currentThread))
        {
            return true;
        }

        return false;
    }

    /// @brief Returns true if the calling thread holds either a shared or exclusive lock
    bool SharedDebugMutex::haveSharedLock() const
    {
        std::scoped_lock lock(m_metaMutex);
        return _haveSharedLock();
    }

    /// @brief Obtain a shared lock on the mutex.
    void SharedDebugMutex::lockShared()
    {
        std::unique_lock lock(m_metaMutex);

        if (_haveLock())
        {
            FATAL_ERROR("Recursive shared lock attempt by thread that holds an exclusive lock");
        }

        if (_haveSharedLock())
        {
            FATAL_ERROR("Recursive shared lock attempt");
        }

        lock.unlock();
        m_guardedMutex.lock_shared();
        lock.lock();

        assert(! _locked());
        m_sharedBy[std::this_thread::get_id()] = true;
    }

    /// @brief Compatibility method for the C++ SharedLockable named requirement.
    void SharedDebugMutex::lock_shared()
    {
        lockShared();
    }

    /**
     * @brief Attempt to obtain a shared lock on the mutex with out blocking.
     * @returns True if the shared lock could be obtained otherwise false.
     */
    bool SharedDebugMutex::tryLockShared()
    {
        std::scoped_lock lock(m_metaMutex);

        if (_haveLock())
        {
            FATAL_ERROR("Recursive shared lock attempt by thread that has an exclusive lock");
        }

        if (_haveSharedLock())
        {
            FATAL_ERROR("Recursive shared lock attempt");
        }

        if (! m_guardedMutex.try_lock_shared())
        {
            return false;
        }

        assert(! _locked());
        m_sharedBy[std::this_thread::get_id()] = true;
        return true;
    }

    /// @brief Compatibility method for use with the C++ SharedLockable named requirement.
    bool SharedDebugMutex::try_lock_shared()
    {
        return tryLockShared();
    }

     /// @brief Release a shared lock on the mutex.
    void SharedDebugMutex::unlockShared()
    {
        std::scoped_lock lock(m_metaMutex);

        if (_haveLock())
        {
            FATAL_ERROR("Attempt to shared unlock a mutex by a thread that holds an exclusive lock");
        }

        if (! _haveSharedLock())
        {
            FATAL_ERROR("Attempt to shared unlock a mutex by a thread that does not have a shared lock");
        }

        assert(! _locked());
        m_guardedMutex.unlock_shared();
        m_sharedBy.erase(std::this_thread::get_id());
    }

    /// @brief Compatibility method for use with the C++ SharedLockable named requirement.
    void SharedDebugMutex::unlock_shared()
    {
        unlockShared();
    }

    ThreadQueue::ThreadQueue(size_t initThreads)
    {
        if (initThreads == 0)
        {
            initThreads = std::thread::hardware_concurrency();
        }

        if (initThreads == 0)
        {
            LOGGER(warn, "Could not detect hardware concurrency; setting number of threads to 1");
            initThreads = 1;
        }

        threads(initThreads);
    }

    ThreadQueue::~ThreadQueue()
    {
        threads(0);
    }

    void ThreadQueue::worker()
    {
        std::unique_lock lock(m_mutex);

        LOGGER(debug, "A new worker thread is born");

        ThreadQueue::m_insideQueueFlag = true;

        while(true)
        {
            LOGGER(trace, "Worker thread is starting a loop iteration");
            m_workerCondVar.wait(lock, [&]
            {
                if (m_jobs.size() > 0)
                {
                    LOGGER(trace, "Worker thread has a job to do; jobs=", m_jobs.size());
                    return true;
                }

                if (m_workers.size() - m_joinQueue.size() > m_numThreads)
                {
                    LOGGER(trace, "Worker thread needs to die");
                    return true;
                }

                LOGGER(trace, "Worker thread has nothing to do");
                return false;
            });

            if (m_workers.size() - m_joinQueue.size() > m_numThreads)
            {
                LOGGER(debug, "Thread is quiting to reduce the number of workers");
                m_joinQueue.push_back(std::this_thread::get_id());
                m_condVar.notify_all();
                return;
            }

            if (m_jobs.size() > 0)
            {
                LOGGER(trace, "Taking job from thread queue; jobs=", m_jobs.size());
                auto job = m_jobs.front();

                m_jobs.pop_front();

                lock.unlock();
                LOGGER(trace, "Executing job from queue");
                job();
                lock.lock();
            }

            LOGGER(trace, "Worker thread is finishing loop iteration");
        }
    }

    bool ThreadQueue::insideQueue() const noexcept
    {
        return m_insideQueueFlag;
    }

    size_t ThreadQueue::threads()
    {
        std::scoped_lock lock(m_mutex);

        return m_numThreads;
    }

    void ThreadQueue::threads(const size_t threads)
    {
        std::scoped_lock lock(m_mutex);

        m_numThreads = threads;
        adjustThreads();
    }

    void ThreadQueue::adjustThreads()
    {
        assert(m_mutex.haveLock());

        LOGGER(debug, "Adjusting number of threads in thread queue to ", m_numThreads);

        if (m_workers.size() == m_numThreads)
        {
            LOGGER(trace, "The number of workers is the same as numThreads");
        }
        else if (m_workers.size() > m_numThreads)
        {
            m_workerCondVar.notify_all();

            m_condVar.wait(m_mutex, [&] { return m_workers.size() - m_joinQueue.size() == m_numThreads; });
            assert(m_workers.size() - m_joinQueue.size() == m_numThreads);

            for (const auto id : m_joinQueue)
            {
                for (auto thread = m_workers.begin(); thread != m_workers.end();)
                {
                    if (thread->get_id() == id)
                    {
                        LOGGER(debug, "Joining thread ", thread->get_id());
                        assert(thread->joinable());
                        thread->join();
                        thread = m_workers.erase(thread);
                    }
                    else
                    {
                        thread++;
                    }
                }
            }
        }
        else
        {
            auto numStart = m_numThreads - m_workers.size();

            LOGGER(trace, "Need to start ", numStart, " threads");

            for (size_t i = 0; i < numStart; i++)
            {
                m_workers.emplace_back(std::bind(&ThreadQueue::worker, this));
            }
        }
    }

    void ThreadQueue::post(const JobType& job)
    {
        std::scoped_lock lock(m_mutex);

        m_jobs.push_back(job);
        // TODO Until more condition variables are added to handle the case of removing threads
        // all waiting threads need to be notified because more threads could be waiting
        // on the condition variable than just threads waiting for a job.
        m_workerCondVar.notify_one();
        LOGGER(trace, "Added job to thread queue; jobs=", m_jobs.size());
    }

    void initThreadQueue(const size_t numThreads)
    {
        std::scoped_lock lock(threadQueueSingletonMutex);
        assert(threadQueueSingleton == nullptr);
        threadQueueSingleton = new ThreadQueue(numThreads);
    }

    void shutdownThreadQueue()
    {
        std::scoped_lock lock(threadQueueSingletonMutex);
        assert(threadQueueSingleton != nullptr);
        delete threadQueueSingleton;
        threadQueueSingleton = nullptr;
    }

    ThreadQueue& threadQueue()
    {
        std::scoped_lock lock(threadQueueSingletonMutex);
        assert(threadQueueSingleton != nullptr);
        return *threadQueueSingleton;
    }

    void threadQueuePost(const ThreadQueue::JobType& job)
    {
        std::scoped_lock lock(threadQueueSingletonMutex);
        assert(threadQueueSingleton != nullptr);
        threadQueueSingleton->post(job);
    }
}
