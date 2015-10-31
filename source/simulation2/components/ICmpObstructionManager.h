/* Copyright (C) 2015 Wildfire Games.
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

#include "simulation2/helpers/Pathfinding.h"

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
 * The @c Rasterize function approximates the current set of shapes onto a 2D grid,
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
	 * 
	 * @param ent entity ID associated with this shape (or INVALID_ENTITY if none)
	 * @param x,z coordinates of center, in world space
	 * @param a angle of rotation (clockwise from +Z direction)
	 * @param w width (size along X axis)
	 * @param h height (size along Z axis)
	 * @param flags a set of EFlags values
	 * @param group primary control group of the shape. Must be a valid control group ID.
	 * @param group2 Optional; secondary control group of the shape. Defaults to INVALID_ENTITY.
	 * @return a valid tag for manipulating the shape
	 * @see StaticShape
	 */
	virtual tag_t AddStaticShape(entity_id_t ent, entity_pos_t x, entity_pos_t z, entity_angle_t a, 
		entity_pos_t w, entity_pos_t h, flags_t flags, entity_id_t group, entity_id_t group2 = INVALID_ENTITY) = 0;

	/**
	 * Register a unit shape.
	 * 
	 * @param ent entity ID associated with this shape (or INVALID_ENTITY if none)
	 * @param x,z coordinates of center, in world space
	 * @param clearance pathfinding clearance of the unit (works as a radius)
	 * @param flags a set of EFlags values
	 * @param group control group (typically the owner entity, or a formation controller entity
	 *	- units ignore collisions with others in the same group)
	 * @return a valid tag for manipulating the shape
	 * @see UnitShape
	 */
	virtual tag_t AddUnitShape(entity_id_t ent, entity_pos_t x, entity_pos_t z, entity_pos_t clearance,
		flags_t flags, entity_id_t group) = 0;

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
	 * Sets the control group of a static shape.
	 * @param tag Tag of the shape to set the control group for. Must be a valid and static shape tag.
	 * @param group Control group entity ID.
	 */
	virtual void SetStaticControlGroup(tag_t tag, entity_id_t group, entity_id_t group2) = 0;

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
	 * @param filter filter to restrict the shapes that are being tested against
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
	 * Collision test a unit shape against the current set of registered shapes, and optionally writes a list of the colliding
	 * shapes' entities to an output list.
	 * 
	 * @param filter filter to restrict the shapes that are being tested against
	 * @param x X coordinate of shape's center
	 * @param z Z coordinate of shape's center
	 * @param clearance clearance of the shape's unit
	 * @param out if non-NULL, all colliding shapes' entities will be added to this list
	 * 
	 * @return true if there is a collision
	 */
	virtual bool TestUnitShape(const IObstructionTestFilter& filter,
		entity_pos_t x, entity_pos_t z, entity_pos_t clearance,
		std::vector<entity_id_t>* out) = 0;

	/**
	 * Convert the current set of shapes onto a navcell grid, for all passability classes contained in @p passClasses.
	 * If @p fullUpdate is false, the function will only go through dirty shapes.
	 * Shapes are expanded by the @p passClasses clearances, by ORing their masks onto the @p grid.
	 */
	virtual void Rasterize(Grid<NavcellData>& grid, const std::vector<PathfinderPassability>& passClasses, bool fullUpdate) = 0;

	/**
	 * Gets dirtiness information and resets it afterwards. Then it's the role of CCmpPathfinder
	 * to pass the information to other components if needed. (AIs, etc.)
	 * The return value is false if an update is unnecessary.
	 */
	virtual void UpdateInformations(GridUpdateInformation& informations) = 0;

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
	 * Returns the entity IDs of all unit shapes that intersect the given
	 * obstruction square, filtering out using the given filter.
	 */
	virtual void GetUnitsOnObstruction(const ObstructionSquare& square, std::vector<entity_id_t>& out, const IObstructionTestFilter& filter) = 0;

	/**
	 * Get the obstruction square representing the given shape.
	 * @param tag tag of shape (must be valid)
	 */
	virtual ObstructionSquare GetObstruction(tag_t tag) = 0;

	virtual ObstructionSquare GetUnitShapeObstruction(entity_pos_t x, entity_pos_t z, entity_pos_t clearance) = 0;

	virtual ObstructionSquare GetStaticShapeObstruction(entity_pos_t x, entity_pos_t z, entity_angle_t a, entity_pos_t w, entity_pos_t h) = 0;

	/**
	 * Set the passability to be restricted to a circular map.
	 */
	virtual void SetPassabilityCircular(bool enabled) = 0;

	virtual bool GetPassabilityCircular() const = 0;

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
	 * Return true if the shape with the specified parameters should be tested for collisions.
	 * This is called for all shapes that would collide, and also for some that wouldn't.
	 * 
	 * @param tag tag of shape being tested
	 * @param flags set of EFlags for the shape
	 * @param group the control group of the shape (typically the shape's unit, or the unit's formation controller, or 0)
	 * @param group2 an optional secondary control group of the shape, or INVALID_ENTITY if none specified. Currently 
	 *               exists only for static shapes.
	 */
	virtual bool TestShape(tag_t tag, flags_t flags, entity_id_t group, entity_id_t group2) const = 0;
};

