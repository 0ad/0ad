/* Copyright (C) 2011 Wildfire Games.
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

#ifndef INCLUDED_ICMPOBSTRUCTIONMANAGER
#define INCLUDED_ICMPOBSTRUCTIONMANAGER

#include "simulation2/system/Interface.h"

#include "simulation2/helpers/Grid.h"
#include "simulation2/helpers/Position.h"

#include "maths/FixedVector2D.h"

class IObstructionTestFilter;

/**
 * Obstruction manager: provides efficient spatial queries over objects in the world.
 *
 * The class deals with two types of shape:
 * "static" shapes, typically representing buildings, which are rectangles with a given
 * width and height and angle;
 * and "unit" shapes, representing units that can move around the world, which have a
 * radius and no rotation. (Units sometimes act as axis-aligned squares, sometimes
 * as approximately circles, due to the algorithm used by the short pathfinder.)
 *
 * Other classes (particularly ICmpObstruction) register shapes with this interface
 * and keep them updated.
 *
 * The @c Test functions provide exact collision tests.
 * The edge of a shape counts as 'inside' the shape, for the purpose of collisions.
 * The functions accept an IObstructionTestFilter argument, which can restrict the
 * set of shapes that are counted as collisions.
 *
 * Units can be marked as either moving or stationary, which simply determines whether
 * certain filters include or exclude them.
 *
 * The @c Rasterise function approximates the current set of shapes onto a 2D grid,
 * for use with tile-based pathfinding.
 */
class ICmpObstructionManager : public IComponent
{
public:
	/**
	 * External identifiers for shapes.
	 * (This is a struct rather than a raw u32 for type-safety.)
	 */
	struct tag_t
	{
		tag_t() : n(0) {}
		explicit tag_t(u32 n) : n(n) {}
		bool valid() { return n != 0; }

		u32 n;
	};

	/**
	 * Boolean flags affecting the obstruction behaviour of a shape.
	 */
	enum EFlags
	{
		FLAG_BLOCK_MOVEMENT     = (1 << 0), // prevents units moving through this shape
		FLAG_BLOCK_FOUNDATION   = (1 << 1), // prevents foundations being placed on this shape
		FLAG_BLOCK_CONSTRUCTION = (1 << 2), // prevents buildings being constructed on this shape
		FLAG_BLOCK_PATHFINDING  = (1 << 3), // prevents the tile pathfinder choosing paths through this shape
		FLAG_MOVING             = (1 << 4)  // indicates this unit is currently moving
	};

	/**
	 * Bitmask of EFlag values.
	 */
	typedef u8 flags_t;

	/**
	 * Set the bounds of the world.
	 * Any point outside the bounds is considered obstructed.
	 * @param x0,z0,x1,z1 Coordinates of the corners of the world
	 */
	virtual void SetBounds(entity_pos_t x0, entity_pos_t z0, entity_pos_t x1, entity_pos_t z1) = 0;

	/**
	 * Register a static shape.
	 * @param ent entity ID associated with this shape (or INVALID_ENTITY if none)
	 * @param x X coordinate of center, in world space
	 * @param z Z coordinate of center, in world space
	 * @param a angle of rotation (clockwise from +Z direction)
	 * @param w width (size along X axis)
	 * @param h height (size along Z axis)
	 * @param flags a set of EFlags values
	 * @return a valid tag for manipulating the shape
	 */
	virtual tag_t AddStaticShape(entity_id_t ent, entity_pos_t x, entity_pos_t z, entity_angle_t a, entity_pos_t w, entity_pos_t h, flags_t flags) = 0;

	/**
	 * Register a unit shape.
	 * @param ent entity ID associated with this shape (or INVALID_ENTITY if none)
	 * @param x X coordinate of center, in world space
	 * @param z Z coordinate of center, in world space
	 * @param r radius (half the unit's width/height)
	 * @param flags a set of EFlags values
	 * @param moving whether the unit is currently moving through the world or is stationary
	 * @return a valid tag for manipulating the shape
	 */
	virtual tag_t AddUnitShape(entity_id_t ent, entity_pos_t x, entity_pos_t z, entity_angle_t r, flags_t flags, entity_id_t group) = 0;

	/**
	 * Adjust the position and angle of an existing shape.
	 * @param tag tag of shape (must be valid)
	 * @param x X coordinate of center, in world space
	 * @param z Z coordinate of center, in world space
	 * @param a angle of rotation (clockwise from +Z direction); ignored for unit shapes
	 */
	virtual void MoveShape(tag_t tag, entity_pos_t x, entity_pos_t z, entity_angle_t a) = 0;

