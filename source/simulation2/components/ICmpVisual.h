/* Copyright (C) 2010 Wildfire Games.
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

#ifndef INCLUDED_ICMPVISUAL
#define INCLUDED_ICMPVISUAL

#include "simulation2/system/Interface.h"

#include "maths/Bound.h"

/**
 * The visual representation of an entity (typically an actor).
 */
class ICmpVisual : public IComponent
{
public:
	/**
	 * Get the world-space bounding box of the object's visual representation.
	 * (Not safe for use in simulation code.)
	 */
	virtual CBound GetBounds() = 0;

	/**
	 * Get the world-space position of the base point of the object's visual representation.
	 * (Not safe for use in simulation code.)
	 */
	virtual CVector3D GetPosition() = 0;

	/**
	 * Start playing the given animation. If there are multiple possible animations then it will
	 * pick one at random (not network-synchronised).
	 * @param name animation name (e.g. "idle", "walk", "melee"; the names are determined by actor XML files)
	 * @param once if true then the animation will play once and freeze at the final frame, else it will loop
	 * @param speed some kind of animation speed multiplier (TODO: work out exactly what the scale is)
	 */
	virtual void SelectAnimation(std::string name, bool once, float speed) = 0;

	DECLARE_INTERFACE_TYPE(Visual)
};

// TODO: rename this to VisualActor, because the interface is actor-specific, maybe?

#endif // INCLUDED_ICMPVISUAL
