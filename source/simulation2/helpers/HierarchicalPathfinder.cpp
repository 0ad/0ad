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

#include "precompiled.h"

#include "HierarchicalPathfinder.h"

#include "graphics/Overlay.h"
#include "ps/Profile.h"
#include "renderer/Scene.h"

// Find the root ID of a region, used by InitRegions
inline u16 RootID(u16 x, const std::vector<u16>& v)
{
	while (v[x] < x)
		x = v[x];

	return x;
}

void HierarchicalPathfinder::Chunk::InitRegions(int ci, int cj, Grid<NavcellData>* grid, pass_class_t passClass)
{
	ENSURE(ci < 256 && cj < 256); // avoid overflows
	m_ChunkI = ci;
	m_ChunkJ = cj;

	memset(m_Regions, 0, sizeof(m_Regions));

	int i0 = ci * CHUNK_SIZE;
	int j0 = cj * CHUNK_SIZE;
	int i1 = std::min(i0 + CHUNK_SIZE, (int)grid->m_W);
	int j1 = std::min(j0 + CHUNK_SIZE, (int)grid->m_H);

	// Efficiently flood-fill the m_Regions grid

	int regionID = 0;
	std::vector<u16> connect;

	u16* pCurrentID = NULL;
	u16 LeftID = 0;
	u16 DownID = 0;
	bool Checked = false; // prevent some unneccessary RootID calls

	connect.reserve(32); // TODO: What's a sensible number?
	connect.push_back(0); // connect[0] = 0

	// Start by filling the grid with 0 for blocked,
	// and regionID for unblocked
	for (int j = j0; j < j1; ++j)
	{
		for (int i = i0; i < i1; ++i)
		{
			pCurrentID = &m_Regions[j-j0][i-i0];
			if (!IS_PASSABLE(grid->get(i, j), passClass))
			{
				*pCurrentID = 0;
				continue;
			}

			if (j > j0)
				DownID = m_Regions[j-1-j0][i-i0];

			if (i == i0)
				LeftID = 0;
			else
				LeftID = m_Regions[j-j0][i-1-i0];

			if (LeftID > 0)
			{
				*pCurrentID = LeftID;
				if (*pCurrentID != DownID && DownID > 0 && !Checked)
				{
					u16 id0 = RootID(DownID, connect);
					u16 id1 = RootID(LeftID, connect);
					Checked = true; // this avoids repeatedly connecting the same IDs

					if (id0 < id1)
						connect[id1] = id0;
					else if (id0 > id1)
						connect[id0] = id1;
				}
				else if (DownID == 0)
					Checked = false;
			}
			else if (DownID > 0)
			{
				*pCurrentID = DownID;
				Checked = false;
			}
			else
			{
				// New ID
				*pCurrentID = ++regionID;
				connect.push_back(regionID);
				Checked = false;
			}
		}
	}

	// Mark connected regions as being the same ID (i.e. the lowest)
	m_RegionsID.clear();
	for (u16 i = 1; i < regionID+1; ++i)
	{
		if (connect[i] != i)
			connect[i] = RootID(i, connect);
		else
			m_RegionsID.push_back(connect[i]);
	}

	// Replace IDs by the root ID
	for (int j = 0; j < CHUNK_SIZE; ++j)
		for (int i = 0; i < CHUNK_SIZE; ++i)
			m_Regions[j][i] = connect[m_Regions[j][i]];
}

/**
 * Returns a RegionID for the given global navcell coords
 * (which must be inside this chunk);
 */
HierarchicalPathfinder::RegionID HierarchicalPathfinder::Chunk::Get(int i, int j) const
{
	ENSURE(i < CHUNK_SIZE && j < CHUNK_SIZE);
	return RegionID(m_ChunkI, m_ChunkJ, m_Regions[j][i]);
}

/**
 * Return the global navcell coords that correspond roughly to the
 * center of the given region in this chunk.
 * (This is not guaranteed to be actually inside the region.)
 */
