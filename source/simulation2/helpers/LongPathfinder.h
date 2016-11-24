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

#ifndef INCLUDED_LONGPATHFINDER
#define INCLUDED_LONGPATHFINDER

#include "Pathfinding.h"
#include "HierarchicalPathfinder.h"

#include "PriorityQueue.h"
#include "graphics/Overlay.h"
#include "renderer/Scene.h"

#define ACCEPT_DIAGONAL_GAPS 0

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

	bool IsUnexplored() { return GetStatus() == STATUS_UNEXPLORED; }
	bool IsOpen() { return GetStatus() == STATUS_OPEN; }
	bool IsClosed() { return GetStatus() == STATUS_CLOSED; }
	void SetStatusOpen() { SetStatus(STATUS_OPEN); }
	void SetStatusClosed() { SetStatus(STATUS_CLOSED); }

	// Get pi,pj coords of predecessor to this tile on best path, given i,j coords of this tile
	inline int GetPredI(int i) { return i + GetPredDI(); }
	inline int GetPredJ(int j) { return j + GetPredDJ(); }

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

	JumpPointCache* jpc;
};

class LongOverlay;

class LongPathfinder
{
public:
	LongPathfinder();
	~LongPathfinder();

	void SetDebugOverlay(bool enabled);

	void SetHierDebugOverlay(bool enabled, const CSimContext *simContext)
	{
		m_PathfinderHier.SetDebugOverlay(enabled, simContext);
	}

	void SetDebugPath(entity_pos_t x0, entity_pos_t z0, const PathGoal& goal, pass_class_t passClass)
	{
		if (!m_DebugOverlay)
			return;

		SAFE_DELETE(m_DebugGrid);
		delete m_DebugPath;
		m_DebugPath = new WaypointPath();
		ComputePath(x0, z0, goal, passClass, *m_DebugPath);
		m_DebugPassClass = passClass;
	}

	void Reload(Grid<NavcellData>* passabilityGrid,
		const std::map<std::string, pass_class_t>& nonPathfindingPassClassMasks,
		const std::map<std::string, pass_class_t>& pathfindingPassClassMasks)
	{
		m_Grid = passabilityGrid;
		ASSERT(passabilityGrid->m_H == passabilityGrid->m_W);
		m_GridSize = passabilityGrid->m_W;

		m_JumpPointCache.clear();

		m_PathfinderHier.Recompute(passabilityGrid, nonPathfindingPassClassMasks, pathfindingPassClassMasks);
	}

	void Update(Grid<NavcellData>* passabilityGrid, const Grid<u8>& dirtinessGrid)
	{
		m_Grid = passabilityGrid;
		ASSERT(passabilityGrid->m_H == passabilityGrid->m_W);
		ASSERT(m_GridSize == passabilityGrid->m_H);

		m_JumpPointCache.clear();

		m_PathfinderHier.Update(passabilityGrid, dirtinessGrid);
	}

	void HierarchicalRenderSubmit(SceneCollector& collector)
	{
		for (size_t i = 0; i < m_PathfinderHier.m_DebugOverlayLines.size(); ++i)
			collector.Submit(&m_PathfinderHier.m_DebugOverlayLines[i]);
	}

	/**
	 * Compute a tile-based path from the given point to the goal, and return the set of waypoints.
	 * The waypoints correspond to the centers of horizontally/vertically adjacent tiles
	 * along the path.
	 */
	void ComputePath(entity_pos_t x0, entity_pos_t z0, const PathGoal& origGoal,
		pass_class_t passClass, WaypointPath& path)
	{
		if (!m_Grid)
		{
			LOGERROR("The pathfinder grid hasn't been setup yet, aborting ComputePath");
			return;
		}

		ComputeJPSPath(x0, z0, origGoal, passClass, path);
	}

	/**
	 * Compute a tile-based path from the given point to the goal, excluding the regions
	 * specified in excludedRegions (which are treated as impassable) and return the set of waypoints.
	 * The waypoints correspond to the centers of horizontally/vertically adjacent tiles
	 * along the path.
	 */
	void ComputePath(entity_pos_t x0, entity_pos_t z0, const PathGoal& origGoal,
		pass_class_t passClass, std::vector<CircularRegion> excludedRegions, WaypointPath& path);

	Grid<u16> GetConnectivityGrid(pass_class_t passClass)
	{
		return m_PathfinderHier.GetConnectivityGrid(passClass);
	}

	void GetDebugData(u32& steps, double& time, Grid<u8>& grid)
	{
		GetDebugDataJPS(steps, time, grid);
	}

	Grid<NavcellData>* m_Grid;
	u16 m_GridSize;

	// Debugging - output from last pathfind operation:
	LongOverlay* m_DebugOverlay;
	PathfindTileGrid* m_DebugGrid;
	u32 m_DebugSteps;
	double m_DebugTime;
	PathGoal m_DebugGoal;
	WaypointPath* m_DebugPath;
	pass_class_t m_DebugPassClass;

private:
	PathCost CalculateHeuristic(int i, int j, int iGoal, int jGoal);
	void ProcessNeighbour(int pi, int pj, int i, int j, PathCost pg, PathfinderState& state);

