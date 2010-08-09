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

#ifndef INCLUDED_ICMPPOSITION
#define INCLUDED_ICMPPOSITION

#include "simulation2/system/Interface.h"

#include "simulation2/helpers/Position.h"
#include "maths/FixedVector3D.h"
#include "maths/FixedVector2D.h"

class CMatrix3D;

/**
 * Represents an entity's position in the world (plus its orientation).
 *
 * Entity positions are determined by the following:
 *   - X, Z coordinates (giving left/right and front/back coordinates on the map)
 *   - Y offset (height; entities always snap to the ground, then are offset by this amount)
 *   - 'Floating' flag (snap to surface of water instead of going underneath)
 * As far as the simulation code is concerned, movements are instantaneous.
 * The only exception is GetInterpolatedTransform, used for rendering, which may
 * interpolate between the previous and current positions.
 * (The "Jump" methods circumvent the interpolation, and should be used whenever immediate
 * movement is needed.)
 *
 * Orientations consist of the following:
 *   - Rotation around upwards 'Y' axis (the common form of rotation)
 *   - Terrain conformance mode, one of:
 *     - Upright (upwards axis is always the world Y axis, e.g. for humans)
 *     - Pitch (rotates backwards and forwards to match the terrain, e.g. for horses)
 *     - PitchRoll (rotates in all directions to match the terrain, e.g. for carts)
 *   - Rotation around relative X (pitch), Z (roll) axes (rare; used for special effects)
 *
 * Entities can also be 'outside the world' (e.g. hidden inside a building), in which
 * case they have no position. Callers <b>must</b> check the entity is in the world, before
 * querying its position.
 */
class ICmpPosition : public IComponent
{
public:
	/**
	 * Returns true if the entity currently exists at a defined position in the world.
	 */
	virtual bool IsInWorld() = 0;

	/**
	 * Causes IsInWorld to return false. (Use MoveTo() or JumpTo() to move back into the world.)
	 */
	virtual void MoveOutOfWorld() = 0;

	/**
	 * Move smoothly to the given location.
	 */
	virtual void MoveTo(entity_pos_t x, entity_pos_t z) = 0;

	/**
	 * Move immediately to the given location, with no interpolation.
	 */
	virtual void JumpTo(entity_pos_t x, entity_pos_t z) = 0;

	/**
	 * Set the vertical offset above the terrain/water surface.
	 */
	virtual void SetHeightOffset(entity_pos_t dy) = 0;

	/**
	 * Returns the current vertical offset above the terrain/water surface.
	 */
	virtual entity_pos_t GetHeightOffset() = 0;

	/**
	 * Returns whether the entity floats on water.
	 */
	virtual bool IsFloating() = 0;

	/**
	 * Returns the current x,y,z position (no interpolation).
	 * Depends on the current terrain heightmap.
	 * Must not be called unless IsInWorld is true.
	 */
	virtual CFixedVector3D GetPosition() = 0;

	/**
	 * Returns the current x,z position (no interpolation).
	 * Must not be called unless IsInWorld is true.
	 */
	virtual CFixedVector2D GetPosition2D() = 0;

	/**
	 * Rotate smoothly to the given angle around the upwards axis.
	 * @param y clockwise radians from the +Z axis.
	 */
	virtual void TurnTo(entity_angle_t y) = 0;

	/**
	 * Rotate immediately to the given angle around the upwards axis.
	 * @param y clockwise radians from the +Z axis.
	 */
	virtual void SetYRotation(entity_angle_t y) = 0;

	/**
	 * Rotate immediately to the given angles around the X (pitch) and Z (roll) axes.
	 * @param x radians around the X axis. (TODO: in which direction?)
	 * @param z radians around the Z axis.
	 */
	virtual void SetXZRotation(entity_angle_t x, entity_angle_t z) = 0;

	// NOTE: we separate Y from XZ because most code will only ever change Y;
	// XZ are typically just used in the editor, and other code should never
	// worry about them

	/**
	 * Returns the current rotation (relative to the upwards axis), as Euler
	 * angles with X=pitch, Y=yaw, Z=roll. (TODO: is that the right way round?)
	 */
	virtual CFixedVector3D GetRotation() = 0;

	// TODO: do we want to add smooth rotation here?

	/**
	 * Get the current interpolated 2D position and orientation, for rendering.
	 * Must not be called unless IsInWorld is true.
	 */
	virtual void GetInterpolatedPosition2D(float frameOffset, float& x, float& z, float& rotY) = 0;

	/**
	 * Get the current interpolated transform matrix, for rendering.
	 * Must not be called unless IsInWorld is true.
	 */
	virtual CMatrix3D GetInterpolatedTransform(float frameOffset, bool forceFloating) = 0;

	DECLARE_INTERFACE_TYPE(Position)
};

#endif // INCLUDED_ICMPPOSITION
