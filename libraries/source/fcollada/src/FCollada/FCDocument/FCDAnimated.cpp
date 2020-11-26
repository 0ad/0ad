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
#include "FCDocument/FCDocument.h"
#include "FCDocument/FCDAnimationChannel.h"
#include "FCDocument/FCDAnimationCurve.h"
#include "FCDocument/FCDAnimationCurveTools.h"
#include "FCDocument/FCDAnimated.h"


//
// FCDAnimated
//

ImplementObjectType(FCDAnimated);

FCDAnimated::FCDAnimated(FCDocument* document, size_t valueCount, const char** _qualifiers, float** _values)
:	FCDObject(document)
,	target(NULL)
{
	arrayElement = -1;

	// Allocate the values/qualifiers/curves arrays
	values.resize(valueCount);
	qualifiers.resize(valueCount);
	curves.resize(valueCount);

	for (size_t i = 0; i < valueCount; ++i)
	{
		values[i] = _values[i];
		qualifiers[i] = _qualifiers[i];
	}

	ResetRelativeAnimationFlag();
}

FCDAnimated::FCDAnimated(FCDObject* object, size_t valueCount, const char** _qualifiers, float** _values)
:	FCDObject(object->GetDocument())
{
	arrayElement = -1;

	// Allocate the values/qualifiers/curves arrays
	values.resize(valueCount);
	qualifiers.resize(valueCount);
	curves.resize(valueCount);

	for (size_t i = 0; i < valueCount; ++i)
	{
		values[i] = _values[i];
		qualifiers[i] = _qualifiers[i];
	}

	// Register this animated value with the document
	GetDocument()->RegisterAnimatedValue(this);

	target = object;
	TrackObject(target);
}

FCDAnimated::~FCDAnimated()
{
	GetDocument()->UnregisterAnimatedValue(this);

	values.clear();
	qualifiers.clear();
	curves.clear();

	UntrackObject(target);
}

// Assigns a curve to a value of the animated element.
bool FCDAnimated::AddCurve(size_t index, FCDAnimationCurve* curve)
{
	FUAssert(index < GetValueCount(), return false);
	curves.at(index).push_back(curve);
	SetNewChildFlag();
	return true;
}

bool FCDAnimated::AddCurve(size_t index, FCDAnimationCurveList& curve)
{
	FUAssert(index < GetValueCount() && !curve.empty(), return false);
	curves.at(index).insert(curves.at(index).end(), curve.begin(), curve.end());
	SetNewChildFlag();
	return true;
}

// Removes the curves affecting a value of the animated element.
bool FCDAnimated::RemoveCurve(size_t index)
{
	FUAssert(index < GetValueCount(), return false);
	bool hasCurve = !curves[index].empty();
	curves[index].clear();
	SetNewChildFlag();
	return hasCurve;
}

const fm::string& FCDAnimated::GetQualifier(size_t index) const
{
	FUAssert(index < GetValueCount(), return emptyString);
	return qualifiers.at(index);
}

// Retrieves an animated value, given a valid qualifier
float* FCDAnimated::FindValue(const fm::string& qualifier)
{
	for (size_t i = 0; i < qualifiers.size(); ++i)
	{
		if (qualifiers[i] == qualifier) return values[i];
	}
	return NULL;
}
const float* FCDAnimated::FindValue(const fm::string& qualifier) const
{
	for (size_t i = 0; i < qualifiers.size(); ++i)
	{
		if (qualifiers[i] == qualifier) return values[i];
	}
	return NULL;
}

// Retrieve the index of a given qualifier
size_t FCDAnimated::FindQualifier(const char* qualifier) const
{
	for (size_t i = 0; i < qualifiers.size(); ++i)
	{
		if (qualifiers[i] == qualifier) return i;
	}

	// Otherwise, check for a matrix element
	int32 matrixElement = FUStringConversion::ParseQualifier(qualifier);
	if (matrixElement >= 0 && matrixElement < (int32) qualifiers.size()) return matrixElement;
	return size_t(-1);
}

// Retrieve the index of a given value pointer
size_t FCDAnimated::FindValue(const float* value) const
{
	for (size_t i = 0; i < values.size(); ++i)
	{
		if (values[i] == value) return i;
	}
	return size_t(-1);
}