	/**
	 * JPS algorithm helper functions
	 * @param detectGoal is not used if m_UseJPSCache is true
	 */
	void AddJumpedHoriz(int i, int j, int di, PathCost g, PathfinderState& state, bool detectGoal);
	int HasJumpedHoriz(int i, int j, int di, PathfinderState& state, bool detectGoal);
	void AddJumpedVert(int i, int j, int dj, PathCost g, PathfinderState& state, bool detectGoal);
	int HasJumpedVert(int i, int j, int dj, PathfinderState& state, bool detectGoal);
	void AddJumpedDiag(int i, int j, int di, int dj, PathCost g, PathfinderState& state);

	/**
	 * See LongPathfinder.cpp for implementation details
	 * TODO: cleanup documentation
	 */
	void ComputeJPSPath(entity_pos_t x0, entity_pos_t z0, const PathGoal& origGoal, pass_class_t passClass, WaypointPath& path);
	void GetDebugDataJPS(u32& steps, double& time, Grid<u8>& grid);

	// Helper functions for ComputePath

	/**
	 * Given a path with an arbitrary collection of waypoints, updates the
	 * waypoints to be nicer. Calls "Testline" between waypoints
	 * so that bended paths can become straight if there's nothing in between
	 * (this happens because A* is 8-direction, and the map isn't actually a grid).
	 * If @param maxDist is non-zero, path waypoints will be espaced by at most @param maxDist.
	 * In that case the distance between (x0, z0) and the first waypoint will also be made less than maxDist.
	 */
	void ImprovePathWaypoints(WaypointPath& path, pass_class_t passClass, entity_pos_t maxDist, entity_pos_t x0, entity_pos_t z0);

	/**
	 * Generate a passability map, stored in the 16th bit of navcells, based on passClass,
	 * but with a set of impassable circular regions.
	 */
	void GenerateSpecialMap(pass_class_t passClass, std::vector<CircularRegion> excludedRegions);

	bool m_UseJPSCache;
	std::map<pass_class_t, shared_ptr<JumpPointCache> > m_JumpPointCache;

	HierarchicalPathfinder m_PathfinderHier;
};

/**
 * Terrain overlay for pathfinder debugging.
 * Renders a representation of the most recent pathfinding operation.
 */
class LongOverlay : public TerrainTextureOverlay
{
public:
	LongPathfinder& m_Pathfinder;

	LongOverlay(LongPathfinder& pathfinder) :
		TerrainTextureOverlay(Pathfinding::NAVCELLS_PER_TILE), m_Pathfinder(pathfinder)
	{
	}

	virtual void BuildTextureRGBA(u8* data, size_t w, size_t h)
	{
		// Grab the debug data for the most recently generated path
		u32 steps;
		double time;
		Grid<u8> debugGrid;
		m_Pathfinder.GetDebugData(steps, time, debugGrid);

		// Render navcell passability
		u8* p = data;
		for (size_t j = 0; j < h; ++j)
		{
			for (size_t i = 0; i < w; ++i)
			{
				SColor4ub color(0, 0, 0, 0);
				if (!IS_PASSABLE(m_Pathfinder.m_Grid->get((int)i, (int)j), m_Pathfinder.m_DebugPassClass))
					color = SColor4ub(255, 0, 0, 127);

				if (debugGrid.m_W && debugGrid.m_H)
				{
					u8 n = debugGrid.get((int)i, (int)j);

					if (n == 1)
						color = SColor4ub(255, 255, 0, 127);
					else if (n == 2)
						color = SColor4ub(0, 255, 0, 127);

					if (m_Pathfinder.m_DebugGoal.NavcellContainsGoal(i, j))
						color = SColor4ub(0, 0, 255, 127);
				}

				*p++ = color.R;
				*p++ = color.G;
				*p++ = color.B;
				*p++ = color.A;
			}
		}

		// Render the most recently generated path
		if (m_Pathfinder.m_DebugPath && !m_Pathfinder.m_DebugPath->m_Waypoints.empty())
		{
			std::vector<Waypoint>& waypoints = m_Pathfinder.m_DebugPath->m_Waypoints;
			u16 ip = 0, jp = 0;
			for (size_t k = 0; k < waypoints.size(); ++k)
			{
				u16 i, j;
				Pathfinding::NearestNavcell(waypoints[k].x, waypoints[k].z, i, j, m_Pathfinder.m_GridSize, m_Pathfinder.m_GridSize);
				if (k == 0)
				{
					ip = i;
					jp = j;
				}
				else
				{
					bool firstCell = true;
					do
					{
						if (data[(jp*w + ip)*4+3] == 0)
						{
							data[(jp*w + ip)*4+0] = 0xFF;
							data[(jp*w + ip)*4+1] = 0xFF;
							data[(jp*w + ip)*4+2] = 0xFF;
							data[(jp*w + ip)*4+3] = firstCell ? 0xA0 : 0x60;
						}
						ip = ip < i ? ip+1 : ip > i ? ip-1 : ip;
						jp = jp < j ? jp+1 : jp > j ? jp-1 : jp;
						firstCell = false;
					}
					while (ip != i || jp != j);
				}
			}
		}
	}
};

#endif // INCLUDED_LONGPATHFINDER
