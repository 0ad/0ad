/* Copyright (C) 2022 Wildfire Games.
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
#include "ICmpObstruction.h"

#include "simulation2/MessageTypes.h"
#include "simulation2/components/ICmpObstructionManager.h"
#include "simulation2/components/ICmpTerrain.h"
#include "simulation2/components/ICmpUnitMotion.h"
#include "simulation2/components/ICmpWaterManager.h"
#include "simulation2/serialization/SerializedTypes.h"

#include "ps/CLogger.h"

template<>
struct SerializeHelper<ICmpObstructionManager::tag_t>
{
	template<typename S>
	void operator()(S& serialize, const char* UNUSED(name), Serialize::qualify<S, ICmpObstructionManager::tag_t> value)
	{
		serialize.NumberU32_Unbounded("tag", value.n);
	}
};

/**
 * Obstruction implementation. This keeps the ICmpPathfinder's model of the world updated when the
 * entities move and die, with shapes derived from ICmpFootprint.
 */
class CCmpObstruction final : public ICmpObstruction
{
public:
	static void ClassInit(CComponentManager& componentManager)
	{
		componentManager.SubscribeToMessageType(MT_PositionChanged);
		componentManager.SubscribeToMessageType(MT_Destroy);
	}

	DEFAULT_COMPONENT_ALLOCATOR(Obstruction)

	typedef ICmpObstructionManager::tag_t tag_t;
	typedef ICmpObstructionManager::flags_t flags_t;

	// Template state:

	EObstructionType m_Type;

	entity_pos_t m_Size0; // radius or width
	entity_pos_t m_Size1; // radius or depth
	flags_t m_TemplateFlags;
	entity_pos_t m_Clearance;

	typedef struct {
		entity_pos_t dx, dz;
		entity_angle_t da;
		entity_pos_t size0, size1;
		flags_t flags;
	} Shape;

	std::vector<Shape> m_Shapes;

	// Dynamic state:

	/// Whether the obstruction is actively obstructing or just an inactive placeholder.
	bool m_Active;
	/// Whether the entity associated with this obstruction is currently moving. Only applicable for
	/// UNIT-type obstructions.
	bool m_Moving;
	/// Whether an obstruction's control group should be kept consistent and
	/// used to set control groups for entities that collide with it.
	bool m_ControlPersist;

	// WORKAROUND: While processing Destroy messages, the obstruction component may receive messages
	// that make it re-enable the obstruction, thus leaving behind dangling obstructions.
	// To avoid that, if this is true, _never_ reactivate the obstruction.
	bool m_IsDestroyed = false;

	/**
	 * Primary control group identifier. Indicates to which control group this entity's shape belongs.
	 * Typically used in combination with obstruction test filters to have member shapes ignore each
	 * other during obstruction tests. Defaults to the entity's ID. Must never be set to INVALID_ENTITY.
	 */
	entity_id_t m_ControlGroup;

	/**
	 * Optional secondary control group identifier. Similar to m_ControlGroup; if set to a valid value,
	 * then this field identifies an additional, secondary control group to which this entity's shape
	 * belongs. Set to INVALID_ENTITY to not assign any secondary group. Defaults to INVALID_ENTITY.
	 *
	 * These are only necessary in case it is not sufficient for an entity to belong to only one control
	 * group. Otherwise, they can be ignored.
	 */
	entity_id_t m_ControlGroup2;

	/// Identifier of this entity's obstruction shape, as registered in the obstruction manager. Contains
	/// structure, but should be treated as opaque here.
	tag_t m_Tag;
	std::vector<tag_t> m_ClusterTags;

	/// Set of flags affecting the behaviour of this entity's obstruction shape.
	flags_t m_Flags;

