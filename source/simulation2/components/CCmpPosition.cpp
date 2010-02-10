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
#include "ICmpPosition.h"

#include "simulation2/MessageTypes.h"

#include "ICmpTerrain.h"

#include "graphics/Terrain.h"
#include "lib/rand.h"
#include "maths/MathUtil.h"
#include "maths/Matrix3D.h"
#include "maths/Vector3D.h"
#include "ps/CLogger.h"

/**
 * Basic ICmpPosition implementation.
 */
class CCmpPosition : public ICmpPosition
{
public:
	static void ClassInit(CComponentManager& componentManager)
	{
		componentManager.SubscribeToMessageType(MT_TurnStart);
		componentManager.SubscribeToMessageType(MT_Interpolate);

		// TODO: if this component turns out to be a performance issue, it should
		// be optimised by creating a new PositionStatic component that doesn't subscribe
		// to messages and doesn't store LastX/LastZ, and that should be used for all
		// entities that don't move
	}

	DEFAULT_COMPONENT_ALLOCATOR(Position)

	const CSimContext* m_Context; // never NULL (after Init/Deserialize)

	// Template state:

	enum
	{
		UPRIGHT = 0,
		PITCH = 1,
		PITCH_ROLL = 2,
	} m_AnchorType;

	entity_pos_t m_YOffset;
	bool m_Floating;
	float m_RotYSpeed; // maximum radians per second, used by InterpolatedRotY to follow RotY

	// Dynamic state:

	bool m_InWorld;
	entity_pos_t m_X, m_Z, m_LastX, m_LastZ; // these values contain undefined junk if !InWorld

	entity_angle_t m_RotX, m_RotY, m_RotZ;
	float m_InterpolatedRotY; // not serialized

	bool m_Dirty; // true if position/rotation has changed since last TurnStart

	/*
	 * Schema: (untested)
	 *
	 * <element name="Position">
	 *   <interleave>
	 *     <element name="Anchor" a:help="Automatic rotation to follow the slope of terrain">
	 *       <choice>
	 *         <value a:help="Always stand straight up">upright</value>
	 *         <value a:help="Rotate backwards and forwards to follow the terrain">pitch</value>
	 *         <value a:help="Rotate in all direction to follow the terrain">pitch-roll</value>
	 *       </choice>
	 *     </element>
	 *     <element name="Altitude" a:help="Height above terrain in metres">
	 *       <data type="float"/>
	 *     </element>
	 *     <element name="Floating" a:help="Whether the entity floats on water">
	 *       <data type="boolean"/>
	 *     </element>
	 *   </interleave>
	 * </element>
	 */
	virtual void Init(const CSimContext& context, const CParamNode& paramNode)
	{
		m_Context = &context;

		std::wstring anchor = paramNode.GetChild("Anchor").ToString();
		if (anchor == L"pitch")
			m_AnchorType = PITCH;
		else if (anchor == L"pitch-roll")
			m_AnchorType = PITCH_ROLL;
		else
			m_AnchorType = UPRIGHT;

		m_InWorld = false;

		m_YOffset = paramNode.GetChild("Altitude").ToFixed();
		m_Floating = paramNode.GetChild("Floating").ToBool();

		m_RotYSpeed = 6.f; // TODO: should get from template

		m_RotX = m_RotY = m_RotZ = entity_angle_t::FromInt(0);
		m_InterpolatedRotY = 0;

		m_Dirty = false;
	}

	virtual void Deinit(const CSimContext& UNUSED(context))
	{
	}

	virtual void Serialize(ISerializer& serialize)
	{
		serialize.Bool("in world", m_InWorld);
		if (m_InWorld)
		{
			serialize.NumberFixed_Unbounded("x", m_X);
			serialize.NumberFixed_Unbounded("z", m_Z);
			serialize.NumberFixed_Unbounded("last x", m_LastX);
			serialize.NumberFixed_Unbounded("last z", m_LastZ);
			// TODO: for efficiency, we probably shouldn't actually store the last position - it doesn't
			// matter if we don't have smooth interpolation after reloading a game
		}
		serialize.NumberFixed_Unbounded("rot x", m_RotX);
		serialize.NumberFixed_Unbounded("rot y", m_RotY);
		serialize.NumberFixed_Unbounded("rot z", m_RotZ);
		serialize.NumberFixed_Unbounded("altitude", m_YOffset);
		serialize.Bool("dirty", m_Dirty);

		if (serialize.IsDebug())
		{
			const char* anchor = "???";
			switch (m_AnchorType)
			{
			case UPRIGHT: anchor = "upright"; break;
			case PITCH: anchor = "pitch"; break;
			case PITCH_ROLL: anchor = "pitch-roll"; break;
			}
			serialize.StringASCII("anchor", anchor, 0, 16);
			serialize.Bool("floating", m_Floating);
		}
	}

	virtual void Deserialize(const CSimContext& context, const CParamNode& paramNode, IDeserializer& deserialize)
	{
		Init(context, paramNode);

		deserialize.Bool(m_InWorld);
		if (m_InWorld)
		{
			deserialize.NumberFixed_Unbounded(m_X);
			deserialize.NumberFixed_Unbounded(m_Z);
			deserialize.NumberFixed_Unbounded(m_LastX);
			deserialize.NumberFixed_Unbounded(m_LastZ);
		}
		deserialize.NumberFixed_Unbounded(m_RotX);
		deserialize.NumberFixed_Unbounded(m_RotY);
		deserialize.NumberFixed_Unbounded(m_RotZ);
		deserialize.NumberFixed_Unbounded(m_YOffset);
		deserialize.Bool(m_Dirty);
		// TODO: should there be range checks on all these values?

		m_InterpolatedRotY = m_RotY.ToFloat();
	}

