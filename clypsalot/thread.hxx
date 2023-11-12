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

#pragma once

#include <cassert>
#include <condition_variable>
#include <functional>
#include <future>
#include <list>
#include <shared_mutex>
#include <thread>
#include <map>
#include <mutex>
#include <vector>

/// @file
namespace Clypsalot
{
    /**
     * @brief Mutex used to aid in development.
     *
     * This is a mutex that allows testing for the current thread holding a lock and catches other
     * usage errors. It is intended to only be used during development and testing because it
     * incurs overhead when used. This mutex is not intended to be used directly by name. Instead
     * use the Mutex type which is std::mutex if NDEBUG is defined and this mutex otherwise.
     * Because this mutex will not be in used for non-debug builds it is important to call the
     * DebugMutex specific methods only from inside assert() statements or when otherwise wrapped
     * with NDEBUG ifdefs.
     */
    class DebugMutex
    {
        friend class Lockable;

        std::mutex m_guardedMutex;
        mutable std::mutex m_metaMutex;
        std::thread::id m_lockedBy;

        bool _haveLock() const;
        bool _locked() const;

        public:
        DebugMutex();
        DebugMutex(const DebugMutex&) = delete;
        virtual ~DebugMutex();
        void operator=(const DebugMutex&) = delete;

        bool locked() const;
        bool haveLock() const;
        size_t lockCount() const;
        void lock();
        void unlock();
        bool tryLock();
        bool try_lock();
    };

    /// @typedef Mutex
    /// @brief See the \ref DebugMutex documentation.
#ifdef NDEBUG
    using Mutex = std::mutex;
#else
    using Mutex = DebugMutex;
#endif

    /**
     * @brief A mixin that adds a Mutex to an object and gives it the Lockable C++ named requirement.
     */
    class Lockable
    {
        protected:
        /// @brief The mutex used for the object.
        mutable Mutex m_mutex;

        public:
        Lockable() = default;
        Lockable(const Lockable&) = delete;
        virtual ~Lockable() = default;
        void operator=(const Lockable&) = delete;
        /// @cond NO_DOCUMENT
#ifndef NDEBUG
        bool haveLock() const;
#endif
        void lock() const;
        void unlock() const;
        bool tryLock() const;
        bool try_lock() const;
        /// @endcond
    };

    /// @brief The SharedMutex version of DebugMutex.
    class SharedDebugMutex
    {
        std::shared_mutex m_guardedMutex;
        mutable std::mutex m_metaMutex;
        std::thread::id m_lockedBy;
        std::map<std::thread::id, bool> m_sharedBy;

        bool _locked() const;
        bool _haveLock() const;
        bool _sharedLocked() const;
        bool _haveSharedLock() const;

        public:
        bool locked() const;
        bool haveLock() const;
        void lock();
        bool tryLock();
        bool try_lock();
        void unlock();
        bool sharedLocked() const;
        bool haveSharedLock() const;
        void lockShared();
        void lock_shared();
        bool tryLockShared();
        bool try_lock_shared();
        void unlockShared();
        void unlock_shared();
    };

    /// @typedef SharedMutex
    /// @brief See the \ref SharedDebugMutex documentation.
#ifdef NDEBUG
    using SharedMutex = std::shared_mutex;
#else
    using SharedMutex = SharedDebugMutex;
#endif

    /// @brief The SharedMutex version of Lockable.
    class SharedLockable
    {
        protected:
        /// @brief The mutex used for the object.
        SharedMutex m_mutex;

        public:
        SharedLockable() = default;
        SharedLockable(const SharedLockable&) = delete;
        ~SharedLockable() = default;
        void operator=(const SharedLockable&) = delete;
        /// @cond NO_DOCUMENT
#ifndef NDEBUG
        bool haveLock();
#endif
        void lock();
        void tryLock();
        void try_lock();
        void unlock();
#ifndef NDEBUG
        bool haveSharedLock();
#endif
        void lockShared();
        bool tryLockShared();
        bool try_lock_shared();
        bool unlockShared();
        bool unlock_shared();
        /// @endcond
    };

    class ThreadQueue : Lockable
    {
        public:
        using JobType = std::function<void ()>;

        private:
        thread_local static bool m_insideQueueFlag;

        size_t m_numThreads = 0;
        std::condition_variable_any m_condVar;
        std::condition_variable_any m_workerCondVar;
        std::vector<std::thread> m_workers;
        std::vector<std::thread::id> m_joinQueue;
        std::list<JobType> m_jobs;

        void adjustThreads();
        void worker();

        public:
        ThreadQueue(const size_t threads);
        ThreadQueue(const ThreadQueue&) = delete;
        ~ThreadQueue();
        void operator=(const ThreadQueue&) = delete;
        bool insideQueue() const noexcept;
        size_t threads();
        void threads(const size_t threads);
        void post(const JobType& job);

        template <typename T>
        T call(const std::function<T ()>& procedure)
        {
            assert(! m_insideQueueFlag);

            std::promise<T> promise;

            post([&promise, &procedure]
            {
                try
                {
                    promise.set_value(procedure());
                }
                catch (...)
                {
                    promise.set_exception(std::current_exception());
                }
            });

            return promise.get_future().get();
        }

        // See GCC bug 85282 for why this uses std::same_as
        // https://gcc.gnu.org/bugzilla/show_bug.cgi?id=85282
        template <std::same_as<void> T>
        void call(const std::function<T ()>& procedure)
        {
            assert(! m_insideQueueFlag);

            std::promise<T> promise;

            post([&promise, &procedure]
            {
                try {
                    procedure();
                    promise.set_value();
                }
                catch(...)
                {
                    promise.set_exception(std::current_exception());
                }
            });

            promise.get_future().get();
            return;
        }
    };

    void initThreadQueue(const size_t numThreads);
    void shutdownThreadQueue();
    ThreadQueue& threadQueue();
    void threadQueuePost(const ThreadQueue::JobType& job);

    /**
     * @brief Execute a procedure inside the thread queue and return the result.
     * @param Any std::function to execute from inside the thread queue.
     * @return The return value from procedure.
     *
     * This method executes the procedure specified as the argument inside the thread
     * queue and blocks the caller until it is done executing. If the procedure returns
     * a value then the value is returned by this method as well. If an exception is
     * generated while executing the procedure then the procedure stops and the exception
     * will be thrown again at the call site.
     *
     * This method addresses a priority inversion problem that exists when interacting
     * with Objects and other things that execute inside the thread queue. In the future
     * the thread queue may run at real time priority but Objects could be locked and interacted
     * with by a non-realtime priority thread. This method allows non-realtime threads to
     * interact with the Object in a normal blocking way and with out doing it from a
     * non-realtime thread.
     *
     * Users of this method should do as little work as possible inside the thread queue reserving
     * it for the time critical tasks.
     *
     * See also: The @ref THREAD_CALL macro to simplify usage with a lambda expression.
     */
    template <typename T>
    T threadQueueCall(const std::function<T ()>& job)
    {
        if (threadQueue().insideQueue())
        {
            return job();
        }

        return threadQueue().call<T>(job);
    }
}