	static std::string GetSchema()
	{
		return
			"<a:example/>"
			"<a:help>Causes this entity to obstruct the motion of other units.</a:help>"
			"<choice>"
				"<element name='Static'>"
					"<attribute name='width'>"
						"<data type='decimal'>"
							"<param name='minInclusive'>1.5</param>"
						"</data>"
					"</attribute>"
					"<attribute name='depth'>"
						"<data type='decimal'>"
							"<param name='minInclusive'>1.5</param>"
						"</data>"
					"</attribute>"
				"</element>"
				"<element name='Unit'>"
					"<empty/>"
				"</element>"
				"<element name='Obstructions'>"
					"<zeroOrMore>"
						"<element>"
							"<anyName/>"
							"<optional>"
								"<attribute name='x'>"
									"<data type='decimal'/>"
								"</attribute>"
							"</optional>"
							"<optional>"
								"<attribute name='z'>"
									"<data type='decimal'/>"
								"</attribute>"
							"</optional>"
							"<attribute name='width'>"
								"<data type='decimal'>"
									"<param name='minInclusive'>1.5</param>"
								"</data>"
							"</attribute>"
							"<attribute name='depth'>"
								"<data type='decimal'>"
									"<param name='minInclusive'>1.5</param>"
								"</data>"
							"</attribute>"
						"</element>"
					"</zeroOrMore>"
				"</element>"
			"</choice>"
			"<element name='Active' a:help='If false, this entity will be ignored in collision tests by other units but can still perform its own collision tests'>"
				"<data type='boolean'/>"
			"</element>"
			"<element name='BlockMovement' a:help='Whether units should be allowed to walk through this entity'>"
				"<data type='boolean'/>"
			"</element>"
			"<element name='BlockPathfinding' a:help='Whether the long-distance pathfinder should avoid paths through this entity. This should only be set for large stationary obstructions'>"
				"<data type='boolean'/>"
			"</element>"
			"<element name='BlockFoundation' a:help='Whether players should be unable to place building foundations on top of this entity. If true, BlockConstruction should be true too'>"
				"<data type='boolean'/>"
			"</element>"
			"<element name='BlockConstruction' a:help='Whether players should be unable to begin constructing buildings placed on top of this entity'>"
				"<data type='boolean'/>"
			"</element>"
			"<element name='DeleteUponConstruction' a:help='Whether this entity should be deleted when construction on a buildings placed on top of this entity is started.'>"
				"<data type='boolean'/>"
			"</element>"
			"<element name='DisableBlockMovement' a:help='If true, BlockMovement will be overridden and treated as false. (This is a special case to handle foundations)'>"
				"<data type='boolean'/>"
			"</element>"
			"<element name='DisableBlockPathfinding' a:help='If true, BlockPathfinding will be overridden and treated as false. (This is a special case to handle foundations)'>"
				"<data type='boolean'/>"
			"</element>"
			"<optional>"
				"<element name='ControlPersist' a:help='If present, the control group of this entity will be given to entities that are colliding with it.'>"
					"<empty/>"
				"</element>"
			"</optional>";
	}

