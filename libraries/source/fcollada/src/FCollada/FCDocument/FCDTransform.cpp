/*
	Copyright (C) 2005-2007 Feeling Software Inc.
	Portions of the code are:
	Copyright (C) 2005-2007 Sony Computer Entertainment America
	
	MIT License: http://www.opensource.org/licenses/mit-license.php
*/
/*
	Based on the FS Import classes:
	Copyright (C) 2005-2006 Feeling Software Inc
	Copyright (C) 2005-2006 Autodesk Media Entertainment
	MIT License: http://www.opensource.org/licenses/mit-license.php
*/

#include "StdAfx.h"
#include "FCDocument.h"
#include "FCDSceneNode.h"
#include "FCDTransform.h"
#include <FMath/FMSkew.h>
#include <FMath/FMAngleAxis.h>
#include <FMath/FMLookAt.h>

//
// FCDTransform
//

ImplementObjectType(FCDTransform);

FCDTransform::FCDTransform(FCDocument* document, FCDSceneNode* _parent)
:	FCDObject(document), parent(_parent)
,	InitializeParameterNoArg(sid)
{}

FCDTransform::~FCDTransform()
{
	parent = NULL;
}

bool FCDTransform::IsInverse(const FCDTransform* UNUSED(transform)) const
{
	return false;
}

void FCDTransform::SetSubId(const fm::string& subId)
{
	sid = FCDObjectWithId::CleanSubId(subId); 
	SetDirtyFlag();
}

void FCDTransform::SetValueChange()
{ 
	SetValueChangedFlag(); 
	// parent == NULL is a valid value in ColladaPhysics.
	if (parent != NULL) parent->SetTransformsDirtyFlag();
}

//
// FCDTTranslation
//

ImplementObjectType(FCDTTranslation);

FCDTTranslation::FCDTTranslation(FCDocument* document, FCDSceneNode* parent)
:	FCDTransform(document, parent)
,	InitializeParameterAnimatable(translation, FMVector3::Zero)
{
}

FCDTTranslation::~FCDTTranslation() {}

FCDTransform* FCDTTranslation::Clone(FCDTransform* _clone) const
{
	FCDTTranslation* clone = NULL;
	if (_clone == NULL) clone = new FCDTTranslation(const_cast<FCDocument*>(GetDocument()), const_cast<FCDSceneNode*>(GetParent()));
	else if (!_clone->HasType(FCDTTranslation::GetClassType())) return _clone;
	else clone = (FCDTTranslation*) _clone;

	clone->translation = translation;
	return clone;
}

FMMatrix44 FCDTTranslation::ToMatrix() const
{
	return FMMatrix44::TranslationMatrix(translation);
}

const FCDAnimated* FCDTTranslation::GetAnimated() const
{
	return translation.GetAnimated();
}

bool FCDTTranslation::IsAnimated() const
{
	return translation.IsAnimated();
}

bool FCDTTranslation::IsInverse(const FCDTransform* transform) const
{
	return transform->GetType() == FCDTransform::TRANSLATION
		&& IsEquivalent(-1.0f * translation, ((const FCDTTranslation*)transform)->translation);
}

//
// FCDTRotation
//

ImplementObjectType(FCDTRotation);

FCDTRotation::FCDTRotation(FCDocument* document, FCDSceneNode* parent)
:	FCDTransform(document, parent)
,	InitializeParameterAnimatable(angleAxis, FMAngleAxis(FMVector3::XAxis, 0.0f))
{
}

FCDTRotation::~FCDTRotation() {}

FCDTransform* FCDTRotation::Clone(FCDTransform* _clone) const
{
	FCDTRotation* clone = NULL;
	if (_clone == NULL) clone = new FCDTRotation(const_cast<FCDocument*>(GetDocument()), const_cast<FCDSceneNode*>(GetParent()));
	else if (!_clone->HasType(FCDTRotation::GetClassType())) return _clone;
	else clone = (FCDTRotation*) _clone;

	clone->angleAxis = angleAxis;
	return clone;
}

FMMatrix44 FCDTRotation::ToMatrix() const
{
	return FMMatrix44::AxisRotationMatrix(angleAxis->axis, FMath::DegToRad(angleAxis->angle));
}

const FCDAnimated* FCDTRotation::GetAnimated() const
{
	return angleAxis.GetAnimated();
}

bool FCDTRotation::IsAnimated() const
{
	return angleAxis.IsAnimated();
}

bool FCDTRotation::IsInverse(const FCDTransform* transform) const
{
	return transform->GetType() == FCDTransform::ROTATION
		&& ((IsEquivalent(angleAxis->axis, ((const FCDTRotation*)transform)->angleAxis->axis)
		&& IsEquivalent(-angleAxis->angle, ((const FCDTRotation*)transform)->angleAxis->angle))
		|| (IsEquivalent(-angleAxis->axis, ((const FCDTRotation*)transform)->angleAxis->axis)
		&& IsEquivalent(angleAxis->angle, ((const FCDTRotation*)transform)->angleAxis->angle)));
}

//
// FCDTScale
//

ImplementObjectType(FCDTScale);

FCDTScale::FCDTScale(FCDocument* document, FCDSceneNode* parent)
:	FCDTransform(document, parent)
,	InitializeParameterAnimatable(scale, FMVector3::One)
{
}

FCDTScale::~FCDTScale() {}

