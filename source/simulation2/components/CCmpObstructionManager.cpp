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

#include "precompiled.h"

#include "simulation2/system/Component.h"
#include "ICmpObstructionManager.h"

#include "ICmpTerrain.h"

#include "simulation2/MessageTypes.h"
#include "simulation2/helpers/Geometry.h"
#include "simulation2/helpers/Rasterize.h"
#include "simulation2/helpers/Render.h"
#include "simulation2/helpers/Spatial.h"
#include "simulation2/serialization/SerializeTemplates.h"

#include "graphics/Overlay.h"
#include "graphics/Terrain.h"
#include "maths/MathUtil.h"
#include "ps/Profile.h"
#include "renderer/Scene.h"
#include "ps/CLogger.h"

// Externally, tags are opaque non-zero positive integers.
// Internally, they are tagged (by shape) indexes into shape lists.
// idx must be non-zero.
#define TAG_IS_VALID(tag) ((tag).valid())
#define TAG_IS_UNIT(tag) (((tag).n & 1) == 0)
#define TAG_IS_STATIC(tag) (((tag).n & 1) == 1)
#define UNIT_INDEX_TO_TAG(idx) tag_t(((idx) << 1) | 0)
#define STATIC_INDEX_TO_TAG(idx) tag_t(((idx) << 1) | 1)
#define TAG_TO_INDEX(tag) ((tag).n >> 1)

/**
 * Internal representation of axis-aligned sometimes-square sometimes-circle shapes for moving units
 */
struct UnitShape
{
	entity_id_t entity;
	entity_pos_t x, z;
	entity_pos_t r; // radius of circle, or half width of square
	entity_pos_t clearance;
	ICmpObstructionManager::flags_t flags;
	entity_id_t group; // control group (typically the owner entity, or a formation controller entity) (units ignore collisions with others in the same group)
};

/**
 * Internal representation of arbitrary-rotation static square shapes for buildings
 */
struct StaticShape
{
	entity_id_t entity;
	entity_pos_t x, z; // world-space coordinates
	CFixedVector2D u, v; // orthogonal unit vectors - axes of local coordinate space
	entity_pos_t hw, hh; // half width/height in local coordinate space
	ICmpObstructionManager::flags_t flags;
	entity_id_t group;
	entity_id_t group2;
};

/**
 * Serialization helper template for UnitShape
 */
struct SerializeUnitShape
{
	template<typename S>
	void operator()(S& serialize, const char* UNUSED(name), UnitShape& value)
	{
		serialize.NumberU32_Unbounded("entity", value.entity);
		serialize.NumberFixed_Unbounded("x", value.x);
		serialize.NumberFixed_Unbounded("z", value.z);
		serialize.NumberFixed_Unbounded("r", value.r);
		serialize.NumberFixed_Unbounded("clearance", value.clearance);
		serialize.NumberU8_Unbounded("flags", value.flags);
		serialize.NumberU32_Unbounded("group", value.group);
	}
};

/**
 * Serialization helper template for StaticShape
 */
struct SerializeStaticShape
{
	template<typename S>
	void operator()(S& serialize, const char* UNUSED(name), StaticShape& value)
	{
		serialize.NumberU32_Unbounded("entity", value.entity);
		serialize.NumberFixed_Unbounded("x", value.x);
		serialize.NumberFixed_Unbounded("z", value.z);
		serialize.NumberFixed_Unbounded("u.x", value.u.X);
		serialize.NumberFixed_Unbounded("u.y", value.u.Y);
		serialize.NumberFixed_Unbounded("v.x", value.v.X);
		serialize.NumberFixed_Unbounded("v.y", value.v.Y);
		serialize.NumberFixed_Unbounded("hw", value.hw);
		serialize.NumberFixed_Unbounded("hh", value.hh);
		serialize.NumberU8_Unbounded("flags", value.flags);
		serialize.NumberU32_Unbounded("group", value.group);
		serialize.NumberU32_Unbounded("group2", value.group2);
	}
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

	SpatialSubdivision m_UnitSubdivision;
	SpatialSubdivision m_StaticSubdivision;

	// TODO: using std::map is a bit inefficient; is there a better way to store these?
	std::map<u32, UnitShape> m_UnitShapes;
	std::map<u32, StaticShape> m_StaticShapes;
	u32 m_UnitShapeNext; // next allocated id
	u32 m_StaticShapeNext;

	bool m_PassabilityCircular;

	entity_pos_t m_WorldX0;
	entity_pos_t m_WorldZ0;
	entity_pos_t m_WorldX1;
	entity_pos_t m_WorldZ1;

	static std::string GetSchema()
	{
		return "<a:component type='system'/><empty/>";
	}

