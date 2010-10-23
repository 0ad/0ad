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
	bool m_Active; // whether the obstruction is obstructing or just an inactive placeholder

	// Dynamic state:

	bool m_Moving;
	entity_id_t m_ControlGroup;
	ICmpObstructionManager::tag_t m_Tag;

	static std::string GetSchema()
	{
		return
			"<a:example/>"
			"<a:help>Causes this entity's footprint to obstruct the motion of other units.</a:help>"
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
			"<optional>"
				"<element name='Inactive' a:help='If this element is present, this entity will be ignored in collision tests by other units but can still perform its own collision tests'>"
					"<empty/>"
				"</element>"
			"</optional>";
	}

	virtual void Init(const CSimContext& UNUSED(context), const CParamNode& paramNode)
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

		if (paramNode.GetChild("Inactive").IsOk())
			m_Active = false;
		else
			m_Active = true;

		m_Tag = ICmpObstructionManager::tag_t();
		m_Moving = false;
		m_ControlGroup = GetEntityId();
	}

	virtual void Deinit(const CSimContext& UNUSED(context))
	{
	}

	template<typename S>
	void SerializeCommon(S& serialize)
	{
		serialize.Bool("moving", m_Moving);
		serialize.NumberU32_Unbounded("control group", m_ControlGroup);
		serialize.NumberU32_Unbounded("tag", m_Tag.n);
	}

	virtual void Serialize(ISerializer& serialize)
	{
		SerializeCommon(serialize);
	}

	virtual void Deserialize(const CSimContext& context, const CParamNode& paramNode, IDeserializer& deserialize)
	{
		Init(context, paramNode);

		SerializeCommon(deserialize);
	}

	virtual void HandleMessage(const CSimContext& context, const CMessage& msg, bool UNUSED(global))
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

			CmpPtr<ICmpObstructionManager> cmpObstructionManager(context, SYSTEM_ENTITY);
			if (cmpObstructionManager.null())
				break;

			if (data.inWorld && m_Tag.valid())
			{
				cmpObstructionManager->MoveShape(m_Tag, data.x, data.z, data.a);
			}
			else if (data.inWorld && !m_Tag.valid())
			{
				// Need to create a new pathfinder shape:
				if (m_Type == STATIC)
					m_Tag = cmpObstructionManager->AddStaticShape(data.x, data.z, data.a, m_Size0, m_Size1);
				else
					m_Tag = cmpObstructionManager->AddUnitShape(data.x, data.z, m_Size0, m_Moving, m_ControlGroup);
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
				CmpPtr<ICmpObstructionManager> cmpObstructionManager(context, SYSTEM_ENTITY);
				if (cmpObstructionManager.null())
					break;

				cmpObstructionManager->RemoveShape(m_Tag);
				m_Tag = ICmpObstructionManager::tag_t();
			}
			break;
		}
		}
	}

	virtual ICmpObstructionManager::tag_t GetObstruction()
	{
		return m_Tag;
	}

	virtual entity_pos_t GetUnitRadius()
	{
		if (m_Type == UNIT)
			return m_Size0;
		else
			return entity_pos_t::Zero();
	}

	virtual bool CheckCollisions()
	{
		CmpPtr<ICmpPosition> cmpPosition(GetSimContext(), GetEntityId());
		if (cmpPosition.null())
			return false;

		CFixedVector2D pos = cmpPosition->GetPosition2D();

		CmpPtr<ICmpObstructionManager> cmpObstructionManager(GetSimContext(), SYSTEM_ENTITY);

		SkipTagObstructionFilter filter(m_Tag); // ignore collisions with self

		if (m_Type == STATIC)
			return cmpObstructionManager->TestStaticShape(filter, pos.X, pos.Y, cmpPosition->GetRotation().Y, m_Size0, m_Size1);
		else
			return cmpObstructionManager->TestUnitShape(filter, pos.X, pos.Y, m_Size0);
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