void HierarchicalPathfinder::Chunk::RegionCenter(u16 r, int& i_out, int& j_out) const
{
	// Find the mean of i,j coords of navcells in this region:

	u32 si = 0, sj = 0; // sum of i,j coords
	u32 n = 0; // number of navcells in region

	cassert(CHUNK_SIZE < 256); // conservative limit to ensure si and sj don't overflow

	for (int j = 0; j < CHUNK_SIZE; ++j)
	{
		for (int i = 0; i < CHUNK_SIZE; ++i)
		{
			if (m_Regions[j][i] == r)
			{
				si += i;
				sj += j;
				n += 1;
			}
		}
	}

	// Avoid divide-by-zero
	if (n == 0)
		n = 1;

	i_out = m_ChunkI * CHUNK_SIZE + si / n;
	j_out = m_ChunkJ * CHUNK_SIZE + sj / n;
}

/**
 * Returns the global navcell coords, and the squared distance to the goal
 * navcell, of whichever navcell inside the given region is closest to
 * that goal.
 */
void HierarchicalPathfinder::Chunk::RegionNavcellNearest(u16 r, int iGoal, int jGoal, int& iBest, int& jBest, u32& dist2Best) const
{
	iBest = 0;
	jBest = 0;
	dist2Best = std::numeric_limits<u32>::max();

	for (int j = 0; j < CHUNK_SIZE; ++j)
	{
		for (int i = 0; i < CHUNK_SIZE; ++i)
		{
			if (m_Regions[j][i] != r)
				continue;

			u32 dist2 = (i + m_ChunkI*CHUNK_SIZE - iGoal)*(i + m_ChunkI*CHUNK_SIZE - iGoal)
				        + (j + m_ChunkJ*CHUNK_SIZE - jGoal)*(j + m_ChunkJ*CHUNK_SIZE - jGoal);

			if (dist2 < dist2Best)
			{
				iBest = i + m_ChunkI*CHUNK_SIZE;
				jBest = j + m_ChunkJ*CHUNK_SIZE;
				dist2Best = dist2;
			}
		}
	}
}

/**
 * Gives the global navcell coords, and the squared distance to the (i0,j0)
 * navcell, of whichever navcell inside the given region and inside the given goal
 * is closest to (i0,j0)
 * Returns true if the goal is inside the region, false otherwise.
 */
