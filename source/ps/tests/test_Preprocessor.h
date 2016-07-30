/* Copyright (C) 2016 Wildfire Games.
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

#include "ps/Preprocessor.h"

class TestPreprocessor : public CxxTest::TestSuite
{
public:
	void test_basic()
	{
		CPreprocessor preproc;
		const char* in = "#define TEST 2\n1+1=TEST\n";
		size_t len = 0;
		char* out = preproc.Parse(in, strlen(in), len);
		TS_ASSERT_EQUALS(std::string(out, len), "\n1+1=2\n");

		// Free output if it's not inside the source string
		if (!(out >= in && out < in + strlen(in)))
			free(out);
	}

	void test_error()
	{
		TestLogger logger;

		CPreprocessor preproc;
		const char* in = "foo\n#if ()\nbar\n";
		size_t len = 0;
		char* out = preproc.Parse(in, strlen(in), len);
		TS_ASSERT_EQUALS(std::string(out, len), "");

		TS_ASSERT_STR_CONTAINS(logger.GetOutput(), "ERROR: Preprocessor error: line 2: Unclosed parenthesis in #if expression\n");

		// Free output if it's not inside the source string
		if (!(out >= in && out < in + strlen(in)))
			free(out);
	}
};
