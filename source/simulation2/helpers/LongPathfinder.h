/* Copyright (C) 2021 Wildfire Games.
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

#ifndef INCLUDED_LONGPATHFINDER
#define INCLUDED_LONGPATHFINDER

#include "Pathfinding.h"

#include "graphics/Overlay.h"
#include "renderer/Scene.h"
#include "renderer/TerrainOverlay.h"
#include "simulation2/helpers/Grid.h"
#include "simulation2/helpers/PriorityQueue.h"

#include <map>

/**
 * Represents the 2D coordinates of a tile.
 * The i/j components are packed into a single u32, since we usually use these
 * objects for equality comparisons and the VC2010 optimizer doesn't seem to automatically
 * compare two u16s in a single operation.
 * TODO: maybe VC2012 will?
 */
struct TileID
{
	TileID() { }

	TileID(u16 i, u16 j) : data((i << 16) | j) { }

	bool operator==(const TileID& b) const
	{
		return data == b.data;
	}

	/// Returns lexicographic order over (i,j)
	bool operator<(const TileID& b) const
	{
		return data < b.data;
	}

	u16 i() const { return data >> 16; }
	u16 j() const { return data & 0xFFFF; }

private:
	u32 data;
};

/**
 * Tile data for A* computation.
 * (We store an array of one of these per terrain tile, so it ought to be optimised for size)
 */
struct PathfindTile
{
public:
	enum {
		STATUS_UNEXPLORED = 0,
		STATUS_OPEN = 1,
		STATUS_CLOSED = 2
	};

	bool IsUnexplored() const { return GetStatus() == STATUS_UNEXPLORED; }
	bool IsOpen() const { return GetStatus() == STATUS_OPEN; }
	bool IsClosed() const { return GetStatus() == STATUS_CLOSED; }
	void SetStatusOpen() { SetStatus(STATUS_OPEN); }
	void SetStatusClosed() { SetStatus(STATUS_CLOSED); }

	// Get pi,pj coords of predecessor to this tile on best path, given i,j coords of this tile
	inline int GetPredI(int i) const { return i + GetPredDI(); }
	inline int GetPredJ(int j) const { return j + GetPredDJ(); }

	inline PathCost GetCost() const { return g; }
	inline void SetCost(PathCost cost) { g = cost; }

private:
	PathCost g; // cost to reach this tile
	u32 data; // 2-bit status; 15-bit PredI; 15-bit PredJ; packed for storage efficiency

public:
	inline u8 GetStatus() const
	{
		return data & 3;
	}

	inline void SetStatus(u8 s)
	{
		ASSERT(s < 4);
		data &= ~3;
		data |= (s & 3);
	}

	int GetPredDI() const
	{
		return (i32)data >> 17;
	}

	int GetPredDJ() const
	{
		return ((i32)data << 15) >> 17;
	}

	// Set the pi,pj coords of predecessor, given i,j coords of this tile
	inline void SetPred(int pi, int pj, int i, int j)
	{
		int di = pi - i;
		int dj = pj - j;
		ASSERT(-16384 <= di && di < 16384);
		ASSERT(-16384 <= dj && dj < 16384);
		data &= 3;
		data |= (((u32)di & 0x7FFF) << 17) | (((u32)dj & 0x7FFF) << 2);
	}
};

struct CircularRegion
{
	CircularRegion(entity_pos_t x, entity_pos_t z, entity_pos_t r) : x(x), z(z), r(r) {}
	entity_pos_t x, z, r;
};

typedef PriorityQueueHeap<TileID, PathCost, PathCost> PriorityQueue;
typedef SparseGrid<PathfindTile> PathfindTileGrid;

class JumpPointCache;

struct PathfinderState
{
	u32 steps; // number of algorithm iterations

	PathGoal goal;

	u16 iGoal, jGoal; // goal tile

	pass_class_t passClass;

	PriorityQueue open;
	// (there's no explicit closed list; it's encoded in PathfindTile)

	PathfindTileGrid* tiles;
	Grid<NavcellData>* terrain;

	PathCost hBest; // heuristic of closest discovered tile to goal
	u16 iBest, jBest; // closest tile

	const JumpPointCache* jpc;
};

class LongOverlay;

class HierarchicalPathfinder;

class LongPathfinder
{
public:
	LongPathfinder();
	~LongPathfinder();

	void SetDebugOverlay(bool enabled);

	void SetDebugPath(const HierarchicalPathfinder& hierPath, entity_pos_t x0, entity_pos_t z0, const PathGoal& goal, pass_class_t passClass)
	{
		if (!m_Debug.Overlay)
			return;

		SAFE_DELETE(m_Debug.Grid);
		delete m_Debug.Path;
		m_Debug.Path = new WaypointPath();
		ComputePath(hierPath, x0, z0, goal, passClass, *m_Debug.Path);
		m_Debug.PassClass = passClass;
	}

