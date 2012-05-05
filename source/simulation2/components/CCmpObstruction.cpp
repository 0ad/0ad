/* Copyright (C) 2012 Wildfire Games.
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

#include "ps/CLogger.h"
#include "simulation2/MessageTypes.h"
#include "simulation2/components/ICmpObstructionManager.h"
#include "simulation2/components/ICmpPosition.h"

/**
 * Obstruction implementation. This keeps the ICmpPathfinder's model of the world updated when the
 * entities move and die, with shapes derived from ICmpFootprint.
 */
class CCmpObstruction : public ICmpObstruction
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

	enum {
		STATIC,
		UNIT
	} m_Type;

	entity_pos_t m_Size0; // radius or width
	entity_pos_t m_Size1; // radius or depth
	flags_t m_TemplateFlags;

	// Dynamic state:

	/// Whether the obstruction is actively obstructing or just an inactive placeholder
	bool m_Active; 
	bool m_Moving;

	/**
	 * Unique identifier for grouping obstruction shapes, typically to have member shapes ignore 
	 * each other during obstruction tests. Defaults to the entity ID.
	 * 
	 * TODO: if needed, perhaps add a mask to specify with respect to which flags members of the
	 * group should ignore each other.
	 */
	entity_id_t m_ControlGroup;
	entity_id_t m_ControlGroup2;

	/// Identifier of this entity's obstruction shape. Contains structure, but should be treated
	/// as opaque here.
	tag_t m_Tag;
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
						"<ref name='positiveDecimal'/>"
					"</attribute>"
					"<attribute name='depth'>"
						"<ref name='positiveDecimal'/>"
					"</attribute>"
				"</element>"
				"<element name='Unit'>"
					"<attribute name='radius'>"
						"<ref name='positiveDecimal'/>"
					"</attribute>"
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
			"<element name='DisableBlockMovement' a:help='If true, BlockMovement will be overridden and treated as false. (This is a special case to handle foundations)'>"
				"<data type='boolean'/>"
			"</element>"
			"<element name='DisableBlockPathfinding' a:help='If true, BlockPathfinding will be overridden and treated as false. (This is a special case to handle foundations)'>"
				"<data type='boolean'/>"
			"</element>";
	}

	virtual void Init(const CParamNode& paramNode)
	{
		if (paramNode.GetChild("Unit").IsOk())
		{
			m_Type = UNIT;
			m_Size0 = m_Size1 = paramNode.GetChild("Unit").GetChild("@radius").ToFixed();
		}
		else
		{
			m_Type = STATIC;
			m_Size0 = paramNode.GetChild("Static").GetChild("@width").ToFixed();
			m_Size1 = paramNode.GetChild("Static").GetChild("@depth").ToFixed();
		}

		m_TemplateFlags = 0;
		if (paramNode.GetChild("BlockMovement").ToBool())
			m_TemplateFlags |= ICmpObstructionManager::FLAG_BLOCK_MOVEMENT;
		if (paramNode.GetChild("BlockPathfinding").ToBool())
			m_TemplateFlags |= ICmpObstructionManager::FLAG_BLOCK_PATHFINDING;
		if (paramNode.GetChild("BlockFoundation").ToBool())
			m_TemplateFlags |= ICmpObstructionManager::FLAG_BLOCK_FOUNDATION;
		if (paramNode.GetChild("BlockConstruction").ToBool())
			m_TemplateFlags |= ICmpObstructionManager::FLAG_BLOCK_CONSTRUCTION;

		m_Flags = m_TemplateFlags;
		if (paramNode.GetChild("DisableBlockMovement").ToBool())
			m_Flags &= (flags_t)(~ICmpObstructionManager::FLAG_BLOCK_MOVEMENT);
		if (paramNode.GetChild("DisableBlockPathfinding").ToBool())
			m_Flags &= (flags_t)(~ICmpObstructionManager::FLAG_BLOCK_PATHFINDING);

		m_Active = paramNode.GetChild("Active").ToBool();

		m_Tag = tag_t();
		m_Moving = false;
		m_ControlGroup = GetEntityId();
		m_ControlGroup2 = INVALID_ENTITY;
	}

	virtual void Deinit()
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
	}

	virtual void Serialize(ISerializer& serialize)
	{
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
		case MT_PositionChanged:
		{
			if (!m_Active)
				break;

			const CMessagePositionChanged& data = static_cast<const CMessagePositionChanged&> (msg);

			if (!data.inWorld && !m_Tag.valid())
				break; // nothing needs to change

			CmpPtr<ICmpObstructionManager> cmpObstructionManager(GetSimContext(), SYSTEM_ENTITY);
			if (!cmpObstructionManager)
				break; // error

			if (data.inWorld && m_Tag.valid())
			{
				cmpObstructionManager->MoveShape(m_Tag, data.x, data.z, data.a);
			}
			else if (data.inWorld && !m_Tag.valid())
			{
				// Need to create a new pathfinder shape:
				if (m_Type == STATIC)
					m_Tag = cmpObstructionManager->AddStaticShape(GetEntityId(),
						data.x, data.z, data.a, m_Size0, m_Size1, m_Flags, m_ControlGroup, m_ControlGroup2);
				else
					m_Tag = cmpObstructionManager->AddUnitShape(GetEntityId(),
						data.x, data.z, m_Size0, (flags_t)(m_Flags | (m_Moving ? ICmpObstructionManager::FLAG_MOVING : 0)), m_ControlGroup);
			}
			else if (!data.inWorld && m_Tag.valid())
			{
				cmpObstructionManager->RemoveShape(m_Tag);
				m_Tag = tag_t();
			}
			break;
		}
		case MT_Destroy:
		{
			if (m_Tag.valid())
			{
				CmpPtr<ICmpObstructionManager> cmpObstructionManager(GetSimContext(), SYSTEM_ENTITY);
				if (!cmpObstructionManager)
					break; // error

				cmpObstructionManager->RemoveShape(m_Tag);
				m_Tag = tag_t();
			}
			break;
		}
		}
	}

	virtual void SetActive(bool active)
	{
		if (active && !m_Active)
		{
			m_Active = true;

			// Construct the obstruction shape

			CmpPtr<ICmpObstructionManager> cmpObstructionManager(GetSimContext(), SYSTEM_ENTITY);
			if (!cmpObstructionManager)
				return; // error

			CmpPtr<ICmpPosition> cmpPosition(GetSimContext(), GetEntityId());
			if (!cmpPosition)
				return; // error

			if (!cmpPosition->IsInWorld())
				return; // don't need an obstruction

			// TODO: code duplication from message handlers
			CFixedVector2D pos = cmpPosition->GetPosition2D();
			if (m_Type == STATIC)
				m_Tag = cmpObstructionManager->AddStaticShape(GetEntityId(),
					pos.X, pos.Y, cmpPosition->GetRotation().Y, m_Size0, m_Size1, m_Flags, m_ControlGroup, m_ControlGroup2);
			else
				m_Tag = cmpObstructionManager->AddUnitShape(GetEntityId(),
					pos.X, pos.Y, m_Size0, (flags_t)(m_Flags | (m_Moving ? ICmpObstructionManager::FLAG_MOVING : 0)), m_ControlGroup);
		}
		else if (!active && m_Active)
		{
			m_Active = false;

			// Delete the obstruction shape

			// TODO: code duplication from message handlers
			if (m_Tag.valid())
			{
				CmpPtr<ICmpObstructionManager> cmpObstructionManager(GetSimContext(), SYSTEM_ENTITY);
				if (!cmpObstructionManager)
					return; // error

				cmpObstructionManager->RemoveShape(m_Tag);
				m_Tag = tag_t();
			}
		}
		// else we didn't change the active status
	}

	virtual void SetDisableBlockMovementPathfinding(bool disabled)
	{
		if (disabled)
		{
			// Remove the blocking flags
			m_Flags &= (flags_t)(~ICmpObstructionManager::FLAG_BLOCK_MOVEMENT);
			m_Flags &= (flags_t)(~ICmpObstructionManager::FLAG_BLOCK_PATHFINDING);
		}
		else
		{
			// Add the blocking flags if the template had enabled them
			m_Flags = (flags_t)(m_Flags | (m_TemplateFlags & ICmpObstructionManager::FLAG_BLOCK_MOVEMENT));
			m_Flags = (flags_t)(m_Flags | (m_TemplateFlags & ICmpObstructionManager::FLAG_BLOCK_PATHFINDING));
		}

		// Reset the shape with the new flags (kind of inefficiently - we
		// should have a ICmpObstructionManager::SetFlags function or something)
		if (m_Active)
		{
			SetActive(false);
			SetActive(true);
		}
	}

	virtual bool GetBlockMovementFlag()
	{
		return (m_TemplateFlags & ICmpObstructionManager::FLAG_BLOCK_MOVEMENT) != 0;
	}

	virtual ICmpObstructionManager::tag_t GetObstruction()
	{
		return m_Tag;
	}

	virtual bool GetObstructionSquare(ICmpObstructionManager::ObstructionSquare& out)
	{
		CmpPtr<ICmpPosition> cmpPosition(GetSimContext(), GetEntityId());
		if (!cmpPosition)
			return false; // error

		CmpPtr<ICmpObstructionManager> cmpObstructionManager(GetSimContext(), SYSTEM_ENTITY);
		if (!cmpObstructionManager)
			return false; // error

		if (!cmpPosition->IsInWorld())
			return false; // no obstruction square

		CFixedVector2D pos = cmpPosition->GetPosition2D();
		if (m_Type == STATIC)
			out = cmpObstructionManager->GetStaticShapeObstruction(pos.X, pos.Y, cmpPosition->GetRotation().Y, m_Size0, m_Size1);
		else
			out = cmpObstructionManager->GetUnitShapeObstruction(pos.X, pos.Y, m_Size0);
		return true;
	}

	virtual entity_pos_t GetUnitRadius()
	{
		if (m_Type == UNIT)
			return m_Size0;
		else
			return entity_pos_t::Zero();
	}

	virtual bool CheckFoundation(std::string className)
	{
		CmpPtr<ICmpPosition> cmpPosition(GetSimContext(), GetEntityId());
		if (!cmpPosition)
			return false; // error

		if (!cmpPosition->IsInWorld())
			return false; // no obstruction

		CFixedVector2D pos = cmpPosition->GetPosition2D();

		CmpPtr<ICmpPathfinder> cmpPathfinder(GetSimContext(), SYSTEM_ENTITY);
		if (!cmpPathfinder)
			return false; // error

		// required precondition to use SkipControlGroupsRequireFlagObstructionFilter
		if (m_ControlGroup == INVALID_ENTITY)
		{
			LOGERROR(L"[CmpObstruction] Cannot test for foundation obstructions; primary control group must be valid");
			return false;
		}

		// Get passability class
		ICmpPathfinder::pass_class_t passClass = cmpPathfinder->GetPassabilityClass(className);

		// Ignore collisions within the same control group, or with other non-foundation-blocking shapes.
		// Note that, since the control group for each entity defaults to the entity's ID, this is typically 
		// equivalent to only ignoring the entity's own shape and other non-foundation-blocking shapes.
		SkipControlGroupsRequireFlagObstructionFilter filter(m_ControlGroup, m_ControlGroup2,
			ICmpObstructionManager::FLAG_BLOCK_FOUNDATION);

		if (m_Type == STATIC)
			return cmpPathfinder->CheckBuildingPlacement(filter, pos.X, pos.Y, cmpPosition->GetRotation().Y, m_Size0, m_Size1, GetEntityId(), passClass);
		else
			return cmpPathfinder->CheckUnitPlacement(filter, pos.X, pos.Y, m_Size0, passClass);
	}

	virtual std::vector<entity_id_t> GetConstructionCollisions()
	{
		std::vector<entity_id_t> ret;

		CmpPtr<ICmpPosition> cmpPosition(GetSimContext(), GetEntityId());
		if (!cmpPosition)
			return ret; // error

		if (!cmpPosition->IsInWorld())
			return ret; // no obstruction

		CFixedVector2D pos = cmpPosition->GetPosition2D();

		CmpPtr<ICmpObstructionManager> cmpObstructionManager(GetSimContext(), SYSTEM_ENTITY);
		if (!cmpObstructionManager)
			return ret; // error

		// required precondition to use SkipControlGroupsRequireFlagObstructionFilter
		if (m_ControlGroup == INVALID_ENTITY)
		{
			LOGERROR(L"[CmpObstruction] Cannot test for construction obstructions; primary control group must be valid");
			return ret;
		}

		// Ignore collisions within the same control group, or with other non-construction-blocking shapes.
		// Note that, since the control group for each entity defaults to the entity's ID, this is typically 
		// equivalent to only ignoring the entity's own shape and other non-construction-blocking shapes. 
		SkipControlGroupsRequireFlagObstructionFilter filter(m_ControlGroup, m_ControlGroup2,
			ICmpObstructionManager::FLAG_BLOCK_CONSTRUCTION);

		if (m_Type == STATIC)
			cmpObstructionManager->TestStaticShape(filter, pos.X, pos.Y, cmpPosition->GetRotation().Y, m_Size0, m_Size1, &ret);
		else
			cmpObstructionManager->TestUnitShape(filter, pos.X, pos.Y, m_Size0, &ret);

		return ret;
	}

	virtual void SetMovingFlag(bool enabled)
	{
		m_Moving = enabled;

		if (m_Tag.valid() && m_Type == UNIT)
		{
			CmpPtr<ICmpObstructionManager> cmpObstructionManager(GetSimContext(), SYSTEM_ENTITY);
			if (cmpObstructionManager)
				cmpObstructionManager->SetUnitMovingFlag(m_Tag, m_Moving);
		}
	}

	virtual void SetControlGroup(entity_id_t group)
	{
		m_ControlGroup = group;
		UpdateControlGroups();
	}

	virtual void SetControlGroup2(entity_id_t group2)
	{
		m_ControlGroup2 = group2;
		UpdateControlGroups();
	}

	virtual entity_id_t GetControlGroup() 
	{
		return m_ControlGroup;
	}

	virtual entity_id_t GetControlGroup2() 
	{
		return m_ControlGroup2;
	}

	void UpdateControlGroups()
	{
		if (m_Tag.valid())
		{
			CmpPtr<ICmpObstructionManager> cmpObstructionManager(GetSimContext(), SYSTEM_ENTITY);
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
			}
		}
	}

};

REGISTER_COMPONENT_TYPE(Obstruction)
