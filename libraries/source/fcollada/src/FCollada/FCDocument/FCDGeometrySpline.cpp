/*
	Copyright (C) 2005-2007 Feeling Software Inc.
	Portions of the code are:
	Copyright (C) 2005-2007 Sony Computer Entertainment America
	
	MIT License: http://www.opensource.org/licenses/mit-license.php
*/

#include "StdAfx.h"
#include "FCDocument/FCDocument.h"
#include "FCDocument/FCDExtra.h"
#include "FCDocument/FCDGeometry.h"
#include "FCDocument/FCDGeometrySource.h"
#include "FCDocument/FCDGeometrySpline.h"
#include "FUtils/FUStringConversion.h"

//
// FCDSpline
//

ImplementObjectType(FCDSpline);

FCDSpline::FCDSpline(FCDocument* document)
:	FCDObject(document)
{
	form = FUDaeSplineForm::OPEN;
}

FCDSpline::~FCDSpline()
{
	cvs.clear();
}

FCDSpline* FCDSpline::Clone(FCDSpline* clone) const
{
	if (clone == NULL) return NULL;

	clone->cvs = cvs;
	clone->name = name;
	clone->form = form;

	return clone;
}

//
// FCDLinearSpline
//

ImplementObjectType(FCDLinearSpline);

FCDLinearSpline::FCDLinearSpline(FCDocument* document)
:	FCDSpline(document)
{
}

FCDLinearSpline::~FCDLinearSpline()
{
}

void FCDLinearSpline::ToBezier(FCDBezierSpline& bezier)
{
	if (!IsValid()) return;
        
    // clear the given spline
    bezier.ClearCVs();

	size_t count = cvs.size();
	bool closed = IsClosed();

	if (closed) 
	{
		bezier.SetClosed(true);
	}

	for (size_t i = 0; i < count; i++)
	{
		FMVector3& cv = cvs[i];
		if (!closed && (i == 0 || i == count - 1))
		{
			// first and last CV on an open spline, 2 times
			bezier.AddCV(cv);
			bezier.AddCV(cv);
		}
		else
		{
			// in between CVs, three times
			bezier.AddCV(cv);
			bezier.AddCV(cv);
			bezier.AddCV(cv);
		}
	}
}

bool FCDLinearSpline::IsValid() const
{
	return cvs.size() >= 2;
}

//
// FCDBezierSpline
//

ImplementObjectType(FCDBezierSpline);

FCDBezierSpline::FCDBezierSpline(FCDocument* document)
:	FCDSpline(document)
{
}

FCDBezierSpline::~FCDBezierSpline()
{
}

void FCDBezierSpline::ToNURBs(FCDNURBSSplineList &toFill) const
{
	// calculate the number of nurb segments
	bool closed = IsClosed();
	int cvsCount = (int)cvs.size();
	int nurbCount = (!closed) ? ((cvsCount - 1) / 3) : (cvsCount / 3);

	// if the spline is closed, ignore the first CV as it is the in-tangent of the first knot
	size_t curCV = (!closed) ? 0 : 1;
	int lastNurb = (!closed) ? nurbCount : (nurbCount - 1);

	for (int i = 0; i < lastNurb; i++)
	{
		FCDNURBSSpline* nurb = new FCDNURBSSpline(const_cast<FCDocument*>(GetDocument()));
		nurb->SetDegree(3);

		// add (degree + 1) CVs to the nurb
		for (size_t i = 0; i < (3+1); i++)
		{
			nurb->AddCV(cvs[curCV++], 1.0f);
		}

		// the last CVs will be the starting point of the next nurb segment
		curCV--;

		// add (degree+1) knots on each side of the knot vector
		for (size_t i = 0; i < (3+1); i++) nurb->AddKnot(0.0f);
		for (size_t i = 0; i < (3+1); i++) nurb->AddKnot(1.0f);

		// nurbs created this way are never closed
		nurb->SetClosed(false);

		// add the nurb
		toFill.push_back(nurb);
	}

	if (closed)
	{
		// we still have one NURB to create
		FCDNURBSSpline* nurb = new FCDNURBSSpline(const_cast<FCDocument*>(GetDocument()));
		nurb->SetDegree(3);

		nurb->AddCV(cvs[cvsCount - 2], 1.0f); // the last knot position
		nurb->AddCV(cvs[cvsCount - 1], 1.0f); // the last knot out-tangent
		nurb->AddCV(cvs[0			 ], 1.0f); // the first knot in-tangent
		nurb->AddCV(cvs[1			 ], 1.0f); // the first knot position

		// add (degree+1) knots on each side of the knot vector
		for (size_t i = 0; i < (3+1); i++) nurb->AddKnot(0.0f);
		for (size_t i = 0; i < (3+1); i++) nurb->AddKnot(1.0f);

		// nurbs created this way are never closed
		nurb->SetClosed(false);

		// add the nurb
		toFill.push_back(nurb);
	}
}

