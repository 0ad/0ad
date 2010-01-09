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
#include "ICmpUnitMotion.h"

#include "ICmpPosition.h"
#include "simulation2/MessageTypes.h"

class CCmpUnitMotion : public ICmpUnitMotion
{
public:
	static void ClassInit(CComponentManager& componentManager)
	{
		componentManager.SubscribeToMessageType(CID_UnitMotion, MT_Update);
	}

	DEFAULT_COMPONENT_ALLOCATOR(UnitMotion)

	// Template state:
	CFixed_23_8 m_Speed; // in units per second

	// Dynamic state:
	bool m_HasTarget;
	entity_pos_t m_TargetX, m_TargetZ; // these values contain undefined junk if !HasTarget

	virtual void Init(const CSimContext& UNUSED(context), const CParamNode& UNUSED(paramNode))
	{
		m_Speed = CFixed_23_8::FromInt(4);
		m_HasTarget = false;
	}

	virtual void Deinit(const CSimContext& UNUSED(context))
	{
	}

	virtual void Serialize(ISerializer& serialize)
	{
		serialize.Bool("has target", m_HasTarget);
		if (m_HasTarget)
		{
			serialize.NumberFixed_Unbounded("target x", m_TargetX);
			serialize.NumberFixed_Unbounded("target z", m_TargetZ);
		}
	}

	virtual void Deserialize(const CSimContext& context, const CParamNode& paramNode, IDeserializer& deserialize)
	{
		Init(context, paramNode);

		deserialize.Bool(m_HasTarget);
		if (m_HasTarget)
		{
			deserialize.NumberFixed_Unbounded(m_TargetX);
			deserialize.NumberFixed_Unbounded(m_TargetZ);
		}
	}

	virtual void HandleMessage(const CSimContext& context, const CMessage& msg)
	{
		switch (msg.GetType())
		{
		case MT_Update:
		{
			CFixed_23_8 dt = static_cast<const CMessageUpdate&> (msg).turnLength;
			Move(context, dt);
			break;
		}
		}
	}

	virtual void MoveToPoint(entity_pos_t x, entity_pos_t z)
	{
		m_HasTarget = true;
		m_TargetX = x;
		m_TargetZ = z;
	}

	void Move(const CSimContext& context, CFixed_23_8 dt);
};

REGISTER_COMPONENT_TYPE(UnitMotion)


void CCmpUnitMotion::Move(const CSimContext& context, CFixed_23_8 dt)
{
	if (!m_HasTarget)
		return;

	CmpPtr<ICmpPosition> cmpPosition(context, GetEntityId());
	if (cmpPosition.null())
		return;

	CFixed_23_8 maxdist = m_Speed.Multiply(dt);

	CFixedVector3D pos = cmpPosition->GetPosition();
	pos.Y = CFixed_23_8::FromInt(0); // remove Y so it doesn't influence our distance calculations

	// We want to move (at most) m_Speed*dt units from pos towards m_Target[XZ]

	CFixedVector3D target(m_TargetX, CFixed_23_8::FromInt(0), m_TargetZ);
	CFixedVector3D offset = target - pos;

	// Face towards the target
	entity_angle_t angle = atan2_approx(offset.X, offset.Z);
	cmpPosition->SetYRotation(angle);

	// If it's close, we can move there directly
	if (offset.Length() <= maxdist)
	{
		cmpPosition->MoveTo(m_TargetX, m_TargetZ);
		m_HasTarget = false;
		return;
	}

	// Otherwise move in the right direction
	offset.Normalize(maxdist);
	pos += offset;
	cmpPosition->MoveTo(pos.X, pos.Z);
}
