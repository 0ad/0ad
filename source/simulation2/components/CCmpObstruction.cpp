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
#include "ICmpPathfinder.h"

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

	ICmpPathfinder::tag_t m_Tag;

	virtual void Init(const CSimContext& UNUSED(context), const CParamNode& UNUSED(paramNode))
	{
		m_Tag = 0;
	}

	virtual void Deinit(const CSimContext& UNUSED(context))
	{
	}

	virtual void Serialize(ISerializer& UNUSED(serialize))
	{
		// TODO: Coordinate with CCmpPathfinder serialisation
	}

	virtual void Deserialize(const CSimContext& context, const CParamNode& paramNode, IDeserializer& UNUSED(deserialize))
	{
		Init(context, paramNode);

		// TODO: Coordinate with CCmpPathfinder serialisation
	}

	virtual void HandleMessage(const CSimContext& context, const CMessage& msg, bool UNUSED(global))
	{
		switch (msg.GetType())
		{
		case MT_PositionChanged:
		{
			const CMessagePositionChanged& data = static_cast<const CMessagePositionChanged&> (msg);

			if (!data.inWorld && !m_Tag)
				break; // nothing to do

			CmpPtr<ICmpPathfinder> cmpPathfinder(context, SYSTEM_ENTITY);
			if (cmpPathfinder.null())
				break;

			if (data.inWorld && m_Tag)
			{
				cmpPathfinder->MoveShape(m_Tag, data.x, data.z, data.a);
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
					m_Tag = cmpPathfinder->AddSquare(data.x, data.z, data.a, size0, size1);
				else
					m_Tag = cmpPathfinder->AddCircle(data.x, data.z, size0);
			}
			else if (!data.inWorld && m_Tag)
			{
				cmpPathfinder->RemoveShape(m_Tag);
				m_Tag = 0;
			}
			break;
		}
		case MT_Destroy:
		{
			if (m_Tag)
			{
				CmpPtr<ICmpPathfinder> cmpPathfinder(context, SYSTEM_ENTITY);
				if (cmpPathfinder.null())
					break;

				cmpPathfinder->RemoveShape(m_Tag);
				m_Tag = 0;
			}
			break;
		}
		}
	}
};

REGISTER_COMPONENT_TYPE(Obstruction)
