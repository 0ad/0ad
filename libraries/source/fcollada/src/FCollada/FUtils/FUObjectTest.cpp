/*
	Copyright (C) 2005-2007 Feeling Software Inc.
	Portions of the code are:
	Copyright (C) 2005-2007 Sony Computer Entertainment America
	
	MIT License: http://www.opensource.org/licenses/mit-license.php
*/

#include "StdAfx.h"
#include "FUTestBed.h"

///////////////////////////////////////////////////////////////////////////////
// Declare a few object and containers to be used by the tests belows
class FUTObject1 : public FUTrackable
{
	DeclareObjectType(FUTrackable);
public:
	FUTObject1(FUObjectContainer<FUTrackable>* container) { container->push_back(this); }
};
ImplementObjectType(FUTObject1);

class FUTObject2 : public FUTrackable
{
	DeclareObjectType(FUTrackable);
public:
	FUTObject2(FUObjectContainer<FUTrackable>* container) { container->push_back(this); }
};
ImplementObjectType(FUTObject2);

class FUTObject1Up : public FUTObject1
{
	DeclareObjectType(FUTObject1);
public:
	FUTObject1Up(FUObjectContainer<FUTrackable>* container) : FUTObject1(container) {}
};
ImplementObjectType(FUTObject1Up);

///////////////////////////////////////////////////////////////////////////////
class FUTSimple1 : public FUTrackable
{
	DeclareObjectType(FUTrackable);
};
class FUTSimple2 : public FUTSimple1
{
	DeclareObjectType(FUTSimple1);
};
class FUTSimple3 : public FUTSimple2
{
	DeclareObjectType(FUTSimple2);
};
class FUTSimple4 : public FUTSimple2
{
	DeclareObjectType(FUTSimple2);
};
ImplementObjectType(FUTSimple1);
ImplementObjectType(FUTSimple2);
ImplementObjectType(FUTSimple3);
ImplementObjectType(FUTSimple4);

///////////////////////////////////////////////////////////////////////////////
TESTSUITE_START(FUObject)

TESTSUITE_TEST(0, SimpleTracking)
	FUObjectContainer<FUTrackable> container;
	FUTObject1* obj1 = new FUTObject1(&container);
	FUTObject2* obj2 = new FUTObject2(&container);
	PassIf(obj1 != NULL && obj2 != NULL);
	PassIf(container.contains(obj1));
	PassIf(container.contains(obj2));
	delete obj1;
	PassIf(container.contains(obj2));
	FailIf(container.contains(obj1));
	delete obj2;
	PassIf(container.empty());
	obj2 = NULL; obj1 = NULL;

	// Verify that non-tracked objects are acceptable
	FUTrackable* obj3 = new FUTrackable();
	PassIf(container.empty());
	SAFE_DELETE(obj3);

TESTSUITE_TEST(1, RTTI)
	FUObjectContainer<FUTrackable> container;
	FUTObject1* obj1 = new FUTObject1(&container);
	FUTObject2* obj2 = new FUTObject2(&container);
	const FUObjectType& type1 = obj1->GetObjectType();
	const FUObjectType& type2 = obj2->GetObjectType();
	FailIf(type1 == type2);
	PassIf(type1 == FUTObject1::GetClassType());
	PassIf(type2 == FUTObject2::GetClassType());

	// Check out up-class RTTI
	FUTObject1Up* up = new FUTObject1Up(&container);
	PassIf(up->GetObjectType() == FUTObject1Up::GetClassType());
	FailIf(up->GetObjectType() == FUTObject1::GetClassType());
	FailIf(up->GetObjectType() == FUTObject2::GetClassType());
	PassIf(up->GetObjectType().Includes(FUTObject1::GetClassType()));
	PassIf(up->GetObjectType().Includes(obj1->GetObjectType()));
	PassIf(up->GetObjectType().GetParent() == FUTObject1::GetClassType());
	SAFE_DELETE(obj1);
	SAFE_DELETE(obj2);
	SAFE_DELETE(up);

TESTSUITE_TEST(2, MultiTracker)
	// Create one object and have multiple trackers tracking it.
	FUTrackedList<> container1;
	FUTrackedList<> container2;
	FUTrackedList<> container3;

	FUTrackable* o = new FUTrackable();
	container1.push_back(o);
	container2.push_back(o);
	container3.push_back(o);
	PassIf(!container1.empty());
	PassIf(!container2.empty());
	PassIf(!container3.empty());
	
	// Now, release the object and verify that all the trackers have been informed
	delete o;
	PassIf(container1.empty());
	PassIf(container2.empty());
	PassIf(container3.empty());

