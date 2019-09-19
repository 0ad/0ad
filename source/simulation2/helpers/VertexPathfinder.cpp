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

#include "VertexPathfinder.h"

#include "lib/timer.h"
#include "ps/Profile.h"
#include "renderer/Scene.h"
#include "simulation2/components/ICmpObstructionManager.h"
#include "simulation2/helpers/PriorityQueue.h"
#include "simulation2/helpers/Render.h"
#include "simulation2/system/SimContext.h"

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

// When computing vertexes to insert into the search graph,
// add a small delta so that the vertexes of an edge don't get interpreted
// as crossing the edge (given minor numerical inaccuracies)
static const entity_pos_t EDGE_EXPAND_DELTA = entity_pos_t::FromInt(1)/16;

/**
 * Check whether a ray from 'a' to 'b' crosses any of the edges.
 * (Edges are one-sided so it's only considered a cross if going from front to back.)
 */
inline static bool CheckVisibility(const CFixedVector2D& a, const CFixedVector2D& b, const std::vector<Edge>& edges)
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

inline static bool CheckVisibilityLeft(const CFixedVector2D& a, const CFixedVector2D& b, const std::vector<EdgeAA>& edges)
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

inline static bool CheckVisibilityRight(const CFixedVector2D& a, const CFixedVector2D& b, const std::vector<EdgeAA>& edges)
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

inline static bool CheckVisibilityBottom(const CFixedVector2D& a, const CFixedVector2D& b, const std::vector<EdgeAA>& edges)
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

inline static bool CheckVisibilityTop(const CFixedVector2D& a, const CFixedVector2D& b, const std::vector<EdgeAA>& edges)
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

typedef PriorityQueueHeap<u16, fixed, fixed> VertexPriorityQueue;

/**
 * Add edges and vertexes to represent the boundaries between passable and impassable
 * navcells (for impassable terrain).
 * Navcells i0 <= i <= i1, j0 <= j <= j1 will be considered.
 */
