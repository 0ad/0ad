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

/**
 * @file
 * Vertex-based algorithm for CCmpPathfinder.
 * Computes paths around the corners of rectangular obstructions.
 *
 * Useful search term for this algorithm: "points of visibility".
 *
 * Since we sometimes want to use this for avoiding moving units, there is no
 * pre-computation - the whole visibility graph is effectively regenerated for
 * each path, and it does A* over that graph.
 *
 * This scales very poorly in the number of obstructions, so it should be used
 * with a limited range and not exceedingly frequently.
 */

#include "precompiled.h"

#include "CCmpPathfinder_Common.h"

#include "lib/timer.h"
#include "ps/Profile.h"
#include "simulation2/components/ICmpObstructionManager.h"
#include "simulation2/helpers/PriorityQueue.h"
#include "simulation2/helpers/Render.h"

/* Quadrant optimisation:
 * (loosely based on GPG2 "Optimizing Points-of-Visibility Pathfinding")
 *
 * Consider the vertex ("@") at a corner of an axis-aligned rectangle ("#"):
 *
 * TL  :  TR
 *     :
 * ####@ - - -
 * #####
 * #####
 * BL ##  BR
 *
 * The area around the vertex is split into TopLeft, BottomRight etc quadrants.
 *
 * If the shortest path reaches this vertex, it cannot continue to a vertex in
 * the BL quadrant (it would be blocked by the shape).
 * Since the shortest path is wrapped tightly around the edges of obstacles,
 * if the path approached this vertex from the TL quadrant,
 * it cannot continue to the TL or TR quadrants (the path could be shorter if it
 * skipped this vertex).
 * Therefore it must continue to a vertex in the BR quadrant (so this vertex is in
 * *that* vertex's TL quadrant).
 *
 * That lets us significantly reduce the search space by quickly discarding vertexes
 * from the wrong quadrants.
 *
 * (This causes badness if the path starts from inside the shape, so we add some hacks
 * for that case.)
 *
 * (For non-axis-aligned rectangles it's harder to do this computation, so we'll
 * not bother doing any discarding for those.)
 */
static const u8 QUADRANT_NONE = 0;
static const u8 QUADRANT_BL = 1;
static const u8 QUADRANT_TR = 2;
static const u8 QUADRANT_TL = 4;
static const u8 QUADRANT_BR = 8;
static const u8 QUADRANT_BLTR = QUADRANT_BL|QUADRANT_TR;
static const u8 QUADRANT_TLBR = QUADRANT_TL|QUADRANT_BR;
static const u8 QUADRANT_ALL = QUADRANT_BLTR|QUADRANT_TLBR;

// A vertex around the corners of an obstruction
// (paths will be sequences of these vertexes)
struct Vertex
{
	enum
	{
		UNEXPLORED,
		OPEN,
		CLOSED,
	};

	CFixedVector2D p;
	fixed g, h;
	u16 pred;
	u8 status;
	u8 quadInward : 4; // the quadrant which is inside the shape (or NONE)
	u8 quadOutward : 4; // the quadrants of the next point on the path which this vertex must be in, given 'pred'
};

// Obstruction edges (paths will not cross any of these).
// When used in the 'edges' list, defines the two points of the edge.
// When used in the 'edgesAA' list, defines the opposing corners of an axis-aligned square
// (from which four individual edges can be trivially computed), requiring p0 <= p1
struct Edge
{
	CFixedVector2D p0, p1;
};

// Axis-aligned obstruction edges.
// p0 defines one end; c1 is either the X or Y coordinate of the other end,
// depending on the context in which this is used.
struct EdgeAA
{
	CFixedVector2D p0;
	fixed c1;
};

// When computing vertexes to insert into the search graph,
// add a small delta so that the vertexes of an edge don't get interpreted
// as crossing the edge (given minor numerical inaccuracies)
static const entity_pos_t EDGE_EXPAND_DELTA = entity_pos_t::FromInt(1)/4;

/**
 * Check whether a ray from 'a' to 'b' crosses any of the edges.
 * (Edges are one-sided so it's only considered a cross if going from front to back.)
 */
