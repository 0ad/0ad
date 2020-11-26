/*
	Copyright (C) 2005-2007 Feeling Software Inc.
	Portions of the code are:
	Copyright (C) 2005-2007 Sony Computer Entertainment America
	
	MIT License: http://www.opensource.org/licenses/mit-license.php
*/

#include "StdAfx.h"
#include "FUtils/FUParameter.h"
#include "FCDocument/FCDocument.h"
#include "FCDocument/FCDAnimated.h"
#include "FCDocument/FCDAnimation.h"
#include "FCDocument/FCDAnimationChannel.h"
#include "FCDocument/FCDAnimationCurve.h"
#include "FCDocument/FCDEntity.h"
#include "FCDocument/FCDLibrary.h"
#include "FCDocument/FCDParameterAnimatable.h"

class FCTestOutsideParameter : public FCDObject
{
public:
	DeclareParameter(float, FUParameterQualifiers::SIMPLE, test1, FC("A large parameter name!"));
	DeclareParameterAnimatable(FMVector3, FUParameterQualifiers::VECTOR, test2, FC("An animatable parameter"));
	DeclareParameterAnimatable(FMVector3, FUParameterQualifiers::COLOR, test3, FC("A complex animatable parameter"));
	DeclareParameterPtr(FCDObject, test4, FC("A simple object pointer"));
	DeclareParameterRef(FCDObject, test5, FC("An object reference!"));
	DeclareParameterList(Float, test6, FC("A float list parameter."));
	DeclareParameterTrackList(FCDObject, test7, FC("An object tracker list."));
	DeclareParameterContainer(FCDObject, test8, FC("An object container."));
	DeclareParameterListAnimatable(FMVector3, FUParameterQualifiers::COLOR, test9, FC("An animatable color list."));

	FCTestOutsideParameter(FCDocument* document)
	:	FCDObject(document)
	,	InitializeParameter(test1, 0.0f)
	,	InitializeParameterAnimatableNoArg(test2)
	,	InitializeParameterAnimatable(test3, FMVector3::XAxis)
	,	InitializeParameter(test4, NULL)
	,	InitializeParameterNoArg(test5)
	,	InitializeParameterNoArg(test6)
	,	InitializeParameterNoArg(test7)
	,	InitializeParameterNoArg(test8)
	,	InitializeParameterAnimatableNoArg(test9)
	{}

	virtual ~FCTestOutsideParameter() {}
};

ImplementParameterObject(FCTestOutsideParameter, FCDObject, test4, new FCDObject(parent->GetDocument()));
ImplementParameterObject(FCTestOutsideParameter, FCDObject, test5, new FCDObject(parent->GetDocument()));
ImplementParameterObject(FCTestOutsideParameter, FCDObject, test7, new FCDObject(parent->GetDocument()));
ImplementParameterObject(FCTestOutsideParameter, FCDObject, test8, new FCDObject(parent->GetDocument()));

TESTSUITE_START(FCDParameter)

TESTSUITE_TEST(0, Simple)
	FUObjectRef<FCDocument> document = FCollada::NewTopDocument();
	FUObjectRef<FCDEntity> entity = new FCDEntity(document);
	entity->SetNote(FC("Noting down."));
	PassIf(IsEquivalent(entity->GetNote(), FC("Noting down.")));

TESTSUITE_TEST(1, Functionality)
	FUObjectRef<FCDocument> document = FCollada::NewTopDocument();
	FUObjectRef<FCTestOutsideParameter> parameter = new FCTestOutsideParameter(document);

	// Simple float parameter
	PassIf(parameter->test1 == 0.0f);
	parameter->test1 = 2.1f;
	PassIf(parameter->test1 == 2.1f);

	// Animatable 3D vector parameter.
	parameter->test2 = FMVector3::One;
	PassIf(parameter->test2 == FMVector3::One);
	PassIf(parameter->test2 - FMVector3::One == FMVector3::Zero);
	PassIf(parameter->test2.GetAnimated() != NULL);
	PassIf(parameter->test2.GetAnimated()->GetValueCount() == 3);

	// Object parameters
	parameter->test4 = parameter->test5;
	PassIf(parameter->test4 == parameter->test5);
	parameter->test5 = NULL;
	PassIf(parameter->test5 == NULL);
	parameter->test4 = new FCDObject(document);
	PassIf(parameter->test4 != NULL);
	PassIf(parameter->test4->GetTrackerCount() == 1);
	parameter->test5 = parameter->test4;
	PassIf(parameter->test5 != NULL);
	PassIf(parameter->test5->GetTrackerCount() == 1);
	parameter->test5 = NULL;
	PassIf(parameter->test4 == NULL);

	// Primitive list parameter
	size_t count = parameter->test6.size();
	PassIf(count == 0 && parameter->test6.empty());
	parameter->test6.push_back(0.52f);
	PassIf(parameter->test6.size() == 1);
	parameter->test6.clear();
	PassIf(parameter->test6.size() == 0);
	parameter->test6.push_back(0.45f);
	PassIf(parameter->test6.size() == 1);
	parameter->test6.erase(0, 1);
	PassIf(parameter->test6.size() == 0);

	// Tracked object list parameter
	FUTrackedPtr<FCDObject> testObject = new FCDObject(document);
	count = parameter->test7.size();
	PassIf(count == 0 && parameter->test7.empty());
	parameter->test7.push_back(testObject);
	PassIf(parameter->test7.size() == 1);
	parameter->test7.clear();
	PassIf(testObject != NULL);
	PassIf(parameter->test7.size() == 0);
	parameter->test7.push_back(testObject);
	PassIf(parameter->test7.size() == 1);
	parameter->test7.erase(0, 1);
	PassIf(parameter->test7.size() == 0);
	testObject->Release();
	PassIf(testObject == NULL);

	// Object container parameter
	testObject = new FCDObject(document);
	count = parameter->test8.size();
	PassIf(count == 0 && parameter->test8.empty());
	parameter->test8.push_back(testObject);
	PassIf(parameter->test8.size() == 1);
	parameter->test8.clear();
	PassIf(testObject == NULL); // this is a container, so testObject should have been released!
	PassIf(parameter->test8.size() == 0);
	parameter->test8.push_back(new FCDObject(document));
	PassIf(parameter->test8.size() == 1);
	parameter->test8.erase(0, 1);
	PassIf(parameter->test8.size() == 0);

