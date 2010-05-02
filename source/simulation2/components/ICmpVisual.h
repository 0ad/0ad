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
#include "maths/Fixed.h"

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
	 * Return the filename of the actor that's being displayed, or the empty string on error.
	 * (Not safe for use in simulation code.)
	 */
	virtual std::wstring GetActor() = 0;

	/**
	 * Return the filename of the actor to be used for projectiles from this unit, or the empty string if none.
	 * (Not safe for use in simulation code.)
	 */
	virtual std::wstring GetProjectileActor() = 0;

	/**
	 * Start playing the given animation. If there are multiple possible animations then it will
	 * pick one at random (not network-synchronised).
	 * If @p soundgroup is specified, then the sound will be played at each 'event' point in the
	 * animation cycle.
	 * @param name animation name (e.g. "idle", "walk", "melee"; the names are determined by actor XML files)
	 * @param once if true then the animation will play once and freeze at the final frame, else it will loop
	 * @param speed animation speed multiplier (typically 1.0 for the default speed)
	 * @param soundgroup VFS path of sound group .xml, relative to audio/, or empty string for none
	 */
	virtual void SelectAnimation(std::string name, bool once, float speed, std::wstring soundgroup) = 0;

	/**
	 * Adjust the timing (offset and speed) of the current animation, so it can match
	 * simulation events.
	 * @param actiontime time between now and when the 'action' event should occur, in msec
	 * @param repeattime time for complete loop of animation, in msec
	 */
	virtual void SetAnimationSync(float actiontime, float repeattime) = 0;

	/**
	 * Set the shading colour that will be modulated with the model's textures.
	 * Default shading is (1, 1, 1, 1).
	 * Alpha should probably be 1 else it's unlikely to work properly.
	 * @param r red component, expected range [0, 1]
	 * @param g green component, expected range [0, 1]
	 * @param b blue component, expected range [0, 1]
	 * @param a alpha component, expected range [0, 1]
	 */
	virtual void SetShadingColour(fixed r, fixed g, fixed b, fixed a) = 0;

	DECLARE_INTERFACE_TYPE(Visual)
};

// TODO: rename this to VisualActor, because the interface is actor-specific, maybe?

#endif // INCLUDED_ICMPVISUAL
