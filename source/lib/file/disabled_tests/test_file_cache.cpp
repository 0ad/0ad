/* Copyright (C) 2009 Wildfire Games.
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