bool HierarchicalPathfinder::Chunk::RegionNearestNavcellInGoal(u16 r, u16 i0, u16 j0, const PathGoal& goal, u16& iOut, u16& jOut, u32& dist2Best) const
{
	// TODO: this should be optimized further.
	// Most used cases empirically seem to be SQUARE, INVERTED_CIRCLE and then POINT and CIRCLE somehwat equally
	iOut = 0;
	jOut = 0;
	dist2Best = std::numeric_limits<u32>::max();

	// Calculate the navcell that contains the center of the goal.
	int gi = (goal.x >> Pathfinding::NAVCELL_SIZE_LOG2).ToInt_RoundToNegInfinity();
	int gj = (goal.z >> Pathfinding::NAVCELL_SIZE_LOG2).ToInt_RoundToNegInfinity();

	switch(goal.type)
	{
	case PathGoal::POINT:
	{
		// i and j can be equal to CHUNK_SIZE on the top and right borders of the map,
		// specially when mapSize is a multiple of CHUNK_SIZE
		int i = std::min((int)CHUNK_SIZE - 1, gi - m_ChunkI * CHUNK_SIZE);
		int j = std::min((int)CHUNK_SIZE - 1, gj - m_ChunkJ * CHUNK_SIZE);
		if (m_Regions[j][i] == r)
		{
			iOut = gi;
			jOut = gj;
			dist2Best = (gi - i0)*(gi - i0)
					  + (gj - j0)*(gj - j0);
			return true;
		}
		return false;
	}
	case PathGoal::CIRCLE:
	case PathGoal::SQUARE:
	{
		// restrict ourselves to a square surrounding the goal.
		int radius = (std::max(goal.hw*3/2,goal.hh*3/2) >> Pathfinding::NAVCELL_SIZE_LOG2).ToInt_RoundToInfinity();
		int imin = std::max(0, gi-m_ChunkI*CHUNK_SIZE-radius);
		int imax = std::min((int)CHUNK_SIZE, gi-m_ChunkI*CHUNK_SIZE+radius+1);
		int jmin = std::max(0, gj-m_ChunkJ*CHUNK_SIZE-radius);
		int jmax = std::min((int)CHUNK_SIZE, gj-m_ChunkJ*CHUNK_SIZE+radius+1);
		bool found = false;
		u32 dist2 = std::numeric_limits<u32>::max();
		for (u16 j = jmin; j < jmax; ++j)
		{
			for (u16 i = imin; i < imax; ++i)
			{
				if (m_Regions[j][i] != r)
					continue;

				if (found)
				{
					dist2 = (i + m_ChunkI*CHUNK_SIZE - i0)*(i + m_ChunkI*CHUNK_SIZE - i0)
						+ (j + m_ChunkJ*CHUNK_SIZE - j0)*(j + m_ChunkJ*CHUNK_SIZE - j0);
					if (dist2 >= dist2Best)
						continue;
				}

				if (goal.NavcellContainsGoal(m_ChunkI * CHUNK_SIZE + i, m_ChunkJ * CHUNK_SIZE + j))
				{
					if (!found)
					{
						found = true;
						dist2 = (i + m_ChunkI*CHUNK_SIZE - i0)*(i + m_ChunkI*CHUNK_SIZE - i0)
							+ (j + m_ChunkJ*CHUNK_SIZE - j0)*(j + m_ChunkJ*CHUNK_SIZE - j0);
					}
					iOut = i + m_ChunkI*CHUNK_SIZE;
					jOut = j + m_ChunkJ*CHUNK_SIZE;
					dist2Best = dist2;
				}
			}
		}
		return found;
	}
	case PathGoal::INVERTED_CIRCLE:
	case PathGoal::INVERTED_SQUARE:
	{
		bool found = false;
		u32 dist2 = std::numeric_limits<u32>::max();
		// loop over all navcells.
		for (u16 j = 0; j < CHUNK_SIZE; ++j)
		{
			for (u16 i = 0; i < CHUNK_SIZE; ++i)
			{
				if (m_Regions[j][i] != r)
					continue;

				if (found)
				{
					dist2 = (i + m_ChunkI*CHUNK_SIZE - i0)*(i + m_ChunkI*CHUNK_SIZE - i0)
						+ (j + m_ChunkJ*CHUNK_SIZE - j0)*(j + m_ChunkJ*CHUNK_SIZE - j0);
					if (dist2 >= dist2Best)
						continue;
				}

				if (goal.NavcellContainsGoal(m_ChunkI * CHUNK_SIZE + i, m_ChunkJ * CHUNK_SIZE + j))
				{
					if (!found)
					{
						found = true;
						dist2 = (i + m_ChunkI*CHUNK_SIZE - i0)*(i + m_ChunkI*CHUNK_SIZE - i0)
							+ (j + m_ChunkJ*CHUNK_SIZE - j0)*(j + m_ChunkJ*CHUNK_SIZE - j0);
					}
					iOut = i + m_ChunkI*CHUNK_SIZE;
					jOut = j + m_ChunkJ*CHUNK_SIZE;
					dist2Best = dist2;
				}
			}
		}
		return found;
	}
	}
	return false;
}

HierarchicalPathfinder::HierarchicalPathfinder() : m_DebugOverlay(NULL)
{
}

HierarchicalPathfinder::~HierarchicalPathfinder()
{
	SAFE_DELETE(m_DebugOverlay);
}

void HierarchicalPathfinder::SetDebugOverlay(bool enabled, const CSimContext* simContext)
{
	if (enabled && !m_DebugOverlay)
	{
		m_DebugOverlay = new HierarchicalOverlay(*this);
		m_DebugOverlayLines.clear();
		m_SimContext = simContext;
		AddDebugEdges(GetPassabilityClass("default"));
	}
	else if (!enabled && m_DebugOverlay)
	{
		SAFE_DELETE(m_DebugOverlay);
		m_DebugOverlayLines.clear();
		m_SimContext = NULL;
	}
}

void HierarchicalPathfinder::RenderSubmit(SceneCollector& collector)
{
	if (!m_DebugOverlay)
		return;

	for (size_t i = 0; i < m_DebugOverlayLines.size(); ++i)
		collector.Submit(&m_DebugOverlayLines[i]);
}

