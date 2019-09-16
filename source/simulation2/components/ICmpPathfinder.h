/* Copyright (C) 2019 Wildfire Games.
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

#ifndef INCLUDED_ICMPPATHFINDER
#define INCLUDED_ICMPPATHFINDER

#include "simulation2/system/Interface.h"

#include "simulation2/components/ICmpObstruction.h"
#include "simulation2/helpers/PathGoal.h"
#include "simulation2/helpers/Pathfinding.h"

#include "maths/FixedVector2D.h"

#include <map>
#include <vector>

class IObstructionTestFilter;

template<typename T> class Grid;

// Returned by asynchronous workers, used to send messages in the main thread.
struct WaypointPath;

struct PathResult
{
	PathResult() = default;
	PathResult(u32 t, entity_id_t n, WaypointPath p) : ticket(t), notify(n), path(p) {};

	u32 ticket;
	entity_id_t notify;
	WaypointPath path;
};

/**
 * Pathfinder algorithms.
 *
 * There are two different modes: a tile-based pathfinder that works over long distances and
 * accounts for terrain costs but ignore units, and a 'short' vertex-based pathfinder that
 * provides precise paths and avoids other units.
 *
 * Both use the same concept of a PathGoal: either a point, circle or square.
 * (If the starting point is inside the goal shape then the path will move outwards
 * to reach the shape's outline.)
 *
 * The output is a list of waypoints.
 */
class ICmpPathfinder : public IComponent
{
public:

	/**
	 * Get the list of all known passability classes.
	 */
	virtual void GetPassabilityClasses(std::map<std::string, pass_class_t>& passClasses) const = 0;

	/**
	* Get the list of passability classes, separating pathfinding classes and others.
	*/
	virtual void GetPassabilityClasses(
		std::map<std::string, pass_class_t>& nonPathfindingPassClasses,
		std::map<std::string, pass_class_t>& pathfindingPassClasses) const = 0;

	/**
	 * Get the tag for a given passability class name.
	 * Logs an error and returns something acceptable if the name is unrecognised.
	 */
	virtual pass_class_t GetPassabilityClass(const std::string& name) const = 0;

	virtual entity_pos_t GetClearance(pass_class_t passClass) const = 0;

	/**
	 * Get the larger clearance in all passability classes.
	 */
	virtual entity_pos_t GetMaximumClearance() const = 0;

	virtual const Grid<NavcellData>& GetPassabilityGrid() = 0;

	/**
	 * Get the accumulated dirtiness information since the last time the AI accessed and flushed it.
	 */
	virtual const GridUpdateInformation& GetAIPathfinderDirtinessInformation() const = 0;
	virtual void FlushAIPathfinderDirtinessInformation() = 0;

	/**
	 * Get a grid representing the distance to the shore of the terrain tile.
	 */
	virtual Grid<u16> ComputeShoreGrid(bool expandOnWater = false) = 0;

	/**
	 * Asynchronous version of ComputePath.
	 * Request a long path computation, asynchronously.
	 * The result will be sent as CMessagePathResult to 'notify'.
	 * Returns a unique non-zero number, which will match the 'ticket' in the result,
	 * so callers can recognise each individual request they make.
	 */
	virtual u32 ComputePathAsync(entity_pos_t x0, entity_pos_t z0, const PathGoal& goal, pass_class_t passClass, entity_id_t notify) = 0;

	/*
	 * Request a long-path computation immediately
	 */
	virtual void ComputePathImmediate(entity_pos_t x0, entity_pos_t z0, const PathGoal& goal, pass_class_t passClass, WaypointPath& ret) const = 0;

	/**
	 * Request a short path computation, asynchronously.
	 * The result will be sent as CMessagePathResult to 'notify'.
	 * Returns a unique non-zero number, which will match the 'ticket' in the result,
	 * so callers can recognise each individual request they make.
	 */
	virtual u32 ComputeShortPathAsync(entity_pos_t x0, entity_pos_t z0, entity_pos_t clearance, entity_pos_t range, const PathGoal& goal, pass_class_t passClass, bool avoidMovingUnits, entity_id_t controller, entity_id_t notify) = 0;

