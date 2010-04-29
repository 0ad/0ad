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

#include "precompiled.h"

#include "simulation2/system/Component.h"
#include "ICmpObstructionManager.h"

#include "simulation2/MessageTypes.h"
#include "simulation2/helpers/Geometry.h"
#include "simulation2/helpers/Render.h"

#include "graphics/Overlay.h"
#include "graphics/Terrain.h"
#include "maths/MathUtil.h"
#include "ps/Overlay.h"
#include "ps/Profile.h"
#include "renderer/Scene.h"

// Externally, tags are opaque non-zero positive integers.
// Internally, they are tagged (by shape) indexes into shape lists.
// idx must be non-zero.
#define TAG_IS_UNIT(tag) (((tag) & 1) == 0)
#define TAG_IS_STATIC(tag) (((tag) & 1) == 1)
#define UNIT_INDEX_TO_TAG(idx) (((idx) << 1) | 0)
#define STATIC_INDEX_TO_TAG(idx) (((idx) << 1) | 1)
#define TAG_TO_INDEX(tag) ((tag) >> 1)

/**
 * Internal representation of axis-aligned sometimes-square sometimes-circle shapes for moving units
 */
struct UnitShape
{
	entity_pos_t x, z;
	entity_pos_t r; // radius of circle, or half width of square
	bool moving; // whether it's currently mobile (and should be generally ignored when pathing)
};

/**
 * Internal representation of arbitrary-rotation static square shapes for buildings
 */
struct StaticShape
{
	entity_pos_t x, z; // world-space coordinates
	CFixedVector2D u, v; // orthogonal unit vectors - axes of local coordinate space
	entity_pos_t hw, hh; // half width/height in local coordinate space
};

class CCmpObstructionManager : public ICmpObstructionManager
{
public:
	static void ClassInit(CComponentManager& componentManager)
	{
		componentManager.SubscribeToMessageType(MT_RenderSubmit); // for debug overlays
	}

	DEFAULT_COMPONENT_ALLOCATOR(ObstructionManager)

	bool m_DebugOverlayEnabled;
	bool m_DebugOverlayDirty;
	std::vector<SOverlayLine> m_DebugOverlayLines;

	// TODO: using std::map is a bit inefficient; is there a better way to store these?
	std::map<u32, UnitShape> m_UnitShapes;
	std::map<u32, StaticShape> m_StaticShapes;
	u32 m_UnitShapeNext; // next allocated id
	u32 m_StaticShapeNext;

	static std::string GetSchema()
	{
		return "<a:component type='system'/><empty/>";
	}

	virtual void Init(const CSimContext& context, const CParamNode& UNUSED(paramNode))
	{
		m_DebugOverlayEnabled = false;
		m_DebugOverlayDirty = true;

		m_UnitShapeNext = 1;
		m_StaticShapeNext = 1;

		m_DirtyID = 1; // init to 1 so default-initialised grids are considered dirty
	}

	virtual void Deinit(const CSimContext& UNUSED(context))
	{
	}

	virtual void Serialize(ISerializer& serialize)
	{
		// TODO: do something here
		// (Do we need to serialise the obstruction state, or is it fine to regenerate it from
		// the original entities after deserialisation?)
	}

	virtual void Deserialize(const CSimContext& context, const CParamNode& paramNode, IDeserializer& deserialize)
	{
		Init(context, paramNode);

		// TODO
	}

	virtual void HandleMessage(const CSimContext& context, const CMessage& msg, bool UNUSED(global))
	{
		switch (msg.GetType())
		{
		case MT_RenderSubmit:
		{
			const CMessageRenderSubmit& msgData = static_cast<const CMessageRenderSubmit&> (msg);
			RenderSubmit(context, msgData.collector);
			break;
		}
		}
	}

	virtual tag_t AddUnitShape(entity_pos_t x, entity_pos_t z, entity_pos_t r, bool moving)
	{
		UnitShape shape = { x, z, r, moving };
		size_t id = m_UnitShapeNext++;
		m_UnitShapes[id] = shape;
		MakeDirty();
		return UNIT_INDEX_TO_TAG(id);
	}