	virtual bool IsInWorld()
	{
		return m_InWorld;
	}

	virtual void MoveOutOfWorld()
	{
		m_InWorld = false;

		m_Dirty = true;
	}

	virtual void MoveTo(entity_pos_t x, entity_pos_t z)
	{
		m_X = x;
		m_Z = z;

		if (!m_InWorld)
		{
			m_InWorld = true;
			m_LastX = m_X;
			m_LastZ = m_Z;
		}

		m_Dirty = true;
	}

	virtual void JumpTo(entity_pos_t x, entity_pos_t z)
	{
		m_LastX = m_X = x;
		m_LastZ = m_Z = z;
		m_InWorld = true;

		m_Dirty = true;
	}

	virtual void SetHeightOffset(entity_pos_t dy)
	{
		m_YOffset = dy;

		m_Dirty = true;
	}

	virtual entity_pos_t GetHeightOffset()
	{
		return m_YOffset;
	}

	virtual CFixedVector3D GetPosition()
	{
		if (!m_InWorld)
		{
			LOGERROR(L"CCmpPosition::GetPosition called on entity when IsInWorld is false");
			return CFixedVector3D();
		}

		entity_pos_t ground;
		CmpPtr<ICmpTerrain> cmpTerrain(*m_Context, SYSTEM_ENTITY);
		if (!cmpTerrain.null())
		{
			ground = cmpTerrain->GetGroundLevel(m_X, m_Z);
			// TODO: do something with m_Floating
		}

		// NOTE: most callers don't actually care about Y; if this is a performance
		// issue then we could add a new method that simply returns X/Z

		return CFixedVector3D(m_X, ground + m_YOffset, m_Z);
	}

	virtual void TurnTo(entity_angle_t y)
	{
		m_RotY = y;

		m_Dirty = true;
	}

	virtual void SetYRotation(entity_angle_t y)
	{
		m_RotY = y;
		m_InterpolatedRotY = m_RotY.ToFloat();

		m_Dirty = true;
	}

	virtual void SetXZRotation(entity_angle_t x, entity_angle_t z)
	{
		m_RotX = x;
		m_RotZ = z;

		m_Dirty = true;
	}

	virtual CFixedVector3D GetRotation()
	{
		return CFixedVector3D(m_RotX, m_RotY, m_RotZ);
	}

	virtual CMatrix3D GetInterpolatedTransform(float frameOffset)
	{
		if (!m_InWorld)
		{
			LOGERROR(L"CCmpPosition::GetInterpolatedTransform called on entity when IsInWorld is false");
			CMatrix3D m;
			m.SetIdentity();
			return m;
		}

		float x = Interpolate(m_LastX.ToFloat(), m_X.ToFloat(), frameOffset);
		float z = Interpolate(m_LastZ.ToFloat(), m_Z.ToFloat(), frameOffset);

		entity_pos_t ground;
		CmpPtr<ICmpTerrain> cmpTerrain(*m_Context, SYSTEM_ENTITY);
		if (!cmpTerrain.null())
		{
			ground = cmpTerrain->GetGroundLevel(m_X, m_Z);
			// TODO: do something with m_Floating
		}

		float y = ground.ToFloat() + m_YOffset.ToFloat();

		// TODO: do something with m_AnchorType

		CMatrix3D m;
		CMatrix3D mXZ;
		float Cos = cosf(m_InterpolatedRotY);
		float Sin = sinf(m_InterpolatedRotY);

		m.SetIdentity();
		m._11 = -Cos;
		m._13 = -Sin;
		m._31 = Sin;
		m._33 = -Cos;

		mXZ.SetIdentity();
		mXZ.SetXRotation(m_RotX.ToFloat());
		mXZ.RotateZ(m_RotZ.ToFloat());
		// TODO: is this correct?
		mXZ = m * mXZ;
		mXZ.Translate(CVector3D(x, y, z));

		return mXZ;
	}

	virtual void HandleMessage(const CSimContext& context, const CMessage& msg, bool UNUSED(global))
	{
		switch (msg.GetType())
		{
		case MT_Interpolate:
		{
			const CMessageInterpolate& msgData = static_cast<const CMessageInterpolate&> (msg);

			float rotY = m_RotY.ToFloat();
			float delta = rotY - m_InterpolatedRotY;
			// Wrap delta to -PI..PI
			delta = fmod(delta + PI, 2*PI); // range -2PI..2PI
			if (delta < 0) delta += 2*PI; // range 0..2PI
			delta -= PI; // range -PI..PI
			// Clamp to max rate
			float deltaClamped = clamp(delta, -m_RotYSpeed*msgData.frameTime, +m_RotYSpeed*msgData.frameTime);
			// Calculate new orientation, in a peculiar way in order to make sure the
			// result gets close to m_orientation (rather than being n*2*PI out)
			m_InterpolatedRotY = rotY + deltaClamped - delta;

			break;
		}
		case MT_TurnStart:
		{
			m_LastX = m_X;
			m_LastZ = m_Z;
			if (m_Dirty)
			{
				if (m_InWorld)
					context.GetComponentManager().PostMessage(GetEntityId(), CMessagePositionChanged(true, m_X, m_Z, m_RotY));
				else
					context.GetComponentManager().PostMessage(GetEntityId(), CMessagePositionChanged(false, entity_pos_t(), entity_pos_t(), entity_angle_t()));
				m_Dirty = false;
			}

			break;
		}
		}
	}
};

REGISTER_COMPONENT_TYPE(Position)
