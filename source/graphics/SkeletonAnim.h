/* Copyright (C) 2009 Wildfire Games.
 * This file is part of 0 A.D.
 *
 * 0 A.D. is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * 0 A.D. is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with 0 A.D.  If not, see <http://www.gnu.org/licenses/>.
 */

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