	virtual tag_t AddStaticShape(entity_pos_t x, entity_pos_t z, entity_angle_t a, entity_pos_t w, entity_pos_t h)
	{
		CFixed_23_8 s, c;
		sincos_approx(a, s, c);
		CFixedVector2D u(c, -s);
		CFixedVector2D v(s, c);

		StaticShape shape = { x, z, u, v, w/2, h/2 };
		size_t id = m_StaticShapeNext++;
		m_StaticShapes[id] = shape;
		MakeDirty();
		return STATIC_INDEX_TO_TAG(id);
	}

	virtual void MoveShape(tag_t tag, entity_pos_t x, entity_pos_t z, entity_angle_t a)
	{
		debug_assert(tag);

		if (TAG_IS_UNIT(tag))
		{
			UnitShape& shape = m_UnitShapes[TAG_TO_INDEX(tag)];
			shape.x = x;
			shape.z = z;
		}
		else
		{
			CFixed_23_8 s, c;
			sincos_approx(a, s, c);
			CFixedVector2D u(c, -s);
			CFixedVector2D v(s, c);

			StaticShape& shape = m_StaticShapes[TAG_TO_INDEX(tag)];
			shape.x = x;
			shape.z = z;
			shape.u = u;
			shape.v = v;
		}

		MakeDirty();
	}

	virtual void SetUnitMovingFlag(tag_t tag, bool moving)
	{
		debug_assert(tag && TAG_IS_UNIT(tag));

		if (TAG_IS_UNIT(tag))
		{
			UnitShape& shape = m_UnitShapes[TAG_TO_INDEX(tag)];
			shape.moving = moving;
		}
	}

	virtual void RemoveShape(tag_t tag)
	{
		debug_assert(tag);

		if (TAG_IS_UNIT(tag))
			m_UnitShapes.erase(TAG_TO_INDEX(tag));
		else
			m_StaticShapes.erase(TAG_TO_INDEX(tag));

		MakeDirty();
	}

	virtual ObstructionSquare GetObstruction(tag_t tag)
	{
		debug_assert(tag);

		if (TAG_IS_UNIT(tag))
		{
			UnitShape& shape = m_UnitShapes[TAG_TO_INDEX(tag)];
			CFixedVector2D u(entity_pos_t::FromInt(1), entity_pos_t::Zero());
			CFixedVector2D v(entity_pos_t::Zero(), entity_pos_t::FromInt(1));
			ObstructionSquare o = { shape.x, shape.z, u, v, shape.r, shape.r };
			return o;
		}
		else
		{
			StaticShape& shape = m_StaticShapes[TAG_TO_INDEX(tag)];
			ObstructionSquare o = { shape.x, shape.z, shape.u, shape.v, shape.hw, shape.hh };
			return o;
		}
	}

	virtual bool TestLine(const IObstructionTestFilter& filter, entity_pos_t x0, entity_pos_t z0, entity_pos_t x1, entity_pos_t z1, entity_pos_t r);
	virtual bool TestStaticShape(const IObstructionTestFilter& filter, entity_pos_t x, entity_pos_t z, entity_pos_t a, entity_pos_t w, entity_pos_t h);
	virtual bool TestUnitShape(const IObstructionTestFilter& filter, entity_pos_t x, entity_pos_t z, entity_pos_t r);

	virtual bool Rasterise(Grid<u8>& grid);
	virtual void GetObstructionsInRange(const IObstructionTestFilter& filter, entity_pos_t x0, entity_pos_t z0, entity_pos_t x1, entity_pos_t z1, std::vector<ObstructionSquare>& squares);
	virtual bool FindMostImportantObstruction(entity_pos_t x, entity_pos_t z, entity_pos_t r, ObstructionSquare& square);

	virtual void SetDebugOverlay(bool enabled)
	{
		m_DebugOverlayEnabled = enabled;
		m_DebugOverlayDirty = true;
		if (!enabled)
			m_DebugOverlayLines.clear();
	}

	void RenderSubmit(const CSimContext& context, SceneCollector& collector);

private:
	// To support lazy updates of grid rasterisations of obstruction data,
	// we maintain a DirtyID here and increment it whenever obstructions change;
	// if a grid has a lower DirtyID then it needs to be updated.

	size_t m_DirtyID;