	virtual void Init(const CParamNode& UNUSED(paramNode))
	{
		m_DebugOverlayEnabled = false;
		m_DebugOverlayDirty = true;

		m_UnitShapeNext = 1;
		m_StaticShapeNext = 1;

		m_Dirty = true;
		m_NeedsGlobalUpdate = true;
		m_DirtinessGrid = NULL;

		m_PassabilityCircular = false;

		m_WorldX0 = m_WorldZ0 = m_WorldX1 = m_WorldZ1 = entity_pos_t::Zero();

		// Initialise with bogus values (these will get replaced when
		// SetBounds is called)
		ResetSubdivisions(entity_pos_t::FromInt(1024), entity_pos_t::FromInt(1024));
	}

	virtual void Deinit()
	{
		SAFE_DELETE(m_DirtinessGrid);
	}

	template<typename S>
	void SerializeCommon(S& serialize)
	{
		SerializeSpatialSubdivision()(serialize, "unit subdiv", m_UnitSubdivision);
		SerializeSpatialSubdivision()(serialize, "static subdiv", m_StaticSubdivision);

		SerializeMap<SerializeU32_Unbounded, SerializeUnitShape>()(serialize, "unit shapes", m_UnitShapes);
		SerializeMap<SerializeU32_Unbounded, SerializeStaticShape>()(serialize, "static shapes", m_StaticShapes);
		serialize.NumberU32_Unbounded("unit shape next", m_UnitShapeNext);
		serialize.NumberU32_Unbounded("static shape next", m_StaticShapeNext);

		serialize.Bool("circular", m_PassabilityCircular);

		serialize.NumberFixed_Unbounded("world x0", m_WorldX0);
		serialize.NumberFixed_Unbounded("world z0", m_WorldZ0);
		serialize.NumberFixed_Unbounded("world x1", m_WorldX1);
		serialize.NumberFixed_Unbounded("world z1", m_WorldZ1);
	}

	virtual void Serialize(ISerializer& serialize)
	{
		// TODO: this could perhaps be optimised by not storing all the obstructions,
		// and instead regenerating them from the other entities on Deserialize

		SerializeCommon(serialize);
	}

	virtual void Deserialize(const CParamNode& paramNode, IDeserializer& deserialize)
	{
		Init(paramNode);

		SerializeCommon(deserialize);
	}

	virtual void HandleMessage(const CMessage& msg, bool UNUSED(global))
	{
		switch (msg.GetType())
		{
		case MT_RenderSubmit:
		{
			const CMessageRenderSubmit& msgData = static_cast<const CMessageRenderSubmit&> (msg);
			RenderSubmit(msgData.collector);
			break;
		}
		}
	}

	virtual void SetBounds(entity_pos_t x0, entity_pos_t z0, entity_pos_t x1, entity_pos_t z1)
	{
		m_WorldX0 = x0;
		m_WorldZ0 = z0;
		m_WorldX1 = x1;
		m_WorldZ1 = z1;
		MakeDirtyAll();

		// Subdivision system bounds:
		ENSURE(x0.IsZero() && z0.IsZero()); // don't bother implementing non-zero offsets yet
		ResetSubdivisions(x1, z1);

		SAFE_DELETE(m_DirtinessGrid);

		CmpPtr<ICmpTerrain> cmpTerrain(GetSystemEntity());
		if (!cmpTerrain)
			return;

		u16 tiles = cmpTerrain->GetTilesPerSide();
		m_DirtinessGrid = new Grid<u8>(tiles*Pathfinding::NAVCELLS_PER_TILE, tiles*Pathfinding::NAVCELLS_PER_TILE);
	}

	void ResetSubdivisions(entity_pos_t x1, entity_pos_t z1)
	{
		// Use 8x8 tile subdivisions
		// (TODO: find the optimal number instead of blindly guessing)
		m_UnitSubdivision.Reset(x1, z1, entity_pos_t::FromInt(8*TERRAIN_TILE_SIZE));
		m_StaticSubdivision.Reset(x1, z1, entity_pos_t::FromInt(8*TERRAIN_TILE_SIZE));

		for (std::map<u32, UnitShape>::iterator it = m_UnitShapes.begin(); it != m_UnitShapes.end(); ++it)
		{
			CFixedVector2D center(it->second.x, it->second.z);
			CFixedVector2D halfSize(it->second.r, it->second.r);
			m_UnitSubdivision.Add(it->first, center - halfSize, center + halfSize);
		}

		for (std::map<u32, StaticShape>::iterator it = m_StaticShapes.begin(); it != m_StaticShapes.end(); ++it)
		{
			CFixedVector2D center(it->second.x, it->second.z);
			CFixedVector2D bbHalfSize = Geometry::GetHalfBoundingBox(it->second.u, it->second.v, CFixedVector2D(it->second.hw, it->second.hh));
			m_StaticSubdivision.Add(it->first, center - bbHalfSize, center + bbHalfSize);
		}
	}

