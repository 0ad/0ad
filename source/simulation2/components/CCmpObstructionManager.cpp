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

#include "simulation2/system/Component.h"
#include "ICmpObstructionManager.h"

#include "ICmpTerrain.h"
#include "ICmpPosition.h"

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
 * Internal representation of axis-aligned circular shapes for moving units
 */
struct UnitShape
{
	entity_id_t entity;
	entity_pos_t x, z;
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
	void operator()(S& serialize, const char* UNUSED(name), UnitShape& value) const
	{
		serialize.NumberU32_Unbounded("entity", value.entity);
		serialize.NumberFixed_Unbounded("x", value.x);
		serialize.NumberFixed_Unbounded("z", value.z);
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
	void operator()(S& serialize, const char* UNUSED(name), StaticShape& value) const
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

	entity_pos_t m_MaxClearance;

	bool m_PassabilityCircular;

	entity_pos_t m_WorldX0;
	entity_pos_t m_WorldZ0;
	entity_pos_t m_WorldX1;
	entity_pos_t m_WorldZ1;
	u16 m_TerrainTiles;

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

		m_UpdateInformations.dirty = true;
		m_UpdateInformations.globallyDirty = true;

		m_PassabilityCircular = false;

		m_WorldX0 = m_WorldZ0 = m_WorldX1 = m_WorldZ1 = entity_pos_t::Zero();
		m_TerrainTiles = 0;

		// Initialise with bogus values (these will get replaced when
		// SetBounds is called)
		ResetSubdivisions(entity_pos_t::FromInt(1024), entity_pos_t::FromInt(1024));
	}

	virtual void Deinit()
	{
	}

	template<typename S>
	void SerializeCommon(S& serialize)
	{
		SerializeSpatialSubdivision()(serialize, "unit subdiv", m_UnitSubdivision);
		SerializeSpatialSubdivision()(serialize, "static subdiv", m_StaticSubdivision);

		serialize.NumberFixed_Unbounded("max clearance", m_MaxClearance);

		SerializeMap<SerializeU32_Unbounded, SerializeUnitShape>()(serialize, "unit shapes", m_UnitShapes);
		SerializeMap<SerializeU32_Unbounded, SerializeStaticShape>()(serialize, "static shapes", m_StaticShapes);
		serialize.NumberU32_Unbounded("unit shape next", m_UnitShapeNext);
		serialize.NumberU32_Unbounded("static shape next", m_StaticShapeNext);

		serialize.Bool("circular", m_PassabilityCircular);

		serialize.NumberFixed_Unbounded("world x0", m_WorldX0);
		serialize.NumberFixed_Unbounded("world z0", m_WorldZ0);
		serialize.NumberFixed_Unbounded("world x1", m_WorldX1);
		serialize.NumberFixed_Unbounded("world z1", m_WorldZ1);
		serialize.NumberU16_Unbounded("terrain tiles", m_TerrainTiles);
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

		m_UpdateInformations.dirtinessGrid = Grid<u8>(m_TerrainTiles*Pathfinding::NAVCELLS_PER_TILE, m_TerrainTiles*Pathfinding::NAVCELLS_PER_TILE);
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

	// NB: on deserialization, this function is not called after the component is reset.
	// So anything that happens here should be safely serialized.
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

		CmpPtr<ICmpTerrain> cmpTerrain(GetSystemEntity());
		if (!cmpTerrain)
			return;

		m_TerrainTiles = cmpTerrain->GetTilesPerSide();
		m_UpdateInformations.dirtinessGrid = Grid<u8>(m_TerrainTiles*Pathfinding::NAVCELLS_PER_TILE, m_TerrainTiles*Pathfinding::NAVCELLS_PER_TILE);

		CmpPtr<ICmpPathfinder> cmpPathfinder(GetSystemEntity());
		if (cmpPathfinder)
			m_MaxClearance = cmpPathfinder->GetMaximumClearance();
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
			CFixedVector2D halfSize(it->second.clearance, it->second.clearance);
			m_UnitSubdivision.Add(it->first, center - halfSize, center + halfSize);
		}

