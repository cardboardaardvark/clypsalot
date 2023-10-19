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
#include <clypsalot/macros.hxx>
#include <clypsalot/thread.hxx>

/// @file
namespace Clypsalot
{
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

    /**
     * @brief Compatibility with the Lockable named requirement
     * @return The return value from \ref DebugMutex::tryLock() "tryLock()".
     */
    bool DebugMutex::try_lock()
    {
        return tryLock();
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
}