	void Init(const CParamNode& paramNode) override
	{
		// The minimum obstruction size is the navcell size * sqrt(2)
		// This is enforced in the schema as a minimum of 1.5
		fixed minObstruction = (Pathfinding::NAVCELL_SIZE.Square() * 2).Sqrt();
		m_TemplateFlags = 0;
		if (paramNode.GetChild("BlockMovement").ToBool())
			m_TemplateFlags |= ICmpObstructionManager::FLAG_BLOCK_MOVEMENT;
		if (paramNode.GetChild("BlockPathfinding").ToBool())
			m_TemplateFlags |= ICmpObstructionManager::FLAG_BLOCK_PATHFINDING;
		if (paramNode.GetChild("BlockFoundation").ToBool())
			m_TemplateFlags |= ICmpObstructionManager::FLAG_BLOCK_FOUNDATION;
		if (paramNode.GetChild("BlockConstruction").ToBool())
			m_TemplateFlags |= ICmpObstructionManager::FLAG_BLOCK_CONSTRUCTION;
		if (paramNode.GetChild("DeleteUponConstruction").ToBool())
			m_TemplateFlags |= ICmpObstructionManager::FLAG_DELETE_UPON_CONSTRUCTION;

		m_Flags = m_TemplateFlags;
		if (paramNode.GetChild("DisableBlockMovement").ToBool())
			m_Flags &= (flags_t)(~ICmpObstructionManager::FLAG_BLOCK_MOVEMENT);
		if (paramNode.GetChild("DisableBlockPathfinding").ToBool())
			m_Flags &= (flags_t)(~ICmpObstructionManager::FLAG_BLOCK_PATHFINDING);

		if (paramNode.GetChild("Unit").IsOk())
		{
			m_Type = UNIT;

			CmpPtr<ICmpUnitMotion> cmpUnitMotion(GetEntityHandle());
			if (cmpUnitMotion)
				m_Clearance = cmpUnitMotion->GetUnitClearance();
		}
		else if (paramNode.GetChild("Static").IsOk())
		{
			m_Type = STATIC;
			m_Size0 = paramNode.GetChild("Static").GetChild("@width").ToFixed();
			m_Size1 = paramNode.GetChild("Static").GetChild("@depth").ToFixed();
			ENSURE(m_Size0 > minObstruction);
			ENSURE(m_Size1 > minObstruction);
		}
		else
		{
			m_Type = CLUSTER;
			CFixedVector2D max = CFixedVector2D(fixed::FromInt(0), fixed::FromInt(0));
			CFixedVector2D min = CFixedVector2D(fixed::FromInt(0), fixed::FromInt(0));
			const CParamNode::ChildrenMap& clusterMap = paramNode.GetChild("Obstructions").GetChildren();
			for(CParamNode::ChildrenMap::const_iterator it = clusterMap.begin(); it != clusterMap.end(); ++it)
			{
				Shape b;
				b.size0 = it->second.GetChild("@width").ToFixed();
				b.size1 = it->second.GetChild("@depth").ToFixed();
				ENSURE(b.size0 > minObstruction);
				ENSURE(b.size1 > minObstruction);
				b.dx = it->second.GetChild("@x").ToFixed();
				b.dz = it->second.GetChild("@z").ToFixed();
				b.da = entity_angle_t::FromInt(0);
				b.flags = m_Flags;
				m_Shapes.push_back(b);
				max.X = std::max(max.X, b.dx + b.size0/2);
				max.Y = std::max(max.Y, b.dz + b.size1/2);
				min.X = std::min(min.X, b.dx - b.size0/2);
				min.Y = std::min(min.Y, b.dz - b.size1/2);
			}
			m_Size0 = fixed::FromInt(2).Multiply(std::max(max.X, -min.X));
			m_Size1 = fixed::FromInt(2).Multiply(std::max(max.Y, -min.Y));
		}

		m_Active = paramNode.GetChild("Active").ToBool();
		m_ControlPersist = paramNode.GetChild("ControlPersist").IsOk();

		m_Tag = tag_t();
		if (m_Type == CLUSTER)
			m_ClusterTags.clear();
		m_Moving = false;
		m_ControlGroup = GetEntityId();
		m_ControlGroup2 = INVALID_ENTITY;
	}

	void Deinit() override
	{
	}

	template<typename S>
	void SerializeCommon(S& serialize)
	{
		serialize.Bool("active", m_Active);
		serialize.Bool("moving", m_Moving);
		serialize.NumberU32_Unbounded("control group", m_ControlGroup);
		serialize.NumberU32_Unbounded("control group 2", m_ControlGroup2);
		serialize.NumberU32_Unbounded("tag", m_Tag.n);
		serialize.NumberU8_Unbounded("flags", m_Flags);
		if (m_Type == CLUSTER)
			Serializer(serialize, "cluster tags", m_ClusterTags);
		if (m_Type == UNIT)
			serialize.NumberFixed_Unbounded("clearance", m_Clearance);
	}

	void Serialize(ISerializer& serialize) override
	{
		SerializeCommon(serialize);
	}

	void Deserialize(const CParamNode& paramNode, IDeserializer& deserialize) override
	{
		Init(paramNode);

		SerializeCommon(deserialize);
	}