static void AddTerrainEdges(std::vector<Edge>& edges, std::vector<Vertex>& vertexes,
	int i0, int j0, int i1, int j1,
	pass_class_t passClass, const Grid<NavcellData>& grid)
{

	// Clamp the coordinates so we won't attempt to sample outside of the grid.
	// (This assumes the outermost ring of navcells (which are always impassable)
	// won't have a boundary with any passable navcells. TODO: is that definitely
	// safe enough?)

	i0 = Clamp(i0, 1, grid.m_W-2);
	j0 = Clamp(j0, 1, grid.m_H-2);
	i1 = Clamp(i1, 1, grid.m_W-2);
	j1 = Clamp(j1, 1, grid.m_H-2);

	for (int j = j0; j <= j1; ++j)
	{
		for (int i = i0; i <= i1; ++i)
		{
			if (IS_PASSABLE(grid.get(i, j), passClass))
				continue;

			if (IS_PASSABLE(grid.get(i+1, j), passClass) && IS_PASSABLE(grid.get(i, j+1), passClass) && IS_PASSABLE(grid.get(i+1, j+1), passClass))
			{
				Vertex vert;
				vert.status = Vertex::UNEXPLORED;
				vert.quadOutward = QUADRANT_ALL;
				vert.quadInward = QUADRANT_BL;
				vert.p = CFixedVector2D(fixed::FromInt(i+1)+EDGE_EXPAND_DELTA, fixed::FromInt(j+1)+EDGE_EXPAND_DELTA).Multiply(Pathfinding::NAVCELL_SIZE);
				vertexes.push_back(vert);
			}

			if (IS_PASSABLE(grid.get(i-1, j), passClass) && IS_PASSABLE(grid.get(i, j+1), passClass) && IS_PASSABLE(grid.get(i-1, j+1), passClass))
			{
				Vertex vert;
				vert.status = Vertex::UNEXPLORED;
				vert.quadOutward = QUADRANT_ALL;
				vert.quadInward = QUADRANT_BR;
				vert.p = CFixedVector2D(fixed::FromInt(i)-EDGE_EXPAND_DELTA, fixed::FromInt(j+1)+EDGE_EXPAND_DELTA).Multiply(Pathfinding::NAVCELL_SIZE);
				vertexes.push_back(vert);
			}

			if (IS_PASSABLE(grid.get(i+1, j), passClass) && IS_PASSABLE(grid.get(i, j-1), passClass) && IS_PASSABLE(grid.get(i+1, j-1), passClass))
			{
				Vertex vert;
				vert.status = Vertex::UNEXPLORED;
				vert.quadOutward = QUADRANT_ALL;
				vert.quadInward = QUADRANT_TL;
				vert.p = CFixedVector2D(fixed::FromInt(i+1)+EDGE_EXPAND_DELTA, fixed::FromInt(j)-EDGE_EXPAND_DELTA).Multiply(Pathfinding::NAVCELL_SIZE);
				vertexes.push_back(vert);
			}

			if (IS_PASSABLE(grid.get(i-1, j), passClass) && IS_PASSABLE(grid.get(i, j-1), passClass) && IS_PASSABLE(grid.get(i-1, j-1), passClass))
			{
				Vertex vert;
				vert.status = Vertex::UNEXPLORED;
				vert.quadOutward = QUADRANT_ALL;
				vert.quadInward = QUADRANT_TR;
				vert.p = CFixedVector2D(fixed::FromInt(i)-EDGE_EXPAND_DELTA, fixed::FromInt(j)-EDGE_EXPAND_DELTA).Multiply(Pathfinding::NAVCELL_SIZE);
				vertexes.push_back(vert);
			}
		}
	}

	// XXX rewrite this stuff
	std::vector<u16> segmentsR;
	std::vector<u16> segmentsL;
	for (int j = j0; j < j1; ++j)
	{
		segmentsR.clear();
		segmentsL.clear();
		for (int i = i0; i <= i1; ++i)
		{
			bool a = IS_PASSABLE(grid.get(i, j+1), passClass);
			bool b = IS_PASSABLE(grid.get(i, j), passClass);
			if (a && !b)
				segmentsL.push_back(i);
			if (b && !a)
				segmentsR.push_back(i);
		}

		if (!segmentsR.empty())
		{
			segmentsR.push_back(0); // sentinel value to simplify the loop
			u16 ia = segmentsR[0];
			u16 ib = ia + 1;
			for (size_t n = 1; n < segmentsR.size(); ++n)
			{
				if (segmentsR[n] == ib)
					++ib;
				else
				{
					CFixedVector2D v0 = CFixedVector2D(fixed::FromInt(ia), fixed::FromInt(j+1)).Multiply(Pathfinding::NAVCELL_SIZE);
					CFixedVector2D v1 = CFixedVector2D(fixed::FromInt(ib), fixed::FromInt(j+1)).Multiply(Pathfinding::NAVCELL_SIZE);
					edges.emplace_back(Edge{ v0, v1 });

					ia = segmentsR[n];
					ib = ia + 1;
				}
			}
		}

		if (!segmentsL.empty())
		{
			segmentsL.push_back(0); // sentinel value to simplify the loop
			u16 ia = segmentsL[0];
			u16 ib = ia + 1;
			for (size_t n = 1; n < segmentsL.size(); ++n)
			{
				if (segmentsL[n] == ib)
					++ib;
				else
				{
					CFixedVector2D v0 = CFixedVector2D(fixed::FromInt(ib), fixed::FromInt(j+1)).Multiply(Pathfinding::NAVCELL_SIZE);
					CFixedVector2D v1 = CFixedVector2D(fixed::FromInt(ia), fixed::FromInt(j+1)).Multiply(Pathfinding::NAVCELL_SIZE);
					edges.emplace_back(Edge{ v0, v1 });

					ia = segmentsL[n];
					ib = ia + 1;
				}
			}
		}
	}
	std::vector<u16> segmentsU;
	std::vector<u16> segmentsD;
	for (int i = i0; i < i1; ++i)
	{
		segmentsU.clear();
		segmentsD.clear();
		for (int j = j0; j <= j1; ++j)
		{
			bool a = IS_PASSABLE(grid.get(i+1, j), passClass);
			bool b = IS_PASSABLE(grid.get(i, j), passClass);
			if (a && !b)
				segmentsU.push_back(j);
			if (b && !a)
				segmentsD.push_back(j);
		}

		if (!segmentsU.empty())
		{
			segmentsU.push_back(0); // sentinel value to simplify the loop
			u16 ja = segmentsU[0];
			u16 jb = ja + 1;
			for (size_t n = 1; n < segmentsU.size(); ++n)
			{
				if (segmentsU[n] == jb)
					++jb;
				else
				{
					CFixedVector2D v0 = CFixedVector2D(fixed::FromInt(i+1), fixed::FromInt(ja)).Multiply(Pathfinding::NAVCELL_SIZE);
					CFixedVector2D v1 = CFixedVector2D(fixed::FromInt(i+1), fixed::FromInt(jb)).Multiply(Pathfinding::NAVCELL_SIZE);
					edges.emplace_back(Edge{ v0, v1 });

					ja = segmentsU[n];
					jb = ja + 1;
				}
			}
		}

		if (!segmentsD.empty())
		{
			segmentsD.push_back(0); // sentinel value to simplify the loop
			u16 ja = segmentsD[0];
			u16 jb = ja + 1;
			for (size_t n = 1; n < segmentsD.size(); ++n)
			{
				if (segmentsD[n] == jb)
					++jb;
				else
				{
					CFixedVector2D v0 = CFixedVector2D(fixed::FromInt(i+1), fixed::FromInt(jb)).Multiply(Pathfinding::NAVCELL_SIZE);
					CFixedVector2D v1 = CFixedVector2D(fixed::FromInt(i+1), fixed::FromInt(ja)).Multiply(Pathfinding::NAVCELL_SIZE);
					edges.emplace_back(Edge{ v0, v1 });

					ja = segmentsD[n];
					jb = ja + 1;
				}
			}
		}
	}
}

