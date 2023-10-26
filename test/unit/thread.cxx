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
#include <condition_variable>

#include <clypsalot/thread.hxx>
#include <clypsalot/macros.hxx>

#include "test/lib/test.hxx"

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
