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
#include "ICmpMotion.h"

#include "ICmpPosition.h"
#include "simulation2/MessageTypes.h"

#include "graphics/Terrain.h"

class CCmpMotionBall : public ICmpMotion
{
public:
	static void ClassInit(CComponentManager& componentManager)
	{
		componentManager.SubscribeToMessageType(MT_Update);
	}

	DEFAULT_COMPONENT_ALLOCATOR(MotionBall)

	// Current speed in metres per second
	float m_SpeedX, m_SpeedZ;

	static std::string GetSchema()
	{
		return "<a:component type='test'/><ref name='anything'/>";
	}

	virtual void Init(const CParamNode& UNUSED(paramNode))
	{
		m_SpeedX = 0;
		m_SpeedZ = 0;
	}

	virtual void Deinit()
	{
	}

	virtual void Serialize(ISerializer& serialize)
	{
		serialize.NumberFloat_Unbounded("speed x", m_SpeedX);
		serialize.NumberFloat_Unbounded("speed z", m_SpeedZ);
	}

	virtual void Deserialize(const CParamNode& paramNode, IDeserializer& deserialize)
	{
		Init(paramNode);
		deserialize.NumberFloat_Unbounded("speed x", m_SpeedX);
		deserialize.NumberFloat_Unbounded("speed z", m_SpeedZ);
	}

	virtual void HandleMessage(const CMessage& msg, bool UNUSED(global))
	{
		switch (msg.GetType())
		{
		case MT_Update:
		{
			fixed dt = static_cast<const CMessageUpdate&> (msg).turnLength;
			Move(dt);
			break;
		}
		}
	}

	void Move(fixed dt);
};

void CCmpMotionBall::Move(fixed dt)
{
	CmpPtr<ICmpPosition> cmpPosition(GetEntityHandle());
	if (!cmpPosition)
		return;

	// TODO: this is all FP-unsafe

	CFixedVector3D pos = cmpPosition->GetPosition();
	float x = pos.X.ToFloat();
	float z = pos.Z.ToFloat();

	CVector3D normal = GetSimContext().GetTerrain().CalcExactNormal(x, z);
	// Flatten the vector, to get the downhill force
	float g = 10.f;
	CVector3D force(g * normal.X, 0.f, g * normal.Z);

	m_SpeedX += force.X;
	m_SpeedZ += force.Z;

	float drag = 0.5f; // fractional decay per second

	float dt_ = dt.ToFloat();

	m_SpeedX *= powf(drag, dt_);
	m_SpeedZ *= powf(drag, dt_);

	cmpPosition->MoveTo(entity_pos_t::FromFloat(x + m_SpeedX * dt_), entity_pos_t::FromFloat(z + m_SpeedZ * dt_));
}

REGISTER_COMPONENT_TYPE(MotionBall)
