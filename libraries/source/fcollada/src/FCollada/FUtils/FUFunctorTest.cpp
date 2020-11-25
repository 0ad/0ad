/*
	Copyright (C) 2005-2007 Feeling Software Inc.
	Portions of the code are:
	Copyright (C) 2005-2007 Sony Computer Entertainment America
	
	MIT License: http://www.opensource.org/licenses/mit-license.php
*/

#include "StdAfx.h"
#include "FUTestBed.h"
#include "FUFunctor.h"

///////////////////////////////////////////////////////////////////////////////
class FUTestClass
{
public:
	long iTestValue[3];

	FUTestClass()
	{
		for (long i = 0; i < 3; ++i)
		{
			iTestValue[i] = 0;
		}
	}

	long GetTestValue(long i)
	{ return iTestValue[i]; }

	void TestFunction0()
	{ iTestValue[0]++; }

	void TestFunction1(long i)
	{ iTestValue[1] += i; }

	void TestFunction2(long i, const long j)
	{ iTestValue[j] += i; }

	bool VerifyValues(long v0, long v1, long v2)
	{
		return v0 == iTestValue[0] && v1 == iTestValue[1] && v2 == iTestValue[2];
	}
};

class FUTestClass2
{
public:
	long iTestValue[3];

	FUTestClass2()
	{
		for (long i = 0; i < 3; ++i)
		{
			iTestValue[i] = 0;
		}
	}

	void TestFunction0()
	{ iTestValue[0]++; }

	void TestFunction1(long i)
	{ iTestValue[1] += i; }

	void TestFunction2(long i, const long j)
	{ iTestValue[j] += i; }

	bool VerifyValues(long v0, long v1, long v2)
	{
		return v0 == iTestValue[0] && v1 == iTestValue[1] && v2 == iTestValue[2];
	}
};

///////////////////////////////////////////////////////////////////////////////
TESTSUITE_START(FUFunctor)

TESTSUITE_TEST(0, DirectUtilisation)
	FUTestClass sTestClass;
	FailIf(sTestClass.iTestValue[0] != 0);
	FUFunctor0<FUTestClass, void> sFunctor1(&sTestClass, &FUTestClass::TestFunction0);
	FailIf(sTestClass.iTestValue[0] != 0);
	sFunctor1();
	FailIf(sTestClass.iTestValue[0] != 1);

	FailIf(sTestClass.iTestValue[1] != 0);
	FUFunctor1<FUTestClass, long , void> sFunctor2(&sTestClass, &FUTestClass::TestFunction1);
	FailIf(sTestClass.iTestValue[1] != 0);
	sFunctor2(2);
	FailIf(sTestClass.iTestValue[1] != 2);

	FailIf(sTestClass.iTestValue[2] != 0);
	FUFunctor2<FUTestClass, long , const long, void> sFunctor3(&sTestClass, &FUTestClass::TestFunction2);
	FailIf(sTestClass.iTestValue[2] != 0);
	sFunctor3(4, 2);
	FailIf(sTestClass.iTestValue[2] != 4);

	FUFunctor1<FUTestClass, long, long> sFunctor4(&sTestClass, &FUTestClass::GetTestValue);
	FailIf(sFunctor4(1) != sTestClass.iTestValue[1]);
	FailIf(sFunctor4(2) != sTestClass.iTestValue[2]);

TESTSUITE_TEST(1, QueuedUsage)
	typedef IFunctor0<void> TIntFunctor0;
	fm::pvector<TIntFunctor0> g_xFunctors0;

	typedef IFunctor1<long, void> TIntFunctor1;
	fm::pvector<TIntFunctor1> g_xFunctors1;

	typedef IFunctor2<long, const long, void> TIntFunctor2;
	fm::pvector<TIntFunctor2> g_xFunctors2;

	FUTestClass sTestClass;
	FUTestClass2 sTestClass2;

	FailIf(sTestClass.iTestValue[0] != 0 || sTestClass2.iTestValue[0] != 0);
	FUFunctor0<FUTestClass, void> sFunctor1(&sTestClass, &FUTestClass::TestFunction0);
	FUFunctor0<FUTestClass2, void> sFunctor2(&sTestClass2, &FUTestClass2::TestFunction0);
	g_xFunctors0.push_back(&sFunctor1);
	g_xFunctors0.push_back(&sFunctor2);
	fm::pvector<TIntFunctor0>::iterator it;
	for (it = g_xFunctors0.begin(); it != g_xFunctors0.end(); ++it)
	{
		(*(*it))();
	}
	FailIf(sTestClass.iTestValue[0] != 1 || sTestClass2.iTestValue[0] != 1);

	FailIf(sTestClass.iTestValue[1] != 0 || sTestClass2.iTestValue[1] != 0);
	FUFunctor1<FUTestClass, long, void> sFunctor3(&sTestClass, &FUTestClass::TestFunction1);
	FUFunctor1<FUTestClass2, long, void> sFunctor4(&sTestClass2, &FUTestClass2::TestFunction1);
	g_xFunctors1.push_back(&sFunctor3);
	g_xFunctors1.push_back(&sFunctor4);
	fm::pvector<TIntFunctor1>::iterator it1;
	for (it1 = g_xFunctors1.begin(); it1 != g_xFunctors1.end(); ++it1)
	{
		(*(*it1))(2);
	}
	FailIf(sTestClass.iTestValue[1] != 2 || sTestClass2.iTestValue[1] != 2);

	FailIf(sTestClass.iTestValue[2] != 0 || sTestClass2.iTestValue[2] != 0);
	FUFunctor2<FUTestClass, long, const long, void> sFunctor5(&sTestClass, &FUTestClass::TestFunction2);
	FUFunctor2<FUTestClass2, long, const long, void> sFunctor6(&sTestClass2, &FUTestClass2::TestFunction2);
	g_xFunctors2.push_back(&sFunctor5);
	g_xFunctors2.push_back(&sFunctor6);
	fm::pvector<TIntFunctor2>::iterator it2;
	for (it2 = g_xFunctors2.begin(); it2 != g_xFunctors2.end(); ++it2)
	{
		(*(*it2))(4, 2);
	}
	FailIf(sTestClass.iTestValue[2] != 4 || sTestClass2.iTestValue[2] != 4);

TESTSUITE_END