	void HandleMessage(const CMessage& msg, bool UNUSED(global)) override
	{
		switch (msg.GetType())
		{
		case MT_PositionChanged:
		{
			if (!m_Active || m_IsDestroyed)
				break;

			const CMessagePositionChanged& data = static_cast<const CMessagePositionChanged&> (msg);

			if (!data.inWorld && !m_Tag.valid())
				break; // nothing needs to change

			CmpPtr<ICmpObstructionManager> cmpObstructionManager(GetSystemEntity());
			if (!cmpObstructionManager)
				break; // error

			if (data.inWorld && m_Tag.valid())
			{
				cmpObstructionManager->MoveShape(m_Tag, data.x, data.z, data.a);

				if (m_Type == CLUSTER)
				{
					for (size_t i = 0; i < m_Shapes.size(); ++i)
					{
						Shape& b = m_Shapes[i];
						fixed s, c;
						sincos_approx(data.a, s, c);
						cmpObstructionManager->MoveShape(m_ClusterTags[i], data.x + b.dx.Multiply(c) + b.dz.Multiply(s), data.z + b.dz.Multiply(c) - b.dx.Multiply(s), data.a + b.da);
					}
				}
			}
			else if (data.inWorld && !m_Tag.valid())
			{
				// Need to create a new pathfinder shape:
				if (m_Type == STATIC)
					m_Tag = cmpObstructionManager->AddStaticShape(GetEntityId(),
						data.x, data.z, data.a, m_Size0, m_Size1, m_Flags, m_ControlGroup, m_ControlGroup2);
				else if (m_Type == UNIT)
					m_Tag = cmpObstructionManager->AddUnitShape(GetEntityId(),
						data.x, data.z, m_Clearance, (flags_t)(m_Flags | (m_Moving ? ICmpObstructionManager::FLAG_MOVING : 0)), m_ControlGroup);
				else
					AddClusterShapes(data.x, data.x, data.a);
			}
			else if (!data.inWorld && m_Tag.valid())
			{
				cmpObstructionManager->RemoveShape(m_Tag);
				m_Tag = tag_t();
				if(m_Type == CLUSTER)
					RemoveClusterShapes();
			}
			break;
		}
		case MT_Destroy:
		{
			// Mark the obstruction as destroyed to prevent reactivating it after this point
			m_IsDestroyed = true;
			m_Active = false;

			if (m_Tag.valid())
			{
				CmpPtr<ICmpObstructionManager> cmpObstructionManager(GetSystemEntity());
				if (!cmpObstructionManager)
					break; // error

				cmpObstructionManager->RemoveShape(m_Tag);
				m_Tag = tag_t();
				if(m_Type == CLUSTER)
					RemoveClusterShapes();
			}
			break;
		}
		}
	}

	void SetActive(bool active) override
	{
		if (active && !m_Active && !m_IsDestroyed)
		{
			m_Active = true;

			// Construct the obstruction shape

			CmpPtr<ICmpObstructionManager> cmpObstructionManager(GetSystemEntity());
			if (!cmpObstructionManager)
				return; // error

			CmpPtr<ICmpPosition> cmpPosition(GetEntityHandle());
			if (!cmpPosition)
				return; // error

			if (!cmpPosition->IsInWorld())
				return; // don't need an obstruction

			// TODO: code duplication from message handlers
			CFixedVector2D pos = cmpPosition->GetPosition2D();
			if (m_Type == STATIC)
				m_Tag = cmpObstructionManager->AddStaticShape(GetEntityId(),
					pos.X, pos.Y, cmpPosition->GetRotation().Y, m_Size0, m_Size1, m_Flags, m_ControlGroup, m_ControlGroup2);
			else if (m_Type == UNIT)
				m_Tag = cmpObstructionManager->AddUnitShape(GetEntityId(),
					pos.X, pos.Y, m_Clearance, (flags_t)(m_Flags | (m_Moving ? ICmpObstructionManager::FLAG_MOVING : 0)), m_ControlGroup);
			else
				AddClusterShapes(pos.X, pos.Y, cmpPosition->GetRotation().Y);

			// Used by UnitMotion to activate/deactivate pushing
			if (m_TemplateFlags & ICmpObstructionManager::FLAG_BLOCK_MOVEMENT)
			{
				CMessageMovementObstructionChanged msg;
				GetSimContext().GetComponentManager().PostMessage(GetEntityId(), msg);
			}
		}
		else if (!active && m_Active)
		{
			m_Active = false;

			// Delete the obstruction shape

			// TODO: code duplication from message handlers
			if (m_Tag.valid())
			{
				CmpPtr<ICmpObstructionManager> cmpObstructionManager(GetSystemEntity());
				if (!cmpObstructionManager)
					return; // error

				cmpObstructionManager->RemoveShape(m_Tag);
				m_Tag = tag_t();
				if (m_Type == CLUSTER)
					RemoveClusterShapes();
			}

			// Used by UnitMotion to activate/deactivate pushing
			if (m_TemplateFlags & ICmpObstructionManager::FLAG_BLOCK_MOVEMENT)
			{
				CMessageMovementObstructionChanged msg;
				GetSimContext().GetComponentManager().PostMessage(GetEntityId(), msg);
			}
		}
		// else we didn't change the active status
	}

