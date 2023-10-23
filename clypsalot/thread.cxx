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
#include <clypsalot/logging.hxx>
#include <clypsalot/macros.hxx>
#include <clypsalot/thread.hxx>
#include <clypsalot/util.hxx>

/// @file
namespace Clypsalot
{
    static ThreadQueue* threadQueueSingleton = nullptr;
    static Mutex threadQueueSingletonMutex;

    bool DebugMutex::_locked() const
    {
        return std::thread::id() != lockedBy;
    }

    /**
     * @brief Identify if the mutex is locked by any thread.
     * @return true if the mutex is locked otherwise false.
     */
    bool DebugMutex::locked() const
    {
        std::unique_lock lock(metaMutex);
        return _locked();
    }

    bool DebugMutex::_haveLock() const
    {
        return std::this_thread::get_id() == lockedBy;
    }

    /**
     * @brief Identify if the calling thread has the mutex locked.
     * @return True if the mutex is locked by the calling thread otherwise false.
     */
    bool DebugMutex::haveLock() const
    {
        std::unique_lock lock(metaMutex);
        return _haveLock();
    }

    /**
     * @brief Lock the mutex.
     *
     * If a deadlock condition or other problem is detected this method will print out
     * diagnostic information to the console then call abort().
     */
    void DebugMutex::lock()
    {
        std::unique_lock lock(metaMutex);

        if (_haveLock())
        {
            FATAL_ERROR("Recursive lock attempt on mutex");
        }

        lock.unlock();
        guardedMutex.lock();
        lock.lock();

        assert(! _locked());
        lockedBy = std::this_thread::get_id();
    }

    /**
     * @brief Unlock the mutex.
     *
     * If an attempt is made to unlock the mutex by a thread that does not have the mutex
     * locked this method will print out diagnostic information to the console then call abort().
     */
    void DebugMutex::unlock()
    {
        std::unique_lock lock(metaMutex);

        if (! _haveLock())
        {
            FATAL_ERROR("Mutex was not locked by the calling thread");
        }

        assert(_locked());
        guardedMutex.unlock();
        lockedBy = std::thread::id();
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
        std::unique_lock lock(metaMutex);

        if (_haveLock())
        {
            FATAL_ERROR("tryLock() would result in recursive locking of mutex");
        }

        auto lockResult = guardedMutex.try_lock();

        if (! lockResult)
        {
            return false;
        }

        assert(! _locked());
        lockedBy = std::this_thread::get_id();
        return lockResult;
    }

#ifndef NDEBUG
    bool Lockable::haveLock() const
    {
        return mutex.haveLock();
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
        mutex.lock();
    }

    void Lockable::unlock() const
    {
        mutex.unlock();
    }

    bool SharedDebugMutex::_locked() const
    {
        return std::thread::id() != lockedBy;
    }

    /// @brief Returns true if any thread holds an exclusive lock on the mutex
    bool SharedDebugMutex::locked() const
    {
        std::unique_lock lock(metaMutex);
        return _locked();
    }

    bool SharedDebugMutex::_haveLock() const
    {
        return std::this_thread::get_id() == lockedBy;
    }

    /// @brief Returns true if the calling thread holds an exclusive lock
    bool SharedDebugMutex::haveLock() const
    {
        std::unique_lock lock(metaMutex);
        return _haveLock();
    }

    /// @brief Obtain an exclusive lock on the shared mutex.
    void SharedDebugMutex::lock()
    {
        std::unique_lock lock(metaMutex);

        if (_haveLock())
        {
            FATAL_ERROR("Recursive exclusive lock attempt on mutex");
        }

        if (_haveSharedLock())
        {
            FATAL_ERROR("Recursive exclusive lock of mutex by thread that has a shared lock");
        }

        lock.unlock();
        guardedMutex.lock();
        lock.lock();

        assert(! _locked());
        assert(sharedBy.size() == 0);
        lockedBy = std::this_thread::get_id();
    }

    /**
     * @brief Try to obtain an exclusive lock with out blocking.
     * @return True if an exclusive lock could be obtained or false otherwise.
     */
    bool SharedDebugMutex::tryLock()
    {
        std::unique_lock lock(metaMutex);

        if (_haveLock())
        {
            FATAL_ERROR("Recursive lock attempt of mutex");
        }

        if (_haveSharedLock())
        {
            FATAL_ERROR("Recursive lock attempt of mutex by thread that has a shared lock");
        }

        if (! guardedMutex.try_lock())
        {
            return false;
        }

        assert(! _locked());
        assert(! _sharedLocked());
        lockedBy = std::this_thread::get_id();

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
        std::unique_lock lock(metaMutex);

        if (std::this_thread::get_id() != lockedBy)
        {
            FATAL_ERROR("Attempt to unlock mutex by thread that does not hold the lock");
        }

        guardedMutex.unlock();
        lockedBy = std::thread::id();
    }

    bool SharedDebugMutex::_sharedLocked() const
    {
        return sharedBy.size() != 0;
    }

    /// @brief Returns true if any thread holds a shared lock on the mutex.
    bool SharedDebugMutex::sharedLocked() const
    {
        std::unique_lock lock(metaMutex);
        return _sharedLocked();
    }

    bool SharedDebugMutex::_haveSharedLock() const
    {
        const auto currentThread = std::this_thread::get_id();

        if (currentThread == lockedBy)
        {
            return true;
        }

        if (sharedBy.contains(currentThread))
        {
            return true;
        }

        return false;
    }

    /// @brief Returns true if the calling thread holds either a shared or exclusive lock
    bool SharedDebugMutex::haveSharedLock() const
    {
        std::unique_lock lock(metaMutex);
        return _haveSharedLock();
    }

