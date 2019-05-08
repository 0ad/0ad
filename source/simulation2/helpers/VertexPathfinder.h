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

#ifndef INCLUDED_VERTEXPATHFINDER
#define INCLUDED_VERTEXPATHFINDER

#include "graphics/Overlay.h"
#include "simulation2/helpers/Pathfinding.h"
#include "simulation2/system/CmpPtr.h"

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
// Defines the two points of the edge.
struct Edge
{
	CFixedVector2D p0, p1;
};

// Axis-aligned obstruction squares (paths will not cross any of these).
// Defines the opposing corners of an axis-aligned square
// (from which four individual edges can be trivially computed), requiring p0 <= p1
struct Square
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

class ICmpObstructionManager;
class CSimContext;
class SceneCollector;

class VertexPathfinder
{
public:
	VertexPathfinder(const u16& mapSize, Grid<NavcellData>* const & terrainOnlyGrid) : m_MapSize(mapSize), m_TerrainOnlyGrid(terrainOnlyGrid), m_DebugOverlay(false) {};

	/**
	 * Compute a precise path from the given point to the goal, and return the set of waypoints.
	 * The path is based on the full set of obstructions that pass the filter, such that
	 * a unit of clearance 'clearance' will be able to follow the path with no collisions.
	 * The path is restricted to a box of radius 'range' from the starting point.
	 * Defined in CCmpPathfinder_Vertex.cpp
	 */
	WaypointPath ComputeShortPath(const AsyncShortPathRequest& request, CmpPtr<ICmpObstructionManager> cmpObstructionManager) const;

	void SetDebugOverlay(bool enabled) { m_DebugOverlay = enabled; }
	void RenderSubmit(SceneCollector& collector);

private:

	void DebugRenderGoal(const CSimContext& simContext, const PathGoal& goal) const;
	void DebugRenderGraph(const CSimContext& simContext, const std::vector<Vertex>& vertexes, const std::vector<Edge>& edges, const std::vector<Square>& edgeSquares) const;
	void DebugRenderEdges(const CSimContext& simContext, bool visible, CFixedVector2D curr, CFixedVector2D npos) const;

	// References to the Pathfinder for convenience.
	const u16& m_MapSize;
	Grid<NavcellData>* const & m_TerrainOnlyGrid;

	std::atomic<bool> m_DebugOverlay;
	mutable std::vector<SOverlayLine> m_DebugOverlayShortPathLines;

	// These vectors are expensive to recreate on every call, so we cache them here.
	// They are made mutable to allow using them in the otherwise const ComputeShortPath.

	mutable std::vector<Edge> m_EdgesUnaligned;
	mutable std::vector<EdgeAA> m_EdgesLeft;
	mutable std::vector<EdgeAA> m_EdgesRight;
	mutable std::vector<EdgeAA> m_EdgesBottom;
	mutable std::vector<EdgeAA> m_EdgesTop;

	// List of obstruction vertexes (plus start/end points); we'll try to find paths through
	// the graph defined by these vertexes.
	mutable std::vector<Vertex> m_Vertexes;
	// List of collision edges - paths must never cross these.
	// (Edges are one-sided so intersections are fine in one direction, but not the other direction.)
	mutable std::vector<Edge> m_Edges;
	mutable std::vector<Square> m_EdgeSquares; // Axis-aligned squares; equivalent to 4 edges.
};

#endif // INCLUDED_VERTEXPATHFINDER