	void SetDisableBlockMovementPathfinding(bool movementDisabled, bool pathfindingDisabled, int32_t shape) override
	{
		flags_t *flags = NULL;
		if (shape == -1)
			flags = &m_Flags;
		else if (m_Type == CLUSTER && shape < (int32_t)m_Shapes.size())
			flags = &m_Shapes[shape].flags;
		else
			return; // error

		// Remove the blocking / pathfinding flags or
		// Add the blocking / pathfinding flags if the template had enabled them
		if (movementDisabled)
			*flags &= (flags_t)(~ICmpObstructionManager::FLAG_BLOCK_MOVEMENT);
		else
			*flags |= (flags_t)(m_TemplateFlags & ICmpObstructionManager::FLAG_BLOCK_MOVEMENT);
		if (pathfindingDisabled)
			*flags &= (flags_t)(~ICmpObstructionManager::FLAG_BLOCK_PATHFINDING);
		else
			*flags |= (flags_t)(m_TemplateFlags & ICmpObstructionManager::FLAG_BLOCK_PATHFINDING);

		// Reset the shape with the new flags (kind of inefficiently - we
		// should have a ICmpObstructionManager::SetFlags function or something)
		if (m_Active)
		{
			SetActive(false);
			SetActive(true);
		}
	}

	bool GetBlockMovementFlag(bool templateOnly) const override
	{
		return m_Active && ((templateOnly ? m_TemplateFlags : m_Flags) & ICmpObstructionManager::FLAG_BLOCK_MOVEMENT) != 0;
	}

	EObstructionType GetObstructionType() const override
	{
		return m_Type;
	}

	ICmpObstructionManager::tag_t GetObstruction() const override
	{
		return m_Tag;
	}

	bool GetPreviousObstructionSquare(ICmpObstructionManager::ObstructionSquare& out) const override
	{
		return GetObstructionSquare(out, true);
	}

	bool GetObstructionSquare(ICmpObstructionManager::ObstructionSquare& out) const override
	{
		return GetObstructionSquare(out, false);
	}

	virtual bool GetObstructionSquare(ICmpObstructionManager::ObstructionSquare& out, bool previousPosition) const
	{
		CmpPtr<ICmpPosition> cmpPosition(GetEntityHandle());
		if (!cmpPosition)
			return false; // error

		CmpPtr<ICmpObstructionManager> cmpObstructionManager(GetSystemEntity());
		if (!cmpObstructionManager)
			return false; // error

		if (!cmpPosition->IsInWorld())
			return false; // no obstruction square

		CFixedVector2D pos;
		if (previousPosition)
			pos = cmpPosition->GetPreviousPosition2D();
		else
			pos = cmpPosition->GetPosition2D();
		if (m_Type == UNIT)
			out = cmpObstructionManager->GetUnitShapeObstruction(pos.X, pos.Y, m_Clearance);
		else
			out = cmpObstructionManager->GetStaticShapeObstruction(pos.X, pos.Y, cmpPosition->GetRotation().Y, m_Size0, m_Size1);
		return true;
	}

	entity_pos_t GetSize() const override
	{
		if (m_Type == UNIT)
			return m_Clearance;
		else
			return CFixedVector2D(m_Size0 / 2, m_Size1 / 2).Length();
	}

	CFixedVector2D GetStaticSize() const override
	{
		return m_Type == STATIC ? CFixedVector2D(m_Size0, m_Size1) : CFixedVector2D();
	}

	void SetUnitClearance(const entity_pos_t& clearance) override
	{
		// This doesn't send a MovementObstructionChanged message
		// because it's a just a workaround init order, and used in UnitMotion directly.
		if (m_Type == UNIT)
			m_Clearance = clearance;
	}