static void SplitAAEdges(const CFixedVector2D& a,
		const std::vector<Edge>& edges,
		const std::vector<Square>& squares,
		std::vector<Edge>& edgesUnaligned,
		std::vector<EdgeAA>& edgesLeft, std::vector<EdgeAA>& edgesRight,
		std::vector<EdgeAA>& edgesBottom, std::vector<EdgeAA>& edgesTop)
{

	for (const Square& square : squares)
	{
		if (a.X <= square.p0.X)
			edgesLeft.emplace_back(EdgeAA{ square.p0, square.p1.Y });
		if (a.X >= square.p1.X)
			edgesRight.emplace_back(EdgeAA{ square.p1, square.p0.Y });
		if (a.Y <= square.p0.Y)
			edgesBottom.emplace_back(EdgeAA{ square.p0, square.p1.X });
		if (a.Y >= square.p1.Y)
			edgesTop.emplace_back(EdgeAA{ square.p1, square.p0.X });
	}

	for (const Edge& edge : edges)
	{
		if (edge.p0.X == edge.p1.X)
		{
			if (edge.p1.Y < edge.p0.Y)
			{
				if (!(a.X <= edge.p0.X))
					continue;
				edgesLeft.emplace_back(EdgeAA{ edge.p1, edge.p0.Y });
			}
			else
			{
				if (!(a.X >= edge.p0.X))
					continue;
				edgesRight.emplace_back(EdgeAA{ edge.p1, edge.p0.Y });
			}
		}
		else if (edge.p0.Y == edge.p1.Y)
		{
			if (edge.p0.X < edge.p1.X)
			{
				if (!(a.Y <= edge.p0.Y))
					continue;
				edgesBottom.emplace_back(EdgeAA{ edge.p0, edge.p1.X });
			}
			else
			{
				if (!(a.Y >= edge.p0.Y))
					continue;
				edgesTop.emplace_back(EdgeAA{ edge.p0, edge.p1.X });
			}
		}
		else
			edgesUnaligned.push_back(edge);
	}
}

/**
 * Functor for sorting edge-squares by approximate proximity to a fixed point.
 */
struct SquareSort
{
	CFixedVector2D src;
	SquareSort(CFixedVector2D src) : src(src) { }
	bool operator()(const Square& a, const Square& b) const
	{
		if ((a.p0 - src).CompareLength(b.p0 - src) < 0)
			return true;
		return false;
	}
};

