/* Copyright (C) 2011 Wildfire Games.
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

#include "ICmpObstructionManager.h"
#include "ICmpPosition.h"

#include "simulation2/MessageTypes.h"

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

	// Template state:

	enum {
		STATIC,
		UNIT
	} m_Type;
	entity_pos_t m_Size0; // radius or width
	entity_pos_t m_Size1; // radius or depth
	u8 m_TemplateFlags;

	// Dynamic state:

	bool m_Active; // whether the obstruction is obstructing or just an inactive placeholder
	bool m_Moving;
	entity_id_t m_ControlGroup;
	ICmpObstructionManager::tag_t m_Tag;
	u8 m_Flags;

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
			m_Flags &= ~ICmpObstructionManager::FLAG_BLOCK_MOVEMENT;
		if (paramNode.GetChild("DisableBlockPathfinding").ToBool())
			m_Flags &= ~ICmpObstructionManager::FLAG_BLOCK_PATHFINDING;

		m_Active = paramNode.GetChild("Active").ToBool();

		m_Tag = ICmpObstructionManager::tag_t();
		m_Moving = false;
		m_ControlGroup = GetEntityId();
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
			if (cmpObstructionManager.null())
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
						data.x, data.z, data.a, m_Size0, m_Size1, m_Flags);
				else
					m_Tag = cmpObstructionManager->AddUnitShape(GetEntityId(),
						data.x, data.z, m_Size0, m_Flags | (m_Moving ? ICmpObstructionManager::FLAG_MOVING : 0), m_ControlGroup);
			}
			else if (!data.inWorld && m_Tag.valid())
			{
				cmpObstructionManager->RemoveShape(m_Tag);
				m_Tag = ICmpObstructionManager::tag_t();
			}
			break;
		}
		case MT_Destroy:
		{
			if (m_Tag.valid())
			{
				CmpPtr<ICmpObstructionManager> cmpObstructionManager(GetSimContext(), SYSTEM_ENTITY);
				if (cmpObstructionManager.null())
					break; // error

				cmpObstructionManager->RemoveShape(m_Tag);
				m_Tag = ICmpObstructionManager::tag_t();
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
			if (cmpObstructionManager.null())
				return; // error

			CmpPtr<ICmpPosition> cmpPosition(GetSimContext(), GetEntityId());
			if (cmpPosition.null())
				return; // error

			if (!cmpPosition->IsInWorld())
				return; // don't need an obstruction

			CFixedVector2D pos = cmpPosition->GetPosition2D();
			if (m_Type == STATIC)
				m_Tag = cmpObstructionManager->AddStaticShape(GetEntityId(),
					pos.X, pos.Y, cmpPosition->GetRotation().Y, m_Size0, m_Size1, m_Flags);
			else
				m_Tag = cmpObstructionManager->AddUnitShape(GetEntityId(),
					pos.X, pos.Y, m_Size0, m_Flags | (m_Moving ? ICmpObstructionManager::FLAG_MOVING : 0), m_ControlGroup);
		}
		else if (!active && m_Active)
		{
			m_Active = false;

			// Delete the obstruction shape

			if (m_Tag.valid())
			{
				CmpPtr<ICmpObstructionManager> cmpObstructionManager(GetSimContext(), SYSTEM_ENTITY);
				if (cmpObstructionManager.null())
					return; // error

				cmpObstructionManager->RemoveShape(m_Tag);
				m_Tag = ICmpObstructionManager::tag_t();
			}
		}
		// else we didn't change the active status
	}

	virtual void SetDisableBlockMovementPathfinding(bool disabled)
	{
		if (disabled)
		{
			// Remove the blocking flags
			m_Flags &= ~ICmpObstructionManager::FLAG_BLOCK_MOVEMENT;
			m_Flags &= ~ICmpObstructionManager::FLAG_BLOCK_PATHFINDING;
		}
		else
		{
			// Add the blocking flags if the template had enabled them
			m_Flags |= (m_TemplateFlags & ICmpObstructionManager::FLAG_BLOCK_MOVEMENT);
			m_Flags |= (m_TemplateFlags & ICmpObstructionManager::FLAG_BLOCK_PATHFINDING);
		}

		// Reset the shape with the new flags (kind of inefficiently - we
		// should have a ICmpObstructionManager::SetFlags function or something)
		if (m_Active)
		{
			SetActive(false);
			SetActive(true);
		}
	}

	virtual ICmpObstructionManager::tag_t GetObstruction()
	{
		return m_Tag;
	}

	virtual bool GetObstructionSquare(ICmpObstructionManager::ObstructionSquare& out)
	{
		CmpPtr<ICmpPosition> cmpPosition(GetSimContext(), GetEntityId());
		if (cmpPosition.null())
			return false; // error

		CmpPtr<ICmpObstructionManager> cmpObstructionManager(GetSimContext(), SYSTEM_ENTITY);
		if (cmpObstructionManager.null())
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

	virtual bool CheckFoundationCollisions()
	{
		CmpPtr<ICmpPosition> cmpPosition(GetSimContext(), GetEntityId());
		if (cmpPosition.null())
			return false; // error

		if (!cmpPosition->IsInWorld())
			return false; // no obstruction

		CFixedVector2D pos = cmpPosition->GetPosition2D();

		CmpPtr<ICmpObstructionManager> cmpObstructionManager(GetSimContext(), SYSTEM_ENTITY);
		if (cmpObstructionManager.null())
			return false; // error

		// Ignore collisions with self, or with non-foundation-blocking obstructions
		SkipTagFlagsObstructionFilter filter(m_Tag, ICmpObstructionManager::FLAG_BLOCK_FOUNDATION);

		if (m_Type == STATIC)
			return cmpObstructionManager->TestStaticShape(filter, pos.X, pos.Y, cmpPosition->GetRotation().Y, m_Size0, m_Size1, NULL);
		else
			return cmpObstructionManager->TestUnitShape(filter, pos.X, pos.Y, m_Size0, NULL);
	}

	virtual std::vector<entity_id_t> GetConstructionCollisions()
	{
		std::vector<entity_id_t> ret;

		CmpPtr<ICmpPosition> cmpPosition(GetSimContext(), GetEntityId());
		if (cmpPosition.null())
			return ret; // error

		if (!cmpPosition->IsInWorld())
			return ret; // no obstruction

		CFixedVector2D pos = cmpPosition->GetPosition2D();

		CmpPtr<ICmpObstructionManager> cmpObstructionManager(GetSimContext(), SYSTEM_ENTITY);
		if (cmpObstructionManager.null())
			return ret; // error

		// Ignore collisions with self, or with non-construction-blocking obstructions
		SkipTagFlagsObstructionFilter filter(m_Tag, ICmpObstructionManager::FLAG_BLOCK_CONSTRUCTION);

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
			if (!cmpObstructionManager.null())
				cmpObstructionManager->SetUnitMovingFlag(m_Tag, m_Moving);
		}
	}

	virtual void SetControlGroup(entity_id_t group)
	{
		m_ControlGroup = group;

		if (m_Tag.valid() && m_Type == UNIT)
		{
			CmpPtr<ICmpObstructionManager> cmpObstructionManager(GetSimContext(), SYSTEM_ENTITY);
			if (!cmpObstructionManager.null())
				cmpObstructionManager->SetUnitControlGroup(m_Tag, m_ControlGroup);
		}
	}

};

REGISTER_COMPONENT_TYPE(Obstruction)