    /// @brief Obtain a shared lock on the mutex.
    void SharedDebugMutex::lockShared()
    {
        std::unique_lock lock(metaMutex);

        if (_haveLock())
        {
            FATAL_ERROR("Recursive shared lock attempt by thread that holds an exclusive lock");
        }

        if (_haveSharedLock())
        {
            FATAL_ERROR("Recursive shared lock attempt");
        }

        lock.unlock();
        guardedMutex.lock_shared();
        lock.lock();

        assert(! _locked());
        sharedBy[std::this_thread::get_id()] = true;
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
        std::unique_lock lock(metaMutex);

        if (_haveLock())
        {
            FATAL_ERROR("Recursive shared lock attempt by thread that has an exclusive lock");
        }

        if (_haveSharedLock())
        {
            FATAL_ERROR("Recursive shared lock attempt");
        }

        if (! guardedMutex.try_lock_shared())
        {
            return false;
        }

        assert(! _locked());
        sharedBy[std::this_thread::get_id()] = true;
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
        std::unique_lock lock(metaMutex);

        if (_haveLock())
        {
            FATAL_ERROR("Attempt to shared unlock a mutex by a thread that holds an exclusive lock");
        }

        if (! _haveSharedLock())
        {
            FATAL_ERROR("Attempt to shared unlock a mutex by a thread that does not have a shared lock");
        }

        assert(! _locked());
        guardedMutex.unlock_shared();
        sharedBy.erase(std::this_thread::get_id());
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
        std::unique_lock lock(mutex);

        LOGGER(debug, "A new worker thread is born");

        while(true)
        {
            LOGGER(trace, "Worker thread is starting a loop iteration");
            condVar.wait(lock, [&]
            {
                if (jobs.size() > 0)
                {
                    LOGGER(trace, "Worker thread has a job to do; jobs=", jobs.size());
                    return true;
                }

                if (workers.size() - joinQueue.size() > numThreads)
                {
                    LOGGER(trace, "Worker thread needs to die");
                    return true;
                }

                LOGGER(trace, "Worker thread has nothing to do");
                return false;
            });

            if (workers.size() - joinQueue.size() > numThreads)
            {
                LOGGER(debug, "Thread is quiting to reduce the number of workers");
                joinQueue.push_back(std::this_thread::get_id());
                condVar.notify_all();
                return;
            }

            if (jobs.size() > 0)
            {
                LOGGER(trace, "Taking job from thread queue; jobs=", jobs.size());
                auto job = jobs.front();

                jobs.pop_front();

                lock.unlock();
                LOGGER(trace, "Executing job from queue");
                job();
                lock.lock();
            }

            LOGGER(trace, "Worker thread is finishing loop iteration");
        }
    }

    size_t ThreadQueue::threads()
    {
        std::unique_lock lock(mutex);

        return numThreads;
    }

    void ThreadQueue::threads(const size_t threads)
    {
        std::unique_lock lock(mutex);

        numThreads = threads;
        adjustThreads();
    }

    void ThreadQueue::adjustThreads()
    {
        assert(mutex.haveLock());

        LOGGER(debug, "Adjusting number of threads in thread queue to ", numThreads);

        if (workers.size() == numThreads)
        {
            LOGGER(trace, "The number of workers is the same as numThreads");
        }
        else if (workers.size() > numThreads)
        {
            condVar.notify_all();

            condVar.wait(mutex, [&] { return workers.size() - joinQueue.size() == numThreads; });
            assert(workers.size() - joinQueue.size() == numThreads);

            for (const auto id : joinQueue)
            {
                for (auto thread = workers.begin(); thread != workers.end();)
                {
                    if (thread->get_id() == id)
                    {
                        LOGGER(debug, "Joining thread ", thread->get_id());
                        assert(thread->joinable());
                        thread->join();
                        thread = workers.erase(thread);
                    }
                    else
                    {
                        LOGGER(trace, "No need to join thread ", thread->get_id());
                        thread++;
                    }
                }
            }
        }
        else
        {
            auto numStart = numThreads - workers.size();

            LOGGER(trace, "Need to start ", numStart, " threads");

            for (size_t i = 0; i < numStart; i++)
            {
                workers.emplace_back(std::bind(&ThreadQueue::worker, this));
            }
        }
    }

    void ThreadQueue::post(const JobType& job)
    {
        std::unique_lock lock(mutex);

        jobs.push_back(job);
        // TODO Until more condition variables are added to handle the case of removing threads
        // all waiting threads need to be notified because more threads could be waiting
        // on the condition variable than just threads waiting for a job.
        condVar.notify_all();
        LOGGER(trace, "Added job to thread queue; jobs=", jobs.size());
    }

    void initThreadQueue(const size_t numThreads)
    {
        std::unique_lock lock(threadQueueSingletonMutex);
        assert(threadQueueSingleton == nullptr);
        threadQueueSingleton = new ThreadQueue(numThreads);
    }

    void shutdownThreadQueue()
    {
        std::unique_lock lock(threadQueueSingletonMutex);
        assert(threadQueueSingleton != nullptr);
        delete threadQueueSingleton;
        threadQueueSingleton = nullptr;
    }

    ThreadQueue& threadQueue()
    {
        std::unique_lock lock(threadQueueSingletonMutex);
        assert(threadQueueSingleton != nullptr);
        return *threadQueueSingleton;
    }

    void threadQueuePost(const ThreadQueue::JobType& job)
    {
        std::unique_lock lock(threadQueueSingletonMutex);
        assert(threadQueueSingleton != nullptr);
        threadQueueSingleton->post(job);
    }
}