WaypointPath VertexPathfinder::ComputeShortPath(const ShortPathRequest& request, CmpPtr<ICmpObstructionManager> cmpObstructionManager) const
{
	PROFILE2("ComputeShortPath");

	DebugRenderGoal(cmpObstructionManager->GetSimContext(), request.goal);

	// Create impassable edges at the max-range boundary, so we can't escape the region
	// where we're meant to be searching

	fixed rangeXMin = request.x0 - request.range;
	fixed rangeXMax = request.x0 + request.range;
	fixed rangeZMin = request.z0 - request.range;
	fixed rangeZMax = request.z0 + request.range;

	// If useful, move the center of the search-space so that it's slightly towards the goal,
	// as the vertex pathfinder tends to be used to get around entities in front of us.
	CFixedVector2D toGoal = CFixedVector2D(request.goal.x, request.goal.z) - CFixedVector2D(request.x0, request.z0);
	if (toGoal.CompareLength(request.range) >= 0)
	{
		fixed toGoalLength = toGoal.Length();
		fixed inv = fixed::FromInt(1) / toGoalLength;
		rangeXMin += (toGoal.Multiply(std::min(toGoalLength / 2, request.range * 3 / 5)).Multiply(inv)).X;
		rangeXMax += (toGoal.Multiply(std::min(toGoalLength / 2, request.range * 3 / 5)).Multiply(inv)).X;
		rangeZMin += (toGoal.Multiply(std::min(toGoalLength / 2, request.range * 3 / 5)).Multiply(inv)).Y;
		rangeZMax += (toGoal.Multiply(std::min(toGoalLength / 2, request.range * 3 / 5)).Multiply(inv)).Y;
	}

	// Add domain edges
	// (Inside-out square, so edges are in reverse from the usual direction.)
	m_Edges.emplace_back(Edge{ CFixedVector2D(rangeXMin, rangeZMin), CFixedVector2D(rangeXMin, rangeZMax) });
	m_Edges.emplace_back(Edge{ CFixedVector2D(rangeXMin, rangeZMax), CFixedVector2D(rangeXMax, rangeZMax) });
	m_Edges.emplace_back(Edge{ CFixedVector2D(rangeXMax, rangeZMax), CFixedVector2D(rangeXMax, rangeZMin) });
	m_Edges.emplace_back(Edge{ CFixedVector2D(rangeXMax, rangeZMin), CFixedVector2D(rangeXMin, rangeZMin) });


	// Add the start point to the graph
	CFixedVector2D posStart(request.x0, request.z0);
	fixed hStart = (posStart - request.goal.NearestPointOnGoal(posStart)).Length();
	Vertex start = { posStart, fixed::Zero(), hStart, 0, Vertex::OPEN, QUADRANT_NONE, QUADRANT_ALL };
	m_Vertexes.push_back(start);
	const size_t START_VERTEX_ID = 0;

	// Add the goal vertex to the graph.
	// Since the goal isn't always a point, this a special magic virtual vertex which moves around - whenever
	// we look at it from another vertex, it is moved to be the closest point on the goal shape to that vertex.
	Vertex end = { CFixedVector2D(request.goal.x, request.goal.z), fixed::Zero(), fixed::Zero(), 0, Vertex::UNEXPLORED, QUADRANT_NONE, QUADRANT_ALL };
	m_Vertexes.push_back(end);
	const size_t GOAL_VERTEX_ID = 1;

	// Find all the obstruction squares that might affect us
	std::vector<ICmpObstructionManager::ObstructionSquare> squares;
	size_t staticShapesNb = 0;
	ControlGroupMovementObstructionFilter filter(request.avoidMovingUnits, request.group);
	cmpObstructionManager->GetStaticObstructionsInRange(filter, rangeXMin - request.clearance, rangeZMin - request.clearance, rangeXMax + request.clearance, rangeZMax + request.clearance, squares);
	staticShapesNb = squares.size();
	cmpObstructionManager->GetUnitObstructionsInRange(filter, rangeXMin - request.clearance, rangeZMin - request.clearance, rangeXMax + request.clearance, rangeZMax + request.clearance, squares);

	// Change array capacities to reduce reallocations
	m_Vertexes.reserve(m_Vertexes.size() + squares.size()*4);
	m_EdgeSquares.reserve(m_EdgeSquares.size() + squares.size()); // (assume most squares are AA)

	entity_pos_t pathfindClearance = request.clearance;

	// Convert each obstruction square into collision edges and search graph vertexes
	for (size_t i = 0; i < squares.size(); ++i)
	{
		CFixedVector2D center(squares[i].x, squares[i].z);
		CFixedVector2D u = squares[i].u;
		CFixedVector2D v = squares[i].v;

		if (i >= staticShapesNb)
			pathfindClearance = request.clearance - entity_pos_t::FromInt(1)/2;

		// Expand the vertexes by the moving unit's collision radius, to find the
		// closest we can get to it

		CFixedVector2D hd0(squares[i].hw + pathfindClearance + EDGE_EXPAND_DELTA,   squares[i].hh + pathfindClearance + EDGE_EXPAND_DELTA);
		CFixedVector2D hd1(squares[i].hw + pathfindClearance + EDGE_EXPAND_DELTA, -(squares[i].hh + pathfindClearance + EDGE_EXPAND_DELTA));

		// Check whether this is an axis-aligned square
		bool aa = (u.X == fixed::FromInt(1) && u.Y == fixed::Zero() && v.X == fixed::Zero() && v.Y == fixed::FromInt(1));

		Vertex vert;
		vert.status = Vertex::UNEXPLORED;
		vert.quadInward = QUADRANT_NONE;
		vert.quadOutward = QUADRANT_ALL;

		vert.p.X = center.X - hd0.Dot(u);
		vert.p.Y = center.Y + hd0.Dot(v);
		if (aa)
		{
			vert.quadInward = QUADRANT_BR;
			vert.quadOutward = (~vert.quadInward) & 0xF;
		}
		if (vert.p.X >= rangeXMin && vert.p.Y >= rangeZMin && vert.p.X <= rangeXMax && vert.p.Y <= rangeZMax)
			m_Vertexes.push_back(vert);

		vert.p.X = center.X - hd1.Dot(u);
		vert.p.Y = center.Y + hd1.Dot(v);
		if (aa)
		{
			vert.quadInward = QUADRANT_TR;
			vert.quadOutward = (~vert.quadInward) & 0xF;
		}
		if (vert.p.X >= rangeXMin && vert.p.Y >= rangeZMin && vert.p.X <= rangeXMax && vert.p.Y <= rangeZMax)
			m_Vertexes.push_back(vert);

		vert.p.X = center.X + hd0.Dot(u);
		vert.p.Y = center.Y - hd0.Dot(v);
		if (aa)
		{
			vert.quadInward = QUADRANT_TL;
			vert.quadOutward = (~vert.quadInward) & 0xF;
		}
		if (vert.p.X >= rangeXMin && vert.p.Y >= rangeZMin && vert.p.X <= rangeXMax && vert.p.Y <= rangeZMax)
			m_Vertexes.push_back(vert);

		vert.p.X = center.X + hd1.Dot(u);
		vert.p.Y = center.Y - hd1.Dot(v);
		if (aa)
		{
			vert.quadInward = QUADRANT_BL;
			vert.quadOutward = (~vert.quadInward) & 0xF;
		}
		if (vert.p.X >= rangeXMin && vert.p.Y >= rangeZMin && vert.p.X <= rangeXMax && vert.p.Y <= rangeZMax)
			m_Vertexes.push_back(vert);

		// Add the edges:

		CFixedVector2D h0(squares[i].hw + pathfindClearance, squares[i].hh + pathfindClearance);
		CFixedVector2D h1(squares[i].hw + pathfindClearance, -(squares[i].hh + pathfindClearance));

		CFixedVector2D ev0(center.X - h0.Dot(u), center.Y + h0.Dot(v));
		CFixedVector2D ev1(center.X - h1.Dot(u), center.Y + h1.Dot(v));
		CFixedVector2D ev2(center.X + h0.Dot(u), center.Y - h0.Dot(v));
		CFixedVector2D ev3(center.X + h1.Dot(u), center.Y - h1.Dot(v));
		if (aa)
			m_EdgeSquares.emplace_back(Square{ ev1, ev3 });
		else
		{
			m_Edges.emplace_back(Edge{ ev0, ev1 });
			m_Edges.emplace_back(Edge{ ev1, ev2 });
			m_Edges.emplace_back(Edge{ ev2, ev3 });
			m_Edges.emplace_back(Edge{ ev3, ev0 });
		}

	}

	// Add terrain obstructions
	{
		u16 i0, j0, i1, j1;
		Pathfinding::NearestNavcell(rangeXMin, rangeZMin, i0, j0, m_MapSize*Pathfinding::NAVCELLS_PER_TILE, m_MapSize*Pathfinding::NAVCELLS_PER_TILE);
		Pathfinding::NearestNavcell(rangeXMax, rangeZMax, i1, j1, m_MapSize*Pathfinding::NAVCELLS_PER_TILE, m_MapSize*Pathfinding::NAVCELLS_PER_TILE);
		AddTerrainEdges(m_Edges, m_Vertexes, i0, j0, i1, j1, request.passClass, *m_TerrainOnlyGrid);
	}

	// Clip out vertices that are inside an edgeSquare (i.e. trivially unreachable)
	for (size_t i = 2; i < m_EdgeSquares.size(); ++i)
	{
		// If the start point is inside the square, ignore it
		if (start.p.X >= m_EdgeSquares[i].p0.X &&
		    start.p.Y >= m_EdgeSquares[i].p0.Y &&
		    start.p.X <= m_EdgeSquares[i].p1.X &&
		    start.p.Y <= m_EdgeSquares[i].p1.Y)
			continue;

		// Remove every non-start/goal vertex that is inside an edgeSquare;
		// since remove() would be inefficient, just mark it as closed instead.
		for (size_t j = 2; j < m_Vertexes.size(); ++j)
			if (m_Vertexes[j].p.X >= m_EdgeSquares[i].p0.X &&
			    m_Vertexes[j].p.Y >= m_EdgeSquares[i].p0.Y &&
			    m_Vertexes[j].p.X <= m_EdgeSquares[i].p1.X &&
			    m_Vertexes[j].p.Y <= m_EdgeSquares[i].p1.Y)
				m_Vertexes[j].status = Vertex::CLOSED;
	}

	ENSURE(m_Vertexes.size() < 65536); // We store array indexes as u16.

	DebugRenderGraph(cmpObstructionManager->GetSimContext(), m_Vertexes, m_Edges, m_EdgeSquares);

	// Do an A* search over the vertex/visibility graph:

	// Since we are just measuring Euclidean distance the heuristic is admissible,
	// so we never have to re-examine a node once it's been moved to the closed set.

	// To save time in common cases, we don't precompute a graph of valid edges between vertexes;
	// we do it lazily instead. When the search algorithm reaches a vertex, we examine every other
	// vertex and see if we can reach it without hitting any collision edges, and ignore the ones
	// we can't reach. Since the algorithm can only reach a vertex once (and then it'll be marked
	// as closed), we won't be doing any redundant visibility computations.

	VertexPriorityQueue open;
	VertexPriorityQueue::Item qiStart = { START_VERTEX_ID, start.h, start.h };
	open.push(qiStart);

	u16 idBest = START_VERTEX_ID;
	fixed hBest = start.h;

	while (!open.empty())
	{
		// Move best tile from open to closed
		VertexPriorityQueue::Item curr = open.pop();
		m_Vertexes[curr.id].status = Vertex::CLOSED;

		// If we've reached the destination, stop
		if (curr.id == GOAL_VERTEX_ID)
		{
			idBest = curr.id;
			break;
		}

		// Sort the edges by distance in order to check those first that have a high probability of blocking a ray.
		// The heuristic based on distance is very rough, especially for squares that are further away;
		// we're also only really interested in the closest squares since they are the only ones that block a lot of rays.
		// Thus we only do a partial sort; the threshold is just a somewhat reasonable value.
		if (m_EdgeSquares.size() > 8)
			std::partial_sort(m_EdgeSquares.begin(), m_EdgeSquares.begin() + 8, m_EdgeSquares.end(), SquareSort(m_Vertexes[curr.id].p));

		m_EdgesUnaligned.clear();
		m_EdgesLeft.clear();
		m_EdgesRight.clear();
		m_EdgesBottom.clear();
		m_EdgesTop.clear();
		SplitAAEdges(m_Vertexes[curr.id].p, m_Edges, m_EdgeSquares, m_EdgesUnaligned, m_EdgesLeft, m_EdgesRight, m_EdgesBottom, m_EdgesTop);

		// Check the lines to every other vertex
		for (size_t n = 0; n < m_Vertexes.size(); ++n)
		{
			if (m_Vertexes[n].status == Vertex::CLOSED)
				continue;

			// If this is the magical goal vertex, move it to near the current vertex
			CFixedVector2D npos;
			if (n == GOAL_VERTEX_ID)
			{
				npos = request.goal.NearestPointOnGoal(m_Vertexes[curr.id].p);

				// To prevent integer overflows later on, we need to ensure all vertexes are
				// 'close' to the source. The goal might be far away (not a good idea but
				// sometimes it happens), so clamp it to the current search range
				npos.X = Clamp(npos.X, rangeXMin + EDGE_EXPAND_DELTA, rangeXMax - EDGE_EXPAND_DELTA);
				npos.Y = Clamp(npos.Y, rangeZMin + EDGE_EXPAND_DELTA, rangeZMax - EDGE_EXPAND_DELTA);
			}
			else
				npos = m_Vertexes[n].p;

			// Work out which quadrant(s) we're approaching the new vertex from
			u8 quad = 0;
			if (m_Vertexes[curr.id].p.X <= npos.X && m_Vertexes[curr.id].p.Y <= npos.Y) quad |= QUADRANT_BL;
			if (m_Vertexes[curr.id].p.X >= npos.X && m_Vertexes[curr.id].p.Y >= npos.Y) quad |= QUADRANT_TR;
			if (m_Vertexes[curr.id].p.X <= npos.X && m_Vertexes[curr.id].p.Y >= npos.Y) quad |= QUADRANT_TL;
			if (m_Vertexes[curr.id].p.X >= npos.X && m_Vertexes[curr.id].p.Y <= npos.Y) quad |= QUADRANT_BR;

			// Check that the new vertex is in the right quadrant for the old vertex
			if (!(m_Vertexes[curr.id].quadOutward & quad) && curr.id != START_VERTEX_ID)
			{
				// Hack: Always head towards the goal if possible, to avoid missing it if it's
				// inside another unit
				if (n != GOAL_VERTEX_ID)
					continue;
			}

			bool visible =
				CheckVisibilityLeft(m_Vertexes[curr.id].p, npos, m_EdgesLeft) &&
				CheckVisibilityRight(m_Vertexes[curr.id].p, npos, m_EdgesRight) &&
				CheckVisibilityBottom(m_Vertexes[curr.id].p, npos, m_EdgesBottom) &&
				CheckVisibilityTop(m_Vertexes[curr.id].p, npos, m_EdgesTop) &&
				CheckVisibility(m_Vertexes[curr.id].p, npos, m_EdgesUnaligned);

			// Render the edges that we examine.
			DebugRenderEdges(cmpObstructionManager->GetSimContext(), visible, m_Vertexes[curr.id].p, npos);

			if (visible)
			{
				fixed g = m_Vertexes[curr.id].g + (m_Vertexes[curr.id].p - npos).Length();

				// If this is a new tile, compute the heuristic distance
				if (m_Vertexes[n].status == Vertex::UNEXPLORED)
				{
					// Add it to the open list:
					m_Vertexes[n].status = Vertex::OPEN;
					m_Vertexes[n].g = g;
					m_Vertexes[n].h = request.goal.DistanceToPoint(npos);
					m_Vertexes[n].pred = curr.id;

					if (n == GOAL_VERTEX_ID)
						m_Vertexes[n].p = npos; // remember the new best goal position

					VertexPriorityQueue::Item t = { (u16)n, g + m_Vertexes[n].h, m_Vertexes[n].h };
					open.push(t);

					// Remember the heuristically best vertex we've seen so far, in case we never actually reach the target
					if (m_Vertexes[n].h < hBest)
					{
						idBest = (u16)n;
						hBest = m_Vertexes[n].h;
					}
				}
				else // must be OPEN
				{
					// If we've already seen this tile, and the new path to this tile does not have a
					// better cost, then stop now
					if (g >= m_Vertexes[n].g)
						continue;

					// Otherwise, we have a better path, so replace the old one with the new cost/parent
					fixed gprev = m_Vertexes[n].g;
					m_Vertexes[n].g = g;
					m_Vertexes[n].pred = curr.id;

					if (n == GOAL_VERTEX_ID)
						m_Vertexes[n].p = npos; // remember the new best goal position

					open.promote((u16)n, gprev + m_Vertexes[n].h, g + m_Vertexes[n].h, m_Vertexes[n].h);
				}
			}
		}
	}

	// Reconstruct the path (in reverse)
	WaypointPath path;
	for (u16 id = idBest; id != START_VERTEX_ID; id = m_Vertexes[id].pred)
		path.m_Waypoints.emplace_back(Waypoint{ m_Vertexes[id].p.X, m_Vertexes[id].p.Y });


	m_Edges.clear();
	m_EdgeSquares.clear();
	m_Vertexes.clear();

	m_EdgesUnaligned.clear();
	m_EdgesLeft.clear();
	m_EdgesRight.clear();
	m_EdgesBottom.clear();
	m_EdgesTop.clear();

	return path;
}