	/**
	 * Mark all previous Rasterise()d grids as dirty
	 */
	void MakeDirty()
	{
		++m_DirtyID;
		m_DebugOverlayDirty = true;
	}

	/**
	 * Test whether a Rasterise()d grid is dirty and needs updating
	 */
	template<typename T>
	bool IsDirty(const Grid<T>& grid)
	{
		return grid.m_DirtyID < m_DirtyID;
	}
};

REGISTER_COMPONENT_TYPE(ObstructionManager)

bool CCmpObstructionManager::TestLine(const IObstructionTestFilter& filter, entity_pos_t x0, entity_pos_t z0, entity_pos_t x1, entity_pos_t z1, entity_pos_t r)
{
	PROFILE("TestLine");

	// TODO: this is all very inefficient, it should use some kind of spatial data structures

	for (std::map<u32, UnitShape>::iterator it = m_UnitShapes.begin(); it != m_UnitShapes.end(); ++it)
	{
		if (!filter.Allowed(UNIT_INDEX_TO_TAG(it->first), it->second.moving))
			continue;

		CFixedVector2D center(it->second.x, it->second.z);
		CFixedVector2D halfSize(it->second.r + r, it->second.r + r);
		CFixedVector2D u(entity_pos_t::FromInt(1), entity_pos_t::Zero());
		CFixedVector2D v(entity_pos_t::Zero(), entity_pos_t::FromInt(1));
		if (Geometry::TestRaySquare(CFixedVector2D(x0, z0) - center, CFixedVector2D(x1, z1) - center, u, v, halfSize))
			return true;
		// If this is slow we could use a specialised TestRayAlignedSquare for axis-aligned squares
	}

	for (std::map<u32, StaticShape>::iterator it = m_StaticShapes.begin(); it != m_StaticShapes.end(); ++it)
	{
		if (!filter.Allowed(STATIC_INDEX_TO_TAG(it->first), false))
			continue;

		CFixedVector2D center(it->second.x, it->second.z);
		CFixedVector2D halfSize(it->second.hw + r, it->second.hh + r);
		if (Geometry::TestRaySquare(CFixedVector2D(x0, z0) - center, CFixedVector2D(x1, z1) - center, it->second.u, it->second.v, halfSize))
			return true;
	}

	return false;
}

bool CCmpObstructionManager::TestStaticShape(const IObstructionTestFilter& filter, entity_pos_t x, entity_pos_t z, entity_pos_t a, entity_pos_t w, entity_pos_t h)
{
	PROFILE("TestStaticShape");

	CFixed_23_8 s, c;
	sincos_approx(a, s, c);
	CFixedVector2D u(c, -s);
	CFixedVector2D v(s, c);
	CFixedVector2D center(x, z);
	CFixedVector2D halfSize(w/2, h/2);

	for (std::map<u32, UnitShape>::iterator it = m_UnitShapes.begin(); it != m_UnitShapes.end(); ++it)
	{
		if (!filter.Allowed(UNIT_INDEX_TO_TAG(it->first), it->second.moving))
			continue;

		CFixedVector2D center1(it->second.x, it->second.z);

		if (Geometry::PointIsInSquare(center1 - center, u, v, CFixedVector2D(halfSize.X + it->second.r, halfSize.Y + it->second.r)))
			return true;
	}

	for (std::map<u32, StaticShape>::iterator it = m_StaticShapes.begin(); it != m_StaticShapes.end(); ++it)
	{
		if (!filter.Allowed(STATIC_INDEX_TO_TAG(it->first), false))
			continue;

		CFixedVector2D center1(it->second.x, it->second.z);
		CFixedVector2D halfSize1(it->second.hw, it->second.hh);
		if (Geometry::TestSquareSquare(center, u, v, halfSize, center1, it->second.u, it->second.v, halfSize1))
			return true;
	}

	return false;
}

