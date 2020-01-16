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

#include "graphics/PreprocessorWrapper.h"
#include "ps/CStr.h"
#include "third_party/ogre3d_preprocessor/OgreGLSLPreprocessor.h"

class TestPreprocessor : public CxxTest::TestSuite
{
public:
	void setUp()
	{
		Ogre::CPreprocessor::ErrorHandler = CPreprocessorWrapper::PyrogenesisShaderError;
	}

	struct PreprocessorResult
	{
		CStr8 output;
		CStr8 loggerOutput;
	};

	PreprocessorResult Parse(const char* in)
	{
		PreprocessorResult result;
		TestLogger logger;
		Ogre::CPreprocessor preproc;
		size_t len = 0;
		char* out = preproc.Parse(in, strlen(in), len);
		result.output = std::string(out, len);
		result.loggerOutput = logger.GetOutput();

		// Free output if it's not inside the source string
		if (!(out >= in && out < in + strlen(in)))
			free(out);

		return result;
	}

	void test_basic()
	{
		PreprocessorResult result = Parse("#define TEST 2\n1+1=TEST\n");
		TS_ASSERT_EQUALS(result.output, "\n1+1=2\n");
	}

	void test_error()
	{
		PreprocessorResult result = Parse("foo\n#if ()\nbar\n");
		TS_ASSERT_EQUALS(result.output, "");
		TS_ASSERT_STR_CONTAINS(result.loggerOutput, "ERROR: Preprocessor error: line 2: Unclosed parenthesis in #if expression\n");
	}

	void test_else()
	{
		PreprocessorResult result = Parse(R"(
			#define FOO 0
			#if FOO
				#define BAR 0
			#else
				#define BAR 42
			#endif
			BAR
		)");
		TS_ASSERT_EQUALS(result.output.Trim(PS_TRIM_BOTH), "42");
	}

	void test_elif()
	{
		PreprocessorResult result = Parse(R"(
			#define FOO 0
			#if FOO
				#define BAR 0
			#elif 1
				#define BAR 42
			#endif
			BAR
		)");
		TS_ASSERT_EQUALS(result.output.Trim(PS_TRIM_BOTH), "42");
	}

	void test_nested_macro()
	{
		PreprocessorResult result = Parse(R"(
			#define FOO(a, b, c, d) func1(a, b + (c * r), 0)
			#define BAR func2
			FOO(x, y, BAR(u, v), w)
		)");
		TS_ASSERT_EQUALS(result.output.Trim(PS_TRIM_BOTH), "func1(x, y + (func2(u, v) * r), 0)");
	}

	void test_division_by_zero_error()
	{
		PreprocessorResult result = Parse(R"(
			#if 2 / 0
			42
			#endif
		)");
		TS_ASSERT_EQUALS(result.output.Trim(PS_TRIM_BOTH), "");
		TS_ASSERT_STR_CONTAINS(result.loggerOutput, "ERROR: Preprocessor error: line 2: Division by zero");
	}
};
