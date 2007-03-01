/*
    Copyright (C) 2005-2007 Feeling Software Inc.
    MIT License: http://www.opensource.org/licenses/mit-license.php
*/

#ifndef _FU_TESTBED_H_
#define _FU_TESTBED_H_

#ifndef _FU_LOG_FILE_H_
#include "FUtils/FULogFile.h"
#endif // _FU_LOG_FILE_H_

class FUTestSuite;

///////////////////////////////////////////////////////////////////////////////
// QTestBed class
//
class FUTestBed
{
private:
	size_t testPassed, testFailed;
	FULogFile fileOut;
	fstring filename;
	bool isVerbose;

public:
	FUTestBed(const fchar* filename, bool isVerbose);

	inline bool IsVerbose() const { return isVerbose; }
	FULogFile& GetLogFile() { return fileOut; }

	bool RunTestbed(FUTestSuite* headTestSuite);
	void RunTestSuite(FUTestSuite* testSuite);
};

///////////////////////////////////////////////////////////////////////////////
// QTestSuite class
//
class FUTestSuite
{
public:
	virtual ~FUTestSuite() {}
	virtual bool RunTest(FUTestBed& testBed, FULogFile& fileOut, bool& __testSuiteDone, size_t testIndex) = 0;
};

///////////////////////////////////////////////////////////////////////////////
// Helpful Macros
//
#if defined(_FU_ASSERT_H_) && defined(_DEBUG)
#define FailIf(a) \
	if ((a)) { \
		fileOut.WriteLine(__FILE__, __LINE__, " Test('%s') failed: %s.", szTestName, #a); \
		extern bool skipAsserts; \
		if (!skipAsserts) { \
			__DEBUGGER_BREAK; \
			skipAsserts = ignoreAssert; } \
		return false; }

#else // _FU_ASSERT_H_ && _DEBUG

#define FailIf(a) \
	if ((a)) { \
		fileOut.WriteLine(__FILE__, __LINE__, " Test('%s') failed: %s.", szTestName, #a); \
		return false; }

#endif // _FU_ASSERT_H_

#define PassIf(a) FailIf(!(a))

#define Fail { bool b = true; FailIf(b); }

///////////////////////////////////////////////////////////////////////////////
// TestSuite Generation Macros.
// Do use the following macros, instead of writing your own.
//
#ifdef ENABLE_TEST
#define TESTSUITE_START(suiteName) \
FUTestSuite* _test##suiteName; \
static class FUTestSuite##suiteName : public FUTestSuite \
{ \
public: \
	FUTestSuite##suiteName() : FUTestSuite() { _test##suiteName = this; } \
	virtual ~FUTestSuite##suiteName() {} \
	virtual bool RunTest(FUTestBed& testBed, FULogFile& fileOut, bool& __testSuiteDone, size_t testIndex) \
	{ \
		switch (testIndex) { \
			case ~0u : { \
				if (testBed.IsVerbose()) { \
					fileOut.WriteLine("Running %s...", #suiteName); \
				}

#define TESTSUITE_TEST(testIndex, testName) \
			} return true; \
		case testIndex: { \
			static const char* szTestName = #testName;

#define TESTSUITE_END \
			} return true; \
		default: { __testSuiteDone = true; return true; } \
		} \
		fileOut.WriteLine(__FILE__, __LINE__, " Test suite implementation error."); \
		return false; \
	} \
} __testSuite;

#define RUN_TESTSUITE(suiteName) \
	extern FUTestSuite* _test##suiteName; \
	testBed.RunTestSuite(_test##suiteName);

#define RUN_TESTBED(testBedObject, szTestSuiteHead, testPassed) { \
	extern FUTestSuite* _test##szTestSuiteHead; \
	testPassed = testBedObject.RunTestbed(_test##szTestSuiteHead); }

#else // ENABLE_TEST

// The code will still be compiled, but the linker should take it out.
#define TESTSUITE_START(suiteName) \
	extern FUTestSuite* _test##suiteName = NULL; \
	inline bool __testCode##suiteName(FULogFile& fileOut, const char* szTestName) { { fileOut; szTestName;
#define TESTSUITE_TEST(testIndex, testName) } {
#define TESTSUITE_END } return true; }
#define RUN_TESTSUITE(suiteName)
#define RUN_TESTBED(testBedObject, szTestSuiteHead, testPassed) testPassed = true;

#endif

#endif // _FU_TESTBED_H_
