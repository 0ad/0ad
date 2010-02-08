/* Copyright (c) 2010 Wildfire Games
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

/* Generated file, do not edit */

#ifndef CXXTEST_RUNNING
#define CXXTEST_RUNNING
#endif

#define _CXXTEST_HAVE_STD
#include "precompiled.h"
#include <cxxtest/TestListener.h>
#include <cxxtest/TestTracker.h>
#include <cxxtest/TestRunner.h>
#include <cxxtest/RealDescriptions.h>

#include "d:\Projects\0ad\svn\source\lib\res\file\tests\test_path.h"

static TestPath suite_TestPath;

static CxxTest::List Tests_TestPath = { 0, 0 };
CxxTest::StaticSuiteDescription suiteDescription_TestPath( "d:\\Projects\\0ad\\svn\\source\\lib\\res\\file\\tests\\test_path.h", 6, "TestPath", suite_TestPath, Tests_TestPath );

static class TestDescription_TestPath_test_conversion : public CxxTest::RealTestDescription {
public:
 TestDescription_TestPath_test_conversion() : CxxTest::RealTestDescription( Tests_TestPath, suiteDescription_TestPath, 9, "test_conversion" ) {}
 void runTest() { suite_TestPath.test_conversion(); }
} testDescription_TestPath_test_conversion;

static class TestDescription_TestPath_test_atom : public CxxTest::RealTestDescription {
public:
 TestDescription_TestPath_test_atom() : CxxTest::RealTestDescription( Tests_TestPath, suiteDescription_TestPath, 33, "test_atom" ) {}
 void runTest() { suite_TestPath.test_atom(); }
} testDescription_TestPath_test_atom;

