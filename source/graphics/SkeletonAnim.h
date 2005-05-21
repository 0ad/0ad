///////////////////////////////////////////////////////////////////////////////
//
// Name:		SkeletonAnim.h
// Author:		Rich Cross
// Contact:		rich@wildfiregames.com
//
///////////////////////////////////////////////////////////////////////////////

#ifndef _SKELETONANIM_H
#define _SKELETONANIM_H

#include "Bound.h"

class CSkeletonAnimDef;


////////////////////////////////////////////////////////////////////////////////////////
// CSkeletonAnim: an instance of a CSkeletonAnimDef, for application onto a model
class CSkeletonAnim
{
public:
	// the name of the action which uses this animation (e.g. "idle")
	CStr m_Name;
	// the raw animation frame data
	CSkeletonAnimDef* m_AnimDef;
	// speed at which this animation runs
	float m_Speed;
	// time(s) during the animation at which the interesting bit(s) happens (msec)
	size_t m_ActionPos;
	size_t m_ActionPos2;
	// object space bounds of the model when this animation is applied to it
	CBound m_ObjectBounds;
};

#endif
