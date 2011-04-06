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
#include "lib/file/vfs/vfs_path.h"

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
	 * Return the short name of the actor that's being displayed, or the empty string on error.
	 * (Not safe for use in simulation code.)
	 */
	virtual std::wstring GetActorShortName() = 0;

	/**
	 * Return the filename of the actor to be used for projectiles from this unit, or the empty string if none.
	 * (Not safe for use in simulation code.)
	 */
	virtual std::wstring GetProjectileActor() = 0;

	/**
	 * Return the exact position where a projectile should be launched from (based on the actor's
	 * ammo prop points).
	 * Returns (0,0,0) if no point can be found.
	 */
	virtual CVector3D GetProjectileLaunchPoint() = 0;

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
	 * Start playing the walk/run animations, scaled to the unit's movement speed.
	 * @param runThreshold movement speed at which to switch to the run animation
	 */
	virtual void SelectMovementAnimation(float runThreshold) = 0;

	/**
	 * Adjust the speed of the current animation, so it can match simulation events.
	 * @param repeattime time for complete loop of animation, in msec
	 */
	virtual void SetAnimationSyncRepeat(float repeattime) = 0;

	/**
	 * Adjust the offset of the current animation, so it can match simulation events.
	 * @param actiontime time between now and when the 'action' event should occur, in msec
	 */
	virtual void SetAnimationSyncOffset(float actiontime) = 0;

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

	/**
	 * Set an arbitrarily-named variable that the model may use to alter its appearance
	 * (e.g. in particle emitter parameter computations).
	 */
	virtual void SetVariable(std::string name, float value) = 0;

	/**
	 * Called when an actor file has been modified and reloaded dynamically.
	 * If this component uses the named actor file, it should regenerate its actor
	 * to pick up the new definitions.
	 */
	virtual void Hotload(const VfsPath& name) = 0;

	DECLARE_INTERFACE_TYPE(Visual)
};

// TODO: rename this to VisualActor, because the interface is actor-specific, maybe?

#endif // INCLUDED_ICMPVISUAL