		for (std::map<u32, StaticShape>::iterator it = m_StaticShapes.begin(); it != m_StaticShapes.end(); ++it)
		{
			CFixedVector2D center(it->second.x, it->second.z);
			CFixedVector2D bbHalfSize = Geometry::GetHalfBoundingBox(it->second.u, it->second.v, CFixedVector2D(it->second.hw, it->second.hh));
			m_StaticSubdivision.Add(it->first, center - bbHalfSize, center + bbHalfSize);
		}
	}

	virtual tag_t AddUnitShape(entity_id_t ent, entity_pos_t x, entity_pos_t z, entity_pos_t clearance, flags_t flags, entity_id_t group)
	{
		UnitShape shape = { ent, x, z, clearance, flags, group };
		u32 id = m_UnitShapeNext++;
		m_UnitShapes[id] = shape;

		m_UnitSubdivision.Add(id, CFixedVector2D(x - clearance, z - clearance), CFixedVector2D(x + clearance, z + clearance));

		MakeDirtyUnit(flags, id, shape);

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

		CFixedVector2D center(x, z);
		CFixedVector2D bbHalfSize = Geometry::GetHalfBoundingBox(u, v, CFixedVector2D(w/2, h/2));
		m_StaticSubdivision.Add(id, center - bbHalfSize, center + bbHalfSize);

		MakeDirtyStatic(flags, id, shape);

		return STATIC_INDEX_TO_TAG(id);
	}

	virtual ObstructionSquare GetUnitShapeObstruction(entity_pos_t x, entity_pos_t z, entity_pos_t clearance) const
	{
		CFixedVector2D u(entity_pos_t::FromInt(1), entity_pos_t::Zero());
		CFixedVector2D v(entity_pos_t::Zero(), entity_pos_t::FromInt(1));
		ObstructionSquare o = { x, z, u, v, clearance, clearance };
		return o;
	}

	virtual ObstructionSquare GetStaticShapeObstruction(entity_pos_t x, entity_pos_t z, entity_angle_t a, entity_pos_t w, entity_pos_t h) const
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

			MakeDirtyUnit(shape.flags, TAG_TO_INDEX(tag), shape); // dirty the old shape region

			m_UnitSubdivision.Move(TAG_TO_INDEX(tag),
				CFixedVector2D(shape.x - shape.clearance, shape.z - shape.clearance),
				CFixedVector2D(shape.x + shape.clearance, shape.z + shape.clearance),
				CFixedVector2D(x - shape.clearance, z - shape.clearance),
				CFixedVector2D(x + shape.clearance, z + shape.clearance));

			shape.x = x;
			shape.z = z;

			MakeDirtyUnit(shape.flags, TAG_TO_INDEX(tag), shape); // dirty the new shape region
		}
		else
		{
			fixed s, c;
			sincos_approx(a, s, c);
			CFixedVector2D u(c, -s);
			CFixedVector2D v(s, c);

			StaticShape& shape = m_StaticShapes[TAG_TO_INDEX(tag)];

			MakeDirtyStatic(shape.flags, TAG_TO_INDEX(tag), shape); // dirty the old shape region

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

			MakeDirtyStatic(shape.flags, TAG_TO_INDEX(tag), shape); // dirty the new shape region
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
				CFixedVector2D(shape.x - shape.clearance, shape.z - shape.clearance),
				CFixedVector2D(shape.x + shape.clearance, shape.z + shape.clearance));

			MakeDirtyUnit(shape.flags, TAG_TO_INDEX(tag), shape);

			m_UnitShapes.erase(TAG_TO_INDEX(tag));
		}
		else
		{
			StaticShape& shape = m_StaticShapes[TAG_TO_INDEX(tag)];

			CFixedVector2D center(shape.x, shape.z);
			CFixedVector2D bbHalfSize = Geometry::GetHalfBoundingBox(shape.u, shape.v, CFixedVector2D(shape.hw, shape.hh));
			m_StaticSubdivision.Remove(TAG_TO_INDEX(tag), center - bbHalfSize, center + bbHalfSize);

			MakeDirtyStatic(shape.flags, TAG_TO_INDEX(tag), shape);

			m_StaticShapes.erase(TAG_TO_INDEX(tag));
		}
	}

	virtual ObstructionSquare GetObstruction(tag_t tag) const
	{
		ENSURE(TAG_IS_VALID(tag));

		if (TAG_IS_UNIT(tag))
		{
			const UnitShape& shape = m_UnitShapes.at(TAG_TO_INDEX(tag));
			CFixedVector2D u(entity_pos_t::FromInt(1), entity_pos_t::Zero());
			CFixedVector2D v(entity_pos_t::Zero(), entity_pos_t::FromInt(1));
			ObstructionSquare o = { shape.x, shape.z, u, v, shape.clearance, shape.clearance };
			return o;
		}
		else
		{
			const StaticShape& shape = m_StaticShapes.at(TAG_TO_INDEX(tag));
			ObstructionSquare o = { shape.x, shape.z, shape.u, shape.v, shape.hw, shape.hh };
			return o;
		}
	}

	virtual fixed DistanceToPoint(entity_id_t ent, entity_pos_t px, entity_pos_t pz) const;
	virtual fixed MaxDistanceToPoint(entity_id_t ent, entity_pos_t px, entity_pos_t pz) const;
	virtual fixed DistanceToTarget(entity_id_t ent, entity_id_t target) const;
	virtual fixed MaxDistanceToTarget(entity_id_t ent, entity_id_t target) const;
	virtual fixed DistanceBetweenShapes(const ObstructionSquare& source, const ObstructionSquare& target) const;
	virtual fixed MaxDistanceBetweenShapes(const ObstructionSquare& source, const ObstructionSquare& target) const;

	virtual bool IsInPointRange(entity_id_t ent, entity_pos_t px, entity_pos_t pz, entity_pos_t minRange, entity_pos_t maxRange, bool opposite) const;
	virtual bool IsInTargetRange(entity_id_t ent, entity_id_t target, entity_pos_t minRange, entity_pos_t maxRange, bool opposite) const;
	virtual bool IsPointInPointRange(entity_pos_t x, entity_pos_t z, entity_pos_t px, entity_pos_t pz, entity_pos_t minRange, entity_pos_t maxRange) const;

	virtual bool TestLine(const IObstructionTestFilter& filter, entity_pos_t x0, entity_pos_t z0, entity_pos_t x1, entity_pos_t z1, entity_pos_t r, bool relaxClearanceForUnits = false) const;
	virtual bool TestStaticShape(const IObstructionTestFilter& filter, entity_pos_t x, entity_pos_t z, entity_pos_t a, entity_pos_t w, entity_pos_t h, std::vector<entity_id_t>* out) const;
	virtual bool TestUnitShape(const IObstructionTestFilter& filter, entity_pos_t x, entity_pos_t z, entity_pos_t r, std::vector<entity_id_t>* out) const;

	virtual void Rasterize(Grid<NavcellData>& grid, const std::vector<PathfinderPassability>& passClasses, bool fullUpdate);
	virtual void GetObstructionsInRange(const IObstructionTestFilter& filter, entity_pos_t x0, entity_pos_t z0, entity_pos_t x1, entity_pos_t z1, std::vector<ObstructionSquare>& squares) const;
	virtual void GetUnitObstructionsInRange(const IObstructionTestFilter& filter, entity_pos_t x0, entity_pos_t z0, entity_pos_t x1, entity_pos_t z1, std::vector<ObstructionSquare>& squares) const;
	virtual void GetStaticObstructionsInRange(const IObstructionTestFilter& filter, entity_pos_t x0, entity_pos_t z0, entity_pos_t x1, entity_pos_t z1, std::vector<ObstructionSquare>& squares) const;
	virtual void GetUnitsOnObstruction(const ObstructionSquare& square, std::vector<entity_id_t>& out, const IObstructionTestFilter& filter, bool strict = false) const;
	virtual void GetStaticObstructionsOnObstruction(const ObstructionSquare& square, std::vector<entity_id_t>& out, const IObstructionTestFilter& filter) const;

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

	virtual void UpdateInformations(GridUpdateInformation& informations)
	{
		if (!m_UpdateInformations.dirtinessGrid.blank())
			informations.MergeAndClear(m_UpdateInformations);
	}

