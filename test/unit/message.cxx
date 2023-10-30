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

#include <condition_variable>

#include <clypsalot/logging.hxx>
#include <clypsalot/macros.hxx>
#include <clypsalot/message.hxx>
#include <clypsalot/thread.hxx>
#include <clypsalot/util.hxx>

#include "test/lib/test.hxx"

using namespace Clypsalot;

#define TEST_STRING_VALUE "Some string."

TEST_MAIN_FUNCTION

struct TestMessage : public Message
{
    const std::string string;

    TestMessage(const std::string& string) :
        string(string)
    { }
};

TEST_CASE(Message_deliver)
{
    MessageProcessor messages;
    std::condition_variable_any condVar;
    Mutex mutex;
    std::unique_lock lock(mutex);
    bool didRun = false;

    messages.registerHandler<TestMessage>([&condVar, &mutex, &didRun] (const TestMessage& message)
    {
        BOOST_CHECK(message.string == TEST_STRING_VALUE);

        std::unique_lock lock(mutex);
        didRun = true;
        condVar.notify_all();
    });

    BOOST_CHECK(didRun == false);
    messages.receive(new TestMessage(TEST_STRING_VALUE));
    condVar.wait(lock, [&didRun]
    {
        return didRun;
    });
    BOOST_CHECK(didRun == true);
}
