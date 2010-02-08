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

#include "d:\Projects\0ad\svn\source\lib\res\file\tests\test_file_cache.h"

static TestFileCache suite_TestFileCache;

static CxxTest::List Tests_TestFileCache = { 0, 0 };
CxxTest::StaticSuiteDescription suiteDescription_TestFileCache( "d:\\Projects\\0ad\\svn\\source\\lib\\res\\file\\tests\\test_file_cache.h", 6, "TestFileCache", suite_TestFileCache, Tests_TestFileCache );

static class TestDescription_TestFileCache_test_cache_allocator : public CxxTest::RealTestDescription {
public:
 TestDescription_TestFileCache_test_cache_allocator() : CxxTest::RealTestDescription( Tests_TestFileCache, suiteDescription_TestFileCache, 10, "test_cache_allocator" ) {}
 void runTest() { suite_TestFileCache.test_cache_allocator(); }
} testDescription_TestFileCache_test_cache_allocator;

