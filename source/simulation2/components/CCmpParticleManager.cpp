/* Copyright (C) 2013 Wildfire Games.
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
#include "ICmpParticleManager.h"

#include "simulation2/MessageTypes.h"

#include "graphics/ParticleManager.h"
#include "renderer/Renderer.h"

class CCmpParticleManager : public ICmpParticleManager
{
public:
	static void ClassInit(CComponentManager& componentManager)
	{
		componentManager.SubscribeToMessageType(MT_Interpolate);
	}

	DEFAULT_COMPONENT_ALLOCATOR(ParticleManager)

	bool useSimTime;

	static std::string GetSchema()
	{
		return "<a:component type='system'/><empty/>";
	}

	virtual void Init(const CParamNode& UNUSED(paramNode))
	{
		useSimTime = true;
	}

	virtual void Deinit()
	{
	}

	virtual void Serialize(ISerializer& UNUSED(serialize))
	{
	}

	virtual void Deserialize(const CParamNode& paramNode, IDeserializer& UNUSED(deserialize))
	{
		Init(paramNode);
	}

	virtual void HandleMessage(const CMessage& msg, bool UNUSED(global))
	{
		switch (msg.GetType())
		{
		case MT_Interpolate:
		{
			const CMessageInterpolate& msgData = static_cast<const CMessageInterpolate&> (msg);
			if (CRenderer::IsInitialised())
			{
				float time = useSimTime ? msgData.deltaSimTime : msgData.deltaRealTime;
				g_Renderer.GetParticleManager().Interpolate(time);
			}
			break;
		}
		}
	}

	virtual void SetUseSimTime(bool flag)
	{
		useSimTime = flag;
	}
};

REGISTER_COMPONENT_TYPE(ParticleManager)