	bool IsControlPersistent() const override
	{
		return m_ControlPersist;
	}

	bool CheckShorePlacement() const override
	{
		ICmpObstructionManager::ObstructionSquare s;
		if (!GetObstructionSquare(s))
			return false;

		CFixedVector2D front = CFixedVector2D(s.x, s.z) + s.v.Multiply(s.hh);
		CFixedVector2D  back = CFixedVector2D(s.x, s.z) - s.v.Multiply(s.hh);

		CmpPtr<ICmpTerrain> cmpTerrain(GetSystemEntity());
		CmpPtr<ICmpWaterManager> cmpWaterManager(GetSystemEntity());
		if (!cmpTerrain || !cmpWaterManager)
			return false;

		// Keep these constants in agreement with the pathfinder.
		return cmpWaterManager->GetWaterLevel(front.X, front.Y) - cmpTerrain->GetGroundLevel(front.X, front.Y) > fixed::FromInt(1) &&
		       cmpWaterManager->GetWaterLevel( back.X,  back.Y) - cmpTerrain->GetGroundLevel( back.X,  back.Y) < fixed::FromInt(2);
	}

	EFoundationCheck CheckFoundation(const std::string& className) const override
	{
		return  CheckFoundation(className, false);
	}

	EFoundationCheck CheckFoundation(const std::string& className, bool onlyCenterPoint) const override
	{
		CmpPtr<ICmpPosition> cmpPosition(GetEntityHandle());
		if (!cmpPosition)
			return FOUNDATION_CHECK_FAIL_ERROR; // error

		if (!cmpPosition->IsInWorld())
			return FOUNDATION_CHECK_FAIL_NO_OBSTRUCTION; // no obstruction

		CFixedVector2D pos = cmpPosition->GetPosition2D();

		CmpPtr<ICmpPathfinder> cmpPathfinder(GetSystemEntity());
		if (!cmpPathfinder)
			return FOUNDATION_CHECK_FAIL_ERROR; // error

		// required precondition to use SkipControlGroupsRequireFlagObstructionFilter
		if (m_ControlGroup == INVALID_ENTITY)
		{
			LOGERROR("[CmpObstruction] Cannot test for foundation obstructions; primary control group must be valid");
			return FOUNDATION_CHECK_FAIL_ERROR;
		}

		// Get passability class
		pass_class_t passClass = cmpPathfinder->GetPassabilityClass(className);

		// Ignore collisions within the same control group, or with other non-foundation-blocking shapes.
		// Note that, since the control group for each entity defaults to the entity's ID, this is typically
		// equivalent to only ignoring the entity's own shape and other non-foundation-blocking shapes.
		SkipControlGroupsRequireFlagObstructionFilter filter(m_ControlGroup, m_ControlGroup2,
			ICmpObstructionManager::FLAG_BLOCK_FOUNDATION);

		if (m_Type == UNIT)
			return cmpPathfinder->CheckUnitPlacement(filter, pos.X, pos.Y, m_Clearance, passClass, onlyCenterPoint);
		else
			return cmpPathfinder->CheckBuildingPlacement(filter, pos.X, pos.Y, cmpPosition->GetRotation().Y, m_Size0, m_Size1, GetEntityId(), passClass, onlyCenterPoint);
	}

	bool CheckDuplicateFoundation() const override
	{
		CmpPtr<ICmpPosition> cmpPosition(GetEntityHandle());
		if (!cmpPosition)
			return false; // error

		if (!cmpPosition->IsInWorld())
			return false; // no obstruction

		CFixedVector2D pos = cmpPosition->GetPosition2D();

		CmpPtr<ICmpObstructionManager> cmpObstructionManager(GetSystemEntity());
		if (!cmpObstructionManager)
			return false; // error

		// required precondition to use SkipControlGroupsRequireFlagObstructionFilter
		if (m_ControlGroup == INVALID_ENTITY)
		{
			LOGERROR("[CmpObstruction] Cannot test for foundation obstructions; primary control group must be valid");
			return false;
		}

		// Ignore collisions with entities unless they block foundations and match both control groups.
		SkipTagRequireControlGroupsAndFlagObstructionFilter filter(m_Tag, m_ControlGroup, m_ControlGroup2,
			ICmpObstructionManager::FLAG_BLOCK_FOUNDATION);

		if (m_Type == UNIT)
			return !cmpObstructionManager->TestUnitShape(filter, pos.X, pos.Y, m_Clearance, NULL);
		else
			return !cmpObstructionManager->TestStaticShape(filter, pos.X, pos.Y, cmpPosition->GetRotation().Y, m_Size0, m_Size1, NULL );
	}