void HierarchicalPathfinder::Recompute(Grid<NavcellData>* grid,
	const std::map<std::string, pass_class_t>& nonPathfindingPassClassMasks,
	const std::map<std::string, pass_class_t>& pathfindingPassClassMasks)
{
	PROFILE2("Hierarchical Recompute");

	m_PassClassMasks = pathfindingPassClassMasks;

	std::map<std::string, pass_class_t> allPassClasses = m_PassClassMasks;
	allPassClasses.insert(nonPathfindingPassClassMasks.begin(), nonPathfindingPassClassMasks.end());

	m_W = grid->m_W;
	m_H = grid->m_H;

	ENSURE((grid->m_W + CHUNK_SIZE - 1) / CHUNK_SIZE < 256 && (grid->m_H + CHUNK_SIZE - 1) / CHUNK_SIZE < 256); // else the u8 Chunk::m_ChunkI will overflow

	// Divide grid into chunks with round-to-positive-infinity
	m_ChunksW = (grid->m_W + CHUNK_SIZE - 1) / CHUNK_SIZE;
	m_ChunksH = (grid->m_H + CHUNK_SIZE - 1) / CHUNK_SIZE;

	m_Chunks.clear();
	m_Edges.clear();

	// Reset global regions.
	m_NextGlobalRegionID = 1;

	for (auto& passClassMask : allPassClasses)
	{
		pass_class_t passClass = passClassMask.second;

		// Compute the regions within each chunk
		m_Chunks[passClass].resize(m_ChunksW*m_ChunksH);
		for (int cj = 0; cj < m_ChunksH; ++cj)
		{
			for (int ci = 0; ci < m_ChunksW; ++ci)
			{
				m_Chunks[passClass].at(cj*m_ChunksW + ci).InitRegions(ci, cj, grid, passClass);
			}
		}

		// Construct the search graph over the regions.
		EdgesMap& edges = m_Edges[passClass];
		RecomputeAllEdges(passClass, edges);

		// Spread global regions.
		std::map<RegionID, GlobalRegionID>& globalRegion = m_GlobalRegions[passClass];
		globalRegion.clear();
		for (u8 cj = 0; cj < m_ChunksH; ++cj)
			for (u8 ci = 0; ci < m_ChunksW; ++ci)
				for (u16 rid : GetChunk(ci, cj, passClass).m_RegionsID)
				{
					RegionID reg{ci,cj,rid};
					if (globalRegion.find(reg) == globalRegion.end())
					{
						GlobalRegionID ID = m_NextGlobalRegionID++;

						globalRegion.insert({ reg, ID });
						// Avoid creating an empty link if possible, FindReachableRegions uses [] which calls the default constructor.
						if (edges.find(reg) != edges.end())
						{
							std::set<RegionID> reachable;
							FindReachableRegions(reg, reachable, passClass);
							for (const RegionID& region : reachable)
								globalRegion.insert({ region, ID });
						}
					}
				}
	}

	if (m_DebugOverlay)
	{
		m_DebugOverlayLines.clear();
		AddDebugEdges(GetPassabilityClass("default"));
	}
}