private:
	// Dynamic updates for the long-range pathfinder
	GridUpdateInformation m_UpdateInformations;
	// These vectors might contain shapes that were deleted
	std::vector<u32> m_DirtyStaticShapes;
	std::vector<u32> m_DirtyUnitShapes;

	/**
	 * Mark all previous Rasterize()d grids as dirty, and the debug display.
	 * Call this when the world bounds have changed.
	 */
	void MakeDirtyAll()
	{
		m_UpdateInformations.dirty = true;
		m_UpdateInformations.globallyDirty = true;
		m_UpdateInformations.dirtinessGrid.reset();

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
		MarkDirtinessGrid(x, z, CFixedVector2D(r, r));
	}

	inline void MarkDirtinessGrid(const entity_pos_t& x, const entity_pos_t& z, const CFixedVector2D& hbox)
	{
		ENSURE(m_UpdateInformations.dirtinessGrid.m_W == m_TerrainTiles*Pathfinding::NAVCELLS_PER_TILE &&
			m_UpdateInformations.dirtinessGrid.m_H == m_TerrainTiles*Pathfinding::NAVCELLS_PER_TILE);
		if (m_TerrainTiles == 0)
			return;

		u16 j0, j1, i0, i1;
		Pathfinding::NearestNavcell(x - hbox.X, z - hbox.Y, i0, j0, m_UpdateInformations.dirtinessGrid.m_W, m_UpdateInformations.dirtinessGrid.m_H);
		Pathfinding::NearestNavcell(x + hbox.X, z + hbox.Y, i1, j1, m_UpdateInformations.dirtinessGrid.m_W, m_UpdateInformations.dirtinessGrid.m_H);

		for (int j = j0; j < j1; ++j)
			for (int i = i0; i < i1; ++i)
				m_UpdateInformations.dirtinessGrid.set(i, j, 1);
	}

	/**
	 * Mark all previous Rasterize()d grids as dirty, if they depend on this shape.
	 * Call this when a static shape has changed.
	 */
	void MakeDirtyStatic(flags_t flags, u32 index, const StaticShape& shape)
	{
		m_DebugOverlayDirty = true;

		if (flags & (FLAG_BLOCK_PATHFINDING | FLAG_BLOCK_FOUNDATION))
		{
			m_UpdateInformations.dirty = true;

			if (std::find(m_DirtyStaticShapes.begin(), m_DirtyStaticShapes.end(), index) == m_DirtyStaticShapes.end())
				m_DirtyStaticShapes.push_back(index);

			// All shapes overlapping the updated part of the grid should be dirtied too.
			// We are going to invalidate the region of the grid corresponding to the modified shape plus its clearance,
			// and we need to get the shapes whose clearance can overlap this area. So we need to extend the search area
			// by two times the maximum clearance.

			CFixedVector2D center(shape.x, shape.z);
			CFixedVector2D hbox = Geometry::GetHalfBoundingBox(shape.u, shape.v, CFixedVector2D(shape.hw, shape.hh));
			CFixedVector2D expand(m_MaxClearance, m_MaxClearance);

			std::vector<u32> staticsNear;
			m_StaticSubdivision.GetInRange(staticsNear, center - hbox - expand*2, center + hbox + expand*2);
			for (u32& staticId : staticsNear)
				if (std::find(m_DirtyStaticShapes.begin(), m_DirtyStaticShapes.end(), staticId) == m_DirtyStaticShapes.end())
					m_DirtyStaticShapes.push_back(staticId);

			std::vector<u32> unitsNear;
			m_UnitSubdivision.GetInRange(unitsNear, center - hbox - expand*2, center + hbox + expand*2);
			for (u32& unitId : unitsNear)
				if (std::find(m_DirtyUnitShapes.begin(), m_DirtyUnitShapes.end(), unitId) == m_DirtyUnitShapes.end())
					m_DirtyUnitShapes.push_back(unitId);

			MarkDirtinessGrid(shape.x, shape.z, hbox + expand);
		}
	}

	/**
	 * Mark all previous Rasterize()d grids as dirty, if they depend on this shape.
	 * Call this when a unit shape has changed.
	 */
	void MakeDirtyUnit(flags_t flags, u32 index, const UnitShape& shape)
	{
		m_DebugOverlayDirty = true;

		if (flags & (FLAG_BLOCK_PATHFINDING | FLAG_BLOCK_FOUNDATION))
		{
			m_UpdateInformations.dirty = true;

			if (std::find(m_DirtyUnitShapes.begin(), m_DirtyUnitShapes.end(), index) == m_DirtyUnitShapes.end())
				m_DirtyUnitShapes.push_back(index);

			// All shapes overlapping the updated part of the grid should be dirtied too.
			// We are going to invalidate the region of the grid corresponding to the modified shape plus its clearance,
			// and we need to get the shapes whose clearance can overlap this area. So we need to extend the search area
			// by two times the maximum clearance.

			CFixedVector2D center(shape.x, shape.z);

			std::vector<u32> staticsNear;
			m_StaticSubdivision.GetNear(staticsNear, center, shape.clearance + m_MaxClearance*2);
			for (u32& staticId : staticsNear)
				if (std::find(m_DirtyStaticShapes.begin(), m_DirtyStaticShapes.end(), staticId) == m_DirtyStaticShapes.end())
					m_DirtyStaticShapes.push_back(staticId);

			std::vector<u32> unitsNear;
			m_UnitSubdivision.GetNear(unitsNear, center, shape.clearance + m_MaxClearance*2);
			for (u32& unitId : unitsNear)
				if (std::find(m_DirtyUnitShapes.begin(), m_DirtyUnitShapes.end(), unitId) == m_DirtyUnitShapes.end())
					m_DirtyUnitShapes.push_back(unitId);

			MarkDirtinessGrid(shape.x, shape.z, shape.clearance + m_MaxClearance);
		}
	}

	/**
	 * Return whether the given point is within the world bounds by at least r
	 */
	inline bool IsInWorld(entity_pos_t x, entity_pos_t z, entity_pos_t r) const
	{
		return (m_WorldX0+r <= x && x <= m_WorldX1-r && m_WorldZ0+r <= z && z <= m_WorldZ1-r);
	}

	/**
	 * Return whether the given point is within the world bounds
	 */
	inline bool IsInWorld(const CFixedVector2D& p) const
	{
		return (m_WorldX0 <= p.X && p.X <= m_WorldX1 && m_WorldZ0 <= p.Y && p.Y <= m_WorldZ1);
	}

	void RasterizeHelper(Grid<NavcellData>& grid, ICmpObstructionManager::flags_t requireMask, bool fullUpdate, pass_class_t appliedMask, entity_pos_t clearance = fixed::Zero()) const;
};