	/*
	 * Request a short-path computation immediately.
	 */
	virtual WaypointPath ComputeShortPathImmediate(const ShortPathRequest& request) const = 0;

	/**
	 * If the debug overlay is enabled, render the path that will computed by ComputePath.
	 */
	virtual void SetDebugPath(entity_pos_t x0, entity_pos_t z0, const PathGoal& goal, pass_class_t passClass) = 0;

	/**
	 * Check whether the given movement line is valid and doesn't hit any obstructions
	 * or impassable terrain.
	 * Returns true if the movement is okay.
	 */
	virtual bool CheckMovement(const IObstructionTestFilter& filter, entity_pos_t x0, entity_pos_t z0, entity_pos_t x1, entity_pos_t z1, entity_pos_t r, pass_class_t passClass) const = 0;

	/**
	 * Check whether a unit placed here is valid and doesn't hit any obstructions
	 * or impassable terrain.
	 * When onlyCenterPoint = true, only check the center tile of the unit
	 * @return ICmpObstruction::FOUNDATION_CHECK_SUCCESS if the placement is okay, else
	 *	a value describing the type of failure.
	 */
	virtual ICmpObstruction::EFoundationCheck CheckUnitPlacement(const IObstructionTestFilter& filter, entity_pos_t x, entity_pos_t z, entity_pos_t r, pass_class_t passClass, bool onlyCenterPoint = false) const = 0;

	/**
	 * Check whether a building placed here is valid and doesn't hit any obstructions
	 * or impassable terrain.
	 * @return ICmpObstruction::FOUNDATION_CHECK_SUCCESS if the placement is okay, else
	 *	a value describing the type of failure.
	 */
	virtual ICmpObstruction::EFoundationCheck CheckBuildingPlacement(const IObstructionTestFilter& filter, entity_pos_t x, entity_pos_t z, entity_pos_t a, entity_pos_t w, entity_pos_t h, entity_id_t id, pass_class_t passClass) const = 0;

	/**
	 * Check whether a building placed here is valid and doesn't hit any obstructions
	 * or impassable terrain.
	 * when onlyCenterPoint = true, only check the center tile of the building
	 * @return ICmpObstruction::FOUNDATION_CHECK_SUCCESS if the placement is okay, else
	 *	a value describing the type of failure.
	 */
	virtual ICmpObstruction::EFoundationCheck CheckBuildingPlacement(const IObstructionTestFilter& filter, entity_pos_t x, entity_pos_t z, entity_pos_t a, entity_pos_t w, entity_pos_t h, entity_id_t id, pass_class_t passClass, bool onlyCenterPoint) const = 0;


	/**
	 * Toggle the storage and rendering of debug info.
	 */
	virtual void SetDebugOverlay(bool enabled) = 0;

	/**
	 * Toggle the storage and rendering of debug info for the hierarchical pathfinder.
	 */
	virtual void SetHierDebugOverlay(bool enabled) = 0;

	/**
	 * Finish computing asynchronous path requests and send the CMessagePathResult messages.
	 */
	virtual void FetchAsyncResultsAndSendMessages() = 0;

	/**
	 * Tell asynchronous pathfinder threads that they can begin computing paths.
	 */
	virtual void StartProcessingMoves(bool useMax) = 0;

	/**
	 * Regenerates the grid based on the current obstruction list, if necessary
	 */
	virtual void UpdateGrid() = 0;

	/**
	 * Returns some stats about the last ComputePath.
	 */
	virtual void GetDebugData(u32& steps, double& time, Grid<u8>& grid) const = 0;

	/**
	 * Sets up the pathfinder passability overlay in Atlas.
	 */
	virtual void SetAtlasOverlay(bool enable, pass_class_t passClass = 0) = 0;

	DECLARE_INTERFACE_TYPE(Pathfinder)
};

#endif // INCLUDED_ICMPPATHFINDER