	std::vector<entity_id_t> GetEntitiesByFlags(flags_t flags) const override
	{
		std::vector<entity_id_t> ret;

		CmpPtr<ICmpObstructionManager> cmpObstructionManager(GetSystemEntity());
		if (!cmpObstructionManager)
			return ret; // error

		// Ignore collisions within the same control group, or with other shapes that don't match the filter.
		// Note that, since the control group for each entity defaults to the entity's ID, this is typically
		// equivalent to only ignoring the entity's own shape and other shapes that don't match the filter.
		SkipControlGroupsRequireFlagObstructionFilter filter(false, m_ControlGroup, m_ControlGroup2, flags);

		ICmpObstructionManager::ObstructionSquare square;
		if (!GetObstructionSquare(square))
			return ret; // error

		cmpObstructionManager->GetUnitsOnObstruction(square, ret, filter, false);
		cmpObstructionManager->GetStaticObstructionsOnObstruction(square, ret, filter);

		return ret;
	}

	std::vector<entity_id_t> GetEntitiesBlockingMovement() const override
	{
		return GetEntitiesByFlags(ICmpObstructionManager::FLAG_BLOCK_MOVEMENT);
	}

	std::vector<entity_id_t> GetEntitiesBlockingConstruction() const override
	{
		return GetEntitiesByFlags(ICmpObstructionManager::FLAG_BLOCK_CONSTRUCTION);
	}

	std::vector<entity_id_t> GetEntitiesDeletedUponConstruction() const override
	{
		return GetEntitiesByFlags(ICmpObstructionManager::FLAG_DELETE_UPON_CONSTRUCTION);
	}

	void SetMovingFlag(bool enabled) override
	{
		m_Moving = enabled;

		if (m_Tag.valid() && m_Type == UNIT)
		{
			CmpPtr<ICmpObstructionManager> cmpObstructionManager(GetSystemEntity());
			if (cmpObstructionManager)
				cmpObstructionManager->SetUnitMovingFlag(m_Tag, m_Moving);
		}
	}

	void SetControlGroup(entity_id_t group) override
	{
		m_ControlGroup = group;
		UpdateControlGroups();
	}

	void SetControlGroup2(entity_id_t group2) override
	{
		m_ControlGroup2 = group2;
		UpdateControlGroups();
	}

	entity_id_t GetControlGroup() const override
	{
		return m_ControlGroup;
	}

	entity_id_t GetControlGroup2() const override
	{
		return m_ControlGroup2;
	}

	void UpdateControlGroups()
	{
		if (m_Tag.valid())
		{
			CmpPtr<ICmpObstructionManager> cmpObstructionManager(GetSystemEntity());
			if (cmpObstructionManager)
			{
				if (m_Type == UNIT)
				{
					cmpObstructionManager->SetUnitControlGroup(m_Tag, m_ControlGroup);
				}
				else if (m_Type == STATIC)
				{
					cmpObstructionManager->SetStaticControlGroup(m_Tag, m_ControlGroup, m_ControlGroup2);
				}
				else
				{
					cmpObstructionManager->SetStaticControlGroup(m_Tag, m_ControlGroup, m_ControlGroup2);
					for (size_t i = 0; i < m_ClusterTags.size(); ++i)
					{
						cmpObstructionManager->SetStaticControlGroup(m_ClusterTags[i], m_ControlGroup, m_ControlGroup2);
					}
				}
			}
		}
	}

