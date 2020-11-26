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
#include "FCDocument/FCDAnimation.h"
#include "FCDocument/FCDAnimationChannel.h"
#include "FCDocument/FCDAsset.h"

//
// FCDAnimation
//

ImplementObjectType(FCDAnimation);
ImplementParameterObject(FCDAnimation, FCDAnimation, children, new FCDAnimation(parent->GetDocument(), parent));
ImplementParameterObject(FCDAnimation, FCDAnimationChannel, channels, new FCDAnimationChannel(parent->GetDocument(), parent));

FCDAnimation::FCDAnimation(FCDocument* document, FCDAnimation* _parent)
:	FCDEntity(document, "Animation")
,	parent(_parent)
,	InitializeParameterNoArg(children)
,	InitializeParameterNoArg(channels)
{
}

FCDAnimation::~FCDAnimation()
{
//	childNodes.clear();
	parent = NULL;
}

FCDEntity* FCDAnimation::Clone(FCDEntity* _clone, bool cloneChildren) const
{
	FCDAnimation* clone = NULL;
	if (_clone == NULL) _clone = clone = new FCDAnimation(const_cast<FCDocument*>(GetDocument()), NULL);
	else if (_clone->HasType(FCDAnimation::GetClassType())) clone = (FCDAnimation*) _clone;

	Parent::Clone(_clone, cloneChildren);

	if (clone != NULL)
	{
		// Clone the channels
		for (const FCDAnimationChannel** it = channels.begin(); it != channels.end(); ++it)
		{
			FCDAnimationChannel* clonedChannel = clone->AddChannel();
			(*it)->Clone(clonedChannel);
		}

		if (cloneChildren)
		{
			// Clone the animation tree children
			for (const FCDAnimation** it = children.begin(); it != children.end(); ++it)
			{
				FCDAnimation* clonedChild = clone->AddChild();
				(*it)->Clone(clonedChild, cloneChildren);
			}
		}
	}

	return _clone;
}

// Creates a new animation entity sub-tree contained within this animation entity tree.
FCDAnimation* FCDAnimation::AddChild()
{
	FCDAnimation* animation = new FCDAnimation(GetDocument(), this);
	children.push_back(animation);
	SetNewChildFlag();
	return children.back();
}

// Adds a new animation channel to this animation entity.
FCDAnimationChannel* FCDAnimation::AddChannel()
{
	FCDAnimationChannel* channel = new FCDAnimationChannel(GetDocument(), this);
	channels.push_back(channel);
	SetNewChildFlag();
	return channels.back();
}

// Look for an animation children with the given COLLADA Id.
const FCDEntity* FCDAnimation::FindDaeId(const fm::string& daeId) const
{
	if (GetDaeId() == daeId) return this;
	
	for (const FCDAnimation** it = children.begin(); it != children.end(); ++it)
	{
		const FCDEntity* found = (*it)->FindDaeId(daeId);
		if (found != NULL) return found;
	}
	return NULL;
}

void FCDAnimation::GetHierarchicalAssets(FCDAssetConstList& assets) const
{
	for (const FCDAnimation* animation = this; animation != NULL; animation = animation->GetParent())
	{
		// Retrieve the asset information structure for this node.
		const FCDAsset* asset = animation->GetAsset();
		if (asset != NULL) assets.push_back(asset);
	}
	assets.push_back(GetDocument()->GetAsset());
}

// Retrieve all the curves created under this animation element, in the animation tree
void FCDAnimation::GetCurves(FCDAnimationCurveList& curves)
{
	// Retrieve the curves for this animation tree element
	for (const FCDAnimationChannel** it = (const FCDAnimationChannel**) channels.begin(); it != channels.end(); ++it)
	{
		size_t channelCurveCount = (*it)->GetCurveCount();
		for (size_t i = 0; i < channelCurveCount; ++i)
		{
			curves.push_back((*it)->GetCurve(i));
		}
	}

	// Retrieve the curves for the animation nodes under this one in the animation tree
	size_t childCount = children.size();
	for (size_t i = 0; i < childCount; ++i)
	{
		children[i]->GetCurves(curves);
	}
}