void VertexPathfinder::DebugRenderGoal(const CSimContext& simContext, const PathGoal& goal) const
{
	if (!m_DebugOverlay)
		return;

	m_DebugOverlayShortPathLines.clear();

	// Render the goal shape
	m_DebugOverlayShortPathLines.push_back(SOverlayLine());
	m_DebugOverlayShortPathLines.back().m_Color = CColor(1, 0, 0, 1);
	switch (goal.type)
	{
		case PathGoal::POINT:
		{
			SimRender::ConstructCircleOnGround(simContext, goal.x.ToFloat(), goal.z.ToFloat(), 0.2f, m_DebugOverlayShortPathLines.back(), true);
			break;
		}
		case PathGoal::CIRCLE:
		case PathGoal::INVERTED_CIRCLE:
		{
			SimRender::ConstructCircleOnGround(simContext, goal.x.ToFloat(), goal.z.ToFloat(), goal.hw.ToFloat(), m_DebugOverlayShortPathLines.back(), true);
			break;
		}
		case PathGoal::SQUARE:
		case PathGoal::INVERTED_SQUARE:
		{
			float a = atan2f(goal.v.X.ToFloat(), goal.v.Y.ToFloat());
			SimRender::ConstructSquareOnGround(simContext, goal.x.ToFloat(), goal.z.ToFloat(), goal.hw.ToFloat()*2, goal.hh.ToFloat()*2, a, m_DebugOverlayShortPathLines.back(), true);
			break;
		}
	}
}

