/* Copyright (C) 2020 Wildfire Games.
 * This file is part of 0 A.D.
 *
 * 0 A.D. is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * 0 A.D. is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with 0 A.D.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "lib/self_test.h"

#include "scriptinterface/ScriptInterface.h"
#include "simulation2/helpers/Grid.h"
#include "simulation2/serialization/DebugSerializer.h"
#include "simulation2/serialization/SerializeTemplates.h"

#include <set>
#include <sstream>
#include <vector>

class TestSerializeTemplates : public CxxTest::TestSuite
{
public:
	void test_Debug_array()
	{
		ScriptInterface script("Test", "Test", g_ScriptRuntime);
		std::stringstream stream;

		CDebugSerializer serialize(script, stream);
		std::array<u32, 6> value = {
			3, 0, 1, 4, 1, 5
		};
		SerializeArray<SerializeU32_Unbounded>()(serialize, "E", value);
		TS_ASSERT_STR_EQUALS(stream.str(), "E: 3\nE: 0\nE: 1\nE: 4\nE: 1\nE: 5\n");
	}

	void test_Debug_vector()
	{
		ScriptInterface script("Test", "Test", g_ScriptRuntime);
		std::stringstream stream;

		CDebugSerializer serialize(script, stream);
		std::vector<u32> value = {
			3, 0, 1, 4, 1, 5
		};
		SerializeVector<SerializeU32_Unbounded>()(serialize, "E", value);
		TS_ASSERT_STR_EQUALS(stream.str(), "length: 6\nE: 3\nE: 0\nE: 1\nE: 4\nE: 1\nE: 5\n");
	}

	void test_Debug_set()
	{
		ScriptInterface script("Test", "Test", g_ScriptRuntime);
		std::stringstream stream;

		CDebugSerializer serialize(script, stream);
		std::set<u32> value = {
			3, 0, 1, 4, 1, 5
		};
		SerializeSet<SerializeU32_Unbounded>()(serialize, "E", value);
		TS_ASSERT_STR_EQUALS(stream.str(), "size: 5\nE: 0\nE: 1\nE: 3\nE: 4\nE: 5\n");
	}

	void test_Debug_grid()
	{
		ScriptInterface script("Test", "Test", g_ScriptRuntime);
		std::stringstream stream;

		CDebugSerializer serialize(script, stream);
		Grid<u16> value;
		value.resize(3,2);
		// Checkerboard pattern.
		for (u8 j = 0; j < value.height(); ++j)
			for (u8 i = 0; i < value.width(); ++i)
				value.set(i, j, ((i % 2) + (j % 2)) % 2);

		SerializedGridCompressed<SerializeU16_Unbounded>()(serialize, "E", value);
		TS_ASSERT_STR_EQUALS(stream.str(), "width: 3\nheight: 2\n"
							 "#: 1\nE: 0\n#: 1\nE: 1\n#: 1\nE: 0\n"
							 "#: 1\nE: 1\n#: 1\nE: 0\n#: 1\nE: 1\n");
	}
};