	virtual tag_t AddUnitShape(entity_id_t ent, entity_pos_t x, entity_pos_t z, entity_pos_t r, entity_pos_t clearance, flags_t flags, entity_id_t group)
	{
		UnitShape shape = { ent, x, z, r, clearance, flags, group };
		u32 id = m_UnitShapeNext++;
		m_UnitShapes[id] = shape;
		MakeDirtyUnit(flags, id);

		m_UnitSubdivision.Add(id, CFixedVector2D(x - r, z - r), CFixedVector2D(x + r, z + r));

		return UNIT_INDEX_TO_TAG(id);
	}

	virtual tag_t AddStaticShape(entity_id_t ent, entity_pos_t x, entity_pos_t z, entity_angle_t a, entity_pos_t w, entity_pos_t h, flags_t flags, entity_id_t group, entity_id_t group2 /* = INVALID_ENTITY */)
	{
		fixed s, c;
		sincos_approx(a, s, c);
		CFixedVector2D u(c, -s);
		CFixedVector2D v(s, c);

		StaticShape shape = { ent, x, z, u, v, w/2, h/2, flags, group, group2 };
		u32 id = m_StaticShapeNext++;
		m_StaticShapes[id] = shape;
		MakeDirtyStatic(flags, id);

		CFixedVector2D center(x, z);
		CFixedVector2D bbHalfSize = Geometry::GetHalfBoundingBox(u, v, CFixedVector2D(w/2, h/2));
		m_StaticSubdivision.Add(id, center - bbHalfSize, center + bbHalfSize);

		return STATIC_INDEX_TO_TAG(id);
	}

	virtual ObstructionSquare GetUnitShapeObstruction(entity_pos_t x, entity_pos_t z, entity_pos_t r)
	{
		CFixedVector2D u(entity_pos_t::FromInt(1), entity_pos_t::Zero());
		CFixedVector2D v(entity_pos_t::Zero(), entity_pos_t::FromInt(1));
		ObstructionSquare o = { x, z, u, v, r, r };
		return o;
	}

	virtual ObstructionSquare GetStaticShapeObstruction(entity_pos_t x, entity_pos_t z, entity_angle_t a, entity_pos_t w, entity_pos_t h)
	{
		fixed s, c;
		sincos_approx(a, s, c);
		CFixedVector2D u(c, -s);
		CFixedVector2D v(s, c);

		ObstructionSquare o = { x, z, u, v, w/2, h/2 };
		return o;
	}

	virtual void MoveShape(tag_t tag, entity_pos_t x, entity_pos_t z, entity_angle_t a)
	{
		ENSURE(TAG_IS_VALID(tag));

		if (TAG_IS_UNIT(tag))
		{
			UnitShape& shape = m_UnitShapes[TAG_TO_INDEX(tag)];

			m_UnitSubdivision.Move(TAG_TO_INDEX(tag),
				CFixedVector2D(shape.x - shape.r, shape.z - shape.r),
				CFixedVector2D(shape.x + shape.r, shape.z + shape.r),
				CFixedVector2D(x - shape.r, z - shape.r),
				CFixedVector2D(x + shape.r, z + shape.r));

			shape.x = x;
			shape.z = z;

			MakeDirtyUnit(shape.flags, TAG_TO_INDEX(tag));
		}
		else
		{
			fixed s, c;
			sincos_approx(a, s, c);
			CFixedVector2D u(c, -s);
			CFixedVector2D v(s, c);

			StaticShape& shape = m_StaticShapes[TAG_TO_INDEX(tag)];

			CFixedVector2D fromBbHalfSize = Geometry::GetHalfBoundingBox(shape.u, shape.v, CFixedVector2D(shape.hw, shape.hh));
			CFixedVector2D toBbHalfSize = Geometry::GetHalfBoundingBox(u, v, CFixedVector2D(shape.hw, shape.hh));
			m_StaticSubdivision.Move(TAG_TO_INDEX(tag),
				CFixedVector2D(shape.x, shape.z) - fromBbHalfSize,
				CFixedVector2D(shape.x, shape.z) + fromBbHalfSize,
				CFixedVector2D(x, z) - toBbHalfSize,
				CFixedVector2D(x, z) + toBbHalfSize);

			shape.x = x;
			shape.z = z;
			shape.u = u;
			shape.v = v;

			MakeDirtyStatic(shape.flags, TAG_TO_INDEX(tag));
		}
	}

	virtual void SetUnitMovingFlag(tag_t tag, bool moving)
	{
		ENSURE(TAG_IS_VALID(tag) && TAG_IS_UNIT(tag));

		if (TAG_IS_UNIT(tag))
		{
			UnitShape& shape = m_UnitShapes[TAG_TO_INDEX(tag)];
			if (moving)
				shape.flags |= FLAG_MOVING;
			else
				shape.flags &= (flags_t)~FLAG_MOVING;

			MakeDirtyDebug();
		}
	}

