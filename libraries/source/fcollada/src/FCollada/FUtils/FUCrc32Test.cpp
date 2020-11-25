/*
	Copyright (C) 2005-2007 Feeling Software Inc.
	Portions of the code are:
	Copyright (C) 2005-2007 Sony Computer Entertainment America
	
	MIT License: http://www.opensource.org/licenses/mit-license.php
*/

#include "StdAfx.h"
#include "FUCrc32.h"
#include "FUTestBed.h"
using namespace FUCrc32;

////////////////////////////////////////////////////////////////////////
static const char* input[10] = 
{
	"Test1",
	"Test2",
	"Nice Test",
	"Global String",
	"Trying Line",
	"Reticulating Splines",
	"Youpi jappent",
	"Coucher, Youpi!",
	"Textures/Hero.dds",
	"Aberration"
};

static crc32 answers[10] =
{ 0x4B73F3E6, 0xD27AA25C, 0x45529E10, 0xDFDD49F8, 0x9D23677B, 0x571C0C0A, 0x37AED322, 0x8C460421, 0x3E310AE8, 0x0A17CC73 };

TESTSUITE_START(FUCrc32)

TESTSUITE_TEST(0, Uniqueness)
	// Check the UTF-8 uniqueness
	fm::vector<crc32> answerList;
	for (size_t i = 0; i < 10; ++i)
	{
		crc32 answer = CRC32(input[i]);
		FailIf(answerList.contains(answer));
		answerList.push_back(answer);
	}

	// Check the Unicode uniqueness
	answerList.clear();
	for (size_t i = 0; i < 10; ++i)
	{
		crc32 answer = CRC32(TO_FSTRING(input[i]));
		FailIf(answerList.contains(answer));
		answerList.push_back(answer);
	}

TESTSUITE_TEST(1, BackwardCompatibility)
	for (size_t i = 0; i < 10; ++i)
	{
		crc32 answer = CRC32(input[i]);
		FailIf(answer != answers[i]);
	}

TESTSUITE_END