inline static bool CheckVisibility(CFixedVector2D a, CFixedVector2D b, const std::vector<Edge>& edges)
{
	CFixedVector2D abn = (b - a).Perpendicular();

	// Edges of general non-axis-aligned shapes
	for (size_t i = 0; i < edges.size(); ++i)
	{
		CFixedVector2D p0 = edges[i].p0;
		CFixedVector2D p1 = edges[i].p1;

		CFixedVector2D d = (p1 - p0).Perpendicular();

		// If 'a' is behind the edge, we can't cross
		fixed q = (a - p0).Dot(d);
		if (q < fixed::Zero())
			continue;

		// If 'b' is in front of the edge, we can't cross
		fixed r = (b - p0).Dot(d);
		if (r > fixed::Zero())
			continue;

		// The ray is crossing the infinitely-extended edge from in front to behind.
		// Check the finite edge is crossing the infinitely-extended ray too.
		// (Given the previous tests, it can only be crossing in one direction.)
		fixed s = (p0 - a).Dot(abn);
		if (s > fixed::Zero())
			continue;

		fixed t = (p1 - a).Dot(abn);
		if (t < fixed::Zero())
			continue;

		return false;
	}

	return true;
}

// Handle the axis-aligned shape edges separately (for performance):
// (These are specialised versions of the general unaligned edge code.
// They assume the caller has already excluded edges for which 'a' is
// on the wrong side.)

inline static bool CheckVisibilityLeft(CFixedVector2D a, CFixedVector2D b, const std::vector<EdgeAA>& edges)
{
	if (a.X >= b.X)
		return true;

	CFixedVector2D abn = (b - a).Perpendicular();

	for (size_t i = 0; i < edges.size(); ++i)
	{
		if (b.X < edges[i].p0.X)
			continue;

		CFixedVector2D p0 (edges[i].p0.X, edges[i].c1);
		fixed s = (p0 - a).Dot(abn);
		if (s > fixed::Zero())
			continue;

		CFixedVector2D p1 (edges[i].p0.X, edges[i].p0.Y);
		fixed t = (p1 - a).Dot(abn);
		if (t < fixed::Zero())
			continue;

		return false;
	}

	return true;
}

inline static bool CheckVisibilityRight(CFixedVector2D a, CFixedVector2D b, const std::vector<EdgeAA>& edges)
{
	if (a.X <= b.X)
		return true;

	CFixedVector2D abn = (b - a).Perpendicular();

	for (size_t i = 0; i < edges.size(); ++i)
	{
		if (b.X > edges[i].p0.X)
			continue;

		CFixedVector2D p0 (edges[i].p0.X, edges[i].c1);
		fixed s = (p0 - a).Dot(abn);
		if (s > fixed::Zero())
			continue;

		CFixedVector2D p1 (edges[i].p0.X, edges[i].p0.Y);
		fixed t = (p1 - a).Dot(abn);
		if (t < fixed::Zero())
			continue;

		return false;
	}

	return true;
}

inline static bool CheckVisibilityBottom(CFixedVector2D a, CFixedVector2D b, const std::vector<EdgeAA>& edges)
{
	if (a.Y >= b.Y)
		return true;

	CFixedVector2D abn = (b - a).Perpendicular();

	for (size_t i = 0; i < edges.size(); ++i)
	{
		if (b.Y < edges[i].p0.Y)
			continue;

		CFixedVector2D p0 (edges[i].p0.X, edges[i].p0.Y);
		fixed s = (p0 - a).Dot(abn);
		if (s > fixed::Zero())
			continue;

		CFixedVector2D p1 (edges[i].c1, edges[i].p0.Y);
		fixed t = (p1 - a).Dot(abn);
		if (t < fixed::Zero())
			continue;

		return false;
	}

	return true;
}

inline static bool CheckVisibilityTop(CFixedVector2D a, CFixedVector2D b, const std::vector<EdgeAA>& edges)
{
	if (a.Y <= b.Y)
		return true;

	CFixedVector2D abn = (b - a).Perpendicular();

	for (size_t i = 0; i < edges.size(); ++i)
	{
		if (b.Y > edges[i].p0.Y)
			continue;

		CFixedVector2D p0 (edges[i].p0.X, edges[i].p0.Y);
		fixed s = (p0 - a).Dot(abn);
		if (s > fixed::Zero())
			continue;

		CFixedVector2D p1 (edges[i].c1, edges[i].p0.Y);
		fixed t = (p1 - a).Dot(abn);
		if (t < fixed::Zero())
			continue;

		return false;
	}

	return true;
}


static CFixedVector2D NearestPointOnGoal(CFixedVector2D pos, const CCmpPathfinder::Goal& goal)
{
	CFixedVector2D g(goal.x, goal.z);

	switch (goal.type)
	{
	case CCmpPathfinder::Goal::POINT:
	{
		return g;
	}

 	case CCmpPathfinder::Goal::CIRCLE:
	{
		CFixedVector2D d = pos - g;
		if (d.IsZero())
			d = CFixedVector2D(fixed::FromInt(1), fixed::Zero()); // some arbitrary direction
		d.Normalize(goal.hw);
		return g + d;
	}

	case CCmpPathfinder::Goal::SQUARE:
	{
		CFixedVector2D halfSize(goal.hw, goal.hh);
		CFixedVector2D d = pos - g;
		return g + Geometry::NearestPointOnSquare(d, goal.u, goal.v, halfSize);
	}

	default:
		debug_warn(L"invalid type");
		return CFixedVector2D();
	}
}

