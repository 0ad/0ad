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
#include "ICmpDecay.h"

#include "simulation2/MessageTypes.h"

#include "ICmpPosition.h"
#include "ICmpTerrain.h"
#include "ICmpVisual.h"

/**
 * Fairly basic decay implementation, for units and buildings etc.
 * The decaying entity remains stationary for some time period, then falls downwards
 * with some initial speed and some acceleration, until it has fully sunk.
 * The sinking depth is determined from the actor's bounding box and the terrain.
 *
 * This currently doesn't work with entities whose ICmpPosition has an initial Y offset.
 *
 * This isn't very efficient (we'll store data and iterate every frame for every entity,
 * not just for corpse entities) - it could be designed more optimally if that's a real problem.
 *
 * Eventually we might want to adjust the decay rate based on user configuration (low-spec
 * machines could have fewer corpses), number of corpses, etc.
 *
 * Must not be used on network-synchronised entities, unless \<Inactive\> is present.
 */
class CCmpDecay : public ICmpDecay
{
public:
	static void ClassInit(CComponentManager& componentManager)
	{
		componentManager.SubscribeToMessageType(MT_Interpolate);
	}

	DEFAULT_COMPONENT_ALLOCATOR(Decay)

	bool m_Active;
	float m_DelayTime;
	float m_SinkRate;
	float m_SinkAccel;

	float m_CurrentTime;
	float m_TotalSinkDepth; // distance we need to sink (derived from bounding box), or -1 if undetermined

	static std::string GetSchema()
	{
		return
			"<element name='DelayTime' a:help='Time to wait before starting to sink, in seconds'>"
				"<ref name='nonNegativeDecimal'/>"
			"</element>"
			"<element name='SinkRate' a:help='Initial rate of sinking, in metres per second'>"
				"<ref name='nonNegativeDecimal'/>"
			"</element>"
			"<element name='SinkAccel' a:help='Acceleration rate of sinking, in metres per second per second'>"
				"<ref name='nonNegativeDecimal'/>"
			"</element>"
			"<optional>"
				"<element name='Inactive' a:help='If this element is present, the entity will not do any decaying'>"
					"<empty/>"
				"</element>"
			"</optional>";
	}

	virtual void Init(const CSimContext& UNUSED(context), const CParamNode& paramNode)
	{
		m_Active = !paramNode.GetChild("Inactive").IsOk();
		m_DelayTime = paramNode.GetChild("DelayTime").ToFixed().ToFloat();
		m_SinkRate = paramNode.GetChild("SinkRate").ToFixed().ToFloat();
		m_SinkAccel = paramNode.GetChild("SinkAccel").ToFixed().ToFloat();

		m_CurrentTime = 0.f;
		m_TotalSinkDepth = -1.f;

		// Detect unsafe misconfiguration
		if (m_Active && !ENTITY_IS_LOCAL(GetEntityId()))
		{
			debug_warn(L"CCmpDecay must not be used on non-local (network-synchronised) entities");
			m_Active = false;
		}
	}

	virtual void Deinit(const CSimContext& UNUSED(context))
	{
	}

	virtual void Serialize(ISerializer& UNUSED(serialize))
	{
		// This component isn't network-synchronised, so don't serialize anything
	}

	virtual void Deserialize(const CSimContext& context, const CParamNode& paramNode, IDeserializer& UNUSED(deserialize))
	{
		Init(context, paramNode);
	}

	virtual void HandleMessage(const CSimContext& UNUSED(context), const CMessage& msg, bool UNUSED(global))
	{
		switch (msg.GetType())
		{
		case MT_Interpolate:
		{
			if (!m_Active)
				break;

			const CMessageInterpolate& msgData = static_cast<const CMessageInterpolate&> (msg);

			CmpPtr<ICmpPosition> cmpPosition(GetSimContext(), GetEntityId());
			if (cmpPosition.null() || !cmpPosition->IsInWorld())
			{
				// If there's no position (this usually shouldn't happen), destroy the unit immediately
				GetSimContext().GetComponentManager().DestroyComponentsSoon(GetEntityId());
				break;
			}

			// Compute the depth the first time this is called
			// (This is a bit of an ugly place to do it but at least we'll be sure
			// the actor component was loaded already)
			if (m_TotalSinkDepth < 0.f)
			{
				m_TotalSinkDepth = 1.f; // minimum so we always sink at least a little

				CmpPtr<ICmpVisual> cmpVisual(GetSimContext(), GetEntityId());
				if (!cmpVisual.null())
				{
					CBound bound = cmpVisual->GetBounds();
					m_TotalSinkDepth = std::max(m_TotalSinkDepth, bound[1].Y - bound[0].Y);
				}

				// If this is a floating unit, we want it to sink all the way under the terrain,
				// so find the difference between its current position and the terrain

				CFixedVector3D pos = cmpPosition->GetPosition();

				CmpPtr<ICmpTerrain> cmpTerrain(GetSimContext(), SYSTEM_ENTITY);
				if (!cmpTerrain.null())
				{
					fixed ground = cmpTerrain->GetGroundLevel(pos.X, pos.Z);
					m_TotalSinkDepth += std::max(0.f, (pos.Y - ground).ToFloat());
				}
			}

			m_CurrentTime += msgData.frameTime;

			if (m_CurrentTime > m_DelayTime)
			{
				float t = m_CurrentTime - m_DelayTime;
				float depth = (m_SinkRate * t) + (m_SinkAccel * t * t);

				cmpPosition->SetHeightOffset(entity_pos_t::FromFloat(-depth));

				if (depth > m_TotalSinkDepth)
					GetSimContext().GetComponentManager().DestroyComponentsSoon(GetEntityId());
			}

			break;
		}
		}
	}
};

REGISTER_COMPONENT_TYPE(Decay)
