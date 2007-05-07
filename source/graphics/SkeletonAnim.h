/**
 * =========================================================================
 * File        : SkeletonAnim.h
 * Project     : 0 A.D.
 * Description : Instance of CSkeletonAnimDef for application onto a model
 * =========================================================================
 */

#ifndef INCLUDED_SKELETONANIM
#define INCLUDED_SKELETONANIM

#include "maths/Bound.h"

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
	// Times during the animation at which the interesting bits happen. Measured
	// as fractions (0..1) of the total animation length.
	// ActionPos is used for melee hits, projectile launches, etc.
	// ActionPos2 is used for loading projectile ammunition.
	float m_ActionPos;
	float m_ActionPos2;
	// object space bounds of the model when this animation is applied to it
	CBound m_ObjectBounds;
};

#endif