void HierarchicalPathfinder::Update(Grid<NavcellData>* grid, const Grid<u8>& dirtinessGrid)
{
	PROFILE3("Hierarchical Update");

	ASSERT(m_NextGlobalRegionID < std::numeric_limits<GlobalRegionID>::max());

	std::map<pass_class_t, std::vector<RegionID> > needNewGlobalRegionMap;

	// Algorithm for the partial update:
	// 1. Loop over chunks.
	// 2. For any dirty chunk:
	//		- remove all regions from the global region map
	//		- remove all edges, by removing the neighbor connection with them and then deleting us
	//		- recreate regions inside the chunk
	//		- reconnect the regions. We may do too much work if we reconnect with a dirty chunk, but that's fine.
	// 3. Recreate global regions.
	// This means that if any chunk changes, we may need to flood (at most once) the whole map.
	// That's quite annoying, but I can't think of an easy way around it.
	// If we could be sure that a region's topology hasn't changed, we could skip removing its global region
	// but that's non trivial as we have no easy way to determine said topology (regions could "switch" IDs on update for now).
	for (u8 cj = 0; cj <  m_ChunksH; ++cj)
	{
		int j0 = cj * CHUNK_SIZE;
		int j1 = std::min(j0 + CHUNK_SIZE, (int)dirtinessGrid.m_H);
		for (u8 ci = 0; ci < m_ChunksW; ++ci)
		{
			// Skip chunks where no navcells are dirty.
			int i0 = ci * CHUNK_SIZE;
			int i1 = std::min(i0 + CHUNK_SIZE, (int)dirtinessGrid.m_W);
			if (!dirtinessGrid.any_set_in_square(i0, j0, i1, j1))
				continue;

			for (const std::pair<std::string, pass_class_t>& passClassMask : m_PassClassMasks)
			{
				pass_class_t passClass = passClassMask.second;
				Chunk& a = m_Chunks[passClass].at(ci + cj*m_ChunksW);

				// Clean up edges and global region ID
				EdgesMap& edgeMap = m_Edges[passClass];
				for (u16 i : a.m_RegionsID)
				{
					RegionID reg{ci, cj, i};
					m_GlobalRegions[passClass].erase(reg);
					for (const RegionID& neighbor : edgeMap[reg])
					{
						edgeMap[neighbor].erase(reg);
						if (edgeMap[neighbor].empty())
							edgeMap.erase(neighbor);
					}
					edgeMap.erase(reg);
				}

				// Recompute regions inside this chunk.
				a.InitRegions(ci, cj, grid, passClass);

				for (u16 i : a.m_RegionsID)
					needNewGlobalRegionMap[passClass].push_back(RegionID{ci, cj, i});

				UpdateEdges(ci, cj, passClass, edgeMap);
			}
		}
	}

	UpdateGlobalRegions(needNewGlobalRegionMap);

	if (m_DebugOverlay)
	{
		m_DebugOverlayLines.clear();
		AddDebugEdges(GetPassabilityClass("default"));
	}
}

void HierarchicalPathfinder::ComputeNeighbors(EdgesMap& edges, Chunk& a, Chunk& b, bool transpose, bool opposite) const
{
	// For each edge between chunks, we loop over every adjacent pair of
	// navcells in the two chunks. If they are both in valid regions
	// (i.e. are passable navcells) then add a graph edge between those regions.
	// (We don't need to test for duplicates since EdgesMap already uses a
	// std::set which will drop duplicate entries.)
	// But as set.insert can be quite slow on large collection, and that we usually
	// try to insert the same values, we cache the previous one for a fast test.
	RegionID raPrev(0,0,0);
	RegionID rbPrev(0,0,0);
	for (int k = 0; k < CHUNK_SIZE; ++k)
	{
		u8 aSide = opposite ? CHUNK_SIZE - 1 : 0;
		u8 bSide = CHUNK_SIZE - 1 - aSide;
		RegionID ra = transpose ? a.Get(k, aSide) : a.Get(aSide, k);
		RegionID rb = transpose ? b.Get(k, bSide) : b.Get(bSide, k);
		if (ra.r && rb.r)
		{
			if (ra == raPrev && rb == rbPrev)
				continue;
			edges[ra].insert(rb);
			edges[rb].insert(ra);
			raPrev = ra;
			rbPrev = rb;
		}
	}
}

/**
 * Connect a chunk's regions to their neighbors. Not optimised for global recomputing.
 */
void HierarchicalPathfinder::UpdateEdges(u8 ci, u8 cj, pass_class_t passClass, EdgesMap& edges)
{
	std::vector<Chunk>& chunks = m_Chunks[passClass];

	Chunk& a = chunks.at(cj*m_ChunksW + ci);

	if (ci > 0)
		ComputeNeighbors(edges, a, chunks.at(cj*m_ChunksW + (ci-1)), false, false);

	if (ci < m_ChunksW-1)
		ComputeNeighbors(edges, a, chunks.at(cj*m_ChunksW + (ci+1)), false, true);

	if (cj > 0)
		ComputeNeighbors(edges, a, chunks.at((cj-1)*m_ChunksW + ci), true, false);

	if (cj < m_ChunksH - 1)
		ComputeNeighbors(edges, a, chunks.at((cj+1)*m_ChunksW + ci), true, true);
}

/**
 * Find edges between regions in all chunks, in an optimised manner (only look at top/left)
 */