bool FCDBezierSpline::IsValid() const
{
	bool s = true;
	if (cvs.size() == 0)
	{
		s = !FUError::Error(FUError::WARNING_LEVEL, FUError::WARNING_SPLINE_CONTROL_INPUT_MISSING);
	}
	return s;
}

//
// FCDNURBSSpline
//

ImplementObjectType(FCDNURBSSpline);

FCDNURBSSpline::FCDNURBSSpline(FCDocument* document)
:	FCDSpline(document)
{
}

FCDNURBSSpline::~FCDNURBSSpline()
{
	weights.clear();
	knots.clear();
}

FCDSpline* FCDNURBSSpline::Clone(FCDSpline* _clone) const
{
	FCDNURBSSpline* clone = NULL;
	if (_clone == NULL) return NULL;
	else if (_clone->HasType(FCDNURBSSpline::GetClassType())) clone = (FCDNURBSSpline*) _clone;

	Parent::Clone(_clone);

	if (clone != NULL)
	{
		// Clone the NURBS-specific spline data
		clone->degree = degree;
		clone->weights = weights;
		clone->knots = knots;
	}

	return _clone;
}

bool FCDNURBSSpline::AddCV(const FMVector3& cv, float weight)
{ 
	if (weight < 0.0f) return false;

	cvs.push_back(cv);
	weights.push_back(weight);
	return true;
}

bool FCDNURBSSpline::IsValid() const
{
	bool s = true;
	if (cvs.size() == 0)
	{
		FUError::Error(FUError::WARNING_LEVEL, FUError::WARNING_SPLINE_CONTROL_INPUT_MISSING);
		s = false;
	}

	if (cvs.size() != weights.size())
	{
		FUError::Error(FUError::ERROR_LEVEL, FUError::ERROR_INVALID_CVS_WEIGHTS);
		s = false;
	}

	if (cvs.size() != knots.size() - degree - 1)
	{
		FUError::Error(FUError::ERROR_LEVEL, FUError::ERROR_INVALID_SPLINE);
		s = false;
	}

	return s;
}

//
// FCDGeometrySpline
//

ImplementObjectType(FCDGeometrySpline);
ImplementParameterObjectNoCtr(FCDGeometrySpline, FCDSpline, splines);

FCDGeometrySpline::FCDGeometrySpline(FCDocument* document, FCDGeometry* _parent)
:	FCDObject(document), parent(_parent)
,	InitializeParameter(type, FUDaeSplineType::UNKNOWN)
,	InitializeParameterNoArg(splines)
{
}

FCDGeometrySpline::~FCDGeometrySpline()
{
	parent = NULL;
}

FCDGeometrySpline* FCDGeometrySpline::Clone(FCDGeometrySpline* clone) const
{
	if (clone == NULL) clone = new FCDGeometrySpline(const_cast<FCDocument*>(GetDocument()), NULL);
	clone->type = type;

	// Clone the spline set.
	for (FCDSplineContainer::const_iterator it = splines.begin(); it != splines.end(); ++it)
	{
		FCDSpline* cloneSpline = clone->AddSpline();
		(*it)->Clone(cloneSpline);
	}

	return clone;
}

bool FCDGeometrySpline::SetType(FUDaeSplineType::Type _type)
{
	while (!splines.empty()) splines.back()->Release();
	type = _type;
	SetDirtyFlag();
	return true;
}

FCDSpline* FCDGeometrySpline::AddSpline(FUDaeSplineType::Type type)
{
	// Retrieve the correct spline type to create.
	if (type == FUDaeSplineType::UNKNOWN) type = GetType();
	else if (type != GetType()) return NULL;

	// Create the correctly-type spline
	FCDSpline* newSpline = NULL;
	switch (type)
	{
	case FUDaeSplineType::LINEAR: newSpline = new FCDLinearSpline(GetDocument()); break;
	case FUDaeSplineType::BEZIER: newSpline = new FCDBezierSpline(GetDocument()); break;
	case FUDaeSplineType::NURBS: newSpline = new FCDNURBSSpline(GetDocument()); break;

	case FUDaeSplineType::UNKNOWN:
	default: return NULL;
	}

	splines.push_back(newSpline);
	SetDirtyFlag();
	return newSpline;
}

size_t FCDGeometrySpline::GetTotalCVCount()
{
	size_t count = 0;
	for (size_t i = 0; i < splines.size(); i++)
	{
		count += splines[i]->GetCVCount();
	}
	return count;
}

void FCDGeometrySpline::ConvertBezierToNURBS(FCDNURBSSplineList &toFill)
{
	if (type != FUDaeSplineType::BEZIER)
	{
		return;
	}

	for (size_t i = 0; i < splines.size(); i++)
	{
		FCDBezierSpline* bez = static_cast<FCDBezierSpline*>(splines[i]);
		bez->ToNURBs(toFill);
	}
	SetDirtyFlag();
}
