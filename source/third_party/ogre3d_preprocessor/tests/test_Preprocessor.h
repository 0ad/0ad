/* Copyright (C) 2021 Wildfire Games.
 * This file is part of 0 A.D.
 *
 * 0 A.D. is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * 0 A.D. is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with 0 A.D.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "lib/self_test.h"

#include "graphics/PreprocessorWrapper.h"
#include "lib/timer.h"
#include "ps/CStr.h"
#include "third_party/ogre3d_preprocessor/OgreGLSLPreprocessor.h"

#include <cctype>
#include <sstream>

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

	// Replaces consecutive spaces/tabs by single space/tab.
	CStr CompressWhiteSpaces(const CStr& source)
	{
		CStr result;
		for (char ch : source)
			if (!std::isblank(ch) || (result.empty() || result.back() != ch))
				result += ch;
		return result;
	}

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

	PreprocessorResult ParseWithIncludes(const char* in, CPreprocessorWrapper::IncludeRetrieverCallback includeCallback)
	{
		PreprocessorResult result;
		TestLogger logger;

		CPreprocessorWrapper preprocessor(includeCallback);
		result.output = preprocessor.Preprocess(in);
		result.loggerOutput = logger.GetOutput();

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

	void test_include()
	{
		bool includeRetrieved = false;
		CPreprocessorWrapper::IncludeRetrieverCallback includeCallback = [&includeRetrieved](
				const CStr& includePath, CStr& out) {
			TS_ASSERT_EQUALS(includePath, "test.h");
			out = "42";
			includeRetrieved = true;
			return true;
		};

		PreprocessorResult result = ParseWithIncludes(
			R"(#include "test.h" // Includes something.)", includeCallback);

		TS_ASSERT(includeRetrieved);
		TS_ASSERT_EQUALS(result.output.Trim(PS_TRIM_BOTH), "#line 1\n42\n#line 2");
	}

	void test_include_double()
	{
		CPreprocessorWrapper::IncludeRetrieverCallback includeCallback = [](
				const CStr& includePath, CStr& out) {
			TS_ASSERT_EQUALS(includePath, "test.h");
			out = "42";
			return true;
		};

		PreprocessorResult result = ParseWithIncludes(R"(
			#include "test.h"
			#include "test.h"
			#include "test.h"
		)", includeCallback);

		TS_ASSERT_EQUALS(result.output.Trim(PS_TRIM_BOTH), "#line 1\n42\n#line 3\n#line 1\n42\n#line 4\n#line 1\n42\n#line 5");
	}

	void test_include_double_with_guards()
	{
		CPreprocessorWrapper::IncludeRetrieverCallback includeCallback = [](
				const CStr& includePath, CStr& out) {
			TS_ASSERT_EQUALS(includePath, "test.h");
			out = R"(#ifndef INCLUDED_TEST
				#define INCLUDED_TEST
				42
			#endif)";
			return true;
		};

		PreprocessorResult result = ParseWithIncludes(R"(
			#include "test.h"
			#include "test.h"
			#include "test.h"
		)", includeCallback);

		TS_ASSERT_EQUALS(result.output.Trim(PS_TRIM_BOTH), "#line 1\n\n\t\t\t\t\n\t\t\t\t42\n\t\t\t\n#line 3\n#line 1\n\n\n\n\n#line 4\n#line 1\n\n\n\n\n#line 5");
	}

	void test_include_invalid_argument()
	{
		int includeRetrievedCounter = 0;
		CPreprocessorWrapper::IncludeRetrieverCallback includeCallback = [&includeRetrievedCounter](
				const CStr& UNUSED(includePath), CStr& out) {
			out = "42";
			++includeRetrievedCounter;
			return true;
		};

		PreprocessorResult result = ParseWithIncludes(R"(
			#include <test.h>
			#include test.h
		)", includeCallback);

		TS_ASSERT_EQUALS(includeRetrievedCounter, 0);
	}

	void test_include_invalid_file()
	{
		CPreprocessorWrapper::IncludeRetrieverCallback includeCallback = [](
				const CStr& UNUSED(includePath), CStr& UNUSED(out)) {
			return false;
		};

		PreprocessorResult result = ParseWithIncludes(R"(
			#include "missed_file.h"
		)", includeCallback);

		TS_ASSERT_STR_CONTAINS(result.loggerOutput, "ERROR: Preprocessor error: line 2: Can't load #include file: 'missed_file.h'");
	}

	void test_include_with_defines()
	{
		CPreprocessorWrapper::IncludeRetrieverCallback includeCallback = [](
				const CStr& UNUSED(includePath), CStr& out) {
			out = R"(
				#if defined(A)
					#define X 41
				#elif defined(B)
					#define X 42
				#else
					#define X 43
				#endif
				#ifdef Y
					#undef Y
					#define Y 256
				#endif
				vec3 color();
			)";
			return true;
		};

		PreprocessorResult result = ParseWithIncludes(R"(
			#define Y 128
			#define B 1
			#include "test.h"
			X Y
		)", includeCallback);

		TS_ASSERT_EQUALS(
			CompressWhiteSpaces(result.output.Trim(PS_TRIM_BOTH)),
			"#line 1\n\n\t\n\n\n\t\n\t\n\n\n\t\n\t\n\t\n\t\n\tvec3 color();\n\t\n#line 5\n\t42 256");
	}

	void test_performance_DISABLED()
	{
		CPreprocessorWrapper::IncludeRetrieverCallback includeCallback = [](
			const CStr& includePath, CStr& out) {
			const size_t dotPosition = includePath.find('.');
			TS_ASSERT_DIFFERS(dotPosition, CStr::npos);
			const int depth = CStr(includePath.substr(0, dotPosition)).ToInt();
			TS_ASSERT_LESS_THAN_EQUALS(0, depth);
			if (depth < 4)
			{
				std::stringstream nextIncludes;
				for (int idx = 0; idx < 16; ++idx)
					nextIncludes << "#include \"" << depth + 1 << ".h\"\n";
				out = nextIncludes.str();
			}
			else
			{
				out = R"(
					42
				)";
			}
			return true;
		};

		const double start = timer_Time();
		PreprocessorResult result = ParseWithIncludes(R"(
			#include "0.h"
		)", includeCallback);
		const double finish = timer_Time();
		printf("Total: %lfs\n", finish - start);
		TS_ASSERT_EQUALS(result.output.Trim(PS_TRIM_BOTH).size(), 2075304u);
	}
};
