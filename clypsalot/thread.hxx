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
        std::mutex guardedMutex;
        mutable std::mutex metaMutex;
        std::thread::id lockedBy;

        bool _haveLock() const;
        bool _locked() const;

        public:
        DebugMutex() = default;
        DebugMutex(const DebugMutex&) = delete;
        ~DebugMutex() = default;
        void operator=(const DebugMutex&) = delete;

        bool locked() const;
        bool haveLock() const;
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
        mutable Mutex mutex;

        public:
        Lockable() = default;
        Lockable(const Lockable&) = delete;
        ~Lockable() = default;
        void operator=(const Lockable&) = delete;
        /// @cond NO_DOCUMENT
        bool haveLock() const;
        void lock() const;
        void unlock() const;
        bool tryLock();
        bool try_lock();
        /// @endcond
    };

    /// @brief The SharedMutex version of DebugMutex.
    class SharedDebugMutex
    {
        std::shared_mutex guardedMutex;
        mutable std::mutex metaMutex;
        std::thread::id lockedBy;
        std::map<std::thread::id, bool> sharedBy;

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
        SharedMutex mutex;

        public:
        SharedLockable() = default;
        SharedLockable(const SharedLockable&) = delete;
        ~SharedLockable() = default;
        void operator=(const SharedLockable&) = delete;
        /// @cond NO_DOCUMENT
        bool haveLock();
        void lock();
        void tryLock();
        void try_lock();
        void unlock();
        bool haveSharedLock();
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
        size_t numThreads = 0;
        std::condition_variable_any condVar;
        std::vector<std::thread> workers;
        std::vector<std::thread::id> joinQueue;
        std::list<JobType> jobs;

        void adjustThreads();
        void worker();

        public:
        ThreadQueue(const size_t threads);
        ThreadQueue(const ThreadQueue&) = delete;
        ~ThreadQueue();
        void operator=(const ThreadQueue&) = delete;
        size_t threads();
        void threads(const size_t threads);
        void post(const JobType& job);

        template <typename T>
        T call(const std::function<T ()>& procedure)
        {
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

        template <>
        void call(const std::function<void ()>& procedure)
        {
            std::promise<void> promise;

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

    template <typename T>
    T threadQueueCall(const std::function<T ()>& job)
    {
        return threadQueue().call<T>(job);
    }
}
