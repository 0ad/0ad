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

#include "../../../source/third_party/encryption/tests/test_sha.h"

static TestEncryptionSha256 suite_TestEncryptionSha256;

static CxxTest::List Tests_TestEncryptionSha256 = { 0, 0 };
CxxTest::StaticSuiteDescription suiteDescription_TestEncryptionSha256( "../../../source/third_party/encryption/tests/test_sha.h", 22, "TestEncryptionSha256", suite_TestEncryptionSha256, Tests_TestEncryptionSha256 );

static class TestDescription_TestEncryptionSha256_test_sha256 : public CxxTest::RealTestDescription {
public:
 TestDescription_TestEncryptionSha256_test_sha256() : CxxTest::RealTestDescription( Tests_TestEncryptionSha256, suiteDescription_TestEncryptionSha256, 25, "test_sha256" ) {}
 void runTest() { suite_TestEncryptionSha256.test_sha256(); }
} testDescription_TestEncryptionSha256_test_sha256;