void HierarchicalPathfinder::RecomputeAllEdges(pass_class_t passClass, EdgesMap& edges)
{
	std::vector<Chunk>& chunks = m_Chunks[passClass];

	edges.clear();

	for (int cj = 0; cj < m_ChunksH; ++cj)
	{
		for (int ci = 0; ci < m_ChunksW; ++ci)
		{
			Chunk& a = chunks.at(cj*m_ChunksW + ci);

			if (ci > 0)
				ComputeNeighbors(edges, a, chunks.at(cj*m_ChunksW + (ci-1)), false, false);

			if (cj > 0)
				ComputeNeighbors(edges, a, chunks.at((cj-1)*m_ChunksW + ci), true, false);
		}
	}
}

/**
 * Debug visualisation of graph edges between regions.
 */
void HierarchicalPathfinder::AddDebugEdges(pass_class_t passClass)
{
	const EdgesMap& edges = m_Edges[passClass];
	const std::vector<Chunk>& chunks = m_Chunks[passClass];

	for (auto& edge : edges)
	{
		for (const RegionID& region: edge.second)
		{
			// Draw a line between the two regions' centers

			int i0, j0, i1, j1;
			chunks[edge.first.cj * m_ChunksW + edge.first.ci].RegionCenter(edge.first.r, i0, j0);
			chunks[region.cj * m_ChunksW + region.ci].RegionCenter(region.r, i1, j1);

			CFixedVector2D a, b;
			Pathfinding::NavcellCenter(i0, j0, a.X, a.Y);
			Pathfinding::NavcellCenter(i1, j1, b.X, b.Y);

			// Push the endpoints inwards a little to avoid overlaps
			CFixedVector2D d = b - a;
			d.Normalize(entity_pos_t::FromInt(1));
			a += d;
			b -= d;

			std::vector<float> xz;
			xz.push_back(a.X.ToFloat());
			xz.push_back(a.Y.ToFloat());
			xz.push_back(b.X.ToFloat());
			xz.push_back(b.Y.ToFloat());

			m_DebugOverlayLines.emplace_back();
			m_DebugOverlayLines.back().m_Color = CColor(1.0, 1.0, 1.0, 1.0);
			SimRender::ConstructLineOnGround(*m_SimContext, xz, m_DebugOverlayLines.back(), true);
		}
	}
}

void HierarchicalPathfinder::UpdateGlobalRegions(const std::map<pass_class_t, std::vector<RegionID> >& needNewGlobalRegionMap)
{
	// Use FindReachableRegions because we cannot be sure, even if we find a non-dirty chunk nearby,
	// that we weren't the only bridge connecting that chunk to the rest of the global region.
	for (const std::pair<pass_class_t, std::vector<RegionID> >& regionsInNeed : needNewGlobalRegionMap)
		for (const RegionID& reg : regionsInNeed.second)
		{
			std::map<RegionID, GlobalRegionID>& globalRegions = m_GlobalRegions[regionsInNeed.first];
			// If we have already been given a region, skip us.
			if (globalRegions.find(reg) != globalRegions.end())
				continue;

			std::set<RegionID> reachable;
			FindReachableRegions(reg, reachable, regionsInNeed.first);

			GlobalRegionID ID = m_NextGlobalRegionID++;

			for (const RegionID& reg : reachable)
				globalRegions[reg] = ID;
		}
}

HierarchicalPathfinder::RegionID HierarchicalPathfinder::Get(u16 i, u16 j, pass_class_t passClass) const
{
	int ci = i / CHUNK_SIZE;
	int cj = j / CHUNK_SIZE;
	ENSURE(ci < m_ChunksW && cj < m_ChunksH);
	return m_Chunks.at(passClass)[cj*m_ChunksW + ci].Get(i % CHUNK_SIZE, j % CHUNK_SIZE);
}

HierarchicalPathfinder::GlobalRegionID HierarchicalPathfinder::GetGlobalRegion(u16 i, u16 j, pass_class_t passClass) const
{
	return GetGlobalRegion(Get(i, j, passClass), passClass);
}

HierarchicalPathfinder::GlobalRegionID HierarchicalPathfinder::GetGlobalRegion(RegionID region, pass_class_t passClass) const
{
	return region.r == 0 ? GlobalRegionID(0) : m_GlobalRegions.at(passClass).at(region);
}