bool CCmpObstructionManager::TestUnitShape(const IObstructionTestFilter& filter, entity_pos_t x, entity_pos_t z, entity_pos_t r)
{
	PROFILE("TestUnitShape");

	CFixedVector2D center(x, z);

	for (std::map<u32, UnitShape>::iterator it = m_UnitShapes.begin(); it != m_UnitShapes.end(); ++it)
	{
		if (!filter.Allowed(UNIT_INDEX_TO_TAG(it->first), it->second.moving))
			continue;

		entity_pos_t r1 = it->second.r;

		if (!(it->second.x + r1 < x - r || it->second.x - r1 > x + r || it->second.z + r1 < z - r || it->second.z - r1 > z + r))
			return true;
	}

	for (std::map<u32, StaticShape>::iterator it = m_StaticShapes.begin(); it != m_StaticShapes.end(); ++it)
	{
		if (!filter.Allowed(STATIC_INDEX_TO_TAG(it->first), false))
			continue;

		CFixedVector2D center1(it->second.x, it->second.z);
		if (Geometry::PointIsInSquare(center1 - center, it->second.u, it->second.v, CFixedVector2D(it->second.hw + r, it->second.hh + r)))
			return true;
	}
	return false;
}

/**
 * Compute the tile indexes on the grid nearest to a given point
 */
static void NearestTile(entity_pos_t x, entity_pos_t z, u16& i, u16& j, u16 w, u16 h)
{
	i = clamp((x / (int)CELL_SIZE).ToInt_RoundToZero(), 0, w-1);
	j = clamp((z / (int)CELL_SIZE).ToInt_RoundToZero(), 0, h-1);
}

/**
 * Returns the position of the center of the given tile
 */
static void TileCenter(u16 i, u16 j, entity_pos_t& x, entity_pos_t& z)
{
	x = entity_pos_t::FromInt(i*(int)CELL_SIZE + CELL_SIZE/2);
	z = entity_pos_t::FromInt(j*(int)CELL_SIZE + CELL_SIZE/2);
}

bool CCmpObstructionManager::Rasterise(Grid<u8>& grid)
{
	if (!IsDirty(grid))
		return false;

	grid.m_DirtyID = m_DirtyID;

	// TODO: this is all hopelessly inefficient
	// What we should perhaps do is have some kind of quadtree storing Shapes so it's
	// quick to invalidate and update small numbers of tiles

	grid.reset();

	for (std::map<u32, StaticShape>::iterator it = m_StaticShapes.begin(); it != m_StaticShapes.end(); ++it)
	{
		CFixedVector2D center(it->second.x, it->second.z);

		// Since we only count tiles whose centers are inside the square,
		// we maybe want to expand the square a bit so we're less likely to think there's
		// free space between buildings when there isn't. But this is just a random guess
		// and needs to be tweaked until everything works nicely.
		entity_pos_t expand = entity_pos_t::FromInt(CELL_SIZE / 2);

		CFixedVector2D halfSize(it->second.hw + expand, it->second.hh + expand);
		CFixedVector2D halfBound = Geometry::GetHalfBoundingBox(it->second.u, it->second.v, halfSize);

		u16 i0, j0, i1, j1;
		NearestTile(center.X - halfBound.X, center.Y - halfBound.Y, i0, j0, grid.m_W, grid.m_H);
		NearestTile(center.X + halfBound.X, center.Y + halfBound.Y, i1, j1, grid.m_W, grid.m_H);
		for (u16 j = j0; j <= j1; ++j)
		{
			for (u16 i = i0; i <= i1; ++i)
			{
				entity_pos_t x, z;
				TileCenter(i, j, x, z);
				if (Geometry::PointIsInSquare(CFixedVector2D(x, z) - center, it->second.u, it->second.v, halfSize))
					grid.set(i, j, 1);
			}
		}
	}

	return true;
}

