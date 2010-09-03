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
 */

#include "precompiled.h"

#include "CCmpPathfinder_Common.h"

#include "ps/Profile.h"
#include "simulation2/components/ICmpObstructionManager.h"
#include "simulation2/helpers/PriorityQueue.h"
#include "simulation2/helpers/Render.h"

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
};

struct Edge
{
	CFixedVector2D p0, p1;
};

// When computing vertexes to insert into the search graph,
// add a small delta so that the vertexes of an edge don't get interpreted
// as crossing the edge (given minor numerical inaccuracies)
static const entity_pos_t EDGE_EXPAND_DELTA = entity_pos_t::FromInt(1)/4;

/**
 * Check whether a ray from 'a' to 'b' crosses any of the edges.
 * (Edges are one-sided so it's only considered a cross if going from front to back.)
 */
static bool CheckVisibility(CFixedVector2D a, CFixedVector2D b, const std::vector<Edge>& edges)
{
	CFixedVector2D abn = (b - a).Perpendicular();

	for (size_t i = 0; i < edges.size(); ++i)
	{
		CFixedVector2D d = (edges[i].p1 - edges[i].p0).Perpendicular();

		// If 'a' is behind the edge, we can't cross
		fixed q = (a - edges[i].p0).Dot(d);
		if (q < fixed::Zero())
			continue;

		// If 'b' is in front of the edge, we can't cross
		fixed r = (b - edges[i].p0).Dot(d);
		if (r > fixed::Zero())
			continue;

		// The ray is crossing the infinitely-extended edge from in front to behind.
		// Check the finite edge is crossing the infinitely-extended ray too.
		// (Given the previous tests, it can only be crossing in one direction.)
		fixed s = (edges[i].p0 - a).Dot(abn);
		fixed t = (edges[i].p1 - a).Dot(abn);
		if (s <= fixed::Zero() && t >= fixed::Zero())
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

typedef PriorityQueueList<u16, fixed> PriorityQueue;

struct TileEdge
{
	u16 i, j;
	enum { TOP, BOTTOM, LEFT, RIGHT } dir;
};

static void AddTerrainEdges(std::vector<Edge>& edges, std::vector<Vertex>& vertexes, u16 i0, u16 j0, u16 i1, u16 j1, fixed r, u8 passClass, const Grid<TerrainTile>& terrain)
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
				if (j > 0 && IS_TERRAIN_PASSABLE(terrain.get(i, j-1), passClass))
				{
					TileEdge e = { i, j, TileEdge::BOTTOM };
					tileEdges.push_back(e);
				}

				if (j < terrain.m_H-1 && IS_TERRAIN_PASSABLE(terrain.get(i, j+1), passClass))
				{
					TileEdge e = { i, j, TileEdge::TOP };
					tileEdges.push_back(e);
				}

				if (i > 0 && IS_TERRAIN_PASSABLE(terrain.get(i-1, j), passClass))
				{
					TileEdge e = { i, j, TileEdge::LEFT };
					tileEdges.push_back(e);
				}

				if (i < terrain.m_W-1 && IS_TERRAIN_PASSABLE(terrain.get(i+1, j), passClass))
				{
					TileEdge e = { i, j, TileEdge::RIGHT };
					tileEdges.push_back(e);
				}
			}
		}
	}

	// TODO: maybe we should precompute these terrain edges since they'll rarely change?

	// TODO: for efficiency (minimising the A* search space), we should coalesce adjoining edges

	// Add all the tile edges to the search edge/vertex lists
	for (size_t n = 0; n < tileEdges.size(); ++n)
	{
		u16 i = tileEdges[n].i;
		u16 j = tileEdges[n].j;
		CFixedVector2D v0, v1;
		Vertex vert;
		vert.status = Vertex::UNEXPLORED;

		switch (tileEdges[n].dir)
		{
		case TileEdge::BOTTOM:
		{
			v0 = CFixedVector2D(fixed::FromInt(i * CELL_SIZE) - r, fixed::FromInt(j * CELL_SIZE) - r);
			v1 = CFixedVector2D(fixed::FromInt((i+1) * CELL_SIZE) + r, fixed::FromInt(j * CELL_SIZE) - r);
			Edge e = { v0, v1 };
			edges.push_back(e);
			vert.p.X = v0.X - EDGE_EXPAND_DELTA; vert.p.Y = v0.Y - EDGE_EXPAND_DELTA; vertexes.push_back(vert);
			vert.p.X = v1.X + EDGE_EXPAND_DELTA; vert.p.Y = v1.Y - EDGE_EXPAND_DELTA; vertexes.push_back(vert);
			break;
		}
		case TileEdge::TOP:
		{
			v0 = CFixedVector2D(fixed::FromInt((i+1) * CELL_SIZE) + r, fixed::FromInt((j+1) * CELL_SIZE) + r);
			v1 = CFixedVector2D(fixed::FromInt(i * CELL_SIZE) - r, fixed::FromInt((j+1) * CELL_SIZE) + r);
			Edge e = { v0, v1 };
			edges.push_back(e);
			vert.p.X = v0.X + EDGE_EXPAND_DELTA; vert.p.Y = v0.Y + EDGE_EXPAND_DELTA; vertexes.push_back(vert);
			vert.p.X = v1.X - EDGE_EXPAND_DELTA; vert.p.Y = v1.Y + EDGE_EXPAND_DELTA; vertexes.push_back(vert);
			break;
		}
		case TileEdge::LEFT:
		{
			v0 = CFixedVector2D(fixed::FromInt(i * CELL_SIZE) - r, fixed::FromInt((j+1) * CELL_SIZE) + r);
			v1 = CFixedVector2D(fixed::FromInt(i * CELL_SIZE) - r, fixed::FromInt(j * CELL_SIZE) - r);
			Edge e = { v0, v1 };
			edges.push_back(e);
			vert.p.X = v0.X - EDGE_EXPAND_DELTA; vert.p.Y = v0.Y + EDGE_EXPAND_DELTA; vertexes.push_back(vert);
			vert.p.X = v1.X - EDGE_EXPAND_DELTA; vert.p.Y = v1.Y - EDGE_EXPAND_DELTA; vertexes.push_back(vert);
			break;
		}
		case TileEdge::RIGHT:
		{
			v0 = CFixedVector2D(fixed::FromInt((i+1) * CELL_SIZE) + r, fixed::FromInt(j * CELL_SIZE) - r);
			v1 = CFixedVector2D(fixed::FromInt((i+1) * CELL_SIZE) + r, fixed::FromInt((j+1) * CELL_SIZE) + r);
			Edge e = { v0, v1 };
			edges.push_back(e);
			vert.p.X = v0.X + EDGE_EXPAND_DELTA; vert.p.Y = v0.Y - EDGE_EXPAND_DELTA; vertexes.push_back(vert);
			vert.p.X = v1.X + EDGE_EXPAND_DELTA; vert.p.Y = v1.Y + EDGE_EXPAND_DELTA; vertexes.push_back(vert);
			break;
		}
		}
	}
}