void CreatePointGoalAt(u16 i, u16 j, PathGoal& goal)
{
	PathGoal newGoal;
	newGoal.type = PathGoal::POINT;
	Pathfinding::NavcellCenter(i, j, newGoal.x, newGoal.z);
	goal = newGoal;
}

bool HierarchicalPathfinder::MakeGoalReachable(u16 i0, u16 j0, PathGoal& goal, pass_class_t passClass) const
{
	PROFILE2("MakeGoalReachable");

	u16 iGoal, jGoal;
	Pathfinding::NearestNavcell(goal.x, goal.z, iGoal, jGoal, m_W, m_H);

	std::set<InterestingRegion, SortByBestToPoint> goalRegions(SortByBestToPoint(i0, j0));
	// This returns goal regions ordered by distance from the best navcell in each region.
	FindGoalRegionsAndBestNavcells(i0, j0, iGoal, jGoal, goal, goalRegions, passClass);

	// Because of the sorting above, we can stop as soon as the first reachable goal region is found.
	for (const InterestingRegion& region : goalRegions)
		if (GetGlobalRegion(region.region, passClass) == GetGlobalRegion(i0, j0, passClass))
		{
			iGoal = region.bestI;
			jGoal = region.bestJ;

			// No need to move reachable point goals.
			if (goal.type != PathGoal::POINT)
				CreatePointGoalAt(iGoal, jGoal, goal);
			return true;
		}

	// Goal wasn't reachable - get the closest navcell in the nearest reachable region.
	std::set<RegionID, SortByCenterToPoint> reachableRegions(SortByCenterToPoint(i0, j0));
	FindReachableRegions(Get(i0, j0, passClass), reachableRegions, passClass);

	FindNearestNavcellInRegions(reachableRegions, iGoal, jGoal, passClass);
	CreatePointGoalAt(iGoal, jGoal, goal);
	return false;
}

void HierarchicalPathfinder::FindNearestPassableNavcell(u16& i, u16& j, pass_class_t passClass) const
{
	std::set<RegionID, SortByCenterToPoint> regions(SortByCenterToPoint(i, j));

	// Construct a set of all regions of all chunks for this pass class
	for (const Chunk& chunk : m_Chunks.at(passClass))
		for (int r : chunk.m_RegionsID)
			regions.insert(RegionID(chunk.m_ChunkI, chunk.m_ChunkJ, r));

	FindNearestNavcellInRegions(regions, i, j, passClass);
}

void HierarchicalPathfinder::FindNearestNavcellInRegions(const std::set<RegionID, SortByCenterToPoint>& regions, u16& iGoal, u16& jGoal, pass_class_t passClass) const
{
	u16 bestI = iGoal, bestJ = jGoal; // Somewhat sensible default-values should regions() be passed empty.
	u32 bestDist = std::numeric_limits<u32>::max();

	// Because regions are sorted by increasing distance, we can ignore regions that are obviously farther than the current best point.
	// Since regions are squares, that happens when the center of a region is at least √2 * CHUNK_SIZE farther than the current best point.
	// Add one to avoid cases where the center navcell is actually slightly off-center (= CHUNK_SIZE is even)
	u32 maxDistFromBest = (fixed::FromInt(3) / 2 * CHUNK_SIZE).ToInt_RoundToInfinity() + 1;
	ENSURE(maxDistFromBest < std::numeric_limits<u16>::max());
	maxDistFromBest *= maxDistFromBest;

	for (const RegionID& region : regions)
	{
		u32 chunkDist = region.DistanceTo(iGoal, jGoal);
		// This might overflow, but only if we are already close to the maximal possible distance, so the condition would probably be false anyways.
		if (bestDist < std::numeric_limits<u32>::max() && chunkDist > maxDistFromBest + bestDist)
			break; // Break, the set is ordered by increased distance so a closer region will not be found.

		int ri, rj;
		u32 dist;
		GetChunk(region.ci, region.cj, passClass).RegionNavcellNearest(region.r, iGoal, jGoal, ri, rj, dist);
		if (dist < bestDist)
		{
			bestI = ri;
			bestJ = rj;
			bestDist = dist;
		}
	}
	iGoal = bestI;
	jGoal = bestJ;
}

