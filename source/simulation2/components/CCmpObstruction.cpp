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

#include "ICmpFootprint.h"
#include "ICmpObstructionManager.h"

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

	const CSimContext* m_Context;

	bool m_Active; // whether the obstruction is obstructing or just an inactive placeholder

	ICmpObstructionManager::tag_t m_Tag;

	static std::string GetSchema()
	{
		return
			"<optional>"
				"<element name='Inactive'><empty/></element>"
			"</optional>";
	}
	virtual void Init(const CSimContext& context, const CParamNode& paramNode)
	{
		m_Context = &context;

		if (paramNode.GetChild("Inactive").IsOk())
			m_Active = false;
		else
			m_Active = true;

		m_Tag = 0;
	}

	virtual void Deinit(const CSimContext& UNUSED(context))
	{
	}

	virtual void Serialize(ISerializer& UNUSED(serialize))
	{
		// TODO: Coordinate with CCmpObstructionManager serialisation
	}

	virtual void Deserialize(const CSimContext& context, const CParamNode& paramNode, IDeserializer& UNUSED(deserialize))
	{
		Init(context, paramNode);

		// TODO: Coordinate with CCmpObstructionManager serialisation
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

			if (!data.inWorld && !m_Tag)
				break; // nothing needs to change

			CmpPtr<ICmpObstructionManager> cmpObstructionManager(context, SYSTEM_ENTITY);
			if (cmpObstructionManager.null())
				break;

			if (data.inWorld && m_Tag)
			{
				cmpObstructionManager->MoveShape(m_Tag, data.x, data.z, data.a);
			}
			else if (data.inWorld && !m_Tag)
			{
				// Need to create a new pathfinder shape:

				CmpPtr<ICmpFootprint> cmpFootprint(context, GetEntityId());
				if (cmpFootprint.null())
					break;

				ICmpFootprint::EShape shape;
				entity_pos_t size0, size1, height;
				cmpFootprint->GetShape(shape, size0, size1, height);

				if (shape == ICmpFootprint::SQUARE)
					m_Tag = cmpObstructionManager->AddSquare(data.x, data.z, data.a, size0, size1);
				else
					m_Tag = cmpObstructionManager->AddCircle(data.x, data.z, size0);
			}
			else if (!data.inWorld && m_Tag)
			{
				cmpObstructionManager->RemoveShape(m_Tag);
				m_Tag = 0;
			}
			break;
		}
		case MT_Destroy:
		{
			if (m_Tag)
			{
				CmpPtr<ICmpObstructionManager> cmpObstructionManager(context, SYSTEM_ENTITY);
				if (cmpObstructionManager.null())
					break;

				cmpObstructionManager->RemoveShape(m_Tag);
				m_Tag = 0;
			}
			break;
		}
		}
	}

	virtual bool CheckCollisions()
	{
		CmpPtr<ICmpFootprint> cmpFootprint(*m_Context, GetEntityId());
		if (cmpFootprint.null())
			return false;

		CmpPtr<ICmpPosition> cmpPosition(*m_Context, GetEntityId());
		if (cmpPosition.null())
			return false;

		ICmpFootprint::EShape shape;
		entity_pos_t size0, size1, height;
		cmpFootprint->GetShape(shape, size0, size1, height);

		CFixedVector3D pos = cmpPosition->GetPosition();

		CmpPtr<ICmpObstructionManager> cmpObstructionManager(*m_Context, SYSTEM_ENTITY);

		SkipTagObstructionFilter filter(m_Tag); // ignore collisions with self

		if (shape == ICmpFootprint::SQUARE)
			return !cmpObstructionManager->TestSquare(filter, pos.X, pos.Z, cmpPosition->GetRotation().Y, size0, size1);
		else
			return !cmpObstructionManager->TestCircle(filter, pos.X, pos.Z, size0);

	}
};

REGISTER_COMPONENT_TYPE(Obstruction)
