/*
	Copyright (C) 2005-2007 Feeling Software Inc.
	Portions of the code are:
	Copyright (C) 2005-2007 Sony Computer Entertainment America
	
	MIT License: http://www.opensource.org/licenses/mit-license.php
*/

#include "StdAfx.h"
#include "FCDocument/FCDocument.h"
#include "FCDocument/FCDAnimated.h"
#include "FCDocument/FCDExtra.h"
#include "FCDocument/FCDGeometry.h"
#include "FCDocument/FCDGeometryMesh.h"
#include "FCDocument/FCDGeometrySpline.h"
#include "FUtils/FUStringConversion.h"

//
// FCDGeometry
//

ImplementObjectType(FCDGeometry);
ImplementParameterObject(FCDGeometry, FCDGeometryMesh, mesh, new FCDGeometryMesh(parent->GetDocument(), parent));
ImplementParameterObject(FCDGeometry, FCDGeometrySpline, spline, new FCDGeometrySpline(parent->GetDocument(), parent));

FCDGeometry::FCDGeometry(FCDocument* document)
:	FCDEntity(document, "Geometry")
,	InitializeParameterNoArg(mesh)
,	InitializeParameterNoArg(spline)
{
}

FCDGeometry::~FCDGeometry()
{
}

// Sets the type of this geometry to mesh and creates an empty mesh structure.
FCDGeometryMesh* FCDGeometry::CreateMesh()
{
	spline = NULL;
	mesh = new FCDGeometryMesh(GetDocument(), this);
	SetNewChildFlag();
	return mesh;
}

// Sets the type of this geometry to spline and creates an empty spline structure.
FCDGeometrySpline* FCDGeometry::CreateSpline()
{
	mesh = NULL;
	spline = new FCDGeometrySpline(GetDocument(), this);
	SetNewChildFlag();
	return spline;
}


FCDEntity* FCDGeometry::Clone(FCDEntity* _clone, bool cloneChildren) const
{
	FCDGeometry* clone = NULL;
	if (_clone == NULL) _clone = clone = new FCDGeometry(const_cast<FCDocument*>(GetDocument()));
	else if (_clone->HasType(FCDGeometry::GetClassType())) clone = (FCDGeometry*) _clone;

	Parent::Clone(_clone, cloneChildren);

	if (clone != NULL)
	{
		// Clone the geometric object
		if (IsMesh())
		{
			FCDGeometryMesh* clonedMesh = clone->CreateMesh();
			GetMesh()->Clone(clonedMesh);
		}
		else if (IsSpline())
		{
			FCDGeometrySpline* clonedSpline = clone->CreateSpline();
			GetSpline()->Clone(clonedSpline);
		}
	}
	return clone;
}
