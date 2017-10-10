/* Copyright (C) 2015 Wildfire Games.
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

#include "ps/CLogger.h"
#include "ps/XML/Xeromyces.h"
#include "ps/XML/RelaxNG.h"

class TestRelaxNG : public CxxTest::TestSuite
{
public:
	void setUp()
	{
		CXeromyces::Startup();
	}

	void tearDown()
	{
		CXeromyces::Terminate();
	}

	void test_basic()
	{
		RelaxNGValidator v;
		TS_ASSERT(v.LoadGrammar("<element xmlns='http://relaxng.org/ns/structure/1.0' name='test'><empty/></element>"));

		TS_ASSERT(v.Validate(L"doc", L"<test/>"));

		{
			TestLogger logger;
			TS_ASSERT(!v.Validate(L"doc", L"<bogus/>"));
			TS_ASSERT_STR_CONTAINS(logger.GetOutput(), "Validation error: doc:1: Expecting element test, got bogus");
		}

		{
			TestLogger logger;
			TS_ASSERT(!v.Validate(L"doc", L"bogus"));
			TS_ASSERT_STR_CONTAINS(logger.GetOutput(), "RelaxNGValidator: Failed to parse document");
		}

		TS_ASSERT(v.Validate(L"doc", L"<test/>"));
	}

	void test_interleave()
	{
		RelaxNGValidator v;
		TS_ASSERT(v.LoadGrammar("<element xmlns='http://relaxng.org/ns/structure/1.0' name='test'><interleave><empty/></interleave></element>"));
		// This currently (libxml2-2.7.7) leaks memory and makes Valgrind complain - see https://bugzilla.gnome.org/show_bug.cgi?id=615767
	}

	void test_datatypes()
	{
		RelaxNGValidator v;
		TS_ASSERT(v.LoadGrammar("<element xmlns='http://relaxng.org/ns/structure/1.0' datatypeLibrary='http://www.w3.org/2001/XMLSchema-datatypes' name='test'><data type='decimal'><param name='minInclusive'>1.5</param></data></element>"));

		TS_ASSERT(v.Validate(L"doc", L"<test>2.0</test>"));

		TestLogger logger;

		TS_ASSERT(!v.Validate(L"doc", L"<test>x</test>"));
		TS_ASSERT(!v.Validate(L"doc", L"<test>1.0</test>"));

		RelaxNGValidator w;
		TS_ASSERT(w.LoadGrammar("<element xmlns='http://relaxng.org/ns/structure/1.0' datatypeLibrary='http://www.w3.org/2001/XMLSchema-datatypes' name='test'><data type='integer'></data></element>"));

		TS_ASSERT(w.Validate(L"doc", L"<test>2</test>"));

		TS_ASSERT(!w.Validate(L"doc", L"<test>x</test>"));
		TS_ASSERT(!w.Validate(L"doc", L"<test>2.0</test>"));
	}

	void test_broken_grammar()
	{
		RelaxNGValidator v;

		TestLogger logger;
		TS_ASSERT(!v.LoadGrammar("whoops"));
		TS_ASSERT_STR_CONTAINS(logger.GetOutput(), "RelaxNGValidator: Failed to compile schema");

		TS_ASSERT(!v.Validate(L"doc", L"<test/>"));
	}
};
