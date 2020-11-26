/*
	Copyright (C) 2005-2007 Feeling Software Inc.
	Portions of the code are:
	Copyright (C) 2005-2007 Sony Computer Entertainment America
	
	MIT License: http://www.opensource.org/licenses/mit-license.php
*/

#include "StdAfx.h"
#include "FUTestBed.h"
#include "FUString.h"

TESTSUITE_START(FUString)

TESTSUITE_TEST(0, Equivalency)
	// Just verify some of the string<->const char* equivalency
	// in both Unicode and UTF-8 versions
	PassIf(IsEquivalent("Tetrahedron", fm::string("Tetrahedron")));
	PassIf(IsEquivalent(fm::string("Tetrahedron"), "Tetrahedron"));
	PassIf(IsEquivalent("My alter-ego", "My alter-ego"));
	
	FailIf(IsEquivalent("MY ALTER-EGO", "My alter-ego")); // should not be case-sensitive.
	FailIf(IsEquivalent("Utopia", "Utopian"));
	FailIf(IsEquivalent(fm::string("Greatness"), "Great"));
	FailIf(IsEquivalent("Il est", "Il  est"));
	PassIf(IsEquivalent("Prometheian", "Prometheian\0\0\0")); // This is the only difference allowed between two strings.

	fm::string sz1 = "Test1";
	fm::string sz2("Test1");
	PassIf(IsEquivalent(sz1, sz2)); // Extra verification in case the compiler optimizes out the string differences above.

TESTSUITE_TEST(1, StringTemplate)
	// Not looking to verify all the combinations,
	// but I do want to exercise all the basic functionality.
	fm::string a("TEST1"), b(a), c("TEST2"), d("VIRUSES", 5), e(3, 'C'), f("abc", 10);
	
	PassIf(d.length() == 5);
	PassIf(f.length() == 10);
	PassIf(IsEquivalent(a, "TEST1"));
	PassIf(a == b);
	PassIf(IsEquivalent(a, b));
	PassIf(a[0] == 'T');
	PassIf(IsEquivalent(d, "VIRUS"));
	PassIf(IsEquivalent(e, "CCC"));
	PassIf(a.length() == 5);
	PassIf(a.substr(0, 4) == c.substr(0, 4));
	FailIf(a == c);
	PassIf(a < c);

	e.append("4");
	b.append(a);
	PassIf(IsEquivalent(e, "CCC4"));
	PassIf(IsEquivalent(b, "TEST1TEST1"));
	PassIf(b.find("TEST1", 0) == 0);
	PassIf(b.find("TEST1", 2) == 5);
	PassIf(b.find("TEST1", 5) == 5);
	PassIf(b.find("TEST1", 7) == fm::string::npos);
	FailIf(c.find("TEST1") != fm::string::npos);
	PassIf(IsEquivalent(a.c_str(), "TEST1"));
	PassIf(d.find_first_of("RST") == 2);
	PassIf(c.find_last_of("T") == 3);

	e.erase(3, 4);
	a.erase(0, fm::string::npos);
	b.clear();
	PassIf(IsEquivalent(e, "CCC"));
	PassIf(e.find_first_of("TEST1ORANYTHINGELSETHATDOESNOTHAVE_?_") == fm::string::npos);
	PassIf(a.length() == 0);
	PassIf(IsEquivalent(a, ""));
	PassIf(a.empty());
	PassIf(a == b);
	PassIf(a.empty());
	PassIf(IsEquivalent(a.c_str(), b)); // c_str() will ensure that the vector<char> is NULL-terminated.
	PassIf(a.empty());
	PassIf(a == b);
	
	a.append('C');
	PassIf(IsEquivalent(a, "C"));
	a.append('D');
	PassIf(IsEquivalent(a, "CD"));
	a.insert(1, "S");
	PassIf(IsEquivalent(a, "CSD"));

TESTSUITE_TEST(2, SubTestSuites)
	RUN_TESTSUITE(FUStringBuilder);
	RUN_TESTSUITE(FUStringConversion);
	RUN_TESTSUITE(FUUniqueStringMap);
#ifndef WIN32
	PassIf(true);
#endif // WIN32

TESTSUITE_END