	void Reload(Grid<NavcellData>* passabilityGrid)
	{
		m_Grid = passabilityGrid;
		ASSERT(passabilityGrid->m_H == passabilityGrid->m_W);
		m_GridSize = passabilityGrid->m_W;

		m_JumpPointCache.clear();
	}

	void Update(Grid<NavcellData>* passabilityGrid)
	{
		m_Grid = passabilityGrid;
		ASSERT(passabilityGrid->m_H == passabilityGrid->m_W);
		ASSERT(m_GridSize == passabilityGrid->m_H);

		m_JumpPointCache.clear();
	}

	/**
	 * Compute a tile-based path from the given point to the goal, and return the set of waypoints.
	 * The waypoints correspond to the centers of horizontally/vertically adjacent tiles
	 * along the path.
	 */
	void ComputePath(const HierarchicalPathfinder& hierPath, entity_pos_t x0, entity_pos_t z0, const PathGoal& origGoal,
	    pass_class_t passClass, WaypointPath& path) const;

	/**
	 * Compute a tile-based path from the given point to the goal, excluding the regions
	 * specified in excludedRegions (which are treated as impassable) and return the set of waypoints.
	 * The waypoints correspond to the centers of horizontally/vertically adjacent tiles
	 * along the path.
	 */
	void ComputePath(const HierarchicalPathfinder& hierPath, entity_pos_t x0, entity_pos_t z0, const PathGoal& origGoal,
		pass_class_t passClass, std::vector<CircularRegion> excludedRegions, WaypointPath& path);

	void GetDebugData(u32& steps, double& time, Grid<u8>& grid) const
	{
		GetDebugDataJPS(steps, time, grid);
	}

	Grid<NavcellData>* m_Grid;
	u16 m_GridSize;

	// Debugging - output from last pathfind operation.
	struct Debug
	{
		// Atomic - used to toggle debugging.
		std::atomic<LongOverlay*> Overlay = nullptr;
		// Mutable - set by ComputeJPSPath (thus possibly from different threads).
		// Synchronized via mutex if necessary.
		mutable PathfindTileGrid* Grid = nullptr;
		mutable u32 Steps;
		mutable double Time;
		mutable PathGoal Goal;

		WaypointPath* Path = nullptr;
		pass_class_t PassClass;
	} m_Debug;

private:
	PathCost CalculateHeuristic(int i, int j, int iGoal, int jGoal) const;
	void ProcessNeighbour(int pi, int pj, int i, int j, PathCost pg, PathfinderState& state) const;

	/**
	 * JPS algorithm helper functions
	 * @param detectGoal is not used if m_UseJPSCache is true
	 */
	void AddJumpedHoriz(int i, int j, int di, PathCost g, PathfinderState& state, bool detectGoal) const;
	int HasJumpedHoriz(int i, int j, int di, PathfinderState& state, bool detectGoal) const;
	void AddJumpedVert(int i, int j, int dj, PathCost g, PathfinderState& state, bool detectGoal) const;
	int HasJumpedVert(int i, int j, int dj, PathfinderState& state, bool detectGoal) const;
	void AddJumpedDiag(int i, int j, int di, int dj, PathCost g, PathfinderState& state) const;

	/**
	 * See LongPathfinder.cpp for implementation details
	 * TODO: cleanup documentation
	 */
	void ComputeJPSPath(const HierarchicalPathfinder& hierPath, entity_pos_t x0, entity_pos_t z0, const PathGoal& origGoal, pass_class_t passClass, WaypointPath& path) const;
	void GetDebugDataJPS(u32& steps, double& time, Grid<u8>& grid) const;

	// Helper functions for ComputePath

	/**
	 * Given a path with an arbitrary collection of waypoints, updates the
	 * waypoints to be nicer.
	 * If @param maxDist is non-zero, path waypoints will be espaced by at most @param maxDist.
	 * In that case the distance between (x0, z0) and the first waypoint will also be made less than maxDist.
	 */
	void ImprovePathWaypoints(WaypointPath& path, pass_class_t passClass, entity_pos_t maxDist, entity_pos_t x0, entity_pos_t z0) const;

	/**
	 * Generate a passability map, stored in the 16th bit of navcells, based on passClass,
	 * but with a set of impassable circular regions.
	 */
	void GenerateSpecialMap(pass_class_t passClass, std::vector<CircularRegion> excludedRegions);

	bool m_UseJPSCache;
	// Mutable may be used here as caching does not change the external const-ness of the Long Range pathfinder.
	// This is thread-safe as it is order independent (no change in the output of the function for a given set of params).
	// Obviously, this means that the cache should actually be a cache and not return different results
	// from what would happen if things hadn't been cached.
	mutable std::map<pass_class_t, std::shared_ptr<JumpPointCache>> m_JumpPointCache;
};

#endif // INCLUDED_LONGPATHFINDER
