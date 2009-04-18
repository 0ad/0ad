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

