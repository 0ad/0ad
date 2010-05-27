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
#include "ICmpWaterManager.h"

#include "renderer/Renderer.h"
#include "renderer/WaterManager.h"

class CCmpWaterManager : public ICmpWaterManager
{
public:
	static void ClassInit(CComponentManager& componentManager)
	{
		componentManager.SubscribeToMessageType(MT_RenderSubmit);
	}

	DEFAULT_COMPONENT_ALLOCATOR(WaterManager)

	entity_pos_t m_WaterHeight;

	static std::string GetSchema()
	{
		return "<a:component type='system'/><empty/>";
	}

	virtual void Init(const CSimContext& UNUSED(context), const CParamNode& UNUSED(paramNode))
	{
		SetWaterLevel(entity_pos_t::FromInt(5));
	}

	virtual void Deinit(const CSimContext& UNUSED(context))
	{
	}

	virtual void Serialize(ISerializer& UNUSED(serialize))
	{
	}

	virtual void Deserialize(const CSimContext& context, const CParamNode& paramNode, IDeserializer& UNUSED(deserialize))
	{
		Init(context, paramNode);
	}

	virtual void HandleMessage(const CSimContext& UNUSED(context), const CMessage& msg, bool UNUSED(global))
	{
		switch (msg.GetType())
		{
		case MT_RenderSubmit:
		{
			// Don't actually do rendering here, but tell the renderer how to draw water
			if (CRenderer::IsInitialised())
				g_Renderer.GetWaterManager()->m_WaterHeight = m_WaterHeight.ToFloat();
			break;
		}
		}
	}

	virtual void SetWaterLevel(entity_pos_t h)
	{
		m_WaterHeight = h;
	}

	virtual entity_pos_t GetWaterLevel(entity_pos_t UNUSED(x), entity_pos_t UNUSED(z))
	{
		return m_WaterHeight;
	}

	virtual float GetExactWaterLevel(float UNUSED(x), float UNUSED(z))
	{
		return m_WaterHeight.ToFloat();
	}
};

REGISTER_COMPONENT_TYPE(WaterManager)
