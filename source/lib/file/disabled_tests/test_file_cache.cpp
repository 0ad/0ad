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

