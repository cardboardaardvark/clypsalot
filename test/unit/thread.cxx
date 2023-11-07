/* Copyright 2023 Tyler Riddle
 *
 * This file is part of Clypsalot. Clypsalot is free software: you can redistribute it and/or modify it under the terms
 * of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License,
 * or (at your option) any later version. Clypsalot is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along with Clypsalot. If not, see
 * <https://www.gnu.org/licenses/>.
 */

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <random>
#include <thread>

#include <clypsalot/error.hxx>
#include <clypsalot/logging.hxx>
#include <clypsalot/macros.hxx>
#include <clypsalot/util.hxx>

#include "test/lib/test.hxx"

#define TORTURE_THREADS 20
#define TORTURE_COUNT 1000000
#define NEW_THREAD(block) std::thread([&] block).join()

using namespace Clypsalot;

TEST_MAIN_FUNCTION

TEST_CASE(threadQueuePost_function)
{
    std::condition_variable_any condVar;
    Mutex mutex;
    std::unique_lock lock(mutex);
    std::thread::id mainThreadId = std::this_thread::get_id();
    bool didRun = false;

    BOOST_CHECK(didRun == false);

    lock.unlock();

    threadQueuePost([&]
    {
        BOOST_CHECK(std::this_thread::get_id() != mainThreadId);
        didRun = true;
        condVar.notify_all();
    });

    lock.lock();

    condVar.wait(lock, [&didRun]
    {
       return didRun;
    });
}

TEST_CASE(THREAD_CALL_MACRO)
{
    const auto ranInThread = THREAD_CALL(
    {
        return std::this_thread::get_id();
    });

    BOOST_CHECK(ranInThread != std::thread::id());
    BOOST_CHECK(ranInThread != std::this_thread::get_id());
}

TEST_CASE(threadCall_reentrant)
{
    const auto mainThreadId = std::this_thread::get_id();
    std::atomic_size_t depth = ATOMIC_VAR_INIT(0);

    threadQueueCall<void>([&]
    {
        const auto threadId = std::this_thread::get_id();

        depth++;
        BOOST_CHECK(threadId != mainThreadId);

        threadQueueCall<void>([&]
        {
            depth++;
            BOOST_CHECK(threadId == std::this_thread::get_id());
        });
    });

    BOOST_CHECK(depth == 2);
}

TEST_CASE(DebugMutex_lock)
{
    DebugMutex mutex;

    BOOST_CHECK(! mutex.locked());
    BOOST_CHECK(! mutex.haveLock());

    mutex.lock();
    BOOST_CHECK(mutex.locked());
    BOOST_CHECK(mutex.haveLock());

    NEW_THREAD(
    {
        BOOST_CHECK(mutex.locked());
        BOOST_CHECK(! mutex.haveLock());
    });

    mutex.unlock();
}

TEST_CASE(DebugMutex_recursive_lock)
{
    DebugMutex mutex;

    BOOST_CHECK(! mutex.locked());
    BOOST_CHECK(! mutex.haveLock());

    mutex.lock();
    BOOST_CHECK_THROW(mutex.lock(), MutexLockError);
    BOOST_CHECK_THROW(mutex.tryLock(), MutexLockError);

    mutex.unlock();
}

TEST_CASE(DebugMutex_unlock)
{
    DebugMutex mutex;

    BOOST_CHECK_NO_THROW(mutex.lock());
    BOOST_CHECK(mutex.locked());
    BOOST_CHECK_NO_THROW(mutex.unlock());
    BOOST_CHECK(! mutex.locked());
    BOOST_CHECK_THROW(mutex.unlock(), MutexUnlockError);
}

TEST_CASE(DebugMutex_unowned_unlock)
{
    DebugMutex mutex;

    mutex.lock();

    NEW_THREAD(
    {
        BOOST_CHECK_THROW(mutex.unlock(), MutexUnlockError);
    });

    BOOST_CHECK_NO_THROW(mutex.unlock());
}

TEST_CASE(DebugMutex_torture)
{
    DebugMutex mutex;
    std::vector<std::thread> threads;
    size_t counter = 0;
    std::random_device dev;
    std::mt19937 rng(dev());
    std::uniform_int_distribution<std::mt19937::result_type> distribution(1,10);
    std::vector<uint_fast32_t> delays;
    std::atomic busy = ATOMIC_VAR_INIT(0);

    delays.reserve(TORTURE_COUNT);

    LOGGER(verbose, "Generating random delay values");
    for (size_t i = 0; i < TORTURE_COUNT; i++)
    {
        auto random = distribution(rng);
        delays.push_back(random);
    }

    BOOST_CHECK(counter == 0);

    LOGGER(verbose, "Starting torture test");
    for (size_t i = 0; i < TORTURE_THREADS; i++)
    {
        threads.emplace_back([&]
        {
            while(true)
            {
                std::unique_lock lock(mutex);

                if (busy)
                {
                    BOOST_FAIL(makeString("Mutex did not actually perform mutual exclusion; counter=", counter));
                }

                busy = true;
                if (counter >= TORTURE_COUNT)
                {
                    LOGGER(debug, "Thread is done");
                    busy = false;
                    return;
                }
                auto count = counter++;
                busy = false;
                lock.unlock();

                auto usec = delays.at(count);
                std::this_thread::sleep_for(std::chrono::microseconds(usec));
            }
        });
    }

    for (auto& thread : threads)
    {
        LOGGER(debug, "Joining thread ", thread.get_id());
        thread.join();
    }

    BOOST_CHECK(counter == TORTURE_COUNT);
}

TEST_CASE(RecursiveDebugMutex_recursive_lock)
{
    RecursiveDebugMutex mutex;

    BOOST_CHECK(mutex.lockCount() == 0);
    BOOST_CHECK(! mutex.locked());
    BOOST_CHECK(! mutex.haveLock());

    BOOST_CHECK_NO_THROW(mutex.lock());
    BOOST_CHECK(mutex.lockCount() == 1);
    BOOST_CHECK(mutex.locked());
    BOOST_CHECK(mutex.haveLock());
    NEW_THREAD(
    {
        BOOST_CHECK(mutex.locked());
        BOOST_CHECK(! mutex.haveLock());
    });

    BOOST_CHECK_NO_THROW(mutex.lock());
    BOOST_CHECK(mutex.lockCount() == 2);

    BOOST_CHECK_NO_THROW(mutex.unlock());
    BOOST_CHECK(mutex.lockCount() == 1);
    BOOST_CHECK(mutex.locked());
    BOOST_CHECK(mutex.haveLock());

    BOOST_CHECK_NO_THROW(mutex.unlock());
    BOOST_CHECK(mutex.lockCount() == 0);
    BOOST_CHECK(! mutex.locked());
    BOOST_CHECK(! mutex.haveLock());

    BOOST_CHECK_THROW(mutex.unlock(), MutexUnlockError);
    BOOST_CHECK(mutex.lockCount() == 0);
    BOOST_CHECK(! mutex.locked());
    BOOST_CHECK(! mutex.haveLock());
}