CFixedVector2D CCmpPathfinder::GetNearestPointOnGoal(CFixedVector2D pos, const CCmpPathfinder::Goal& goal)
{
	return NearestPointOnGoal(pos, goal);
	// (It's intentional that we don't put the implementation inside this
	// function, to avoid the (admittedly unmeasured and probably trivial)
	// cost of a virtual call inside ComputeShortPath)
}

typedef PriorityQueueList<u16, fixed> PriorityQueue;

struct TileEdge
{
	u16 i, j;
	enum { TOP, BOTTOM, LEFT, RIGHT } dir;
};

static void AddTerrainEdges(std::vector<Edge>& edgesAA, std::vector<Vertex>& vertexes,
	u16 i0, u16 j0, u16 i1, u16 j1, fixed r,
	ICmpPathfinder::pass_class_t passClass, const Grid<TerrainTile>& terrain)
{
	PROFILE("AddTerrainEdges");

	std::vector<TileEdge> tileEdges;

	// Find all edges between tiles of differently passability statuses
	for (u16 j = j0; j <= j1; ++j)
	{
		for (u16 i = i0; i <= i1; ++i)
		{
			if (!IS_TERRAIN_PASSABLE(terrain.get(i, j), passClass))
			{
				bool any = false; // whether we're adding any edges of this tile

				if (j > 0 && IS_TERRAIN_PASSABLE(terrain.get(i, j-1), passClass))
				{
					TileEdge e = { i, j, TileEdge::BOTTOM };
					tileEdges.push_back(e);
					any = true;
				}

				if (j < terrain.m_H-1 && IS_TERRAIN_PASSABLE(terrain.get(i, j+1), passClass))
				{
					TileEdge e = { i, j, TileEdge::TOP };
					tileEdges.push_back(e);
					any = true;
				}

				if (i > 0 && IS_TERRAIN_PASSABLE(terrain.get(i-1, j), passClass))
				{
					TileEdge e = { i, j, TileEdge::LEFT };
					tileEdges.push_back(e);
					any = true;
				}

				if (i < terrain.m_W-1 && IS_TERRAIN_PASSABLE(terrain.get(i+1, j), passClass))
				{
					TileEdge e = { i, j, TileEdge::RIGHT };
					tileEdges.push_back(e);
					any = true;
				}

				// If we want to add any edge, then add the whole square to the axis-aligned-edges list.
				// (The inner edges are redundant but it's easier than trying to split the squares apart.)
				if (any)
				{
					CFixedVector2D v0 = CFixedVector2D(fixed::FromInt(i * (int)CELL_SIZE) - r, fixed::FromInt(j * (int)CELL_SIZE) - r);
					CFixedVector2D v1 = CFixedVector2D(fixed::FromInt((i+1) * (int)CELL_SIZE) + r, fixed::FromInt((j+1) * (int)CELL_SIZE) + r);
					Edge e = { v0, v1 };
					edgesAA.push_back(e);
				}
			}
		}
	}

	// TODO: maybe we should precompute these terrain edges since they'll rarely change?

	// TODO: for efficiency (minimising the A* search space), we should coalesce adjoining edges

	// Add all the tile outer edges to the search vertex lists
	for (size_t n = 0; n < tileEdges.size(); ++n)
	{
		u16 i = tileEdges[n].i;
		u16 j = tileEdges[n].j;
		CFixedVector2D v0, v1;
		Vertex vert;
		vert.status = Vertex::UNEXPLORED;
		vert.quadOutward = QUADRANT_ALL;

		switch (tileEdges[n].dir)
		{
		case TileEdge::BOTTOM:
		{
			v0 = CFixedVector2D(fixed::FromInt(i * (int)CELL_SIZE) - r, fixed::FromInt(j * (int)CELL_SIZE) - r);
			v1 = CFixedVector2D(fixed::FromInt((i+1) * (int)CELL_SIZE) + r, fixed::FromInt(j * (int)CELL_SIZE) - r);
			vert.p.X = v0.X - EDGE_EXPAND_DELTA; vert.p.Y = v0.Y - EDGE_EXPAND_DELTA; vert.quadInward = QUADRANT_TR; vertexes.push_back(vert);
			vert.p.X = v1.X + EDGE_EXPAND_DELTA; vert.p.Y = v1.Y - EDGE_EXPAND_DELTA; vert.quadInward = QUADRANT_TL; vertexes.push_back(vert);
			break;
		}
		case TileEdge::TOP:
		{
			v0 = CFixedVector2D(fixed::FromInt((i+1) * (int)CELL_SIZE) + r, fixed::FromInt((j+1) * (int)CELL_SIZE) + r);
			v1 = CFixedVector2D(fixed::FromInt(i * (int)CELL_SIZE) - r, fixed::FromInt((j+1) * (int)CELL_SIZE) + r);
			vert.p.X = v0.X + EDGE_EXPAND_DELTA; vert.p.Y = v0.Y + EDGE_EXPAND_DELTA; vert.quadInward = QUADRANT_BL; vertexes.push_back(vert);
			vert.p.X = v1.X - EDGE_EXPAND_DELTA; vert.p.Y = v1.Y + EDGE_EXPAND_DELTA; vert.quadInward = QUADRANT_BR; vertexes.push_back(vert);
			break;
		}
		case TileEdge::LEFT:
		{
			v0 = CFixedVector2D(fixed::FromInt(i * (int)CELL_SIZE) - r, fixed::FromInt((j+1) * (int)CELL_SIZE) + r);
			v1 = CFixedVector2D(fixed::FromInt(i * (int)CELL_SIZE) - r, fixed::FromInt(j * (int)CELL_SIZE) - r);
			vert.p.X = v0.X - EDGE_EXPAND_DELTA; vert.p.Y = v0.Y + EDGE_EXPAND_DELTA; vert.quadInward = QUADRANT_BR; vertexes.push_back(vert);
			vert.p.X = v1.X - EDGE_EXPAND_DELTA; vert.p.Y = v1.Y - EDGE_EXPAND_DELTA; vert.quadInward = QUADRANT_TR; vertexes.push_back(vert);
			break;
		}
		case TileEdge::RIGHT:
		{
			v0 = CFixedVector2D(fixed::FromInt((i+1) * (int)CELL_SIZE) + r, fixed::FromInt(j * (int)CELL_SIZE) - r);
			v1 = CFixedVector2D(fixed::FromInt((i+1) * (int)CELL_SIZE) + r, fixed::FromInt((j+1) * (int)CELL_SIZE) + r);
			vert.p.X = v0.X + EDGE_EXPAND_DELTA; vert.p.Y = v0.Y - EDGE_EXPAND_DELTA; vert.quadInward = QUADRANT_TL; vertexes.push_back(vert);
			vert.p.X = v1.X + EDGE_EXPAND_DELTA; vert.p.Y = v1.Y + EDGE_EXPAND_DELTA; vert.quadInward = QUADRANT_BL; vertexes.push_back(vert);
			break;
		}
		}
	}
}