TESTSUITE_TEST(2, AnimatedListParameter)
	FUObjectRef<FCDocument> document = FCollada::NewTopDocument();
	FUObjectRef<FCTestOutsideParameter> parameter = new FCTestOutsideParameter(document);
	FCDAnimation* animation = document->GetAnimationLibrary()->AddEntity();
	FCDAnimationChannel* channel = animation->AddChannel();

	// Animated list parameter.
	// Simple operations
#define TESTP parameter->test9
	PassIf(TESTP.size() == 0);
	TESTP.push_back(FMVector3::XAxis);
	TESTP.push_back(FMVector3::YAxis);
	TESTP.push_back(FMVector3::ZAxis);
	PassIf(TESTP.size() == 3);
	FailIf(TESTP.IsAnimated());
	FailIf(TESTP.IsAnimated(0));
	PassIf(TESTP.GetAnimatedValues().empty());
	PassIf(TESTP.GetAnimated(2) != NULL);
	FailIf(TESTP.GetAnimatedValues().empty());
	FailIf(TESTP.IsAnimated(2));
	TESTP.push_front(FMVector3::XAxis);
	TESTP.push_front(FMVector3::YAxis);
	TESTP.push_front(FMVector3::ZAxis);
	PassIf(TESTP.GetAnimated(2) != NULL);
	PassIf(TESTP.at(1) == FMVector3::YAxis);
	PassIf(TESTP.at(4) == FMVector3::YAxis);
	PassIf(!TESTP.IsAnimated(5));
	PassIf(!TESTP.IsAnimated(4));
	PassIf(TESTP.GetAnimated(2)->GetArrayElement() == 2);

	// List insertion tests.
	TESTP.GetAnimated(2)->AddCurve(0, channel->AddCurve());
	PassIf(TESTP.IsAnimated());
	PassIf(TESTP.IsAnimated(2));
	PassIf(!TESTP.IsAnimated(1));
	PassIf(!TESTP.IsAnimated(3));
	TESTP.insert(2, FMVector3::XAxis); // should move the curve up to index 3!
	PassIf(TESTP.IsAnimated(3));
	PassIf(!TESTP.IsAnimated(2));
	PassIf(!TESTP.IsAnimated(4));
	PassIf(TESTP.GetAnimated(3)->GetArrayElement() == 3);
	TESTP.insert(5, FMVector3::YAxis); // no movement of the curve.
	PassIf(TESTP.IsAnimated(3));
	PassIf(!TESTP.IsAnimated(2));
	PassIf(!TESTP.IsAnimated(4));
	PassIf(TESTP.GetAnimated(4)->GetArrayElement() == 4);

	// List removal tests.
	TESTP.erase(0); // should move the curve back to index 2.
	PassIf(TESTP.IsAnimated(2));
	PassIf(!TESTP.IsAnimated(1));
	PassIf(!TESTP.IsAnimated(3));
	PassIf(TESTP.GetAnimated(2)->GetArrayElement() == 2);
	TESTP.erase(0, 4); // nothing should be animated anymore.
	FailIf(TESTP.IsAnimated());

	// List resizing tests.
	TESTP.clear();
	FailIf(TESTP.IsAnimated());
	TESTP.resize(4);
	FailIf(TESTP.IsAnimated());
	TESTP.GetAnimated(1)->AddCurve(0, channel->AddCurve());
	PassIf(TESTP.IsAnimated());
	PassIf(TESTP.IsAnimated(1));
	TESTP.resize(6);
	PassIf(TESTP.IsAnimated());
	PassIf(TESTP.IsAnimated(1));
	TESTP.resize(1);
	FailIf(TESTP.IsAnimated());

TESTSUITE_END
