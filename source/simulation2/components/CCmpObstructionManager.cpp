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
#include "simulation2/helpers/Render.h"

#include "graphics/Overlay.h"
#include "graphics/Terrain.h"
#include "maths/FixedVector2D.h"
#include "maths/MathUtil.h"
#include "ps/Overlay.h"
#include "ps/Profile.h"
#include "renderer/Scene.h"

// Externally, tags are opaque non-zero positive integers.
// Internally, they are tagged (by shape) indexes into shape lists.
// idx must be non-zero.
#define TAG_IS_CIRCLE(tag) (((tag) & 1) == 0)
#define TAG_IS_SQUARE(tag) (((tag) & 1) == 1)
#define CIRCLE_INDEX_TO_TAG(idx) (((idx) << 1) | 0)
#define SQUARE_INDEX_TO_TAG(idx) (((idx) << 1) | 1)
#define TAG_TO_INDEX(tag) ((tag) >> 1)

/**
 * Internal representation of circle shapes
 */
struct Circle
{
	entity_pos_t x, z, r;
};

/**
 * Internal representation of square shapes
 */
struct Square
{
	entity_pos_t x, z;
	entity_angle_t a;
	entity_pos_t w, h;
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
	std::map<u32, Circle> m_Circles;
	std::map<u32, Square> m_Squares;
	u32 m_CircleNext; // next allocated id
	u32 m_SquareNext;