static void SplitAAEdges(CFixedVector2D a,
		const std::vector<Edge>& edgesAA,
		std::vector<EdgeAA>& edgesLeft, std::vector<EdgeAA>& edgesRight,
		std::vector<EdgeAA>& edgesBottom, std::vector<EdgeAA>& edgesTop)
{
	edgesLeft.reserve(edgesAA.size());
	edgesRight.reserve(edgesAA.size());
	edgesBottom.reserve(edgesAA.size());
	edgesTop.reserve(edgesAA.size());

	for (size_t i = 0; i < edgesAA.size(); ++i)
	{
		if (a.X <= edgesAA[i].p0.X)
		{
			EdgeAA e = { edgesAA[i].p0, edgesAA[i].p1.Y };
			edgesLeft.push_back(e);
		}
		if (a.X >= edgesAA[i].p1.X)
		{
			EdgeAA e = { edgesAA[i].p1, edgesAA[i].p0.Y };
			edgesRight.push_back(e);
		}
		if (a.Y <= edgesAA[i].p0.Y)
		{
			EdgeAA e = { edgesAA[i].p0, edgesAA[i].p1.X };
			edgesBottom.push_back(e);
		}
		if (a.Y >= edgesAA[i].p1.Y)
		{
			EdgeAA e = { edgesAA[i].p1, edgesAA[i].p0.X };
			edgesTop.push_back(e);
		}
	}
}

/**
 * Functor for sorting edges by approximate proximity to a fixed point.
 */
struct EdgeSort
{
	CFixedVector2D src;
	EdgeSort(CFixedVector2D src) : src(src) { }
	bool operator()(const Edge& a, const Edge& b)
	{
		if ((a.p0 - src).CompareLength(b.p0 - src) < 0)
			return true;
		return false;
	}
};