FCDTransform* FCDTScale::Clone(FCDTransform* _clone) const
{
	FCDTScale* clone = NULL;
	if (_clone == NULL) clone = new FCDTScale(const_cast<FCDocument*>(GetDocument()), const_cast<FCDSceneNode*>(GetParent()));
	else if (!_clone->HasType(FCDTScale::GetClassType())) return _clone;
	else clone = (FCDTScale*) _clone;

	clone->scale = scale;
	return clone;
}

FMMatrix44 FCDTScale::ToMatrix() const
{
	return FMMatrix44::ScaleMatrix(scale);
}

const FCDAnimated* FCDTScale::GetAnimated() const
{
	return scale.GetAnimated();
}

bool FCDTScale::IsAnimated() const
{
	return scale.IsAnimated();
}

//
// FCDTMatrix
//

ImplementObjectType(FCDTMatrix);

FCDTMatrix::FCDTMatrix(FCDocument* document, FCDSceneNode* parent)
:	FCDTransform(document, parent)
,	InitializeParameterAnimatable(transform, FMMatrix44::Identity)
{
}

FCDTMatrix::~FCDTMatrix() {}

FCDTransform* FCDTMatrix::Clone(FCDTransform* _clone) const
{
	FCDTMatrix* clone = NULL;
	if (_clone == NULL) clone = new FCDTMatrix(const_cast<FCDocument*>(GetDocument()), const_cast<FCDSceneNode*>(GetParent()));
	else if (!_clone->HasType(FCDTMatrix::GetClassType())) return _clone;
	else clone = (FCDTMatrix*) _clone;

	clone->transform = transform;
	return clone;
}

const FCDAnimated* FCDTMatrix::GetAnimated() const
{
	return transform.GetAnimated();
}

bool FCDTMatrix::IsAnimated() const
{
	return transform.IsAnimated();
}

//
// FCDTLookAt
//

ImplementObjectType(FCDTLookAt);

FCDTLookAt::FCDTLookAt(FCDocument* document, FCDSceneNode* parent)
:	FCDTransform(document, parent)
,	InitializeParameterAnimatable(lookAt, FMLookAt(FMVector3::Zero, -FMVector3::ZAxis, FMVector3::YAxis))
{
}

FCDTLookAt::~FCDTLookAt() {}

FCDTransform* FCDTLookAt::Clone(FCDTransform* _clone) const
{
	FCDTLookAt* clone = NULL;
	if (_clone == NULL) clone = new FCDTLookAt(const_cast<FCDocument*>(GetDocument()), const_cast<FCDSceneNode*>(GetParent()));
	else if (!_clone->HasType(FCDTLookAt::GetClassType())) return _clone;
	else clone = (FCDTLookAt*) _clone;

	clone->lookAt = lookAt;
	return clone;
}

FMMatrix44 FCDTLookAt::ToMatrix() const
{
	return FMMatrix44::LookAtMatrix(lookAt->position, lookAt->target, lookAt->up);
}

const FCDAnimated* FCDTLookAt::GetAnimated() const
{
	return lookAt.GetAnimated();
}

bool FCDTLookAt::IsAnimated() const
{
	return lookAt.IsAnimated();
}

//
// FCDTSkew
//

ImplementObjectType(FCDTSkew);

FCDTSkew::FCDTSkew(FCDocument* document, FCDSceneNode* parent)
:	FCDTransform(document, parent)
,	InitializeParameterAnimatable(skew, FMSkew(FMVector3::XAxis, FMVector3::YAxis, 0.0f))
{
}

FCDTSkew::~FCDTSkew()
{
}

FCDTransform* FCDTSkew::Clone(FCDTransform* _clone) const
{
	FCDTSkew* clone = NULL;
	if (_clone == NULL) clone = new FCDTSkew(const_cast<FCDocument*>(GetDocument()), const_cast<FCDSceneNode*>(GetParent()));
	else if (!_clone->HasType(FCDTSkew::GetClassType())) return _clone;
	else clone = (FCDTSkew*) _clone;

	clone->skew = skew;
	return clone;
}

FMMatrix44 FCDTSkew::ToMatrix() const
{
	float v[4][4];

	float s = tanf(FMath::DegToRad(skew->angle));

	for (int row = 0; row < 3; ++row)
	{
		for (int col = 0; col < 3; ++col)
		{
			v[col][row] = ((row == col) ? 1.0f : 0.0f) + s * skew->rotateAxis[col] * skew->aroundAxis[row];
		}
	}

	v[0][3] = v[1][3] = v[2][3] = 0.0f;
	v[3][0] = v[3][1] = v[3][2] = 0.0f;
	v[3][3] = 1.0f;

	return FMMatrix44((float*) v);
}

const FCDAnimated* FCDTSkew::GetAnimated() const
{
	return skew.GetAnimated();
}

bool FCDTSkew::IsAnimated() const
{
	// Only angle may be animated for now.
	return skew.IsAnimated();
}

// Creates a new COLLADA transform, given a transform type.
FCDTransform* FCDTFactory::CreateTransform(FCDocument* document, FCDSceneNode* parent, FCDTransform::Type type)
{
	switch (type)
	{
	case FCDTransform::TRANSLATION: return new FCDTTranslation(document, parent);
	case FCDTransform::ROTATION: return new FCDTRotation(document, parent);
	case FCDTransform::SCALE: return new FCDTScale(document, parent);
	case FCDTransform::SKEW: return new FCDTSkew(document, parent);
	case FCDTransform::MATRIX: return new FCDTMatrix(document, parent);
	case FCDTransform::LOOKAT: return new FCDTLookAt(document, parent);
	default: return NULL;
	}
}