void CCmpObstructionManager::GetObstructionsInRange(const IObstructionTestFilter& filter, entity_pos_t x0, entity_pos_t z0, entity_pos_t x1, entity_pos_t z1, std::vector<ObstructionSquare>& squares)
{
	// TODO: this should be made faster with quadtrees or whatever
	PROFILE("GetObstructionsInRange");

	for (std::map<u32, UnitShape>::iterator it = m_UnitShapes.begin(); it != m_UnitShapes.end(); ++it)
	{
		if (!filter.Allowed(UNIT_INDEX_TO_TAG(it->first), it->second.moving))
			continue;

		entity_pos_t r = it->second.r;

		// Skip this object if it's completely outside the requested range
		if (it->second.x + r < x0 || it->second.x - r > x1 || it->second.z + r < z0 || it->second.z - r > z1)
			continue;

		CFixedVector2D u(entity_pos_t::FromInt(1), entity_pos_t::Zero());
		CFixedVector2D v(entity_pos_t::Zero(), entity_pos_t::FromInt(1));
		ObstructionSquare s = { it->second.x, it->second.z, u, v, r, r };
		squares.push_back(s);
	}

	for (std::map<u32, StaticShape>::iterator it = m_StaticShapes.begin(); it != m_StaticShapes.end(); ++it)
	{
		if (!filter.Allowed(STATIC_INDEX_TO_TAG(it->first), false))
			continue;

		entity_pos_t r = it->second.hw + it->second.hh; // overestimate the max dist of an edge from the center

		// Skip this object if its overestimated bounding box is completely outside the requested range
		if (it->second.x + r < x0 || it->second.x - r > x1 || it->second.z + r < z0 || it->second.z - r > z1)
			continue;

		// TODO: maybe we should use Geometry::GetHalfBoundingBox to be more precise?

		ObstructionSquare s = { it->second.x, it->second.z, it->second.u, it->second.v, it->second.hw, it->second.hh };
		squares.push_back(s);
	}
}

bool CCmpObstructionManager::FindMostImportantObstruction(entity_pos_t x, entity_pos_t z, entity_pos_t r, ObstructionSquare& square)
{
	std::vector<ObstructionSquare> squares;

	CFixedVector2D center(x, z);

	// First look for obstructions that are covering the exact target point
	NullObstructionFilter filter;
	GetObstructionsInRange(filter, x, z, x, z, squares);
	// Building squares are more important but returned last, so check backwards
	for (std::vector<ObstructionSquare>::reverse_iterator it = squares.rbegin(); it != squares.rend(); ++it)
	{
		CFixedVector2D halfSize(it->hw, it->hh);
		if (Geometry::PointIsInSquare(CFixedVector2D(it->x, it->z) - center, it->u, it->v, halfSize))
		{
			square = *it;
			return true;
		}
	}

	// Then look for obstructions that cover the target point when expanded by r
	// (i.e. if the target is not inside an object but closer than we can get to it)

	// TODO: actually do that
	// (This might matter when you tell a unit to walk too close to the edge of a building)

	return false;
}

void CCmpObstructionManager::RenderSubmit(const CSimContext& context, SceneCollector& collector)
{
	if (!m_DebugOverlayEnabled)
		return;

	CColor defaultColour(0, 0, 1, 1);
	CColor movingColour(1, 0, 1, 1);

	// If the shapes have changed, then regenerate all the overlays
	if (m_DebugOverlayDirty)
	{
		m_DebugOverlayLines.clear();

		for (std::map<u32, UnitShape>::iterator it = m_UnitShapes.begin(); it != m_UnitShapes.end(); ++it)
		{
			m_DebugOverlayLines.push_back(SOverlayLine());
			m_DebugOverlayLines.back().m_Color = (it->second.moving ? movingColour : defaultColour);
			SimRender::ConstructSquareOnGround(context, it->second.x.ToFloat(), it->second.z.ToFloat(), it->second.r.ToFloat()*2, it->second.r.ToFloat()*2, 0, m_DebugOverlayLines.back());
		}

		for (std::map<u32, StaticShape>::iterator it = m_StaticShapes.begin(); it != m_StaticShapes.end(); ++it)
		{
			m_DebugOverlayLines.push_back(SOverlayLine());
			m_DebugOverlayLines.back().m_Color = defaultColour;
			float a = atan2(it->second.v.X.ToFloat(), it->second.v.Y.ToFloat());
			SimRender::ConstructSquareOnGround(context, it->second.x.ToFloat(), it->second.z.ToFloat(), it->second.hw.ToFloat()*2, it->second.hh.ToFloat()*2, a, m_DebugOverlayLines.back());
		}

		m_DebugOverlayDirty = false;
	}

	for (size_t i = 0; i < m_DebugOverlayLines.size(); ++i)
		collector.Submit(&m_DebugOverlayLines[i]);
}