REGISTER_COMPONENT_TYPE(ObstructionManager)

/**
 * DistanceTo function family, all end up in calculating a vector length, DistanceBetweenShapes or
 * MaxDistanceBetweenShapes. The MaxFoo family calculates the opposite edge opposite edge distance.
 * When the distance is undefined we return -1.
 */
fixed CCmpObstructionManager::DistanceToPoint(entity_id_t ent, entity_pos_t px, entity_pos_t pz) const
{
	ObstructionSquare obstruction;
	CmpPtr<ICmpObstruction> cmpObstruction(GetSimContext(), ent);
	if (cmpObstruction && cmpObstruction->GetObstructionSquare(obstruction))
	{
		ObstructionSquare point;
		point.x = px;
		point.z = pz;
		return DistanceBetweenShapes(obstruction, point);
	}

	CmpPtr<ICmpPosition> cmpPosition(GetSimContext(), ent);
	if (!cmpPosition || !cmpPosition->IsInWorld())
		return fixed::FromInt(-1);

	return (CFixedVector2D(cmpPosition->GetPosition2D().X, cmpPosition->GetPosition2D().Y) - CFixedVector2D(px, pz)).Length();
}

fixed CCmpObstructionManager::MaxDistanceToPoint(entity_id_t ent, entity_pos_t px, entity_pos_t pz) const
{
	ObstructionSquare obstruction;
	CmpPtr<ICmpObstruction> cmpObstruction(GetSimContext(), ent);
	if (!cmpObstruction || !cmpObstruction->GetObstructionSquare(obstruction))
	{
		ObstructionSquare point;
		point.x = px;
		point.z = pz;
		return MaxDistanceBetweenShapes(obstruction, point);
	}

	CmpPtr<ICmpPosition> cmpPosition(GetSimContext(), ent);
	if (!cmpPosition || !cmpPosition->IsInWorld())
		return fixed::FromInt(-1);

	return (CFixedVector2D(cmpPosition->GetPosition2D().X, cmpPosition->GetPosition2D().Y) - CFixedVector2D(px, pz)).Length();
}

fixed CCmpObstructionManager::DistanceToTarget(entity_id_t ent, entity_id_t target) const
{
	ObstructionSquare obstruction;
	CmpPtr<ICmpObstruction> cmpObstruction(GetSimContext(), ent);
	if (!cmpObstruction || !cmpObstruction->GetObstructionSquare(obstruction))
	{
		CmpPtr<ICmpPosition> cmpPosition(GetSimContext(), ent);
		if (!cmpPosition || !cmpPosition->IsInWorld())
			return fixed::FromInt(-1);
		return DistanceToPoint(target, cmpPosition->GetPosition2D().X, cmpPosition->GetPosition2D().Y);
	}

	ObstructionSquare target_obstruction;
	CmpPtr<ICmpObstruction> cmpObstructionTarget(GetSimContext(), target);
	if (!cmpObstructionTarget || !cmpObstructionTarget->GetObstructionSquare(target_obstruction))
	{
		CmpPtr<ICmpPosition> cmpPositionTarget(GetSimContext(), target);
		if (!cmpPositionTarget || !cmpPositionTarget->IsInWorld())
			return fixed::FromInt(-1);
		return DistanceToPoint(ent, cmpPositionTarget->GetPosition2D().X, cmpPositionTarget->GetPosition2D().Y);
	}

	return DistanceBetweenShapes(obstruction, target_obstruction);
}

fixed CCmpObstructionManager::MaxDistanceToTarget(entity_id_t ent, entity_id_t target) const
{
	ObstructionSquare obstruction;
	CmpPtr<ICmpObstruction> cmpObstruction(GetSimContext(), ent);
	if (!cmpObstruction || !cmpObstruction->GetObstructionSquare(obstruction))
	{
		CmpPtr<ICmpPosition> cmpPosition(GetSimContext(), ent);
		if (!cmpPosition || !cmpPosition->IsInWorld())
			return fixed::FromInt(-1);
		return MaxDistanceToPoint(target, cmpPosition->GetPosition2D().X, cmpPosition->GetPosition2D().Y);
	}

	ObstructionSquare target_obstruction;
	CmpPtr<ICmpObstruction> cmpObstructionTarget(GetSimContext(), target);
	if (!cmpObstructionTarget || !cmpObstructionTarget->GetObstructionSquare(target_obstruction))
	{
		CmpPtr<ICmpPosition> cmpPositionTarget(GetSimContext(), target);
		if (!cmpPositionTarget || !cmpPositionTarget->IsInWorld())
			return fixed::FromInt(-1);
		return MaxDistanceToPoint(ent, cmpPositionTarget->GetPosition2D().X, cmpPositionTarget->GetPosition2D().Y);
	}

	return MaxDistanceBetweenShapes(obstruction, target_obstruction);
}