void CCmpPathfinder::ComputeShortPath(const IObstructionTestFilter& filter, entity_pos_t x0, entity_pos_t z0, entity_pos_t r, entity_pos_t range, const Goal& goal, u8 passClass, Path& path)
{
	UpdateGrid(); // TODO: only need to bother updating if the terrain changed

	PROFILE("ComputeShortPath");

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
			float a = atan2(goal.v.X.ToFloat(), goal.v.Y.ToFloat());
			SimRender::ConstructSquareOnGround(GetSimContext(), goal.x.ToFloat(), goal.z.ToFloat(), goal.hw.ToFloat()*2, goal.hh.ToFloat()*2, a, m_DebugOverlayShortPathLines.back(), true);
			break;
		}
		}
	}

	// List of collision edges - paths must never cross these.
	// (Edges are one-sided so intersections are fine in one direction, but not the other direction.)
	std::vector<Edge> edges;

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

	CFixedVector2D goalVec(goal.x, goal.z);

	// List of obstruction vertexes (plus start/end points); we'll try to find paths through
	// the graph defined by these vertexes
	std::vector<Vertex> vertexes;

	// Add the start point to the graph
	Vertex start = { CFixedVector2D(x0, z0), fixed::Zero(), (CFixedVector2D(x0, z0) - goalVec).Length(), 0, Vertex::OPEN };
	vertexes.push_back(start);
	const size_t START_VERTEX_ID = 0;

	// Add the goal vertex to the graph.
	// Since the goal isn't always a point, this a special magic virtual vertex which moves around - whenever
	// we look at it from another vertex, it is moved to be the closest point on the goal shape to that vertex.
	Vertex end = { CFixedVector2D(goal.x, goal.z), fixed::Zero(), fixed::Zero(), 0, Vertex::UNEXPLORED };
	vertexes.push_back(end);
	const size_t GOAL_VERTEX_ID = 1;

	// Add terrain obstructions
	{
		u16 i0, j0, i1, j1;
		NearestTile(rangeXMin, rangeZMin, i0, j0);
		NearestTile(rangeXMax, rangeZMax, i1, j1);
		AddTerrainEdges(edges, vertexes, i0, j0, i1, j1, r, passClass, *m_Grid);
	}

	// Find all the obstruction squares that might affect us
	CmpPtr<ICmpObstructionManager> cmpObstructionManager(GetSimContext(), SYSTEM_ENTITY);
	std::vector<ICmpObstructionManager::ObstructionSquare> squares;
	cmpObstructionManager->GetObstructionsInRange(filter, rangeXMin - r, rangeZMin - r, rangeXMax + r, rangeZMax + r, squares);

	// Resize arrays to reduce reallocations
	vertexes.reserve(vertexes.size() + squares.size()*4);
	edges.reserve(edges.size() + squares.size()*4);

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

		Vertex vert;
		vert.status = Vertex::UNEXPLORED;
		vert.p.X = center.X - hd0.Dot(u); vert.p.Y = center.Y + hd0.Dot(v); vertexes.push_back(vert);
		vert.p.X = center.X - hd1.Dot(u); vert.p.Y = center.Y + hd1.Dot(v); vertexes.push_back(vert);
		vert.p.X = center.X + hd0.Dot(u); vert.p.Y = center.Y - hd0.Dot(v); vertexes.push_back(vert);
		vert.p.X = center.X + hd1.Dot(u); vert.p.Y = center.Y - hd1.Dot(v); vertexes.push_back(vert);

		// Add the four edges

		CFixedVector2D h0(squares[i].hw + r, squares[i].hh + r);
		CFixedVector2D h1(squares[i].hw + r, -(squares[i].hh + r));

		CFixedVector2D ev0(center.X - h0.Dot(u), center.Y + h0.Dot(v));
		CFixedVector2D ev1(center.X - h1.Dot(u), center.Y + h1.Dot(v));
		CFixedVector2D ev2(center.X + h0.Dot(u), center.Y - h0.Dot(v));
		CFixedVector2D ev3(center.X + h1.Dot(u), center.Y - h1.Dot(v));
		Edge e0 = { ev0, ev1 };
		Edge e1 = { ev1, ev2 };
		Edge e2 = { ev2, ev3 };
		Edge e3 = { ev3, ev0 };
		edges.push_back(e0);
		edges.push_back(e1);
		edges.push_back(e2);
		edges.push_back(e3);

		// TODO: should clip out vertexes and edges that are outside the range,
		// to reduce the search space
	}

	debug_assert(vertexes.size() < 65536); // we store array indexes as u16

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

		// Check the lines to every other vertex
		for (size_t n = 0; n < vertexes.size(); ++n)
		{
			if (vertexes[n].status == Vertex::CLOSED)
				continue;

			// If this is the magical goal vertex, move it to near the current vertex
			CFixedVector2D npos;
			if (n == GOAL_VERTEX_ID)
				npos = NearestPointOnGoal(vertexes[curr.id].p, goal);
			else
				npos = vertexes[n].p;

			bool visible = CheckVisibility(vertexes[curr.id].p, npos, edges);

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
					if (n == GOAL_VERTEX_ID)
						vertexes[n].p = npos; // remember the new best goal position
					PriorityQueue::Item t = { (u16)n, g + vertexes[n].h };
					open.push(t);

					// Remember the heuristically best vertex we've seen so far, in case we never actually reach the target
					if (vertexes[n].h < hBest)
					{
						idBest = n;
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
					if (n == GOAL_VERTEX_ID)
						vertexes[n].p = npos; // remember the new best goal position
					open.promote((u16)n, g + vertexes[n].h);
					continue;
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

