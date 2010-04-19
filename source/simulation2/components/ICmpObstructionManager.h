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

#ifndef INCLUDED_ICMPOBSTRUCTIONMANAGER
#define INCLUDED_ICMPOBSTRUCTIONMANAGER

#include "simulation2/system/Interface.h"

#include "simulation2/helpers/Grid.h"
#include "simulation2/helpers/Position.h"

class IObstructionTestFilter;

/**
 * Obstruction manager: provides efficient spatial queries over objects in the world.
 *
 * The class exposes the abstraction of "shapes", which represent circles or squares
 * with certain properties.
 * Other classes (particularly ICmpObstruction) register shapes with this interface
 * and keep them updated.
 *
 * The @c Test functions provide exact collision tests.
 * The edge of a shape counts as 'inside' the shape, for the purpose of collisions.
 * The functions accept an IObstructionTestFilter argument, which can restrict the
 * set of shapes that are counted as collisions.
 *
 * The @c Rasterise function approximates the current set of shapes onto a 2D grid,
 * primarily for pathfinding.
 */
class ICmpObstructionManager : public IComponent
{
public:
	/**
	 * External identifiers for shapes. Valid tags are guaranteed to be non-zero.
	 */
	typedef u32 tag_t;

	/**
	 * Register a circle.
	 * @param x X coordinate of center, in world space
	 * @param z Z coordinate of center, in world space
	 * @param r radius
	 * @return a valid tag for manipulating the shape
	 */
	virtual tag_t AddCircle(entity_pos_t x, entity_pos_t z, entity_pos_t r) = 0;

	/**
	 * Register a square.
	 * @param x X coordinate of center, in world space
	 * @param z Z coordinate of center, in world space
	 * @param a angle of rotation (clockwise from +Z direction)
	 * @param w width (size along X axis)
	 * @param h height (size along Z axis)
	 * @return a valid tag for manipulating the shape
	 */
	virtual tag_t AddSquare(entity_pos_t x, entity_pos_t z, entity_angle_t a, entity_pos_t w, entity_pos_t h) = 0;

	/**
	 * Adjust the position and angle of an existing shape.
	 * @param tag tag of shape (must be valid)
	 * @param x X coordinate of center, in world space
	 * @param z Z coordinate of center, in world space
	 * @param a angle of rotation (clockwise from +Z direction); ignored for circles
	 */
	virtual void MoveShape(tag_t tag, entity_pos_t x, entity_pos_t z, entity_angle_t a) = 0;

	/**
	 * Remove an existing shape. The tag will be made invalid and must not be used after this.
	 * @param tag tag of shape (must be valid)
	 */
	virtual void RemoveShape(tag_t tag) = 0;

	/**
	 * Collision test a flat-ended thick line against the current set of shapes.
	 * @param filter filter to restrict the shapes that are counted
	 * @param x0 X coordinate of line's first point
	 * @param z0 Z coordinate of line's first point
	 * @param x1 X coordinate of line's second point
	 * @param z1 Z coordinate of line's second point
	 * @param r radius (half width) of line
	 * @return false if there is a collision
	 */
	virtual bool TestLine(const IObstructionTestFilter& filter, entity_pos_t x0, entity_pos_t z0, entity_pos_t x1, entity_pos_t z1, entity_pos_t r) = 0;

	/**
	 * Collision test a circle against the current set of shapes.
	 * @param filter filter to restrict the shapes that are counted
	 * @param x X coordinate of center
	 * @param z Z coordinate of center
	 * @param r radius of circle
	 * @return false if there is a collision
	 */
	virtual bool TestCircle(const IObstructionTestFilter& filter, entity_pos_t x, entity_pos_t z, entity_pos_t r) = 0;

	/**
	 * Collision test a square against the current set of shapes.
	 * @param filter filter to restrict the shapes that are counted
	 * @param x X coordinate of center
	 * @param z Z coordinate of center
	 * @param a angle of rotation (clockwise from +Z direction)
	 * @param w width (size along X axis)
	 * @param h height (size along Z axis)
	 * @return false if there is a collision
	 */
	virtual bool TestSquare(const IObstructionTestFilter& filter, entity_pos_t x, entity_pos_t z, entity_pos_t a, entity_pos_t w, entity_pos_t h) = 0;

	/**
	 * Convert the current set of shapes onto a grid.
	 * Tiles that are partially or completely intersected by a shape will be set to 1;
	 * others will be set to 0.
	 * This is very cheap if the grid has been rasterised before and the set of shapes has not changed.
	 * @param grid the grid to be updated
	 * @return true if any changes were made to the grid, false if it was already up-to-date
	 */
	virtual bool Rasterise(Grid<u8>& grid) = 0;

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
	virtual ~IObstructionTestFilter() {}

	/**
	 * Return true if the shape should be counted for collisions.
	 * This is called for all shapes that would collide, and also for some that wouldn't.
	 * @param tag tag of shape being tested
	 */
	virtual bool Allowed(ICmpObstructionManager::tag_t tag) const = 0;
};

/**
 * Obstruction test filter that accepts all shapes.
 */
class NullObstructionFilter : public IObstructionTestFilter
{
public:
	virtual bool Allowed(ICmpObstructionManager::tag_t UNUSED(tag)) const { return true; }
};

/**
 * Obstruction test filter that rejects a specific shape.
 */
class SkipTagObstructionFilter : public IObstructionTestFilter
{
	ICmpObstructionManager::tag_t m_Tag;
public:
	SkipTagObstructionFilter(ICmpObstructionManager::tag_t tag) : m_Tag(tag) {}
	virtual bool Allowed(ICmpObstructionManager::tag_t tag) const { return tag != m_Tag; }
};

#endif // INCLUDED_ICMPOBSTRUCTIONMANAGER
