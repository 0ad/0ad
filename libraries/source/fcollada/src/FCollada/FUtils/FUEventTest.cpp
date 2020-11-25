/*
	Copyright (C) 2005-2007 Feeling Software Inc.
	Portions of the code are:
	Copyright (C) 2005-2007 Sony Computer Entertainment America
	
	MIT License: http://www.opensource.org/licenses/mit-license.php
*/

#include "StdAfx.h"
#include "FUTestBed.h"
#include "FUEvent.h"

///////////////////////////////////////////////////////////////////////////////
class FUTCaller
{
public:
	FUEvent0* event0;
	FUEvent2<long, long>* event2;

	FUTCaller()
	{
		event0 = new FUEvent0();
		event2 = new FUEvent2<long, long>();
	}

	~FUTCaller()
	{
		SAFE_DELETE(event0);
		SAFE_DELETE(event2);
	}
};

class FUTCallee
{
public:
	bool isCallback0_1Called;
	bool isCallback0_2Called;
	bool isCallback2Called;

	long callback2Data1;
	long callback2Data2;

	FUTCallee()
	{
		isCallback0_1Called = false;
		isCallback0_2Called = false;
		isCallback2Called = false;

		callback2Data1 = -1;
		callback2Data2 = -1;
	}

	void Callback0_1()
	{
		isCallback0_1Called = true;
	}

	void Callback0_2()
	{
		isCallback0_2Called = true;
	}

	void Callback2(long data1, long data2)
	{
		isCallback2Called = true;
		callback2Data1 = data1;
		callback2Data2 = data2;
	}
};

class FUTDumbCaller
{
public:
	FUEvent0 event0;
	FUEvent1<FUTDumbCaller*> event1;
	FUEvent2<FUTDumbCaller*, long> event2;
};

// Test release of event handlers while an event is happening.
class FUTAntisocialCallee
{
public:
	FUTDumbCaller* caller;
	bool c0, c1, c2;	// have the callbacks been called?

	FUTAntisocialCallee()
	{
		caller = NULL;
		c0 = c1 = c2 = false;
	}

	void Callback0()
	{
		caller->event0.ReleaseHandler(this, &FUTAntisocialCallee::Callback0);
		c0 = true;
	}

	void Callback1(FUTDumbCaller* caller2)
	{
		caller2->event1.ReleaseHandler(this, &FUTAntisocialCallee::Callback1);
		c1 = true; 
	}

	void Callback2(FUTDumbCaller* caller2, long /* data2 */)
	{
		caller2->event2.ReleaseHandler(this, &FUTAntisocialCallee::Callback2);
		c2 = true;
	}

	bool isSuccess()
	{
		return c0 && c1 && c2;
	}
};


///////////////////////////////////////////////////////////////////////////////
TESTSUITE_START(FUEvent)	

TESTSUITE_TEST(0, Callback0)
	FUTCaller caller;
	FUTCallee callee;
	FailIf(caller.event0->GetHandlerCount() != 0);
	caller.event0->InsertHandler(&callee, &FUTCallee::Callback0_1);
	FailIf(caller.event0->GetHandlerCount() != 1);

	(*(caller.event0))();

	FailIf(!callee.isCallback0_1Called);
	caller.event0->ReleaseHandler(&callee, &FUTCallee::Callback0_1);

TESTSUITE_TEST(1, Callback2)
	FUTCaller caller;
	FUTCallee callee;
	caller.event2->InsertHandler(&callee, &FUTCallee::Callback2);

	(*(caller.event2))(72, 55);
	FailIf(!callee.isCallback2Called);
	FailIf(callee.callback2Data1 != 72);
	FailIf(callee.callback2Data2 != 55);

	callee.isCallback2Called = false;
	(*(caller.event2))(44, 79);
	FailIf(!callee.isCallback2Called);
	FailIf(callee.callback2Data1 != 44);
	FailIf(callee.callback2Data2 != 79);
	caller.event2->ReleaseHandler(&callee, &FUTCallee::Callback2);

TESTSUITE_TEST(2, MultipleCallback0)
	FUTCaller caller;
	FUTCallee callee;
	caller.event0->InsertHandler(&callee, &FUTCallee::Callback0_1);
	caller.event0->InsertHandler(&callee, &FUTCallee::Callback0_2);
	FailIf(caller.event0->GetHandlerCount() != 2);

	callee.isCallback0_1Called = false;
	callee.isCallback0_2Called = false;
	(*(caller.event0))();
	FailIf(!callee.isCallback0_1Called);
	FailIf(!callee.isCallback0_2Called);

	FailIf(caller.event0->GetHandlerCount() != 2);
	caller.event0->ReleaseHandler(&callee, &FUTCallee::Callback0_1);
	FailIf(caller.event0->GetHandlerCount() != 1);
	caller.event0->ReleaseHandler(&callee, &FUTCallee::Callback0_2);
	FailIf(caller.event0->GetHandlerCount() != 0);

TESTSUITE_TEST(3, CallbacksThatReleaseImmediately)
	FUTDumbCaller caller;
	FUTAntisocialCallee callee;
	callee.caller = &caller;

	caller.event0.InsertHandler(&callee, &FUTAntisocialCallee::Callback0);
	caller.event1.InsertHandler(&callee, &FUTAntisocialCallee::Callback1);
	caller.event2.InsertHandler(&callee, &FUTAntisocialCallee::Callback2);
	
	FailIf(caller.event0.GetHandlerCount() != 1);
	FailIf(caller.event1.GetHandlerCount() != 1);
	FailIf(caller.event2.GetHandlerCount() != 1);
	
	// Test the callbacks
	(caller.event0)();
	(caller.event1)(&caller);
	(caller.event2)(&caller, 42);

	FailIf(!callee.isSuccess());
	FailIf(caller.event0.GetHandlerCount() != 0);
	FailIf(caller.event1.GetHandlerCount() != 0);
	FailIf(caller.event2.GetHandlerCount() != 0);

TESTSUITE_END