	virtual void Init(const CSimContext& context, const CParamNode& UNUSED(paramNode))
	{
		m_DebugOverlayEnabled = false;
		m_DebugOverlayDirty = true;

		m_CircleNext = 1;
		m_SquareNext = 1;

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

	virtual tag_t AddCircle(entity_pos_t x, entity_pos_t z, entity_pos_t r)
	{
		Circle c = { x, z, r };
		size_t id = m_CircleNext++;
		m_Circles[id] = c;
		MakeDirty();
		return CIRCLE_INDEX_TO_TAG(id);
	}

	virtual tag_t AddSquare(entity_pos_t x, entity_pos_t z, entity_angle_t a, entity_pos_t w, entity_pos_t h)
	{
		Square s = { x, z, a, w, h };
		size_t id = m_SquareNext++;
		m_Squares[id] = s;
		MakeDirty();
		return SQUARE_INDEX_TO_TAG(id);
	}

	virtual void MoveShape(tag_t tag, entity_pos_t x, entity_pos_t z, entity_angle_t a)
	{
		debug_assert(tag);

		if (TAG_IS_CIRCLE(tag))
		{
			Circle& c = m_Circles[TAG_TO_INDEX(tag)];
			c.x = x;
			c.z = z;
		}
		else
		{
			Square& s = m_Squares[TAG_TO_INDEX(tag)];
			s.x = x;
			s.z = z;
			s.a = a;
		}

		MakeDirty();
	}

	virtual void RemoveShape(tag_t tag)
	{
		debug_assert(tag);

		if (TAG_IS_CIRCLE(tag))
			m_Circles.erase(TAG_TO_INDEX(tag));
		else
			m_Squares.erase(TAG_TO_INDEX(tag));

		MakeDirty();
	}

	virtual bool TestLine(const IObstructionTestFilter& filter, entity_pos_t x0, entity_pos_t z0, entity_pos_t x1, entity_pos_t z1, entity_pos_t r);
	virtual bool TestCircle(const IObstructionTestFilter& filter, entity_pos_t x, entity_pos_t z, entity_pos_t r);
	virtual bool TestSquare(const IObstructionTestFilter& filter, entity_pos_t x, entity_pos_t z, entity_pos_t a, entity_pos_t w, entity_pos_t h);

	virtual bool Rasterise(Grid<u8>& grid);

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

// Detect intersection between ray (0,0)-L and circle with center M radius r
// (Only counts intersections from the outside to the inside)
static bool IntersectRayCircle(CFixedVector2D l, CFixedVector2D m, entity_pos_t r)
{
	// TODO: this should all be checked and tested etc, it's just a rough first attempt for now...

	// Intersections at (t * l.X - m.X)^2 * (t * l.Y - m.Y) = r^2
	// so solve the quadratic for t:

#define DOT(u, v) ( ((i64)u.X.GetInternalValue()*(i64)v.X.GetInternalValue()) + ((i64)u.Y.GetInternalValue()*(i64)v.Y.GetInternalValue()) )
	i64 a = DOT(l, l);
	if (a == 0)
		return false; // avoid divide-by-zero later
	i64 b = DOT(l, m)*-2;
	i64 c = DOT(m, m) - r.GetInternalValue()*r.GetInternalValue();
	i64 d = b*b - 4*a*c; // TODO: overflow breaks stuff here
	if (d < 0) // no solutions
		return false;
	// Find the time of first intersection (entering the circle)
	i64 t2a = (-b - isqrt64(d)); // don't divide by 2a explicitly, to avoid rounding errors
	if ((a > 0 && t2a < 0) || (a < 0 && t2a > 0)) // if t2a/2a < 0 then intersection was before the ray
		return false;
	if (t2a >= 2*a) // intersection was after the ray
		return false;
//	printf("isct (%f,%f) (%f,%f) %f a=%lld b=%lld c=%lld d=%lld t2a=%lld\n", l.X.ToDouble(), l.Y.ToDouble(), m.X.ToDouble(), m.Y.ToDouble(), r.ToDouble(), a, b, c, d, t2a);
	return true;
}

bool CCmpObstructionManager::TestLine(const IObstructionTestFilter& filter, entity_pos_t x0, entity_pos_t z0, entity_pos_t x1, entity_pos_t z1, entity_pos_t r)
{
	PROFILE("TestLine");

	// TODO: this is all very inefficient, it should use some kind of spatial data structures

	// Ray-circle intersections
	for (std::map<u32, Circle>::iterator it = m_Circles.begin(); it != m_Circles.end(); ++it)
	{
		if (!filter.Allowed(CIRCLE_INDEX_TO_TAG(it->first)))
			continue;

		if (IntersectRayCircle(CFixedVector2D(x1 - x0, z1 - z0), CFixedVector2D(it->second.x - x0, it->second.z - z0), it->second.r + r))
			return false;
	}

	// Ray-square intersections
	for (std::map<u32, Square>::iterator it = m_Squares.begin(); it != m_Squares.end(); ++it)
	{
		if (!filter.Allowed(SQUARE_INDEX_TO_TAG(it->first)))
			continue;

		// XXX need some kind of square intersection code
		if (IntersectRayCircle(CFixedVector2D(x1 - x0, z1 - z0), CFixedVector2D(it->second.x - x0, it->second.z - z0), it->second.w/2 + r))
			return false;
	}

	return true;
}

bool CCmpObstructionManager::TestCircle(const IObstructionTestFilter& filter, entity_pos_t x, entity_pos_t z, entity_pos_t r)
{
	PROFILE("TestCircle");

	// Circle-circle intersections
	for (std::map<u32, Circle>::iterator it = m_Circles.begin(); it != m_Circles.end(); ++it)
	{
		if (!filter.Allowed(CIRCLE_INDEX_TO_TAG(it->first)))
			continue;

		if (CFixedVector2D(it->second.x - x, it->second.z - z).Length() <= it->second.r + r)
			return false;
	}

	// Circle-square intersections
	for (std::map<u32, Square>::iterator it = m_Squares.begin(); it != m_Squares.end(); ++it)
	{
		if (!filter.Allowed(SQUARE_INDEX_TO_TAG(it->first)))
			continue;

		// XXX need some kind of square intersection code
		if (CFixedVector2D(it->second.x - x, it->second.z - z).Length() <= it->second.w/2 + r)
			return false;
	}

	return true;
}

bool CCmpObstructionManager::TestSquare(const IObstructionTestFilter& filter, entity_pos_t x, entity_pos_t z, entity_pos_t a, entity_pos_t w, entity_pos_t h)
{
	// XXX need to implement this
	return TestCircle(filter, x, z, w/2);
}

/**
 * Compute the tile indexes on the grid nearest to a given point
 */
static void NearestTile(entity_pos_t x, entity_pos_t z, u16& i, u16& j, u16 w, u16 h)
{
	i = clamp((x / CELL_SIZE).ToInt_RoundToZero(), 0, w-1);
	j = clamp((z / CELL_SIZE).ToInt_RoundToZero(), 0, h-1);
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

	for (std::map<u32, Circle>::iterator it = m_Circles.begin(); it != m_Circles.end(); ++it)
	{
		// TODO: need to handle larger circles (r != 0)
		u16 i, j;
		NearestTile(it->second.x, it->second.z, i, j, grid.m_W, grid.m_H);
		grid.set(i, j, 1);
	}

	for (std::map<u32, Square>::iterator it = m_Squares.begin(); it != m_Squares.end(); ++it)
	{
		// TODO: need to handle rotations (a != 0)
		entity_pos_t x0 = it->second.x - it->second.w/2;
		entity_pos_t z0 = it->second.z - it->second.h/2;
		entity_pos_t x1 = it->second.x + it->second.w/2;
		entity_pos_t z1 = it->second.z + it->second.h/2;
		u16 i0, j0, i1, j1;
		NearestTile(x0, z0, i0, j0, grid.m_W, grid.m_H); // TODO: should be careful about rounding on edges
		NearestTile(x1, z1, i1, j1, grid.m_W, grid.m_H);
		for (u16 j = j0; j <= j1; ++j)
			for (u16 i = i0; i <= i1; ++i)
				grid.set(i, j, 1);
	}

	return true;
}

void CCmpObstructionManager::RenderSubmit(const CSimContext& context, SceneCollector& collector)
{
	if (!m_DebugOverlayEnabled)
		return;

	CColor defaultColour(0, 0, 1, 1);

	// If the shapes have changed, then regenerate all the overlays
	if (m_DebugOverlayDirty)
	{
		m_DebugOverlayLines.clear();

		for (std::map<u32, Circle>::iterator it = m_Circles.begin(); it != m_Circles.end(); ++it)
		{
			m_DebugOverlayLines.push_back(SOverlayLine());
			m_DebugOverlayLines.back().m_Color = defaultColour;
			SimRender::ConstructCircleOnGround(context, it->second.x.ToFloat(), it->second.z.ToFloat(), it->second.r.ToFloat(), m_DebugOverlayLines.back());
		}

		for (std::map<u32, Square>::iterator it = m_Squares.begin(); it != m_Squares.end(); ++it)
		{
			m_DebugOverlayLines.push_back(SOverlayLine());
			m_DebugOverlayLines.back().m_Color = defaultColour;
			SimRender::ConstructSquareOnGround(context, it->second.x.ToFloat(), it->second.z.ToFloat(), it->second.w.ToFloat(), it->second.h.ToFloat(), it->second.a.ToFloat(), m_DebugOverlayLines.back());
		}

		m_DebugOverlayDirty = false;
	}

	for (size_t i = 0; i < m_DebugOverlayLines.size(); ++i)
		collector.Submit(&m_DebugOverlayLines[i]);
}
