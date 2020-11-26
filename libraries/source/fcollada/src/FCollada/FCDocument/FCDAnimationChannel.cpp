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
#include "FCDocument/FCDAnimationChannel.h"
#include "FCDocument/FCDAnimationCurve.h"

//
// FCDAnimationChannel
//

ImplementObjectType(FCDAnimationChannel);
ImplementParameterObject(FCDAnimationChannel, FCDAnimationCurve, curves, new FCDAnimationCurve(parent->GetDocument(), parent));

FCDAnimationChannel::FCDAnimationChannel(FCDocument* document, FCDAnimation* _parent)
:	FCDObject(document), parent(_parent)
,	InitializeParameterNoArg(curves)
{
}

FCDAnimationChannel::~FCDAnimationChannel()
{
	parent = NULL;
}

FCDAnimationChannel* FCDAnimationChannel::Clone(FCDAnimationChannel* clone) const
{
	if (clone == NULL) clone = new FCDAnimationChannel(const_cast<FCDocument*>(GetDocument()), NULL);

	// Clone the curves
	for (const FCDAnimationCurve** it = curves.begin(); it != curves.end(); ++it)
	{
		FCDAnimationCurve* clonedCurve = clone->AddCurve();
		(*it)->Clone(clonedCurve, false);
	}

	return clone;
}

FCDAnimationCurve* FCDAnimationChannel::AddCurve()
{
	FCDAnimationCurve* curve = new FCDAnimationCurve(GetDocument(), this);
	curves.push_back(curve);
	SetNewChildFlag();
	return curve;
}