/**
 * Obstruction test filter that will test against all shapes.
 */
class NullObstructionFilter : public IObstructionTestFilter
{
public:
	virtual bool TestShape(tag_t UNUSED(tag), flags_t UNUSED(flags), entity_id_t UNUSED(group), entity_id_t UNUSED(group2)) const
	{
		return true;
	}
};

/**
 * Obstruction test filter that will test only against stationary (i.e. non-moving) shapes.
 */
class StationaryOnlyObstructionFilter : public IObstructionTestFilter
{
public:
	virtual bool TestShape(tag_t UNUSED(tag), flags_t flags, entity_id_t UNUSED(group), entity_id_t UNUSED(group2)) const
	{
		return !(flags & ICmpObstructionManager::FLAG_MOVING);
	}
};

/**
 * Obstruction test filter that reject shapes in a given control group,
 * and rejects shapes that don't block unit movement, and optionally rejects moving shapes.
 */
class ControlGroupMovementObstructionFilter : public IObstructionTestFilter
{
	bool m_AvoidMoving;
	entity_id_t m_Group;

public:
	ControlGroupMovementObstructionFilter(bool avoidMoving, entity_id_t group) :
		m_AvoidMoving(avoidMoving), m_Group(group)
	{}

	virtual bool TestShape(tag_t UNUSED(tag), flags_t flags, entity_id_t group, entity_id_t group2) const
	{
		if (group == m_Group || (group2 != INVALID_ENTITY && group2 == m_Group))
			return false;

		if (!(flags & ICmpObstructionManager::FLAG_BLOCK_MOVEMENT))
			return false;

		if ((flags & ICmpObstructionManager::FLAG_MOVING) && !m_AvoidMoving)
			return false;

		return true;
	}
};

/**
 * Obstruction test filter that will test only against shapes that:
 *     - are part of neither one of the specified control groups
 *     - AND, depending on the value of the 'exclude' argument:
 *       - have at least one of the specified flags set.
 *       - OR have none of the specified flags set.
 * 
 * The first (primary) control group to reject shapes from must be specified and valid. The secondary
 * control group to reject entities from may be set to INVALID_ENTITY to not use it.
 * 
 * This filter is useful to e.g. allow foundations within the same control group to be placed and 
 * constructed arbitrarily close together (e.g. for wall pieces that need to link up tightly).
 */
class SkipControlGroupsRequireFlagObstructionFilter : public IObstructionTestFilter
{
	bool m_Exclude;
	entity_id_t m_Group;
	entity_id_t m_Group2;
	flags_t m_Mask;

public:
	SkipControlGroupsRequireFlagObstructionFilter(bool exclude, entity_id_t group1, entity_id_t group2, flags_t mask) : 
		m_Exclude(exclude), m_Group(group1), m_Group2(group2), m_Mask(mask)
	{
		Init();
	}