	void ResolveFoundationCollisions() const override
	{
		if (m_Type == UNIT)
			return;

		CmpPtr<ICmpObstructionManager> cmpObstructionManager(GetSystemEntity());
		if (!cmpObstructionManager)
			return;

		CmpPtr<ICmpPosition> cmpPosition(GetEntityHandle());
		if (!cmpPosition)
			return; // error

		if (!cmpPosition->IsInWorld())
			return; // no obstruction

		CFixedVector2D pos = cmpPosition->GetPosition2D();

		// Ignore collisions within the same control group, or with other non-foundation-blocking shapes.
		// Note that, since the control group for each entity defaults to the entity's ID, this is typically
		// equivalent to only ignoring the entity's own shape and other non-foundation-blocking shapes.
		SkipControlGroupsRequireFlagObstructionFilter filter(m_ControlGroup, m_ControlGroup2,
			ICmpObstructionManager::FLAG_BLOCK_FOUNDATION);

		std::vector<entity_id_t> collisions;
		if (cmpObstructionManager->TestStaticShape(filter, pos.X, pos.Y, cmpPosition->GetRotation().Y, m_Size0, m_Size1, &collisions))
		{
			std::vector<entity_id_t> persistentEnts, normalEnts;

			if (m_ControlPersist)
				persistentEnts.push_back(m_ControlGroup);
			else
				normalEnts.push_back(GetEntityId());

			for (std::vector<entity_id_t>::iterator it = collisions.begin(); it != collisions.end(); ++it)
			{
				entity_id_t ent = *it;
				if (ent == INVALID_ENTITY)
					continue;

				CmpPtr<ICmpObstruction> cmpObstruction(GetSimContext(), ent);
				if (!cmpObstruction->IsControlPersistent())
					normalEnts.push_back(ent);
				else
					persistentEnts.push_back(cmpObstruction->GetControlGroup());
			}

			// The collision can't be resolved without usable persistent control groups.
			if (persistentEnts.empty())
				return;

			// Attempt to replace colliding entities' control groups with a persistent one.
			for (const entity_id_t normalEnt : normalEnts)
			{
				CmpPtr<ICmpObstruction> cmpObstruction(GetSimContext(), normalEnt);
				for (const entity_id_t persistent : normalEnts)
				{
					entity_id_t group = cmpObstruction->GetControlGroup();

					// Only clobber 'default' control groups.
					if (group == normalEnt)
						cmpObstruction->SetControlGroup(persistent);
					else if (cmpObstruction->GetControlGroup2() == INVALID_ENTITY && group != persistent)
						cmpObstruction->SetControlGroup2(persistent);
				}
			}
		}
	}
protected:

	inline void AddClusterShapes(entity_pos_t x, entity_pos_t z, entity_angle_t a)
	{
		CmpPtr<ICmpObstructionManager> cmpObstructionManager(GetSystemEntity());
		if (!cmpObstructionManager)
			return; // error

		flags_t flags = m_Flags;
		// Disable block movement and block pathfinding for the obstruction shape
		flags &= (flags_t)(~ICmpObstructionManager::FLAG_BLOCK_MOVEMENT);
		flags &= (flags_t)(~ICmpObstructionManager::FLAG_BLOCK_PATHFINDING);

		m_Tag = cmpObstructionManager->AddStaticShape(GetEntityId(),
			x, z, a, m_Size0, m_Size1, flags, m_ControlGroup, m_ControlGroup2);

		fixed s, c;
		sincos_approx(a, s, c);

		for (size_t i = 0; i < m_Shapes.size(); ++i)
		{
			Shape& b = m_Shapes[i];
			tag_t tag = cmpObstructionManager->AddStaticShape(GetEntityId(),
				x + b.dx.Multiply(c) + b.dz.Multiply(s), z + b.dz.Multiply(c) - b.dx.Multiply(s), a + b.da, b.size0, b.size1, b.flags, m_ControlGroup, m_ControlGroup2);
			m_ClusterTags.push_back(tag);
		}
	}

	inline void RemoveClusterShapes()
	{
		CmpPtr<ICmpObstructionManager> cmpObstructionManager(GetSystemEntity());
		if (!cmpObstructionManager)
			return; // error

		for (size_t i = 0; i < m_ClusterTags.size(); ++i)
		{
			if (m_ClusterTags[i].valid())
			{
				cmpObstructionManager->RemoveShape(m_ClusterTags[i]);
			}
		}
		m_ClusterTags.clear();
	}

};

REGISTER_COMPONENT_TYPE(Obstruction)