void VertexPathfinder::DebugRenderGraph(const CSimContext& simContext, const std::vector<Vertex>& vertexes, const std::vector<Edge>& edges, const std::vector<Square>& edgeSquares) const
{
	if (!m_DebugOverlay)
		return;

#define PUSH_POINT(p) STMT(xz.push_back(p.X.ToFloat()); xz.push_back(p.Y.ToFloat()))
	// Render the vertexes as little Pac-Man shapes to indicate quadrant direction
	for (size_t i = 0; i < vertexes.size(); ++i)
	{
		m_DebugOverlayShortPathLines.emplace_back();
		m_DebugOverlayShortPathLines.back().m_Color = CColor(1, 1, 0, 1);

		float x = vertexes[i].p.X.ToFloat();
		float z = vertexes[i].p.Y.ToFloat();

		float a0 = 0, a1 = 0;
		// Get arc start/end angles depending on quadrant (if any)
		if      (vertexes[i].quadInward == QUADRANT_BL) { a0 = -0.25f; a1 = 0.50f; }
		else if (vertexes[i].quadInward == QUADRANT_TR) { a0 =  0.25f; a1 = 1.00f; }
		else if (vertexes[i].quadInward == QUADRANT_TL) { a0 = -0.50f; a1 = 0.25f; }
		else if (vertexes[i].quadInward == QUADRANT_BR) { a0 =  0.00f; a1 = 0.75f; }

		if (a0 == a1)
			SimRender::ConstructCircleOnGround(simContext, x, z, 0.5f,
											   m_DebugOverlayShortPathLines.back(), true);
		else
			SimRender::ConstructClosedArcOnGround(simContext, x, z, 0.5f,
												  a0 * ((float)M_PI*2.0f), a1 * ((float)M_PI*2.0f),
												  m_DebugOverlayShortPathLines.back(), true);
	}

	// Render the edges
	for (size_t i = 0; i < edges.size(); ++i)
	{
		m_DebugOverlayShortPathLines.emplace_back();
		m_DebugOverlayShortPathLines.back().m_Color = CColor(0, 1, 1, 1);
		std::vector<float> xz;
		PUSH_POINT(edges[i].p0);
		PUSH_POINT(edges[i].p1);

		// Add an arrowhead to indicate the direction
		CFixedVector2D d = edges[i].p1 - edges[i].p0;
		d.Normalize(fixed::FromInt(1)/8);
		CFixedVector2D p2 = edges[i].p1 - d*2;
		CFixedVector2D p3 = p2 + d.Perpendicular();
		CFixedVector2D p4 = p2 - d.Perpendicular();
		PUSH_POINT(p3);
		PUSH_POINT(p4);
		PUSH_POINT(edges[i].p1);

		SimRender::ConstructLineOnGround(simContext, xz, m_DebugOverlayShortPathLines.back(), true);
	}
#undef PUSH_POINT

	// Render the axis-aligned squares
	for (size_t i = 0; i < edgeSquares.size(); ++i)
	{
		m_DebugOverlayShortPathLines.push_back(SOverlayLine());
		m_DebugOverlayShortPathLines.back().m_Color = CColor(0, 1, 1, 1);
		std::vector<float> xz;
		Square s = edgeSquares[i];
		xz.push_back(s.p0.X.ToFloat());
		xz.push_back(s.p0.Y.ToFloat());
		xz.push_back(s.p0.X.ToFloat());
		xz.push_back(s.p1.Y.ToFloat());
		xz.push_back(s.p1.X.ToFloat());
		xz.push_back(s.p1.Y.ToFloat());
		xz.push_back(s.p1.X.ToFloat());
		xz.push_back(s.p0.Y.ToFloat());
		xz.push_back(s.p0.X.ToFloat());
		xz.push_back(s.p0.Y.ToFloat());
		SimRender::ConstructLineOnGround(simContext, xz, m_DebugOverlayShortPathLines.back(), true);
	}
}

