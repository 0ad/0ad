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

#ifndef INCLUDED_HIERPATHFINDER
#define INCLUDED_HIERPATHFINDER

#include "Pathfinding.h"

#include "renderer/TerrainOverlay.h"
#include "Render.h"
#include "graphics/SColor.h"

/**
 * Hierarchical pathfinder.
 *
 * Deals with connectivity (can point A reach point B?).
 *
 * The navcell-grid representation of the map is split into fixed-size chunks.
 * Within a chunk, each maximal set of adjacently-connected passable navcells
 * is defined as a region.
 * Each region is a vertex in the hierarchical pathfinder's graph.
 * When two regions in adjacent chunks are connected by passable navcells,
 * the graph contains an edge between the corresponding two vertexes.
 * By design, there can never be an edge between two regions in the same chunk.
 *
 * Those fixed-size chunks are used to efficiently compute "global regions" by effectively flood-filling.
 * Those can then be used to immediately determine if two reachables points are connected.
 *
 * The main use of this class is to convert an arbitrary PathGoal to a reachable navcell.
 * This happens in MakeGoalReachable.
 *
 */

#ifdef TEST
class TestCmpPathfinder;
class TestHierarchicalPathfinder;
#endif

class HierarchicalOverlay;
class SceneCollector;

class HierarchicalPathfinder
{
#ifdef TEST
	friend class TestCmpPathfinder;
	friend class TestHierarchicalPathfinder;
#endif
public:
	typedef u32 GlobalRegionID;

	struct RegionID
	{
		u8 ci, cj; // chunk ID
		u16 r; // unique-per-chunk local region ID

		RegionID(u8 ci, u8 cj, u16 r) : ci(ci), cj(cj), r(r) { }

		bool operator<(const RegionID& b) const
		{
			// Sort by chunk ID, then by per-chunk region ID
			if (ci < b.ci)
				return true;
			if (b.ci < ci)
				return false;
			if (cj < b.cj)
				return true;
			if (b.cj < cj)
				return false;
			return r < b.r;
		}

		bool operator==(const RegionID& b) const
		{
			return ((ci == b.ci) && (cj == b.cj) && (r == b.r));
		}

		// Returns the distance from the center to the point (i, j)
		inline u32 DistanceTo(u16 i, u16 j) const
		{
			return (ci * CHUNK_SIZE + CHUNK_SIZE/2 - i) * (ci * CHUNK_SIZE + CHUNK_SIZE/2 - i) +
			       (cj * CHUNK_SIZE + CHUNK_SIZE/2 - j) * (cj * CHUNK_SIZE + CHUNK_SIZE/2 - j);
		}

	};

	HierarchicalPathfinder();
	~HierarchicalPathfinder();

	void SetDebugOverlay(bool enabled, const CSimContext* simContext);

	// Non-pathfinding grids will never be recomputed on calling HierarchicalPathfinder::Update
	void Recompute(Grid<NavcellData>* passabilityGrid,
		const std::map<std::string, pass_class_t>& nonPathfindingPassClassMasks,
		const std::map<std::string, pass_class_t>& pathfindingPassClassMasks);

	void Update(Grid<NavcellData>* grid, const Grid<u8>& dirtinessGrid);

	RegionID Get(u16 i, u16 j, pass_class_t passClass) const;

	GlobalRegionID GetGlobalRegion(u16 i, u16 j, pass_class_t passClass) const;
	GlobalRegionID GetGlobalRegion(RegionID region, pass_class_t passClass) const;

	/**
	 * Updates @p goal so that it's guaranteed to be reachable from the navcell
	 * @p i0, @p j0 (which is assumed to be on a passable navcell).
	 *
	 * If the goal is not reachable, it is replaced with a point goal nearest to
	 * the goal center.
	 *
	 * In the case of a non-point reachable goal, it is replaced with a point goal
	 * at the reachable navcell of the goal which is nearest to the starting navcell.
	 *
	 * @returns true if the goal was reachable, false otherwise.
	 */
	bool MakeGoalReachable(u16 i0, u16 j0, PathGoal& goal, pass_class_t passClass) const;

	/**
	 * Updates @p i, @p j (which is assumed to be an impassable navcell)
	 * to the nearest passable navcell.
	 */
	void FindNearestPassableNavcell(u16& i, u16& j, pass_class_t passClass) const;

	/**
	 * Generates the connectivity grid associated with the given pass_class
	 */
	Grid<u16> GetConnectivityGrid(pass_class_t passClass) const;

	pass_class_t GetPassabilityClass(const std::string& name) const
	{
		auto it = m_PassClassMasks.find(name);
		if (it != m_PassClassMasks.end())
			return it->second;

		LOGERROR("Invalid passability class name '%s'", name.c_str());
		return 0;
	}

	void RenderSubmit(SceneCollector& collector);

private:
	static const u8 CHUNK_SIZE = 96; // number of navcells per side
									 // TODO: figure out best number. Probably 64 < n < 128

	struct Chunk
	{
		u8 m_ChunkI, m_ChunkJ; // chunk ID
		std::vector<u16> m_RegionsID; // IDs of local regions, 0 (impassable) excluded
		u16 m_Regions[CHUNK_SIZE][CHUNK_SIZE]; // local region ID per navcell

		cassert(CHUNK_SIZE*CHUNK_SIZE/2 < 65536); // otherwise we could overflow m_RegionsID with a checkerboard pattern

		void InitRegions(int ci, int cj, Grid<NavcellData>* grid, pass_class_t passClass);

		RegionID Get(int i, int j) const;

		void RegionCenter(u16 r, int& i, int& j) const;