// Return the update target of this animations
void FCDAnimated::SetTargetObject(FCDObject* _target)
{ 
	UntrackObject(target);
	target = _target;
	TrackObject(target);
} 

// Returns whether any of the contained curves are non-NULL
bool FCDAnimated::HasCurve() const
{
	FCDAnimationCurveListList::const_iterator cit;
	for (cit = curves.begin(); cit != curves.end() && (*cit).empty(); ++cit) {}
	return cit != curves.end();
}

// Create one multi-curve out of this animated value's single curves
FCDAnimationMultiCurve* FCDAnimated::CreateMultiCurve() const
{
	FloatList defaultValues;
	size_t count = values.size();
	defaultValues.resize(count);
	for (size_t i = 0; i < count; ++i) defaultValues[i] = (*values[i]);

	fm::pvector<const FCDAnimationCurve> toMerge;
	toMerge.resize(count);
	for (size_t i = 0; i < count; ++i) toMerge[i] = (!curves[i].empty()) ? curves[i][0] : NULL;
	return FCDAnimationCurveTools::MergeCurves(toMerge, defaultValues);
}

// Create one multi-curve out of the single curves from many FCDAnimated objects
FCDAnimationMultiCurve* FCDAnimated::CreateMultiCurve(const FCDAnimatedList& toMerge)
{
	// Calculate the total dimension of the curve to create
	size_t count = 0;
	for (FCDAnimatedList::const_iterator cit = toMerge.begin(); cit != toMerge.end(); ++cit)
	{
		count += (*cit)->GetValueCount();
	}

	// Generate the list of default values and the list of curves
	FloatList defaultValues(count, 0.0f);
	FCDAnimationCurveConstList curves(count);
	size_t offset = 0;
	for (FCDAnimatedList::const_iterator cit = toMerge.begin(); cit != toMerge.end(); ++cit)
	{
		size_t localCount = (*cit)->GetValueCount();
		for (size_t i = 0; i < localCount; ++i)
		{
			defaultValues[offset + i] = *(*cit)->GetValue(i);
			curves[offset + i] = (*cit)->GetCurve(i);
		}
		offset += localCount;
	}

	return FCDAnimationCurveTools::MergeCurves(curves, defaultValues);
}

// Sample the animated values for a given time
void FCDAnimated::Evaluate(float time)
{
	size_t valueCount = values.size();
	size_t curveCount = curves.size();
	size_t count = min(curveCount, valueCount);
	for (size_t i = 0; i < count; ++i)
	{
		if (!curves[i].empty())
		{
			// Retrieve the curve and the corresponding value
			FCDAnimationCurve* curve = curves[i][0];
			if (curve == NULL) continue;
			float* value = values[i];
			if (value == NULL) continue;

			// Evaluate the curve at this time
			(*value) = curve->Evaluate(time);
			if (target != NULL) target->SetValueChange();
		}
	}
}

FCDAnimated* FCDAnimated::Clone(FCDocument* document) const
{
	// Generate the constant arrays for the qualifiers and the value pointers.
	size_t valueCount = GetValueCount();
	typedef const char* ccharptr;
	typedef float* floatptr;
	ccharptr* cloneQualifiers = new ccharptr[valueCount];
	floatptr* cloneValues = new floatptr[valueCount];
	for (size_t i = 0; i < valueCount; ++i)
	{
		cloneQualifiers[i] = qualifiers[i].c_str();
		cloneValues[i] = const_cast<float*>(values[i]);
	}

	// Clone this animated.
	FCDAnimated* clone = new FCDAnimated(document, GetValueCount(), cloneQualifiers, cloneValues);
	clone->arrayElement = arrayElement;
	for (size_t i = 0; i < curves.size(); ++i) 
	{
		for (size_t j = 0; j < curves[i].size(); ++j)
		{
			FCDAnimationCurve* clonedCurve = const_cast<FCDAnimationCurve*>(curves[i][j])->GetParent()->AddCurve();
			curves[i][j]->Clone(clonedCurve);
			clone->AddCurve(i, clonedCurve);
		}
	}

	SAFE_DELETE_ARRAY(cloneQualifiers);
	SAFE_DELETE_ARRAY(cloneValues);
	return clone;
}