template<typename Ordering>
void HierarchicalPathfinder::FindReachableRegions(RegionID from, std::set<RegionID, Ordering>& reachable, pass_class_t passClass) const
{
	// Flood-fill the region graph, starting at 'from',
	// collecting all the regions that are reachable via edges
	reachable.insert(from);

	const EdgesMap& edgeMap = m_Edges.at(passClass);
	if (edgeMap.find(from) == edgeMap.end())
		return;

	std::vector<RegionID> open;
	open.reserve(64);
	open.push_back(from);

	while (!open.empty())
	{
		RegionID curr = open.back();
		open.pop_back();

		for (const RegionID& region : edgeMap.at(curr))
			// Add to the reachable set; if this is the first time we added
			// it then also add it to the open list
			if (reachable.insert(region).second)
				open.push_back(region);
	}
}

void HierarchicalPathfinder::FindGoalRegionsAndBestNavcells(u16 i0, u16 j0, u16 gi, u16 gj, const PathGoal& goal, std::set<InterestingRegion, SortByBestToPoint>& regions, pass_class_t passClass) const
{
	if (goal.type == PathGoal::POINT)
	{
		RegionID region = Get(gi, gj, passClass);
		if (region.r > 0)
			regions.insert({region, gi, gj});
		return;
	}

	// For non-point cases, we'll test each region inside the bounds of the goal.
	// we might occasionally test a few too many for circles but it's not too bad.
	// Note that this also works in the Inverse-circle / Inverse-square case
	// Since our ranges are inclusive, we will necessarily test at least the perimeter/outer bound of the goal.
	// If we find a navcell, great, if not, well then we'll be surrounded by an impassable barrier.
	// Since in the Inverse-XX case we're supposed to start inside, then we can't ever reach the goal so it's good enough.
	// It's not worth it to skip the "inner" regions since we'd need ranges above CHUNK_SIZE for that to start mattering
	// (and even then not always) and that just doesn't happen for Inverse-XX goals
	int size = (std::max(goal.hh, goal.hw) * 3 / 2).ToInt_RoundToInfinity();

	u16 bestI, bestJ;
	u32 c; // Unused.

	for (u8 sz = std::max(0,(gj - size) / CHUNK_SIZE); sz <= std::min(m_ChunksH-1, (gj + size + 1) / CHUNK_SIZE); ++sz)
		for (u8 sx = std::max(0,(gi - size) / CHUNK_SIZE); sx <= std::min(m_ChunksW-1, (gi + size + 1) / CHUNK_SIZE); ++sx)
		{
			const Chunk& chunk = GetChunk(sx, sz, passClass);
			for (u16 i : chunk.m_RegionsID)
				if (chunk.RegionNearestNavcellInGoal(i, i0, j0, goal, bestI, bestJ, c))
					regions.insert({RegionID{sx, sz, i}, bestI, bestJ});
		}
}

void HierarchicalPathfinder::FillRegionOnGrid(const RegionID& region, pass_class_t passClass, u16 value, Grid<u16>& grid) const
{
	ENSURE(grid.m_W == m_W && grid.m_H == m_H);

	int i0 = region.ci * CHUNK_SIZE;
	int j0 = region.cj * CHUNK_SIZE;

	const Chunk& c = m_Chunks.at(passClass)[region.cj * m_ChunksW + region.ci];

	for (int j = 0; j < CHUNK_SIZE; ++j)
		for (int i = 0; i < CHUNK_SIZE; ++i)
			if (c.m_Regions[j][i] == region.r)
				grid.set(i0 + i, j0 + j, value);
}

Grid<u16> HierarchicalPathfinder::GetConnectivityGrid(pass_class_t passClass) const
{
	Grid<u16> connectivityGrid(m_W, m_H);
	connectivityGrid.reset();

	u16 idx = 1;

	for (size_t i = 0; i < m_W; ++i)
	{
		for (size_t j = 0; j < m_H; ++j)
		{
			if (connectivityGrid.get(i, j) != 0)
				continue;

			RegionID from = Get(i, j, passClass);
			if (from.r == 0)
				continue;

			std::set<RegionID> reachable;
			FindReachableRegions(from, reachable, passClass);

			for (const RegionID& region : reachable)
				FillRegionOnGrid(region, passClass, idx, connectivityGrid);

			++idx;
		}
	}

	return connectivityGrid;
}
