/*
	Copyright (C) 2005-2007 Feeling Software Inc.
	Portions of the code are:
	Copyright (C) 2005-2007 Sony Computer Entertainment America
	
	MIT License: http://www.opensource.org/licenses/mit-license.php
*/

#include "StdAfx.h"
#include "FCDocument/FCDocument.h"
#include "FCDocument/FCDAnimation.h"
#include "FCDocument/FCDAnimationChannel.h"
#include "FCDocument/FCDAnimationClip.h"
#include "FCDocument/FCDAnimationCurve.h"
#include "FCDocument/FCDAnimated.h"

//
// FCDAnimationClip
//

ImplementObjectType(FCDAnimationClip);
ImplementParameterObjectNoCtr(FCDAnimationClip, FCDEntityInstance, animations);

FCDAnimationClip::FCDAnimationClip(FCDocument* document)
:	FCDEntity(document, "AnimationClip")
,	InitializeParameter(start, 0.0f)
,	InitializeParameter(end, 0.0f)
,	InitializeParameterNoArg(animations)
{
}

FCDAnimationClip::~FCDAnimationClip()
{
	curves.clear();
}

void FCDAnimationClip::AddClipCurve(FCDAnimationCurve* curve)
{
	curve->RegisterAnimationClip(this);
	curves.push_back(curve);
	SetNewChildFlag();
}

FCDEntity* FCDAnimationClip::Clone(FCDEntity* _clone, bool cloneChildren) const
{
	FCDAnimationClip* clone = NULL;
	if (_clone == NULL) _clone = clone = new FCDAnimationClip(const_cast<FCDocument*>(GetDocument()));
	else if (_clone->HasType(FCDAnimationClip::GetClassType())) clone = (FCDAnimationClip*) _clone;

	Parent::Clone(_clone, cloneChildren);

	if (clone != NULL)
	{
		// Copy the generic animation clip parameters
		clone->start = start;
		clone->end = end;

		// If requested, clone the animation curves as well.
		for (FCDAnimationCurveTrackList::const_iterator it = curves.begin(); it != curves.end(); ++it)
		{
			if (cloneChildren)
			{
				FCDAnimationCurve* clonedCurve = (*it)->Clone(NULL, false);
				clonedCurve->AddClip(clone);
				clone->AddClipCurve(clonedCurve);
			}
		}
	}

	return _clone;
}

FCDEntityInstance* FCDAnimationClip::AddInstanceAnimation()
{ 
	FCDEntityInstance* newInstance = FCDEntityInstanceFactory::CreateInstance(GetDocument(), NULL, FCDEntity::ANIMATION);
	animations.push_back(newInstance);
	return newInstance;
}

FCDEntityInstance* FCDAnimationClip::AddInstanceAnimation(FCDAnimation* animation)
{
	FCDEntityInstance* newInstance = FCDEntityInstanceFactory::CreateInstance(GetDocument(), NULL, animation);
	animations.push_back(newInstance);
	return newInstance;
}