	virtual void SetUnitControlGroup(tag_t tag, entity_id_t group)
	{
		ENSURE(TAG_IS_VALID(tag) && TAG_IS_UNIT(tag));

		if (TAG_IS_UNIT(tag))
		{
			UnitShape& shape = m_UnitShapes[TAG_TO_INDEX(tag)];
			shape.group = group;
		}
	}

	virtual void SetStaticControlGroup(tag_t tag, entity_id_t group, entity_id_t group2)
	{
		ENSURE(TAG_IS_VALID(tag) && TAG_IS_STATIC(tag));

		if (TAG_IS_STATIC(tag))
		{
			StaticShape& shape = m_StaticShapes[TAG_TO_INDEX(tag)];
			shape.group = group;
			shape.group2 = group2;
		}
	}

	virtual void RemoveShape(tag_t tag)
	{
		ENSURE(TAG_IS_VALID(tag));

		if (TAG_IS_UNIT(tag))
		{
			UnitShape& shape = m_UnitShapes[TAG_TO_INDEX(tag)];
			m_UnitSubdivision.Remove(TAG_TO_INDEX(tag),
				CFixedVector2D(shape.x - shape.r, shape.z - shape.r),
				CFixedVector2D(shape.x + shape.r, shape.z + shape.r));

			MakeDirtyUnit(shape.flags, TAG_TO_INDEX(tag));
			m_UnitShapes.erase(TAG_TO_INDEX(tag));
		}
		else
		{
			StaticShape& shape = m_StaticShapes[TAG_TO_INDEX(tag)];

			CFixedVector2D center(shape.x, shape.z);
			CFixedVector2D bbHalfSize = Geometry::GetHalfBoundingBox(shape.u, shape.v, CFixedVector2D(shape.hw, shape.hh));
			m_StaticSubdivision.Remove(TAG_TO_INDEX(tag), center - bbHalfSize, center + bbHalfSize);

			MakeDirtyStatic(shape.flags, TAG_TO_INDEX(tag));
			m_StaticShapes.erase(TAG_TO_INDEX(tag));
		}
	}

