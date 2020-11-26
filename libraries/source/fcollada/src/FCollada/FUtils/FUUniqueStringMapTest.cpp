/*
	Copyright (C) 2005-2007 Feeling Software Inc.
	Portions of the code are:
	Copyright (C) 2005-2007 Sony Computer Entertainment America
	
	MIT License: http://www.opensource.org/licenses/mit-license.php
*/

#include "StdAfx.h"
#include "FUUniqueStringMap.h"
#include "FUTestBed.h"
#include "FMath/FMRandom.h"

#define VERIFY_UNIQUE(c) { \
	for (StringList::iterator itN = uniqueNames.begin(); itN != uniqueNames.end(); ++itN) { \
		FailIf((*itN) == (c)); \
	} }

TESTSUITE_START(FUUniqueStringMap)

TESTSUITE_TEST(0, Uniqueness)
	StringList uniqueNames;

	FUSUniqueStringMap names;
	FailIf(names.contains("Test"));
	FailIf(names.contains("Test0"));
	FailIf(names.contains("Test1"));
	FailIf(names.contains("Test2"));

	// Add a first name: should always be unique
	fm::string name1 = "Test";
	names.insert(name1);
	PassIf(names.contains("Test"));
	PassIf(name1 == "Test");
	uniqueNames.push_back(name1);

	// Add a second name that should also be unique
	fm::string name2 = "Glad";
	names.insert(name2);
	PassIf(names.contains("Glad"));
	PassIf(name2 == "Glad");
	uniqueNames.push_back(name2);

	// Add the first name a couple more times
	fm::string name3 = name1;
	names.insert(name3);
	PassIf(names.contains(name3));
	VERIFY_UNIQUE(name3);
	uniqueNames.push_back(name3);

	name3 = name1;
	names.insert(name3);
	PassIf(names.contains(name3));
	VERIFY_UNIQUE(name3);
	uniqueNames.push_back(name3);

	// Add the second name a couple more times
	name3 = name2;
	names.insert(name3);
	PassIf(names.contains(name3));
	VERIFY_UNIQUE(name3);
	uniqueNames.push_back(name3);

	name3 = name2;
	names.insert(name3);
	PassIf(names.contains(name3));
	VERIFY_UNIQUE(name3);
	uniqueNames.push_back(name3);

	// There should now be 6 unique names, so pick some randomly and verify that we're always getting more unique names.
	for (uint32 i = 0; i < 5; ++i)
	{
		uint32 index = FMRandom::GetUInt32((uint32) uniqueNames.size());
		name3 = uniqueNames[index];
		names.insert(name3);
		PassIf(names.contains(name3));
		VERIFY_UNIQUE(name3);
		uniqueNames.push_back(name3);
	}

TESTSUITE_TEST(1, NameGeneration)
	FUSUniqueStringMap names;

	// Add a first name: should always be unique.
	fm::string name1 = "Test55";
	names.insert(name1);
	PassIf(names.contains("Test55"));
	PassIf(name1 == "Test55");

	// Add a second name: number at the end should be incremented.
	fm::string name2 = "Test55";
	names.insert(name2);
	PassIf(names.contains("Test55"));
	PassIf(names.contains("Test56"));
	PassIf(name2 == "Test56");

	// Add a third name: number at the end should be +2.
	fm::string name3 = "Test55";
	names.insert(name3);
	PassIf(names.contains("Test55"));
	PassIf(names.contains("Test56"));
	PassIf(names.contains("Test57"));
	PassIf(name3 == "Test57");

TESTSUITE_TEST(2, BadDataCase)
	FUSUniqueStringMap names;

	// Does adding the empty string work?
	PassIf(!names.contains(""));
	names.insert("");
	PassIf(names.contains(""));
	names.erase("");
	PassIf(!names.contains(""));

	// Does adding pure numbers work?
	names.insert("");
	names.insert("2");
	PassIf(names.contains("2"));
	PassIf(names.contains(""));
	FailIf(names.contains("3"));
	fm::string p;
	names.insert(p);
	PassIf(names.contains(p));

	// Does adding many similarly-prefixed strings work?
	FUSStringBuilder builder;
	names.insert("Gamma");
	for (uint32 i = 0; i < 512; ++i)
	{
		builder.set("Gamma");
		builder.append((uint32) i);
		fm::string p = builder.ToString();
		FailIf(names.contains(p));
		names.insert(p);
		PassIf(names.contains(p));
	}

	// Try name-clash in this case.
	p = "Gamma";
	PassIf(names.contains(p));
	names.insert(p);
	PassIf(names.contains(p));
	PassIf(IsEquivalent(p, "Gamma512"));

	// Remove a few random entries in the map and check for correct operation.
	names.erase("Gamma513");
	names.erase("Gamma512");
	names.erase("Gamma511");
	names.erase("Gamma51");
	names.erase("Gamma53");
	names.erase("Gamma");
	names.erase("Gamma27");
	PassIf(names.contains("Gamma0"));
	PassIf(names.contains("Gamma510"));
	PassIf(!names.contains("Gamma51"));
	PassIf(!names.contains("Gamma"));
	names.insert("Gamma0");
	PassIf(names.contains("Gamma511"));
	names.insert(p = "Gamma");
	PassIf(IsEquivalent(p, "Gamma"));
	names.insert(p = "Gamma53");
	PassIf(IsEquivalent(p, "Gamma53"));
	names.insert(p = "Gamma52");
	PassIf(IsEquivalent(p, "Gamma512"));

TESTSUITE_END
