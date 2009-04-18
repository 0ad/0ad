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

