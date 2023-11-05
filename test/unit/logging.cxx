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

#include <clypsalot/logging.hxx>

#include "test/lib/test.hxx"

using namespace Clypsalot;

TEST_MAIN_FUNCTION

TEST_CASE(logSeverityNames_global)
{
    size_t tested = 0;

    BOOST_CHECK(logSeverityNames.size() == 8);

    for (const auto& name : logSeverityNames)
    {
        BOOST_CHECK_NO_THROW(logSeverity(name));
        tested++;
    }

    BOOST_CHECK(tested == logSeverityNames.size());
}