// Test was taken out, the feature was removed because it simply wasn't used.
// TESTSUITE_TEST(3, MultiContainer)

TESTSUITE_TEST(3, ContainedObjectPointer)
	// Create a couple of objects and have multiple containers tracking them.
	FUObjectContainer<FUTrackable>* container = new FUObjectContainer<FUTrackable>();
	FUTrackedPtr<> smartPointer;
	FUTrackable* obj = container->Add();
	smartPointer = obj;
	FailIf(smartPointer != obj);
	PassIf(smartPointer == obj);
	PassIf(obj == smartPointer);
	FailIf(obj != smartPointer);
	PassIf(smartPointer->GetObjectType() == FUTrackable::GetClassType());
	PassIf((*smartPointer).GetObjectType() == FUTrackable::GetClassType());

	// Delete the container and verify that the pointer got updated.
	delete container;
	PassIf(obj != smartPointer);
	PassIf(smartPointer == NULL);
	FailIf(smartPointer == obj);

TESTSUITE_TEST(4, ObjectReference)
	FUTrackedPtr<>* smartPointer1 = new FUTrackedPtr<>();
	FUTrackedPtr<>* smartPointer2 = new FUTrackedPtr<>();
	FUObjectRef<FUTrackable>* smartReference = new FUObjectRef<FUTrackable>();
	FUTrackable* testObj = new FUTrackable();

	// Assign the test object and verify that releasing the pointer doesn't affect
	// the other pointer or the reference.
	*smartPointer1 = testObj;
	*smartPointer2 = testObj;
	*smartReference = testObj;
	SAFE_DELETE(smartPointer1);
	PassIf(*smartReference == testObj);
	PassIf(*smartPointer2 == testObj);

	// Verify that when the reference is assigned something else: the pointers
	// get cleared properly.
	smartPointer1 = new FUTrackedPtr<>(testObj);
	*smartReference = new FUTrackable();
	PassIf(*smartPointer1 == NULL);
	PassIf(*smartPointer2 == NULL);
	PassIf(*smartReference != NULL);

	// Verify that when the reference is deleted, the pointer do get cleared.
	*smartPointer1 = *smartReference;
	*smartPointer2 = *smartReference;
	PassIf(*smartPointer1 != NULL);
	PassIf(*smartPointer2 != NULL);
	PassIf(*smartPointer1 == *smartPointer2);
	SAFE_DELETE(smartReference);
	PassIf(*smartPointer1 == NULL);
	PassIf(*smartPointer2 == NULL);

	SAFE_DELETE(smartPointer1);
	SAFE_DELETE(smartPointer2);

TESTSUITE_TEST(5, ObjectContainerErase)
	// Create a couple of objects and have multiple containers tracking them.
	FUTrackedList<> container1, container2;
	FUTrackable* o = new FUTrackable();
	container1.push_back(o);
	container2.push_back(o);
	PassIf(!container1.empty() && !container2.empty());
	container1.erase(o);
	PassIf(container1.empty() && !container2.empty());
	container2.pop_back();
	PassIf(container1.empty() && container2.empty());
	SAFE_DELETE(o);
	PassIf(container1.empty() && container2.empty());

TESTSUITE_TEST(6, ObjectContainerIteration)
	// Create one container and iterate over two items to verify
	// that the underneath vector works after recent changes.
	FUTrackedList<> container;
	FUTrackable* o1 = new FUTrackable();
	FUTrackable* o2 = new FUTrackable();
	container.push_back(o1);
	container.push_back(o2);

	PassIf(container[0] == o1);
	PassIf(container[1] == o2);
	FUTrackedList<>::iterator it = container.begin();
	PassIf((*it) == o1);
	++it;
	PassIf((*it) == o2);
	SAFE_DELETE(o1);
	SAFE_DELETE(o2);

TESTSUITE_TEST(7, Shortcuts)
	FUTSimple1 t1;
	FUTSimple3 t3;
	FUTSimple4 t4;

	PassIf(t1.IsType(FUTSimple1::GetClassType()));
	PassIf(t1.HasType(FUTrackable::GetClassType()));
	FailIf(t1.IsType(FUTrackable::GetClassType()));

	PassIf(t3.IsType(FUTSimple3::GetClassType()));
	FailIf(t3.IsType(FUTSimple2::GetClassType()));
	PassIf(t3.HasType(FUTSimple2::GetClassType()));
	PassIf(t3.HasType(FUTSimple1::GetClassType()));
	PassIf(t3.HasType(FUTrackable::GetClassType()));

	PassIf(DynamicCast<FUTSimple2>(&t3) == &t3);
	PassIf(DynamicCast<FUTSimple3>(&t4) == NULL);

TESTSUITE_END
