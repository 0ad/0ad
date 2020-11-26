/*
	Copyright (C) 2005-2007 Feeling Software Inc.
	Portions of the code are:
	Copyright (C) 2005-2007 Sony Computer Entertainment America
	
	MIT License: http://www.opensource.org/licenses/mit-license.php
*/

#include "StdAfx.h"
#include "FUTestBed.h"

FCOLLADA_EXPORT bool FUTestBed_skipAsserts = false;

//
// FUTestbed
//

FUTestBed::FUTestBed(const fchar* _filename, bool _isVerbose)
:	testPassed(0), testFailed(0)
,	fileOut(_filename), filename(_filename), isVerbose(_isVerbose)
{
}

bool FUTestBed::RunTestbed(FUTestSuite* headTestSuite)
{
	if (headTestSuite == NULL) return false;

	testPassed = testFailed = 0;

	RunTestSuite(headTestSuite);

	if (isVerbose)
	{
		fileOut.WriteLine("---------------------------------");
		fileOut.WriteLine("Tests passed: %u.", (uint32) testPassed);
		fileOut.WriteLine("Tests failed: %u.", (uint32) testFailed);
		fileOut.WriteLine("");
		fileOut.Flush();

#ifdef _WIN32
		char sz[1024];
		snprintf(sz, 1024, "Testbed score: [%u/%u]", testPassed, testFailed + testPassed);
		sz[1023] = 0;

		size_t returnCode = IDOK;
		returnCode = MessageBox(NULL, TO_FSTRING(sz).c_str(), FC("Testbed"), MB_OKCANCEL);
		if (returnCode == IDCANCEL)
		{
			snprintf(sz, 1024, "write %s ", filename.c_str());
			sz[1023] = 0;
			system(sz);
			return false;
		}
#endif
	}
	return true;
}

void FUTestBed::RunTestSuite(FUTestSuite* testSuite)
{
	if (testSuite == NULL) return;

	bool testSuiteDone = false;
	testSuite->RunTest(*this, fileOut, testSuiteDone, (size_t) ~0);
	for (size_t j = 0; !testSuiteDone; ++j)
	{
		bool passed = testSuite->RunTest(*this, fileOut, testSuiteDone, j);
		if (testSuiteDone) break;
		if (passed) testPassed++;
		else testFailed++;
	}
}