	SkipControlGroupsRequireFlagObstructionFilter(entity_id_t group1, entity_id_t group2, flags_t mask) : 
		m_Exclude(false), m_Group(group1), m_Group2(group2), m_Mask(mask)
	{
		Init();
	}

	virtual bool TestShape(tag_t UNUSED(tag), flags_t flags, entity_id_t group, entity_id_t group2) const
	{
		// Don't test shapes that share one or more of our control groups.
		if (group == m_Group || group == m_Group2 || (group2 != INVALID_ENTITY &&
				(group2 == m_Group || group2 == m_Group2)))
			return false;

		// If m_Exclude is true, don't test against shapes that have any of the
		// obstruction flags specified in m_Mask.
		if (m_Exclude)
			return (flags & m_Mask) == 0;

		// Otherwise, only include shapes that match at least one flag in m_Mask.
		return (flags & m_Mask) != 0;
	}
private:
	void Init()
	{
		// the primary control group to filter out must be valid
		ENSURE(m_Group != INVALID_ENTITY);

		// for simplicity, if m_Group2 is INVALID_ENTITY (i.e. not used), then set it equal to m_Group
		// so that we have fewer special cases to consider in TestShape().
		if (m_Group2 == INVALID_ENTITY)
			m_Group2 = m_Group;
	}
};

/**
 * Obstruction test filter that will test only against shapes that:
 *     - are part of both of the specified control groups
 *     - AND have at least one of the specified flags set.
 * 
 * The first (primary) control group to include shapes from must be specified and valid.
 * 
 * This filter is useful for preventing entities with identical control groups
 * from colliding e.g. building a new wall segment on top of an existing wall)
 *
 * @todo This filter needs test cases.
 */
class SkipTagRequireControlGroupsAndFlagObstructionFilter : public IObstructionTestFilter
{
	tag_t m_Tag;
	entity_id_t m_Group;
	entity_id_t m_Group2;
	flags_t m_Mask;

public:
	SkipTagRequireControlGroupsAndFlagObstructionFilter(tag_t tag, entity_id_t group1, entity_id_t group2, flags_t mask) : 
		m_Tag(tag), m_Group(group1), m_Group2(group2), m_Mask(mask)
	{
		ENSURE(m_Group != INVALID_ENTITY);
	}

	virtual bool TestShape(tag_t tag, flags_t flags, entity_id_t group, entity_id_t group2) const
	{
		// To be included in testing, a shape must not have the specified tag, and must
		// match at least one of the flags in m_Mask, as well as both control groups.
		return (tag.n != m_Tag.n && (flags & m_Mask) != 0 && ((group == m_Group
			&& group2 == m_Group2) || (group2 == m_Group && group == m_Group2)));
	}
};

/**
 * Obstruction test filter that will test only against shapes that do not have the specified tag set.
 */
class SkipTagObstructionFilter : public IObstructionTestFilter
{
	tag_t m_Tag;
public:
	SkipTagObstructionFilter(tag_t tag) : m_Tag(tag)
	{
	}

	virtual bool TestShape(tag_t tag, flags_t UNUSED(flags), entity_id_t UNUSED(group), entity_id_t UNUSED(group2)) const
	{
		return tag.n != m_Tag.n;
	}
};

/**
 * Obstruction test filter that will test only against shapes that:
 *    - do not have the specified tag
 *    - AND have at least one of the specified flags set.
 */
class SkipTagRequireFlagsObstructionFilter : public IObstructionTestFilter
{
	tag_t m_Tag;
	flags_t m_Mask;
public:
	SkipTagRequireFlagsObstructionFilter(tag_t tag, flags_t mask) : m_Tag(tag), m_Mask(mask)
	{
	}

	virtual bool TestShape(tag_t tag, flags_t flags, entity_id_t UNUSED(group), entity_id_t UNUSED(group2)) const
	{
		return (tag.n != m_Tag.n && (flags & m_Mask) != 0);
	}
};

#endif // INCLUDED_ICMPOBSTRUCTIONMANAGER