fixed CCmpObstructionManager::DistanceBetweenShapes(const ObstructionSquare& source, const ObstructionSquare& target) const
{
	// Sphere-sphere collision.
	if (source.hh == fixed::Zero() && target.hh == fixed::Zero())
		return (CFixedVector2D(target.x, target.z) - CFixedVector2D(source.x, source.z)).Length() - source.hw - target.hw;

	// Square to square.
	if (source.hh != fixed::Zero() && target.hh != fixed::Zero())
		return Geometry::DistanceSquareToSquare(
			CFixedVector2D(target.x, target.z) - CFixedVector2D(source.x, source.z),
			source.u, source.v, CFixedVector2D(source.hw, source.hh),
			target.u, target.v, CFixedVector2D(target.hw, target.hh));

	// To cover both remaining cases, shape a is the square one, shape b is the circular one.
	const ObstructionSquare& a = source.hh == fixed::Zero() ? target : source;
	const ObstructionSquare& b = source.hh == fixed::Zero() ? source : target;
	return Geometry::DistanceToSquare(
		CFixedVector2D(b.x, b.z) - CFixedVector2D(a.x, a.z),
		a.u, a.v, CFixedVector2D(a.hw, a.hh), true) - b.hw;
}

fixed CCmpObstructionManager::MaxDistanceBetweenShapes(const ObstructionSquare& source, const ObstructionSquare& target) const
{
	// Sphere-sphere collision.
	if (source.hh == fixed::Zero() && target.hh == fixed::Zero())
		return (CFixedVector2D(target.x, target.z) - CFixedVector2D(source.x, source.z)).Length() + source.hw + target.hw;

	// Square to square.
	if (source.hh != fixed::Zero() && target.hh != fixed::Zero())
		return Geometry::MaxDistanceSquareToSquare(
			CFixedVector2D(target.x, target.z) - CFixedVector2D(source.x, source.z),
			source.u, source.v, CFixedVector2D(source.hw, source.hh),
			target.u, target.v, CFixedVector2D(target.hw, target.hh));

	// To cover both remaining cases, shape a is the square one, shape b is the circular one.
	const ObstructionSquare& a = source.hh == fixed::Zero() ? target : source;
	const ObstructionSquare& b = source.hh == fixed::Zero() ? source : target;
	return Geometry::MaxDistanceToSquare(
		CFixedVector2D(b.x, b.z) - CFixedVector2D(a.x, a.z),
		a.u, a.v, CFixedVector2D(a.hw, a.hh), true) + b.hw;
}

/**
 * IsInRange function family depending on the DistanceTo family.
 *
 * In range if the edge to edge distance is inferior to maxRange
 * and if the opposite edge to opposite edge distance is greater than minRange when the opposite bool is true
 * or when the opposite bool is false the edge to edge distance is more than minRange.
 *
 * Using the opposite egde for minRange means that a unit is in range of a building if it is farther than
 * clearance-buildingsize, which is generally going to be negative (and thus this returns true).
 * NB: from a game POV, this means units can easily fire on buildings, which is good,
 * but it also means that buildings can easily fire on units. Buildings are usually meant
 * to fire from the edge, not the opposite edge, so this looks odd. For this reason one can choose
 * to set the opposite bool false and use the edge to egde distance.
 *
 * We don't use squares because the are likely to overflow.
 * We use a 0.0001 margin to avoid rounding errors.
 */
bool CCmpObstructionManager::IsInPointRange(entity_id_t ent, entity_pos_t px, entity_pos_t pz, entity_pos_t minRange, entity_pos_t maxRange, bool opposite) const
{
	fixed dist = DistanceToPoint(ent, px, pz);
	// Treat -1 max range as infinite
	return dist != fixed::FromInt(-1) &&
	      (dist <= (maxRange + fixed::FromFloat(0.0001)) || maxRange < fixed::Zero()) &&
	      (opposite ? MaxDistanceToPoint(ent, px, pz) : dist) >= minRange - fixed::FromFloat(0.0001);
}

bool CCmpObstructionManager::IsInTargetRange(entity_id_t ent, entity_id_t target, entity_pos_t minRange, entity_pos_t maxRange, bool opposite) const
{
	fixed dist = DistanceToTarget(ent, target);
	// Treat -1 max range as infinite
	return dist != fixed::FromInt(-1) &&
	      (dist <= (maxRange + fixed::FromFloat(0.0001)) || maxRange < fixed::Zero()) &&
	      (opposite ? MaxDistanceToTarget(ent, target) : dist) >= minRange - fixed::FromFloat(0.0001);
}
bool CCmpObstructionManager::IsPointInPointRange(entity_pos_t x, entity_pos_t z, entity_pos_t px, entity_pos_t pz, entity_pos_t minRange, entity_pos_t maxRange) const
{
	entity_pos_t distance = (CFixedVector2D(x, z) - CFixedVector2D(px, pz)).Length();
	// Treat -1 max range as infinite
	return (distance <= (maxRange + fixed::FromFloat(0.0001)) || maxRange < fixed::Zero()) &&
	        distance >= minRange - fixed::FromFloat(0.0001);
}