FCDAnimated* FCDAnimated::Clone(FCDAnimated* clone) const
{
	if (clone != NULL)
	{
		// Clone the miscellaneous parameters.
		clone->arrayElement = arrayElement;

		// Clone the qualifiers and the curve lists for each value.
		size_t valueCount = min(GetValueCount(), clone->GetValueCount());
		for (size_t i = 0; i < valueCount; ++i)
		{
			clone->qualifiers[i] = qualifiers[i];
			clone->curves[i] = curves[i];
		}
	}
	return clone;
}

void FCDAnimated::OnObjectReleased(FUTrackable* object)
{
	FUAssert(object == target, return);
	target = NULL;

	// Delete ourselves.  We have no job left, if something
	// wants us they can reconstruct from FCDAnimationChannel
	Release();
}

//
// FCDAnimatedCustom
//

ImplementObjectType(FCDAnimatedCustom);

static const char* customAnimatedTemporaryQualifier = "";
static float* customAnimatedTemporaryValue = NULL;

FCDAnimatedCustom::FCDAnimatedCustom(FCDObject* object)
:	FCDAnimated(object, 1, &customAnimatedTemporaryQualifier, &customAnimatedTemporaryValue)
,	dummy(0.0f)
{
	values[0] = &dummy;

	// Register this animated value with the document
	GetDocument()->RegisterAnimatedValue(this);
}

void FCDAnimatedCustom::Copy(const FCDAnimated* copy)
{
	if (copy != NULL)
	{
		Resize(copy->GetValueCount(), NULL, false);
		copy->Clone(this);
	}
}

void FCDAnimatedCustom::Resize(size_t count, const char** _qualifiers, bool prependDot)
{
	FUAssert(count >= values.size(), return); // don't loose information, but growing is fine.
	values.reserve(count); while (values.size() < count) values.push_back(&dummy);
	qualifiers.resize(count);
	curves.resize(count);

	for (size_t i = 0; i < count && _qualifiers != NULL && *_qualifiers != 0; ++i)
	{
		qualifiers[i] = (prependDot ? fm::string(".") : fm::string("")) + *(_qualifiers++);
	}
}

void FCDAnimatedCustom::Resize(const StringList& _qualifiers, bool prependDot)
{
	size_t count = _qualifiers.size();
	FUAssert(count >= values.size(), return); // don't loose information, but growing is fine.
	values.reserve(count); while (values.size() < count) values.push_back(&dummy);
	qualifiers.resize(count);
	curves.resize(count);

	for (size_t i = 0; i < count; ++i)
	{
		qualifiers[i] = (prependDot ? fm::string(".") : fm::string("")) + _qualifiers[i];
	}
}

namespace FCDAnimatedStandardQualifiers
{
	FCOLLADA_EXPORT const char* EMPTY[1] = { "" };
	FCOLLADA_EXPORT const char* XYZW[4] = { ".X", ".Y", ".Z", ".W" };
	FCOLLADA_EXPORT const char* RGBA[4] = { ".R", ".G", ".B", ".A" };

	FCOLLADA_EXPORT const char* ROTATE_AXIS[4] = { ".X", ".Y", ".Z", ".ANGLE" };
	FCOLLADA_EXPORT const char* SKEW[7] = { ".ROTATEX", ".ROTATEY", ".ROTATEZ", ".AROUNDX", ".AROUNDY", ".AROUNDZ", ".ANGLE" };
	FCOLLADA_EXPORT const char* MATRIX[16] = { "(0)(0)", "(1)(0)", "(2)(0)", "(3)(0)", "(0)(1)", "(1)(1)", "(2)(1)", "(3)(1)", "(0)(2)", "(1)(2)", "(2)(2)", "(3)(2)", "(0)(3)", "(1)(3)", "(2)(3)", "(3)(3)" };
	FCOLLADA_EXPORT const char* LOOKAT[9] = { ".POSITIONX", ".POSITIONY", ".POSITIONZ", ".TARGETX", ".TARGETY", ".TARGETZ", ".UPX", ".UPY", ".UPZ" };
};