	virtual ObstructionSquare GetObstruction(tag_t tag)
	{
		ENSURE(TAG_IS_VALID(tag));

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
	virtual bool TestStaticShape(const IObstructionTestFilter& filter, entity_pos_t x, entity_pos_t z, entity_pos_t a, entity_pos_t w, entity_pos_t h, std::vector<entity_id_t>* out);
	virtual bool TestUnitShape(const IObstructionTestFilter& filter, entity_pos_t x, entity_pos_t z, entity_pos_t r, std::vector<entity_id_t>* out);

	virtual void Rasterize(Grid<u16>& grid, const entity_pos_t& expand, ICmpObstructionManager::flags_t requireMask, u16 setMask);
	virtual void GetObstructionsInRange(const IObstructionTestFilter& filter, entity_pos_t x0, entity_pos_t z0, entity_pos_t x1, entity_pos_t z1, std::vector<ObstructionSquare>& squares);
	virtual void GetUnitsOnObstruction(const ObstructionSquare& square, std::vector<entity_id_t>& out, const IObstructionTestFilter& filter);

	virtual void SetPassabilityCircular(bool enabled)
	{
		m_PassabilityCircular = enabled;
		MakeDirtyAll();

		CMessageObstructionMapShapeChanged msg;
		GetSimContext().GetComponentManager().BroadcastMessage(msg);
	}

	virtual bool GetPassabilityCircular() const
	{
		return m_PassabilityCircular;
	}

	virtual void SetDebugOverlay(bool enabled)
	{
		m_DebugOverlayEnabled = enabled;
		m_DebugOverlayDirty = true;
		if (!enabled)
			m_DebugOverlayLines.clear();
	}

	void RenderSubmit(SceneCollector& collector);

	virtual bool GetDirtinessData(Grid<u8>& dirtinessGrid, bool& globalUpdateNeeded)
	{
		if (!m_Dirty || !m_DirtinessGrid)
		{
			globalUpdateNeeded = false;
			return false;
		}

		dirtinessGrid = *m_DirtinessGrid;
		globalUpdateNeeded = m_NeedsGlobalUpdate;

		m_Dirty = false;
		m_NeedsGlobalUpdate = false;
		m_DirtinessGrid->reset();

		return true;
	}

private:
	// Dynamic updates for the long-range pathfinder
	bool m_Dirty;
	bool m_NeedsGlobalUpdate;
	Grid<u8>* m_DirtinessGrid;

	/**
	 * Mark all previous Rasterize()d grids as dirty, and the debug display.
	 * Call this when the world bounds have changed.
	 */
	void MakeDirtyAll()
	{
		m_Dirty = true;
		m_NeedsGlobalUpdate = true;
		m_DebugOverlayDirty = true;
	}

	/**
	 * Mark the debug display as dirty.
	 * Call this when nothing has changed except a unit's 'moving' flag.
	 */
	void MakeDirtyDebug()
	{
		m_DebugOverlayDirty = true;
	}

	inline void MarkDirtinessGrid(const entity_pos_t& x, const entity_pos_t& z, const entity_pos_t& r)
	{
		if (!m_DirtinessGrid)
			return;

		u16 j0, j1, i0, i1;
		Pathfinding::NearestNavcell(x - r, z - r, i0, j0, m_DirtinessGrid->m_W, m_DirtinessGrid->m_H);
		Pathfinding::NearestNavcell(x + r, z + r, i1, j1, m_DirtinessGrid->m_W, m_DirtinessGrid->m_H);

		for (int j = j0; j < j1; ++j)
			for (int i = i0; i < i1; ++i)
				m_DirtinessGrid->set(i, j, 1);
	}

	/**
	 * Mark all previous Rasterize()d grids as dirty, if they depend on this shape.
	 * Call this when a static shape has changed.
	 */
	void MakeDirtyStatic(flags_t flags, u32 index)
	{
		if (flags & (FLAG_BLOCK_PATHFINDING | FLAG_BLOCK_FOUNDATION))
		{
			m_Dirty = true;

			auto it = m_StaticShapes.find(index);
			if (it != m_StaticShapes.end())
				MarkDirtinessGrid(it->second.x, it->second.z, std::max(it->second.hw, it->second.hh));
		}

		m_DebugOverlayDirty = true;
	}

	/**
	 * Mark all previous Rasterize()d grids as dirty, if they depend on this shape.
	 * Call this when a unit shape has changed.
	 */
	void MakeDirtyUnit(flags_t flags, u32 index)
	{
		if (flags & (FLAG_BLOCK_PATHFINDING | FLAG_BLOCK_FOUNDATION))
		{
			m_Dirty = true;

			auto it = m_UnitShapes.find(index);
			if (it != m_UnitShapes.end())
				MarkDirtinessGrid(it->second.x, it->second.z, it->second.r);
		}

		m_DebugOverlayDirty = true;
	}

	/**
	 * Return whether the given point is within the world bounds by at least r
	 */
	bool IsInWorld(entity_pos_t x, entity_pos_t z, entity_pos_t r)
	{
		return (m_WorldX0+r <= x && x <= m_WorldX1-r && m_WorldZ0+r <= z && z <= m_WorldZ1-r);
	}

	/**
	 * Return whether the given point is within the world bounds
	 */
	bool IsInWorld(CFixedVector2D p)
	{
		return (m_WorldX0 <= p.X && p.X <= m_WorldX1 && m_WorldZ0 <= p.Y && p.Y <= m_WorldZ1);
	}
};

REGISTER_COMPONENT_TYPE(ObstructionManager)

bool CCmpObstructionManager::TestLine(const IObstructionTestFilter& filter, entity_pos_t x0, entity_pos_t z0, entity_pos_t x1, entity_pos_t z1, entity_pos_t r)
{
	PROFILE("TestLine");

	// Check that both end points are within the world (which means the whole line must be)
	if (!IsInWorld(x0, z0, r) || !IsInWorld(x1, z1, r))
		return true;

	CFixedVector2D posMin (std::min(x0, x1) - r, std::min(z0, z1) - r);
	CFixedVector2D posMax (std::max(x0, x1) + r, std::max(z0, z1) + r);

	std::vector<entity_id_t> unitShapes;
	m_UnitSubdivision.GetInRange(unitShapes, posMin, posMax);
	for (size_t i = 0; i < unitShapes.size(); ++i)
	{
		std::map<u32, UnitShape>::iterator it = m_UnitShapes.find(unitShapes[i]);
		ENSURE(it != m_UnitShapes.end());

		if (!filter.TestShape(UNIT_INDEX_TO_TAG(it->first), it->second.flags, it->second.group, INVALID_ENTITY))
			continue;

		CFixedVector2D center(it->second.x, it->second.z);
		CFixedVector2D halfSize(it->second.r + r, it->second.r + r);
		if (Geometry::TestRayAASquare(CFixedVector2D(x0, z0) - center, CFixedVector2D(x1, z1) - center, halfSize))
			return true;
	}

	std::vector<entity_id_t> staticShapes;
	m_StaticSubdivision.GetInRange(staticShapes, posMin, posMax);
	for (size_t i = 0; i < staticShapes.size(); ++i)
	{
		std::map<u32, StaticShape>::iterator it = m_StaticShapes.find(staticShapes[i]);
		ENSURE(it != m_StaticShapes.end());

		if (!filter.TestShape(STATIC_INDEX_TO_TAG(it->first), it->second.flags, it->second.group, it->second.group2))
			continue;

		CFixedVector2D center(it->second.x, it->second.z);
		CFixedVector2D halfSize(it->second.hw + r, it->second.hh + r);
		if (Geometry::TestRaySquare(CFixedVector2D(x0, z0) - center, CFixedVector2D(x1, z1) - center, it->second.u, it->second.v, halfSize))
			return true;
	}

	return false;
}

bool CCmpObstructionManager::TestStaticShape(const IObstructionTestFilter& filter,
	entity_pos_t x, entity_pos_t z, entity_pos_t a, entity_pos_t w, entity_pos_t h,
	std::vector<entity_id_t>* out)
{
	PROFILE("TestStaticShape");

	// TODO: should use the subdivision stuff here, if performance is non-negligible

	if (out)
		out->clear();

	fixed s, c;
	sincos_approx(a, s, c);
	CFixedVector2D u(c, -s);
	CFixedVector2D v(s, c);
	CFixedVector2D center(x, z);
	CFixedVector2D halfSize(w/2, h/2);

	// Check that all corners are within the world (which means the whole shape must be)
	if (!IsInWorld(center + u.Multiply(halfSize.X) + v.Multiply(halfSize.Y)) ||
		!IsInWorld(center + u.Multiply(halfSize.X) - v.Multiply(halfSize.Y)) ||
		!IsInWorld(center - u.Multiply(halfSize.X) + v.Multiply(halfSize.Y)) ||
		!IsInWorld(center - u.Multiply(halfSize.X) - v.Multiply(halfSize.Y)))
	{
		if (out)
			out->push_back(INVALID_ENTITY); // no entity ID, so just push an arbitrary marker
		else
			return true;
	}

	for (std::map<u32, UnitShape>::iterator it = m_UnitShapes.begin(); it != m_UnitShapes.end(); ++it)
	{
		if (!filter.TestShape(UNIT_INDEX_TO_TAG(it->first), it->second.flags, it->second.group, INVALID_ENTITY))
			continue;

		CFixedVector2D center1(it->second.x, it->second.z);

		if (Geometry::PointIsInSquare(center1 - center, u, v, CFixedVector2D(halfSize.X + it->second.r, halfSize.Y + it->second.r)))
		{
			if (out)
				out->push_back(it->second.entity);
			else
				return true;
		}
	}

	for (std::map<u32, StaticShape>::iterator it = m_StaticShapes.begin(); it != m_StaticShapes.end(); ++it)
	{
		if (!filter.TestShape(STATIC_INDEX_TO_TAG(it->first), it->second.flags, it->second.group, it->second.group2))
			continue;

		CFixedVector2D center1(it->second.x, it->second.z);
		CFixedVector2D halfSize1(it->second.hw, it->second.hh);
		if (Geometry::TestSquareSquare(center, u, v, halfSize, center1, it->second.u, it->second.v, halfSize1))
		{
			if (out)
				out->push_back(it->second.entity);
			else
				return true;
		}
	}

	if (out)
		return !out->empty(); // collided if the list isn't empty
	else
		return false; // didn't collide, if we got this far
}

bool CCmpObstructionManager::TestUnitShape(const IObstructionTestFilter& filter,
	entity_pos_t x, entity_pos_t z, entity_pos_t r,
	std::vector<entity_id_t>* out)
{
	PROFILE("TestUnitShape");

	// TODO: should use the subdivision stuff here, if performance is non-negligible

	// Check that the shape is within the world
	if (!IsInWorld(x, z, r))
	{
		if (out)
			out->push_back(INVALID_ENTITY); // no entity ID, so just push an arbitrary marker
		else
			return true;
	}

	CFixedVector2D center(x, z);

	for (std::map<u32, UnitShape>::iterator it = m_UnitShapes.begin(); it != m_UnitShapes.end(); ++it)
	{
		if (!filter.TestShape(UNIT_INDEX_TO_TAG(it->first), it->second.flags, it->second.group, INVALID_ENTITY))
			continue;

		entity_pos_t r1 = it->second.r;

		if (!(it->second.x + r1 < x - r || it->second.x - r1 > x + r || it->second.z + r1 < z - r || it->second.z - r1 > z + r))
		{
			if (out)
				out->push_back(it->second.entity);
			else
				return true;
		}
	}

	for (std::map<u32, StaticShape>::iterator it = m_StaticShapes.begin(); it != m_StaticShapes.end(); ++it)
	{
		if (!filter.TestShape(STATIC_INDEX_TO_TAG(it->first), it->second.flags, it->second.group, it->second.group2))
			continue;

		CFixedVector2D center1(it->second.x, it->second.z);
		if (Geometry::PointIsInSquare(center1 - center, it->second.u, it->second.v, CFixedVector2D(it->second.hw + r, it->second.hh + r)))
		{
			if (out)
				out->push_back(it->second.entity);
			else
				return true;

		}
	}

	if (out)
		return !out->empty(); // collided if the list isn't empty
	else
		return false; // didn't collide, if we got this far
}

void CCmpObstructionManager::Rasterize(Grid<u16>& grid, const entity_pos_t& expand, ICmpObstructionManager::flags_t requireMask, u16 setMask)
{
	PROFILE3("Rasterize");

	// Since m_DirtyID is only updated for pathfinding/foundation blocking shapes,
	// NeedUpdate+Rasterize will only be accurate for that subset of shapes.
	// (If we ever want to support rasterizing more shapes, we need to improve
	// the dirty-detection system too.)
	ENSURE(!(requireMask & ~(FLAG_BLOCK_PATHFINDING|FLAG_BLOCK_FOUNDATION)));

	// Cells are only marked as blocked if the whole cell is strictly inside the shape.
	// (That ensures the shape's geometric border is always reachable.)

	// TODO: it might be nice to rasterize with rounded corners for large 'expand' values.

	// (This could be implemented much more efficiently.)

	for (auto& pair : m_StaticShapes)
	{
		const StaticShape& shape = pair.second;
		if (!(shape.flags & requireMask))
			continue;

		ObstructionSquare square = { shape.x, shape.z, shape.u, shape.v, shape.hw, shape.hh };
		SimRasterize::Spans spans;
		SimRasterize::RasterizeRectWithClearance(spans, square, expand, Pathfinding::NAVCELL_SIZE);
		for (SimRasterize::Span& span : spans)
		{
			i16 j = span.j;
			if (j >= 0 && j <= grid.m_H)
			{
				i16 i0 = std::max(span.i0, (i16)0);
				i16 i1 = std::min(span.i1, (i16)grid.m_W);
				for (i16 i = i0; i < i1; ++i)
					grid.set(i, j, grid.get(i, j) | setMask);
			}
		}
	}

	for (auto& pair : m_UnitShapes)
	{
		CFixedVector2D center(pair.second.x, pair.second.z);

		if (!(pair.second.flags & requireMask))
			continue;

		entity_pos_t r = pair.second.r + expand;

		u16 i0, j0, i1, j1;
		Pathfinding::NearestNavcell(center.X - r, center.Y - r, i0, j0, grid.m_W, grid.m_H);
		Pathfinding::NearestNavcell(center.X + r, center.Y + r, i1, j1, grid.m_W, grid.m_H);
		for (u16 j = j0+1; j < j1; ++j)
			for (u16 i = i0+1; i < i1; ++i)
				grid.set(i, j, grid.get(i, j) | setMask);
	}
}

void CCmpObstructionManager::GetObstructionsInRange(const IObstructionTestFilter& filter, entity_pos_t x0, entity_pos_t z0, entity_pos_t x1, entity_pos_t z1, std::vector<ObstructionSquare>& squares)
{
	PROFILE("GetObstructionsInRange");

	ENSURE(x0 <= x1 && z0 <= z1);

	std::vector<entity_id_t> unitShapes;
	m_UnitSubdivision.GetInRange(unitShapes, CFixedVector2D(x0, z0), CFixedVector2D(x1, z1));
	for (entity_id_t& unitShape : unitShapes)
	{
		auto it = m_UnitShapes.find(unitShape);
		ENSURE(it != m_UnitShapes.end());

		if (!filter.TestShape(UNIT_INDEX_TO_TAG(it->first), it->second.flags, it->second.group, INVALID_ENTITY))
			continue;

		entity_pos_t r = it->second.r;

		// Skip this object if it's completely outside the requested range
		if (it->second.x + r < x0 || it->second.x - r > x1 || it->second.z + r < z0 || it->second.z - r > z1)
			continue;

		CFixedVector2D u(entity_pos_t::FromInt(1), entity_pos_t::Zero());
		CFixedVector2D v(entity_pos_t::Zero(), entity_pos_t::FromInt(1));
		squares.emplace_back(ObstructionSquare{ it->second.x, it->second.z, u, v, r, r });
	}

	std::vector<entity_id_t> staticShapes;
	m_StaticSubdivision.GetInRange(staticShapes, CFixedVector2D(x0, z0), CFixedVector2D(x1, z1));
	for (entity_id_t& staticShape : staticShapes)
	{
		auto it = m_StaticShapes.find(staticShape);
		ENSURE(it != m_StaticShapes.end());

		if (!filter.TestShape(STATIC_INDEX_TO_TAG(it->first), it->second.flags, it->second.group, it->second.group2))
			continue;

		entity_pos_t r = it->second.hw + it->second.hh; // overestimate the max dist of an edge from the center

		// Skip this object if its overestimated bounding box is completely outside the requested range
		if (it->second.x + r < x0 || it->second.x - r > x1 || it->second.z + r < z0 || it->second.z - r > z1)
			continue;

		// TODO: maybe we should use Geometry::GetHalfBoundingBox to be more precise?

		squares.emplace_back(ObstructionSquare{ it->second.x, it->second.z, it->second.u, it->second.v, it->second.hw, it->second.hh });
	}
}

void CCmpObstructionManager::GetUnitsOnObstruction(const ObstructionSquare& square, std::vector<entity_id_t>& out, const IObstructionTestFilter& filter)
{
	PROFILE3("GetUnitsOnObstruction");

	// In order to avoid getting units on impassable cells, we want to find all
	// units s.t. the RasterizeRectWithClearance of the building's shape with the
	// unit's clearance covers the navcell the unit is on.

	CmpPtr<ICmpPathfinder> cmpPathfinder(GetSystemEntity());
	if (!cmpPathfinder)
		return;

	entity_pos_t maxClearance = cmpPathfinder->GetMaximumClearance();

	std::vector<entity_id_t> unitShapes;
	CFixedVector2D center(square.x, square.z);
	CFixedVector2D expandedBox =
		Geometry::GetHalfBoundingBox(square.u, square.v, CFixedVector2D(square.hw, square.hh)) +
		CFixedVector2D(maxClearance, maxClearance);
	m_UnitSubdivision.GetInRange(unitShapes, center - expandedBox, center + expandedBox);

	std::map<entity_pos_t, SimRasterize::Spans> rasterizedRects;

	for (entity_id_t& unitShape : unitShapes)
	{
		auto it = m_UnitShapes.find(unitShape);
		ENSURE(it != m_UnitShapes.end());

		UnitShape& shape = it->second;

		if (!filter.TestShape(UNIT_INDEX_TO_TAG(unitShape), shape.flags, shape.group, INVALID_ENTITY))
			continue;

		if (rasterizedRects.find(shape.clearance) == rasterizedRects.end())
		{
			SimRasterize::Spans& newSpans = rasterizedRects[shape.clearance];
			SimRasterize::RasterizeRectWithClearance(newSpans, square, shape.clearance, Pathfinding::NAVCELL_SIZE);
		}

		SimRasterize::Spans& spans = rasterizedRects[shape.clearance];

		// Check whether the unit's center is on a navcell that's in
		// any of the spans

		u16 i = (shape.x / Pathfinding::NAVCELL_SIZE).ToInt_RoundToNegInfinity();
		u16 j = (shape.z / Pathfinding::NAVCELL_SIZE).ToInt_RoundToNegInfinity();

		for (size_t k = 0; k < spans.size(); ++k)
		{
			if (j == spans[k].j && spans[k].i0 <= i && i < spans[k].i1)
				out.push_back(shape.entity);
		}
	}

	// TODO : Should we expand by r here?
}

void CCmpObstructionManager::RenderSubmit(SceneCollector& collector)
{
	if (!m_DebugOverlayEnabled)
		return;

	CColor defaultColor(0, 0, 1, 1);
	CColor movingColor(1, 0, 1, 1);
	CColor boundsColor(1, 1, 0, 1);

	// If the shapes have changed, then regenerate all the overlays
	if (m_DebugOverlayDirty)
	{
		m_DebugOverlayLines.clear();

		m_DebugOverlayLines.push_back(SOverlayLine());
		m_DebugOverlayLines.back().m_Color = boundsColor;
		SimRender::ConstructSquareOnGround(GetSimContext(),
				(m_WorldX0+m_WorldX1).ToFloat()/2.f, (m_WorldZ0+m_WorldZ1).ToFloat()/2.f,
				(m_WorldX1-m_WorldX0).ToFloat(), (m_WorldZ1-m_WorldZ0).ToFloat(),
				0, m_DebugOverlayLines.back(), true);

		for (std::map<u32, UnitShape>::iterator it = m_UnitShapes.begin(); it != m_UnitShapes.end(); ++it)
		{
			m_DebugOverlayLines.push_back(SOverlayLine());
			m_DebugOverlayLines.back().m_Color = ((it->second.flags & FLAG_MOVING) ? movingColor : defaultColor);
			SimRender::ConstructSquareOnGround(GetSimContext(), it->second.x.ToFloat(), it->second.z.ToFloat(), it->second.r.ToFloat()*2, it->second.r.ToFloat()*2, 0, m_DebugOverlayLines.back(), true);
		}

		for (std::map<u32, StaticShape>::iterator it = m_StaticShapes.begin(); it != m_StaticShapes.end(); ++it)
		{
			m_DebugOverlayLines.push_back(SOverlayLine());
			m_DebugOverlayLines.back().m_Color = defaultColor;
			float a = atan2f(it->second.v.X.ToFloat(), it->second.v.Y.ToFloat());
			SimRender::ConstructSquareOnGround(GetSimContext(), it->second.x.ToFloat(), it->second.z.ToFloat(), it->second.hw.ToFloat()*2, it->second.hh.ToFloat()*2, a, m_DebugOverlayLines.back(), true);
		}

		m_DebugOverlayDirty = false;
	}

	for (size_t i = 0; i < m_DebugOverlayLines.size(); ++i)
		collector.Submit(&m_DebugOverlayLines[i]);
}