bool CCmpObstructionManager::TestLine(const IObstructionTestFilter& filter, entity_pos_t x0, entity_pos_t z0, entity_pos_t x1, entity_pos_t z1, entity_pos_t r, bool relaxClearanceForUnits) const
{
	PROFILE("TestLine");

	// Check that both end points are within the world (which means the whole line must be)
	if (!IsInWorld(x0, z0, r) || !IsInWorld(x1, z1, r))
		return true;

	CFixedVector2D posMin (std::min(x0, x1) - r, std::min(z0, z1) - r);
	CFixedVector2D posMax (std::max(x0, x1) + r, std::max(z0, z1) + r);

	// actual radius used for unit-unit collisions. If relaxClearanceForUnits, will be smaller to allow more overlap.
	entity_pos_t unitUnitRadius = r;
	if (relaxClearanceForUnits)
		unitUnitRadius -= entity_pos_t::FromInt(1)/2;

	std::vector<entity_id_t> unitShapes;
	m_UnitSubdivision.GetInRange(unitShapes, posMin, posMax);
	for (const entity_id_t& shape : unitShapes)
	{
		std::map<u32, UnitShape>::const_iterator it = m_UnitShapes.find(shape);
		ENSURE(it != m_UnitShapes.end());

		if (!filter.TestShape(UNIT_INDEX_TO_TAG(it->first), it->second.flags, it->second.group, INVALID_ENTITY))
			continue;

		CFixedVector2D center(it->second.x, it->second.z);
		CFixedVector2D halfSize(it->second.clearance + unitUnitRadius, it->second.clearance + unitUnitRadius);
		if (Geometry::TestRayAASquare(CFixedVector2D(x0, z0) - center, CFixedVector2D(x1, z1) - center, halfSize))
			return true;
	}

	std::vector<entity_id_t> staticShapes;
	m_StaticSubdivision.GetInRange(staticShapes, posMin, posMax);
	for (const entity_id_t& shape : staticShapes)
	{
		std::map<u32, StaticShape>::const_iterator it = m_StaticShapes.find(shape);
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
	std::vector<entity_id_t>* out) const
{
	PROFILE("TestStaticShape");

	if (out)
		out->clear();

	fixed s, c;
	sincos_approx(a, s, c);
	CFixedVector2D u(c, -s);
	CFixedVector2D v(s, c);
	CFixedVector2D center(x, z);
	CFixedVector2D halfSize(w/2, h/2);
	CFixedVector2D corner1 = u.Multiply(halfSize.X) + v.Multiply(halfSize.Y);
	CFixedVector2D corner2 = u.Multiply(halfSize.X) - v.Multiply(halfSize.Y);

	// Check that all corners are within the world (which means the whole shape must be)
	if (!IsInWorld(center + corner1) || !IsInWorld(center + corner2) ||
	    !IsInWorld(center - corner1) || !IsInWorld(center - corner2))
	{
		if (out)
			out->push_back(INVALID_ENTITY); // no entity ID, so just push an arbitrary marker
		else
			return true;
	}

	fixed bbHalfWidth = std::max(corner1.X.Absolute(), corner2.X.Absolute());
	fixed bbHalfHeight = std::max(corner1.Y.Absolute(), corner2.Y.Absolute());
	CFixedVector2D posMin(x - bbHalfWidth, z - bbHalfHeight);
	CFixedVector2D posMax(x + bbHalfWidth, z + bbHalfHeight);

	std::vector<entity_id_t> unitShapes;
	m_UnitSubdivision.GetInRange(unitShapes, posMin, posMax);
	for (entity_id_t& shape : unitShapes)
	{
		std::map<u32, UnitShape>::const_iterator it = m_UnitShapes.find(shape);
		ENSURE(it != m_UnitShapes.end());

		if (!filter.TestShape(UNIT_INDEX_TO_TAG(it->first), it->second.flags, it->second.group, INVALID_ENTITY))
			continue;

		CFixedVector2D center1(it->second.x, it->second.z);

		if (Geometry::PointIsInSquare(center1 - center, u, v, CFixedVector2D(halfSize.X + it->second.clearance, halfSize.Y + it->second.clearance)))
		{
			if (out)
				out->push_back(it->second.entity);
			else
				return true;
		}
	}

	std::vector<entity_id_t> staticShapes;
	m_StaticSubdivision.GetInRange(staticShapes, posMin, posMax);
	for (entity_id_t& shape : staticShapes)
	{
		std::map<u32, StaticShape>::const_iterator it = m_StaticShapes.find(shape);
		ENSURE(it != m_StaticShapes.end());

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
	entity_pos_t x, entity_pos_t z, entity_pos_t clearance,
	std::vector<entity_id_t>* out) const
{
	PROFILE("TestUnitShape");

	// Check that the shape is within the world
	if (!IsInWorld(x, z, clearance))
	{
		if (out)
			out->push_back(INVALID_ENTITY); // no entity ID, so just push an arbitrary marker
		else
			return true;
	}

	CFixedVector2D center(x, z);
	CFixedVector2D posMin(x - clearance, z - clearance);
	CFixedVector2D posMax(x + clearance, z + clearance);

	std::vector<entity_id_t> unitShapes;
	m_UnitSubdivision.GetInRange(unitShapes, posMin, posMax);
	for (const entity_id_t& shape : unitShapes)
	{
		std::map<u32, UnitShape>::const_iterator it = m_UnitShapes.find(shape);
		ENSURE(it != m_UnitShapes.end());

		if (!filter.TestShape(UNIT_INDEX_TO_TAG(it->first), it->second.flags, it->second.group, INVALID_ENTITY))
			continue;

		entity_pos_t c1 = it->second.clearance;

		if (!(
			it->second.x + c1 < x - clearance ||
			it->second.x - c1 > x + clearance ||
			it->second.z + c1 < z - clearance ||
			it->second.z - c1 > z + clearance))
		{
			if (out)
				out->push_back(it->second.entity);
			else
				return true;
		}
	}

	std::vector<entity_id_t> staticShapes;
	m_StaticSubdivision.GetInRange(staticShapes, posMin, posMax);
	for (const entity_id_t& shape : staticShapes)
	{
		std::map<u32, StaticShape>::const_iterator it = m_StaticShapes.find(shape);
		ENSURE(it != m_StaticShapes.end());

		if (!filter.TestShape(STATIC_INDEX_TO_TAG(it->first), it->second.flags, it->second.group, it->second.group2))
			continue;

		CFixedVector2D center1(it->second.x, it->second.z);
		if (Geometry::PointIsInSquare(center1 - center, it->second.u, it->second.v, CFixedVector2D(it->second.hw + clearance, it->second.hh + clearance)))
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

void CCmpObstructionManager::Rasterize(Grid<NavcellData>& grid, const std::vector<PathfinderPassability>& passClasses, bool fullUpdate)
{
	PROFILE3("Rasterize Obstructions");

	// Cells are only marked as blocked if the whole cell is strictly inside the shape.
	// (That ensures the shape's geometric border is always reachable.)

	// Pass classes will get shapes rasterized on them depending on their Obstruction value.
	// Classes with another value than "pathfinding" should not use Clearance.

	std::map<entity_pos_t, u16> pathfindingMasks;
	u16 foundationMask = 0;
	for (const PathfinderPassability& passability : passClasses)
	{
		switch (passability.m_Obstructions)
		{
		case PathfinderPassability::PATHFINDING:
		{
			std::map<entity_pos_t, u16>::iterator it = pathfindingMasks.find(passability.m_Clearance);
			if (it == pathfindingMasks.end())
				pathfindingMasks[passability.m_Clearance] = passability.m_Mask;
			else
				it->second |= passability.m_Mask;
			break;
		}
		case PathfinderPassability::FOUNDATION:
			foundationMask |= passability.m_Mask;
			break;
		default:
			continue;
		}
	}

	// FLAG_BLOCK_PATHFINDING and FLAG_BLOCK_FOUNDATION are the only flags taken into account by MakeDirty* functions,
	// so they should be the only ones rasterized using with the help of m_Dirty*Shapes vectors.

	for (auto& maskPair : pathfindingMasks)
		RasterizeHelper(grid, FLAG_BLOCK_PATHFINDING, fullUpdate, maskPair.second, maskPair.first);

	RasterizeHelper(grid, FLAG_BLOCK_FOUNDATION, fullUpdate, foundationMask);

	m_DirtyStaticShapes.clear();
	m_DirtyUnitShapes.clear();
}

void CCmpObstructionManager::RasterizeHelper(Grid<NavcellData>& grid, ICmpObstructionManager::flags_t requireMask, bool fullUpdate, pass_class_t appliedMask, entity_pos_t clearance) const
{
	for (auto& pair : m_StaticShapes)
	{
		const StaticShape& shape = pair.second;
		if (!(shape.flags & requireMask))
			continue;

		if (!fullUpdate && std::find(m_DirtyStaticShapes.begin(), m_DirtyStaticShapes.end(), pair.first) == m_DirtyStaticShapes.end())
			continue;

		// TODO: it might be nice to rasterize with rounded corners for large 'expand' values.
		ObstructionSquare square = { shape.x, shape.z, shape.u, shape.v, shape.hw, shape.hh };
		SimRasterize::Spans spans;
		SimRasterize::RasterizeRectWithClearance(spans, square, clearance, Pathfinding::NAVCELL_SIZE);
		for (SimRasterize::Span& span : spans)
		{
			i16 j = Clamp(span.j, (i16)0, (i16)(grid.m_H-1));
			i16 i0 = std::max(span.i0, (i16)0);
			i16 i1 = std::min(span.i1, (i16)grid.m_W);

			for (i16 i = i0; i < i1; ++i)
				grid.set(i, j, grid.get(i, j) | appliedMask);
		}
	}

	for (auto& pair : m_UnitShapes)
	{
		if (!(pair.second.flags & requireMask))
			continue;

		if (!fullUpdate && std::find(m_DirtyUnitShapes.begin(), m_DirtyUnitShapes.end(), pair.first) == m_DirtyUnitShapes.end())
			continue;

		CFixedVector2D center(pair.second.x, pair.second.z);
		entity_pos_t r = pair.second.clearance + clearance;

		u16 i0, j0, i1, j1;
		Pathfinding::NearestNavcell(center.X - r, center.Y - r, i0, j0, grid.m_W, grid.m_H);
		Pathfinding::NearestNavcell(center.X + r, center.Y + r, i1, j1, grid.m_W, grid.m_H);
		for (u16 j = j0+1; j < j1; ++j)
			for (u16 i = i0+1; i < i1; ++i)
				grid.set(i, j, grid.get(i, j) | appliedMask);
	}
}

void CCmpObstructionManager::GetObstructionsInRange(const IObstructionTestFilter& filter, entity_pos_t x0, entity_pos_t z0, entity_pos_t x1, entity_pos_t z1, std::vector<ObstructionSquare>& squares) const
{
	GetUnitObstructionsInRange(filter, x0, z0, x1, z1, squares);
	GetStaticObstructionsInRange(filter, x0, z0, x1, z1, squares);
}

void CCmpObstructionManager::GetUnitObstructionsInRange(const IObstructionTestFilter& filter, entity_pos_t x0, entity_pos_t z0, entity_pos_t x1, entity_pos_t z1, std::vector<ObstructionSquare>& squares) const
{
	PROFILE("GetObstructionsInRange");

	ENSURE(x0 <= x1 && z0 <= z1);

	std::vector<entity_id_t> unitShapes;
	m_UnitSubdivision.GetInRange(unitShapes, CFixedVector2D(x0, z0), CFixedVector2D(x1, z1));
	for (entity_id_t& unitShape : unitShapes)
	{
		std::map<u32, UnitShape>::const_iterator it = m_UnitShapes.find(unitShape);
		ENSURE(it != m_UnitShapes.end());

		if (!filter.TestShape(UNIT_INDEX_TO_TAG(it->first), it->second.flags, it->second.group, INVALID_ENTITY))
			continue;

		entity_pos_t c = it->second.clearance;

		// Skip this object if it's completely outside the requested range
		if (it->second.x + c < x0 || it->second.x - c > x1 || it->second.z + c < z0 || it->second.z - c > z1)
			continue;

		CFixedVector2D u(entity_pos_t::FromInt(1), entity_pos_t::Zero());
		CFixedVector2D v(entity_pos_t::Zero(), entity_pos_t::FromInt(1));
		squares.emplace_back(ObstructionSquare{ it->second.x, it->second.z, u, v, c, c });
	}
}

void CCmpObstructionManager::GetStaticObstructionsInRange(const IObstructionTestFilter& filter, entity_pos_t x0, entity_pos_t z0, entity_pos_t x1, entity_pos_t z1, std::vector<ObstructionSquare>& squares) const
{
	PROFILE("GetObstructionsInRange");

	ENSURE(x0 <= x1 && z0 <= z1);

	std::vector<entity_id_t> staticShapes;
	m_StaticSubdivision.GetInRange(staticShapes, CFixedVector2D(x0, z0), CFixedVector2D(x1, z1));
	for (entity_id_t& staticShape : staticShapes)
	{
		std::map<u32, StaticShape>::const_iterator it = m_StaticShapes.find(staticShape);
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

void CCmpObstructionManager::GetUnitsOnObstruction(const ObstructionSquare& square, std::vector<entity_id_t>& out, const IObstructionTestFilter& filter, bool strict) const
{
	PROFILE("GetUnitsOnObstruction");

	// In order to avoid getting units on impassable cells, we want to find all
	// units subject to the RasterizeRectWithClearance of the building's shape with the
	// unit's clearance covers the navcell the unit is on.

	std::vector<entity_id_t> unitShapes;
	CFixedVector2D center(square.x, square.z);
	CFixedVector2D expandedBox =
		Geometry::GetHalfBoundingBox(square.u, square.v, CFixedVector2D(square.hw, square.hh)) +
		CFixedVector2D(m_MaxClearance, m_MaxClearance);
	m_UnitSubdivision.GetInRange(unitShapes, center - expandedBox, center + expandedBox);

	std::map<entity_pos_t, SimRasterize::Spans> rasterizedRects;

	for (const u32& unitShape : unitShapes)
	{
		std::map<u32, UnitShape>::const_iterator it = m_UnitShapes.find(unitShape);
		ENSURE(it != m_UnitShapes.end());

		const UnitShape& shape = it->second;

		if (!filter.TestShape(UNIT_INDEX_TO_TAG(unitShape), shape.flags, shape.group, INVALID_ENTITY))
			continue;

		if (rasterizedRects.find(shape.clearance) == rasterizedRects.end())
		{
			// The rasterization is an approximation of the real shapes.
			// Depending on your use, you may want to be more or less strict on the rasterization,
			// ie this may either return some units that aren't actually on the shape (if strict is set)
			// or this may not return some units that are on the shape (if strict is not set).
			// Foundations need to be non-strict, as otherwise it sometimes detects the builder units
			// as being on the shape, so it orders them away.
			SimRasterize::Spans& newSpans = rasterizedRects[shape.clearance];
			if (strict)
				SimRasterize::RasterizeRectWithClearance(newSpans, square, shape.clearance, Pathfinding::NAVCELL_SIZE);
			else
				SimRasterize::RasterizeRectWithClearance(newSpans, square, shape.clearance-Pathfinding::CLEARANCE_EXTENSION_RADIUS, Pathfinding::NAVCELL_SIZE);
		}

		SimRasterize::Spans& spans = rasterizedRects[shape.clearance];

		// Check whether the unit's center is on a navcell that's in
		// any of the spans

		u16 i = (shape.x / Pathfinding::NAVCELL_SIZE).ToInt_RoundToNegInfinity();
		u16 j = (shape.z / Pathfinding::NAVCELL_SIZE).ToInt_RoundToNegInfinity();

		for (const SimRasterize::Span& span : spans)
		{
			if (j == span.j && span.i0 <= i && i < span.i1)
			{
				out.push_back(shape.entity);
				break;
			}
		}
	}
}

void CCmpObstructionManager::GetStaticObstructionsOnObstruction(const ObstructionSquare& square, std::vector<entity_id_t>& out, const IObstructionTestFilter& filter) const
{
	PROFILE("GetStaticObstructionsOnObstruction");

	std::vector<entity_id_t> staticShapes;
	CFixedVector2D center(square.x, square.z);
	CFixedVector2D expandedBox = Geometry::GetHalfBoundingBox(square.u, square.v, CFixedVector2D(square.hw, square.hh));
	m_StaticSubdivision.GetInRange(staticShapes, center - expandedBox, center + expandedBox);

	for (const u32& staticShape : staticShapes)
	{
		std::map<u32, StaticShape>::const_iterator it = m_StaticShapes.find(staticShape);
		ENSURE(it != m_StaticShapes.end());

		const StaticShape& shape = it->second;

		if (!filter.TestShape(STATIC_INDEX_TO_TAG(staticShape), shape.flags, shape.group, shape.group2))
			continue;

		if (Geometry::TestSquareSquare(
		    center,
		    square.u,
		    square.v,
		    CFixedVector2D(square.hw, square.hh),
		    CFixedVector2D(shape.x, shape.z),
		    shape.u,
		    shape.v,
		    CFixedVector2D(shape.hw, shape.hh)))
		{
			out.push_back(shape.entity);
		}
	}
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
			SimRender::ConstructSquareOnGround(GetSimContext(), it->second.x.ToFloat(), it->second.z.ToFloat(), it->second.clearance.ToFloat()*2, it->second.clearance.ToFloat()*2, 0, m_DebugOverlayLines.back(), true);
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