void CCmpPathfinder::ComputeShortPath(const IObstructionTestFilter& filter,
	entity_pos_t x0, entity_pos_t z0, entity_pos_t r,
	entity_pos_t range, const Goal& goal, pass_class_t passClass, Path& path)
{
	UpdateGrid(); // TODO: only need to bother updating if the terrain changed

	PROFILE("ComputeShortPath");
//	ScopeTimer UID__(L"ComputeShortPath");

	m_DebugOverlayShortPathLines.clear();

	if (m_DebugOverlay)
	{
		// Render the goal shape
		m_DebugOverlayShortPathLines.push_back(SOverlayLine());
		m_DebugOverlayShortPathLines.back().m_Color = CColor(1, 0, 0, 1);
		switch (goal.type)
		{
		case CCmpPathfinder::Goal::POINT:
		{
			SimRender::ConstructCircleOnGround(GetSimContext(), goal.x.ToFloat(), goal.z.ToFloat(), 0.2f, m_DebugOverlayShortPathLines.back(), true);
			break;
		}
		case CCmpPathfinder::Goal::CIRCLE:
		{
			SimRender::ConstructCircleOnGround(GetSimContext(), goal.x.ToFloat(), goal.z.ToFloat(), goal.hw.ToFloat(), m_DebugOverlayShortPathLines.back(), true);
			break;
		}
		case CCmpPathfinder::Goal::SQUARE:
		{
			float a = atan2f(goal.v.X.ToFloat(), goal.v.Y.ToFloat());
			SimRender::ConstructSquareOnGround(GetSimContext(), goal.x.ToFloat(), goal.z.ToFloat(), goal.hw.ToFloat()*2, goal.hh.ToFloat()*2, a, m_DebugOverlayShortPathLines.back(), true);
			break;
		}
		}
	}

	// List of collision edges - paths must never cross these.
	// (Edges are one-sided so intersections are fine in one direction, but not the other direction.)
	std::vector<Edge> edges;
	std::vector<Edge> edgesAA; // axis-aligned squares

	// Create impassable edges at the max-range boundary, so we can't escape the region
	// where we're meant to be searching
	fixed rangeXMin = x0 - range;
	fixed rangeXMax = x0 + range;
	fixed rangeZMin = z0 - range;
	fixed rangeZMax = z0 + range;
	{
		// (The edges are the opposite direction to usual, so it's an inside-out square)
		Edge e0 = { CFixedVector2D(rangeXMin, rangeZMin), CFixedVector2D(rangeXMin, rangeZMax) };
		Edge e1 = { CFixedVector2D(rangeXMin, rangeZMax), CFixedVector2D(rangeXMax, rangeZMax) };
		Edge e2 = { CFixedVector2D(rangeXMax, rangeZMax), CFixedVector2D(rangeXMax, rangeZMin) };
		Edge e3 = { CFixedVector2D(rangeXMax, rangeZMin), CFixedVector2D(rangeXMin, rangeZMin) };
		edges.push_back(e0);
		edges.push_back(e1);
		edges.push_back(e2);
		edges.push_back(e3);
	}

	// List of obstruction vertexes (plus start/end points); we'll try to find paths through
	// the graph defined by these vertexes
	std::vector<Vertex> vertexes;

	// Add the start point to the graph
	CFixedVector2D posStart(x0, z0);
	fixed hStart = (posStart - NearestPointOnGoal(posStart, goal)).Length();
	Vertex start = { posStart, fixed::Zero(), hStart, 0, Vertex::OPEN, QUADRANT_NONE, QUADRANT_ALL };
	vertexes.push_back(start);
	const size_t START_VERTEX_ID = 0;

	// Add the goal vertex to the graph.
	// Since the goal isn't always a point, this a special magic virtual vertex which moves around - whenever
	// we look at it from another vertex, it is moved to be the closest point on the goal shape to that vertex.
	Vertex end = { CFixedVector2D(goal.x, goal.z), fixed::Zero(), fixed::Zero(), 0, Vertex::UNEXPLORED, QUADRANT_NONE, QUADRANT_ALL };
	vertexes.push_back(end);
	const size_t GOAL_VERTEX_ID = 1;

	// Add terrain obstructions
	{
		u16 i0, j0, i1, j1;
		NearestTile(rangeXMin, rangeZMin, i0, j0);
		NearestTile(rangeXMax, rangeZMax, i1, j1);
		AddTerrainEdges(edgesAA, vertexes, i0, j0, i1, j1, r, passClass, *m_Grid);
	}

	// Find all the obstruction squares that might affect us
	CmpPtr<ICmpObstructionManager> cmpObstructionManager(GetSimContext(), SYSTEM_ENTITY);
	std::vector<ICmpObstructionManager::ObstructionSquare> squares;
	cmpObstructionManager->GetObstructionsInRange(filter, rangeXMin - r, rangeZMin - r, rangeXMax + r, rangeZMax + r, squares);

	// Resize arrays to reduce reallocations
	vertexes.reserve(vertexes.size() + squares.size()*4);
	edgesAA.reserve(edgesAA.size() + squares.size()); // (assume most squares are AA)

	// Convert each obstruction square into collision edges and search graph vertexes
	for (size_t i = 0; i < squares.size(); ++i)
	{
		CFixedVector2D center(squares[i].x, squares[i].z);
		CFixedVector2D u = squares[i].u;
		CFixedVector2D v = squares[i].v;

		// Expand the vertexes by the moving unit's collision radius, to find the
		// closest we can get to it

		CFixedVector2D hd0(squares[i].hw + r + EDGE_EXPAND_DELTA, squares[i].hh + r + EDGE_EXPAND_DELTA);
		CFixedVector2D hd1(squares[i].hw + r + EDGE_EXPAND_DELTA, -(squares[i].hh + r + EDGE_EXPAND_DELTA));

		// Check whether this is an axis-aligned square
		bool aa = (u.X == fixed::FromInt(1) && u.Y == fixed::Zero() && v.X == fixed::Zero() && v.Y == fixed::FromInt(1));

		Vertex vert;
		vert.status = Vertex::UNEXPLORED;
		vert.quadInward = QUADRANT_NONE;
		vert.quadOutward = QUADRANT_ALL;
		vert.p.X = center.X - hd0.Dot(u); vert.p.Y = center.Y + hd0.Dot(v); if (aa) vert.quadInward = QUADRANT_BR; vertexes.push_back(vert);
		vert.p.X = center.X - hd1.Dot(u); vert.p.Y = center.Y + hd1.Dot(v); if (aa) vert.quadInward = QUADRANT_TR; vertexes.push_back(vert);
		vert.p.X = center.X + hd0.Dot(u); vert.p.Y = center.Y - hd0.Dot(v); if (aa) vert.quadInward = QUADRANT_TL; vertexes.push_back(vert);
		vert.p.X = center.X + hd1.Dot(u); vert.p.Y = center.Y - hd1.Dot(v); if (aa) vert.quadInward = QUADRANT_BL; vertexes.push_back(vert);

		// Add the edges:

		CFixedVector2D h0(squares[i].hw + r, squares[i].hh + r);
		CFixedVector2D h1(squares[i].hw + r, -(squares[i].hh + r));

		CFixedVector2D ev0(center.X - h0.Dot(u), center.Y + h0.Dot(v));
		CFixedVector2D ev1(center.X - h1.Dot(u), center.Y + h1.Dot(v));
		CFixedVector2D ev2(center.X + h0.Dot(u), center.Y - h0.Dot(v));
		CFixedVector2D ev3(center.X + h1.Dot(u), center.Y - h1.Dot(v));
		if (aa)
		{
			Edge e = { ev1, ev3 };
			edgesAA.push_back(e);
		}
		else
		{
			Edge e0 = { ev0, ev1 };
			Edge e1 = { ev1, ev2 };
			Edge e2 = { ev2, ev3 };
			Edge e3 = { ev3, ev0 };
			edges.push_back(e0);
			edges.push_back(e1);
			edges.push_back(e2);
			edges.push_back(e3);
		}

		// TODO: should clip out vertexes and edges that are outside the range,
		// to reduce the search space
	}

	ENSURE(vertexes.size() < 65536); // we store array indexes as u16

	if (m_DebugOverlay)
	{
		// Render the obstruction edges
		for (size_t i = 0; i < edges.size(); ++i)
		{
			m_DebugOverlayShortPathLines.push_back(SOverlayLine());
			m_DebugOverlayShortPathLines.back().m_Color = CColor(0, 1, 1, 1);
			std::vector<float> xz;
			xz.push_back(edges[i].p0.X.ToFloat());
			xz.push_back(edges[i].p0.Y.ToFloat());
			xz.push_back(edges[i].p1.X.ToFloat());
			xz.push_back(edges[i].p1.Y.ToFloat());
			SimRender::ConstructLineOnGround(GetSimContext(), xz, m_DebugOverlayShortPathLines.back(), true);
		}

		for (size_t i = 0; i < edgesAA.size(); ++i)
		{
			m_DebugOverlayShortPathLines.push_back(SOverlayLine());
			m_DebugOverlayShortPathLines.back().m_Color = CColor(0, 1, 1, 1);
			std::vector<float> xz;
			xz.push_back(edgesAA[i].p0.X.ToFloat());
			xz.push_back(edgesAA[i].p0.Y.ToFloat());
			xz.push_back(edgesAA[i].p0.X.ToFloat());
			xz.push_back(edgesAA[i].p1.Y.ToFloat());
			xz.push_back(edgesAA[i].p1.X.ToFloat());
			xz.push_back(edgesAA[i].p1.Y.ToFloat());
			xz.push_back(edgesAA[i].p1.X.ToFloat());
			xz.push_back(edgesAA[i].p0.Y.ToFloat());
			xz.push_back(edgesAA[i].p0.X.ToFloat());
			xz.push_back(edgesAA[i].p0.Y.ToFloat());
			SimRender::ConstructLineOnGround(GetSimContext(), xz, m_DebugOverlayShortPathLines.back(), true);
		}
	}

	// Do an A* search over the vertex/visibility graph:

	// Since we are just measuring Euclidean distance the heuristic is admissible,
	// so we never have to re-examine a node once it's been moved to the closed set.

	// To save time in common cases, we don't precompute a graph of valid edges between vertexes;
	// we do it lazily instead. When the search algorithm reaches a vertex, we examine every other
	// vertex and see if we can reach it without hitting any collision edges, and ignore the ones
	// we can't reach. Since the algorithm can only reach a vertex once (and then it'll be marked
	// as closed), we won't be doing any redundant visibility computations.

	PROFILE_START("A*");

	PriorityQueue open;
	PriorityQueue::Item qiStart = { START_VERTEX_ID, start.h };
	open.push(qiStart);

	u16 idBest = START_VERTEX_ID;
	fixed hBest = start.h;

	while (!open.empty())
	{
		// Move best tile from open to closed
		PriorityQueue::Item curr = open.pop();
		vertexes[curr.id].status = Vertex::CLOSED;

		// If we've reached the destination, stop
		if (curr.id == GOAL_VERTEX_ID)
		{
			idBest = curr.id;
			break;
		}

		// Sort the edges so ones nearer this vertex are checked first by CheckVisibility,
		// since they're more likely to block the rays
		std::sort(edgesAA.begin(), edgesAA.end(), EdgeSort(vertexes[curr.id].p));

		std::vector<EdgeAA> edgesLeft;
		std::vector<EdgeAA> edgesRight;
		std::vector<EdgeAA> edgesBottom;
		std::vector<EdgeAA> edgesTop;
		SplitAAEdges(vertexes[curr.id].p, edgesAA, edgesLeft, edgesRight, edgesBottom, edgesTop);

		// Check the lines to every other vertex
		for (size_t n = 0; n < vertexes.size(); ++n)
		{
			if (vertexes[n].status == Vertex::CLOSED)
				continue;

			// If this is the magical goal vertex, move it to near the current vertex
			CFixedVector2D npos;
			if (n == GOAL_VERTEX_ID)
			{
				npos = NearestPointOnGoal(vertexes[curr.id].p, goal);

				// To prevent integer overflows later on, we need to ensure all vertexes are
				// 'close' to the source. The goal might be far away (not a good idea but
				// sometimes it happens), so clamp it to the current search range
				npos.X = clamp(npos.X, rangeXMin, rangeXMax);
				npos.Y = clamp(npos.Y, rangeZMin, rangeZMax);
			}
			else
			{
				npos = vertexes[n].p;
			}

			// Work out which quadrant(s) we're approaching the new vertex from
			u8 quad = 0;
			if (vertexes[curr.id].p.X <= npos.X && vertexes[curr.id].p.Y <= npos.Y) quad |= QUADRANT_BL;
			if (vertexes[curr.id].p.X >= npos.X && vertexes[curr.id].p.Y >= npos.Y) quad |= QUADRANT_TR;
			if (vertexes[curr.id].p.X <= npos.X && vertexes[curr.id].p.Y >= npos.Y) quad |= QUADRANT_TL;
			if (vertexes[curr.id].p.X >= npos.X && vertexes[curr.id].p.Y <= npos.Y) quad |= QUADRANT_BR;

			// Check that the new vertex is in the right quadrant for the old vertex
			if (!(vertexes[curr.id].quadOutward & quad))
			{
				// Hack: Always head towards the goal if possible, to avoid missing it if it's
				// inside another unit
				if (n != GOAL_VERTEX_ID)
				{
					continue;
				}
			}

			bool visible =
				CheckVisibilityLeft(vertexes[curr.id].p, npos, edgesLeft) &&
				CheckVisibilityRight(vertexes[curr.id].p, npos, edgesRight) &&
				CheckVisibilityBottom(vertexes[curr.id].p, npos, edgesBottom) &&
				CheckVisibilityTop(vertexes[curr.id].p, npos, edgesTop) &&
				CheckVisibility(vertexes[curr.id].p, npos, edges);

			/*
			// Render the edges that we examine
			m_DebugOverlayShortPathLines.push_back(SOverlayLine());
			m_DebugOverlayShortPathLines.back().m_Color = visible ? CColor(0, 1, 0, 0.5) : CColor(1, 0, 0, 0.5);
			std::vector<float> xz;
			xz.push_back(vertexes[curr.id].p.X.ToFloat());
			xz.push_back(vertexes[curr.id].p.Y.ToFloat());
			xz.push_back(npos.X.ToFloat());
			xz.push_back(npos.Y.ToFloat());
			SimRender::ConstructLineOnGround(GetSimContext(), xz, m_DebugOverlayShortPathLines.back(), false);
			//*/

			if (visible)
			{
				fixed g = vertexes[curr.id].g + (vertexes[curr.id].p - npos).Length();

				// If this is a new tile, compute the heuristic distance
				if (vertexes[n].status == Vertex::UNEXPLORED)
				{
					// Add it to the open list:
					vertexes[n].status = Vertex::OPEN;
					vertexes[n].g = g;
					vertexes[n].h = DistanceToGoal(npos, goal);
					vertexes[n].pred = curr.id;

					// If this is an axis-aligned shape, the path must continue in the same quadrant
					// direction (but not go into the inside of the shape).
					// Hack: If we started *inside* a shape then perhaps headed to its corner (e.g. the unit
					// was very near another unit), don't restrict further pathing.
					if (vertexes[n].quadInward && !(curr.id == START_VERTEX_ID && g < fixed::FromInt(8)))
						vertexes[n].quadOutward = ((~vertexes[n].quadInward) & quad) & 0xF;

					if (n == GOAL_VERTEX_ID)
						vertexes[n].p = npos; // remember the new best goal position

					PriorityQueue::Item t = { (u16)n, g + vertexes[n].h };
					open.push(t);

					// Remember the heuristically best vertex we've seen so far, in case we never actually reach the target
					if (vertexes[n].h < hBest)
					{
						idBest = (u16)n;
						hBest = vertexes[n].h;
					}
				}
				else // must be OPEN
				{
					// If we've already seen this tile, and the new path to this tile does not have a
					// better cost, then stop now
					if (g >= vertexes[n].g)
						continue;

					// Otherwise, we have a better path, so replace the old one with the new cost/parent
					vertexes[n].g = g;
					vertexes[n].pred = curr.id;

					// If this is an axis-aligned shape, the path must continue in the same quadrant
					// direction (but not go into the inside of the shape).
					if (vertexes[n].quadInward)
						vertexes[n].quadOutward = ((~vertexes[n].quadInward) & quad) & 0xF;

					if (n == GOAL_VERTEX_ID)
						vertexes[n].p = npos; // remember the new best goal position

					open.promote((u16)n, g + vertexes[n].h);
				}
			}
		}
	}

	// Reconstruct the path (in reverse)
	for (u16 id = idBest; id != START_VERTEX_ID; id = vertexes[id].pred)
	{
		Waypoint w = { vertexes[id].p.X, vertexes[id].p.Y };
		path.m_Waypoints.push_back(w);
	}

	PROFILE_END("A*");
}