		void RegionNavcellNearest(u16 r, int iGoal, int jGoal, int& iBest, int& jBest, u32& dist2Best) const;

		bool RegionNearestNavcellInGoal(u16 r, u16 i0, u16 j0, const PathGoal& goal, u16& iOut, u16& jOut, u32& dist2Best) const;

#ifdef TEST
		bool operator==(const Chunk& b) const
		{
			return (m_ChunkI == b.m_ChunkI && m_ChunkJ == b.m_ChunkJ && m_RegionsID.size() == b.m_RegionsID.size() && memcmp(&m_Regions, &b.m_Regions, sizeof(u16) * CHUNK_SIZE * CHUNK_SIZE) == 0);
		}
#endif
	};

	const Chunk& GetChunk(u8 ci, u8 cj, pass_class_t passClass) const
	{
		return m_Chunks.at(passClass).at(cj * m_ChunksW + ci);
	}

	typedef std::map<RegionID, std::set<RegionID> > EdgesMap;

	void ComputeNeighbors(EdgesMap& edges, Chunk& a, Chunk& b, bool transpose, bool opposite) const;
	void RecomputeAllEdges(pass_class_t passClass, EdgesMap& edges);
	void UpdateEdges(u8 ci, u8 cj, pass_class_t passClass, EdgesMap& edges);

	void UpdateGlobalRegions(const std::map<pass_class_t, std::vector<RegionID> >& needNewGlobalRegionMap);

	/**
	 * Returns all reachable regions, optionally ordered in a specific manner.
	 */
	template<typename Ordering>
	void FindReachableRegions(RegionID from, std::set<RegionID, Ordering>& reachable, pass_class_t passClass) const;

	struct SortByCenterToPoint
	{
		SortByCenterToPoint(u16 i, u16 j): gi(i), gj(j) {};
		bool operator()(const HierarchicalPathfinder::RegionID& a, const HierarchicalPathfinder::RegionID& b) const
		{
			if (a.DistanceTo(gi, gj) < b.DistanceTo(gi, gj))
				return true;
			if (a.DistanceTo(gi, gj) > b.DistanceTo(gi, gj))
				return false;
			return a.r < b.r;
		}
		u16 gi, gj;
	};

	void FindNearestNavcellInRegions(const std::set<RegionID, SortByCenterToPoint>& regions,
									 u16& iGoal, u16& jGoal, pass_class_t passClass) const;

	struct InterestingRegion {
		RegionID region;
		u16 bestI;
		u16 bestJ;
	};

	struct SortByBestToPoint
	{
		SortByBestToPoint(u16 i, u16 j): gi(i), gj(j) {};
		bool operator()(const InterestingRegion& a, const InterestingRegion& b) const
		{
			if ((a.bestI - gi) * (a.bestI - gi) + (a.bestJ - gj) * (a.bestJ - gj) < (b.bestI - gi) * (b.bestI - gi) + (b.bestJ - gj) * (b.bestJ - gj))
				return true;
			if ((a.bestI - gi) * (a.bestI - gi) + (a.bestJ - gj) * (a.bestJ - gj) > (b.bestI - gi) * (b.bestI - gi) + (b.bestJ - gj) * (b.bestJ - gj))
				return false;
			return a.region.r < b.region.r;
		}
		u16 gi, gj;
	};

	// Returns the region along with the best cell for optimisation.
	void FindGoalRegionsAndBestNavcells(u16 i0, u16 j0, u16 gi, u16 gj, const PathGoal& goal, std::set<InterestingRegion, SortByBestToPoint>& regions, pass_class_t passClass) const;

	void FillRegionOnGrid(const RegionID& region, pass_class_t passClass, u16 value, Grid<u16>& grid) const;

	u16 m_W, m_H;
	u8 m_ChunksW, m_ChunksH;
	std::map<pass_class_t, std::vector<Chunk> > m_Chunks;

	std::map<pass_class_t, EdgesMap> m_Edges;

	std::map<pass_class_t, std::map<RegionID, GlobalRegionID> > m_GlobalRegions;
	GlobalRegionID m_NextGlobalRegionID;

	// Passability classes for which grids will be updated when calling Update
	std::map<std::string, pass_class_t> m_PassClassMasks;

	void AddDebugEdges(pass_class_t passClass);
	HierarchicalOverlay* m_DebugOverlay;
	const CSimContext* m_SimContext; // Used for drawing the debug lines

public:
	std::vector<SOverlayLine> m_DebugOverlayLines;
};

class HierarchicalOverlay : public TerrainTextureOverlay
{
public:
	HierarchicalPathfinder& m_PathfinderHier;

	HierarchicalOverlay(HierarchicalPathfinder& pathfinderHier) :
		TerrainTextureOverlay(Pathfinding::NAVCELLS_PER_TILE), m_PathfinderHier(pathfinderHier)
	{
	}

	virtual void BuildTextureRGBA(u8* data, size_t w, size_t h)
	{
		pass_class_t passClass = m_PathfinderHier.GetPassabilityClass("default");

		for (size_t j = 0; j < h; ++j)
		{
			for (size_t i = 0; i < w; ++i)
			{
				SColor4ub color;

				HierarchicalPathfinder::RegionID rid = m_PathfinderHier.Get(i, j, passClass);
				if (rid.r == 0)
					color = SColor4ub(0, 0, 0, 0);
				else if (rid.r == 0xFFFF)
					color = SColor4ub(255, 0, 255, 255);
				else
					color = GetColor(rid.r + rid.ci*5 + rid.cj*7, 127);

				*data++ = color.R;
				*data++ = color.G;
				*data++ = color.B;
				*data++ = color.A;
			}
		}
	}
};


#endif // INCLUDED_HIERPATHFINDER