	/**
	 * Set whether a unit shape is moving or stationary.
	 * @param tag tag of shape (must be valid and a unit shape)
	 * @param moving whether the unit is currently moving through the world or is stationary
	 */
	virtual void SetUnitMovingFlag(tag_t tag, bool moving) = 0;

	/**
	 * Set the control group of a unit shape.
	 * @param tag tag of shape (must be valid and a unit shape)
	 * @param group control group entity ID
	 */
	virtual void SetUnitControlGroup(tag_t tag, entity_id_t group) = 0;

	/**
	 * Remove an existing shape. The tag will be made invalid and must not be used after this.
	 * @param tag tag of shape (must be valid)
	 */
	virtual void RemoveShape(tag_t tag) = 0;

	/**
	 * Collision test a flat-ended thick line against the current set of shapes.
	 * The line caps extend by @p r beyond the end points.
	 * Only intersections going from outside to inside a shape are counted.
	 * @param filter filter to restrict the shapes that are counted
	 * @param x0 X coordinate of line's first point
	 * @param z0 Z coordinate of line's first point
	 * @param x1 X coordinate of line's second point
	 * @param z1 Z coordinate of line's second point
	 * @param r radius (half width) of line
	 * @return true if there is a collision
	 */
	virtual bool TestLine(const IObstructionTestFilter& filter, entity_pos_t x0, entity_pos_t z0, entity_pos_t x1, entity_pos_t z1, entity_pos_t r) = 0;

	/**
	 * Collision test a static square shape against the current set of shapes.
	 * @param filter filter to restrict the shapes that are counted
	 * @param x X coordinate of center
	 * @param z Z coordinate of center
	 * @param a angle of rotation (clockwise from +Z direction)
	 * @param w width (size along X axis)
	 * @param h height (size along Z axis)
	 * @param out if non-NULL, all colliding shapes' entities will be added to this list
	 * @return true if there is a collision
	 */
	virtual bool TestStaticShape(const IObstructionTestFilter& filter,
		entity_pos_t x, entity_pos_t z, entity_pos_t a, entity_pos_t w, entity_pos_t h,
		std::vector<entity_id_t>* out) = 0;

	/**
	 * Collision test a unit shape against the current set of shapes.
	 * @param filter filter to restrict the shapes that are counted
	 * @param x X coordinate of center
	 * @param z Z coordinate of center
	 * @param r radius (half the unit's width/height)
	 * @param out if non-NULL, all colliding shapes' entities will be added to this list
	 * @return true if there is a collision
	 */
	virtual bool TestUnitShape(const IObstructionTestFilter& filter,
		entity_pos_t x, entity_pos_t z, entity_pos_t r,
		std::vector<entity_id_t>* out) = 0;

	/**
	 * Bit-flags for Rasterise.
	 */
	enum TileObstruction
	{
		TILE_OBSTRUCTED_PATHFINDING = (1 << 0), // set if the tile pathfinder should consider this tile blocked
		TILE_OBSTRUCTED_FOUNDATION = (1 << 1), // set if the AI foundation placement algorithm should consider this tile blocked
		TILE_OUTOFBOUNDS = (1 << 2) // set if this tile is outside the world boundaries
	};

	/**
	 * Convert the current set of shapes onto a grid.
	 * Tiles that are intersected by a pathfind-blocking shape will have TILE_OBSTRUCTED_PATHFINDING set;
	 * tiles that are intersected by a foundation-blocking shape will also have TILE_OBSTRUCTED_FOUNDATION;
	 * tiles that are outside the world bounds will also have TILE_OUTOFBOUNDS;
	 * others will be set to 0.
	 * This is very cheap if the grid has been rasterised before and the set of shapes has not changed.
	 * @param grid the grid to be updated
	 * @return true if any changes were made to the grid, false if it was already up-to-date
	 */
	virtual bool Rasterise(Grid<u8>& grid) = 0;

	/**
	 * Standard representation for all types of shapes, for use with geometry processing code.
	 */
	struct ObstructionSquare
	{
		entity_pos_t x, z; // position of center
		CFixedVector2D u, v; // 'horizontal' and 'vertical' orthogonal unit vectors, representing orientation
		entity_pos_t hw, hh; // half width, half height of square
	};

	/**
	 * Find all the obstructions that are inside (or partially inside) the given range.
	 * @param filter filter to restrict the shapes that are counted
	 * @param x0 X coordinate of left edge of range
	 * @param z0 Z coordinate of bottom edge of range
	 * @param x1 X coordinate of right edge of range
	 * @param z1 Z coordinate of top edge of range
	 * @param squares output list of obstructions
	 */
	virtual void GetObstructionsInRange(const IObstructionTestFilter& filter, entity_pos_t x0, entity_pos_t z0, entity_pos_t x1, entity_pos_t z1, std::vector<ObstructionSquare>& squares) = 0;