bool CCmpPathfinder::CheckMovement(const IObstructionTestFilter& filter,
	entity_pos_t x0, entity_pos_t z0, entity_pos_t x1, entity_pos_t z1, entity_pos_t r,
	pass_class_t passClass)
{
	CmpPtr<ICmpObstructionManager> cmpObstructionManager(GetSimContext(), SYSTEM_ENTITY);
	if (cmpObstructionManager.null())
		return false;

	if (cmpObstructionManager->TestLine(filter, x0, z0, x1, z1, r))
		return false;

	// Test against terrain:

	// (TODO: this could probably be a tiny bit faster by not reusing all the vertex computation code)

	UpdateGrid();

	std::vector<Edge> edgesAA;
	std::vector<Vertex> vertexes;

	u16 i0, j0, i1, j1;
	NearestTile(std::min(x0, x1) - r, std::min(z0, z1) - r, i0, j0);
	NearestTile(std::max(x0, x1) + r, std::max(z0, z1) + r, i1, j1);
	AddTerrainEdges(edgesAA, vertexes, i0, j0, i1, j1, r, passClass, *m_Grid);

	CFixedVector2D a(x0, z0);
	CFixedVector2D b(x1, z1);

	std::vector<EdgeAA> edgesLeft;
	std::vector<EdgeAA> edgesRight;
	std::vector<EdgeAA> edgesBottom;
	std::vector<EdgeAA> edgesTop;
	SplitAAEdges(a, edgesAA, edgesLeft, edgesRight, edgesBottom, edgesTop);

	bool visible =
		CheckVisibilityLeft(a, b, edgesLeft) &&
		CheckVisibilityRight(a, b, edgesRight) &&
		CheckVisibilityBottom(a, b, edgesBottom) &&
		CheckVisibilityTop(a, b, edgesTop);

	return visible;
}