void VertexPathfinder::DebugRenderEdges(const CSimContext& UNUSED(simContext), bool UNUSED(visible), CFixedVector2D UNUSED(curr), CFixedVector2D UNUSED(npos)) const
{
	if (!m_DebugOverlay)
		return;

	// Disabled by default.
	/*
	m_DebugOverlayShortPathLines.push_back(SOverlayLine());
	m_DebugOverlayShortPathLines.back().m_Color = visible ? CColor(0, 1, 0, 0.5) : CColor(1, 0, 0, 0.5);
	m_DebugOverlayShortPathLines.push_back(SOverlayLine());
	m_DebugOverlayShortPathLines.back().m_Color = visible ? CColor(0, 1, 0, 0.5) : CColor(1, 0, 0, 0.5);
	std::vector<float> xz;
	xz.push_back(curr.X.ToFloat());
	xz.push_back(curr.Y.ToFloat());
	xz.push_back(npos.X.ToFloat());
	xz.push_back(npos.Y.ToFloat());
	SimRender::ConstructLineOnGround(simContext, xz, m_DebugOverlayShortPathLines.back(), false);
	SimRender::ConstructLineOnGround(simContext, xz, m_DebugOverlayShortPathLines.back(), false);
	*/
}

void VertexPathfinder::RenderSubmit(SceneCollector& collector)
{
	if (!m_DebugOverlay)
		return;

	for (size_t i = 0; i < m_DebugOverlayShortPathLines.size(); ++i)
		collector.Submit(&m_DebugOverlayShortPathLines[i]);
}