	/**
	 * Find a single obstruction that blocks a unit at the given point with the given radius.
	 * Static obstructions (buildings) are more important than unit obstructions, and
	 * obstructions that cover the given point are more important than those that only cover
	 * the point expanded by the radius.
	 */
	virtual bool FindMostImportantObstruction(const IObstructionTestFilter& filter, entity_pos_t x, entity_pos_t z, entity_pos_t r, ObstructionSquare& square) = 0;

	/**
	 * Get the obstruction square representing the given shape.
	 * @param tag tag of shape (must be valid)
	 */
	virtual ObstructionSquare GetObstruction(tag_t tag) = 0;

	virtual ObstructionSquare GetUnitShapeObstruction(entity_pos_t x, entity_pos_t z, entity_pos_t r) = 0;

	virtual ObstructionSquare GetStaticShapeObstruction(entity_pos_t x, entity_pos_t z, entity_angle_t a, entity_pos_t w, entity_pos_t h) = 0;

	/**
	 * Set the passability to be restricted to a circular map.
	 */
	virtual void SetPassabilityCircular(bool enabled) = 0;

	/**
	 * Toggle the rendering of debug info.
	 */
	virtual void SetDebugOverlay(bool enabled) = 0;

	DECLARE_INTERFACE_TYPE(ObstructionManager)
};

/**
 * Interface for ICmpObstructionManager @c Test functions to filter out unwanted shapes.
 */
class IObstructionTestFilter
{
public:
	typedef ICmpObstructionManager::tag_t tag_t;
	typedef ICmpObstructionManager::flags_t flags_t;

	virtual ~IObstructionTestFilter() {}

	/**
	 * Return true if the shape should be counted for collisions.
	 * This is called for all shapes that would collide, and also for some that wouldn't.
	 * @param tag tag of shape being tested
	 * @param flags set of EFlags for the shape
	 * @param group the control group (typically the shape's unit, or the unit's formation controller, or 0)
	 */
	virtual bool Allowed(tag_t tag, flags_t flags, entity_id_t group) const = 0;
};

/**
 * Obstruction test filter that accepts all shapes.
 */
class NullObstructionFilter : public IObstructionTestFilter
{
public:
	virtual bool Allowed(tag_t UNUSED(tag), flags_t UNUSED(flags), entity_id_t UNUSED(group)) const
	{
		return true;
	}
};

/**
 * Obstruction test filter that accepts all non-moving shapes.
 */
class StationaryObstructionFilter : public IObstructionTestFilter
{
public:
	virtual bool Allowed(tag_t UNUSED(tag), flags_t flags, entity_id_t UNUSED(group)) const
	{
		return !(flags & ICmpObstructionManager::FLAG_MOVING);
	}
};

/**
 * Obstruction test filter that reject shapes in a given control group,
 * and optionally rejects moving shapes,
 * and rejects shapes that don't block unit movement.
 */
class ControlGroupMovementObstructionFilter : public IObstructionTestFilter
{
	bool m_AvoidMoving;
	entity_id_t m_Group;
public:
	ControlGroupMovementObstructionFilter(bool avoidMoving, entity_id_t group) :
		m_AvoidMoving(avoidMoving), m_Group(group)
	{
	}

	virtual bool Allowed(tag_t UNUSED(tag), flags_t flags, entity_id_t group) const
	{
		if (group == m_Group)
			return false;
		if (!(flags & ICmpObstructionManager::FLAG_BLOCK_MOVEMENT))
			return false;
		if ((flags & ICmpObstructionManager::FLAG_MOVING) && !m_AvoidMoving)
			return false;
		return true;
	}
};

/**
 * Obstruction test filter that rejects a specific shape.
 */
class SkipTagObstructionFilter : public IObstructionTestFilter
{
	tag_t m_Tag;
public:
	SkipTagObstructionFilter(tag_t tag) : m_Tag(tag)
	{
	}

	virtual bool Allowed(tag_t tag, flags_t UNUSED(flags), entity_id_t UNUSED(group)) const
	{
		return tag.n != m_Tag.n;
	}
};

/**
 * Obstruction test filter that rejects a specific shape, and requires the given flags.
 */
class SkipTagFlagsObstructionFilter : public IObstructionTestFilter
{
	tag_t m_Tag;
	flags_t m_Mask;
public:
	SkipTagFlagsObstructionFilter(tag_t tag, flags_t mask) : m_Tag(tag), m_Mask(mask)
	{
	}

	virtual bool Allowed(tag_t tag, flags_t flags, entity_id_t UNUSED(group)) const
	{
		return (tag.n != m_Tag.n && (flags & m_Mask) != 0);
	}
};

#endif // INCLUDED_ICMPOBSTRUCTIONMANAGER
