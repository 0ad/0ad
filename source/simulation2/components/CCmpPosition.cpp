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
#include "ICmpPosition.h"

#include "simulation2/MessageTypes.h"

#include "ICmpTerrain.h"
#include "ICmpWaterManager.h"

#include "graphics/Terrain.h"
#include "lib/rand.h"
#include "maths/MathUtil.h"
#include "maths/Matrix3D.h"
#include "maths/Vector3D.h"
#include "maths/Vector2D.h"
#include "ps/CLogger.h"
#include "ICmpTerritoryManager.h"

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

	// Template state:

	enum
	{
		UPRIGHT = 0,
		PITCH = 1,
		PITCH_ROLL = 2,
		ROLL=3,
	} m_AnchorType;

	bool m_Floating;
	float m_RotYSpeed; // maximum radians per second, used by InterpolatedRotY to follow RotY

	// Dynamic state:

	bool m_InWorld;
	// m_LastX/Z contain the position from the start of the most recent turn
	// m_PrevX/Z conatain the position from the turn before that
	entity_pos_t m_X, m_Z, m_LastX, m_LastZ, m_PrevX, m_PrevZ; // these values contain undefined junk if !InWorld

	entity_pos_t m_Y, m_LastYDifference; // either the relative or the absolute Y coordinate
	bool m_RelativeToGround; // whether m_Y is relative to terrain/water plane, or an absolute height

	entity_angle_t m_RotX, m_RotY, m_RotZ;

	player_id_t m_Territory;

	// not serialized:
	float m_InterpolatedRotX, m_InterpolatedRotY, m_InterpolatedRotZ;
	float m_LastInterpolatedRotX, m_LastInterpolatedRotZ; // not serialized

	bool m_NeedInitialXZRotation;

	static std::string GetSchema()
	{
		return
			"<a:help>Allows this entity to exist at a location (and orientation) in the world, and defines some details of the positioning.</a:help>"
			"<a:example>"
				"<Anchor>upright</Anchor>"
				"<Altitude>0.0</Altitude>"
				"<Floating>false</Floating>"
				"<TurnRate>6.0</TurnRate>"
			"</a:example>"
			"<element name='Anchor' a:help='Automatic rotation to follow the slope of terrain'>"
				"<choice>"
					"<value a:help='Always stand straight up (e.g. humans)'>upright</value>"
					"<value a:help='Rotate backwards and forwards to follow the terrain (e.g. animals)'>pitch</value>"
					"<value a:help='Rotate sideways to follow the terrain'>roll</value>"
					"<value a:help='Rotate in all directions to follow the terrain (e.g. carts)'>pitch-roll</value>"
				"</choice>"
			"</element>"
			"<element name='Altitude' a:help='Height above terrain in metres'>"
				"<data type='decimal'/>"
			"</element>"
			"<element name='Floating' a:help='Whether the entity floats on water'>"
				"<data type='boolean'/>"
			"</element>"
			"<element name='TurnRate' a:help='Maximum graphical rotation speed around Y axis, in radians per second'>"
				"<ref name='positiveDecimal'/>"
			"</element>";
	}

	virtual void Init(const CParamNode& paramNode)
	{
		std::wstring anchor = paramNode.GetChild("Anchor").ToString();
		if (anchor == L"pitch")
			m_AnchorType = PITCH;
		else if (anchor == L"pitch-roll")
			m_AnchorType = PITCH_ROLL;
		else if (anchor == L"roll")
			m_AnchorType = ROLL;
		else
			m_AnchorType = UPRIGHT;

		m_InWorld = false;

		m_LastYDifference = entity_pos_t::Zero();
		m_Y = paramNode.GetChild("Altitude").ToFixed();
		m_RelativeToGround = true;
		m_Floating = paramNode.GetChild("Floating").ToBool();

		m_RotYSpeed = paramNode.GetChild("TurnRate").ToFixed().ToFloat();

		m_RotX = m_RotY = m_RotZ = entity_angle_t::FromInt(0);
		m_InterpolatedRotX = m_InterpolatedRotY = m_InterpolatedRotZ = 0.f;
		m_LastInterpolatedRotX = m_LastInterpolatedRotZ = 0.f;
		m_Territory = INVALID_PLAYER;

		m_NeedInitialXZRotation = false;
	}

	virtual void Deinit()
	{
	}

	virtual void Serialize(ISerializer& serialize)
	{
		serialize.Bool("in world", m_InWorld);
		if (m_InWorld)
		{
			serialize.NumberFixed_Unbounded("x", m_X);
			serialize.NumberFixed_Unbounded("y", m_Y);
			serialize.NumberFixed_Unbounded("z", m_Z);
			serialize.NumberFixed_Unbounded("last x", m_LastX);
			serialize.NumberFixed_Unbounded("last y diff", m_LastYDifference);
			serialize.NumberFixed_Unbounded("last z", m_LastZ);
		}
		serialize.NumberI32_Unbounded("territory", m_Territory);
		serialize.NumberFixed_Unbounded("rot x", m_RotX);
		serialize.NumberFixed_Unbounded("rot y", m_RotY);
		serialize.NumberFixed_Unbounded("rot z", m_RotZ);
		serialize.NumberFixed_Unbounded("altitude", m_Y);
		serialize.Bool("relative", m_RelativeToGround);
		serialize.Bool("floating", m_Floating);

		if (serialize.IsDebug())
		{
			const char* anchor = "???";
			switch (m_AnchorType)
			{
			case PITCH: 
				anchor = "pitch"; 
				break;

			case PITCH_ROLL: 
				anchor = "pitch-roll"; 
				break;
			
			case ROLL:
				anchor = "roll";
				break;

			case UPRIGHT: // upright is the default
			default: 
				anchor = "upright"; 
				break;
			}
			serialize.StringASCII("anchor", anchor, 0, 16);
		}
	}

	virtual void Deserialize(const CParamNode& paramNode, IDeserializer& deserialize)
	{
		Init(paramNode);

		deserialize.Bool("in world", m_InWorld);
		if (m_InWorld)
		{
			deserialize.NumberFixed_Unbounded("x", m_X);
			deserialize.NumberFixed_Unbounded("y", m_Y);
			deserialize.NumberFixed_Unbounded("z", m_Z);
			deserialize.NumberFixed_Unbounded("last x", m_LastX);
			deserialize.NumberFixed_Unbounded("last y diff", m_LastYDifference);
			deserialize.NumberFixed_Unbounded("last z", m_LastZ);
		}
		deserialize.NumberI32_Unbounded("territory", m_Territory);
		deserialize.NumberFixed_Unbounded("rot x", m_RotX);
		deserialize.NumberFixed_Unbounded("rot y", m_RotY);
		deserialize.NumberFixed_Unbounded("rot z", m_RotZ);
		deserialize.NumberFixed_Unbounded("altitude", m_Y);
		deserialize.Bool("relative", m_RelativeToGround);
		deserialize.Bool("floating", m_Floating);
		// TODO: should there be range checks on all these values?

		m_InterpolatedRotY = m_RotY.ToFloat();

		if (m_InWorld)
			UpdateXZRotation();
	}

	virtual bool IsInWorld()
	{
		return m_InWorld;
	}

	virtual void MoveOutOfWorld()
	{
		m_InWorld = false;

		AdvertisePositionChanges();
	}

	virtual void MoveTo(entity_pos_t x, entity_pos_t z)
	{
		m_X = x;
		m_Z = z;

		if (!m_InWorld)
		{
			m_InWorld = true;
			m_LastX = m_PrevX = m_X;
			m_LastZ = m_PrevZ = m_Z;
			m_LastYDifference = entity_pos_t::Zero();
		}

		AdvertisePositionChanges();
	}
	
	virtual void MoveAndTurnTo(entity_pos_t x, entity_pos_t z, entity_angle_t ry)
	{
		m_X = x;
		m_Z = z;
		m_RotY = ry;
		
		if (!m_InWorld)
		{
			m_InWorld = true;
			m_LastX = m_PrevX = m_X;
			m_LastZ = m_PrevZ = m_Z;
			m_LastYDifference = entity_pos_t::Zero();
		}
		
		AdvertisePositionChanges();
	}

	virtual void JumpTo(entity_pos_t x, entity_pos_t z)
	{
		m_LastX = m_PrevX = m_X = x;
		m_LastZ = m_PrevZ = m_Z = z;
		m_InWorld = true;

		UpdateXZRotation();

		m_LastInterpolatedRotX = m_InterpolatedRotX;
		m_LastInterpolatedRotZ = m_InterpolatedRotZ;

		AdvertisePositionChanges();
	}

	virtual void SetHeightOffset(entity_pos_t dy)
	{
		// subtract the offset and replace with a new offset
		m_LastYDifference = dy - GetHeightOffset();
		m_Y += m_LastYDifference;
		AdvertisePositionChanges();
	}

	virtual entity_pos_t GetHeightOffset()
	{
		if (m_RelativeToGround)
			return m_Y;
		// not relative to the ground, so the height offset is m_Y - ground height
		entity_pos_t baseY;
		CmpPtr<ICmpTerrain> cmpTerrain(GetSystemEntity());
		if (cmpTerrain)
			baseY = cmpTerrain->GetGroundLevel(m_X, m_Z);

		if (m_Floating)
		{
			CmpPtr<ICmpWaterManager> cmpWaterManager(GetSystemEntity());
			if (cmpWaterManager)
				baseY = std::max(baseY, cmpWaterManager->GetWaterLevel(m_X, m_Z));
		}
		return m_Y - baseY;
	}

	virtual void SetHeightFixed(entity_pos_t y)
	{
		// subtract the absolute height and replace it with a new absolute height
		m_LastYDifference = y - GetHeightFixed();
		m_Y += m_LastYDifference;
		AdvertisePositionChanges();
	}

	virtual entity_pos_t GetHeightFixed()
	{
		if (!m_RelativeToGround)
			return m_Y;
		// relative to the ground, so the fixed height = ground height + m_Y
		entity_pos_t baseY;
		CmpPtr<ICmpTerrain> cmpTerrain(GetSystemEntity());
		if (cmpTerrain)
			baseY = cmpTerrain->GetGroundLevel(m_X, m_Z);

		if (m_Floating)
		{
			CmpPtr<ICmpWaterManager> cmpWaterManager(GetSystemEntity());
			if (cmpWaterManager)
				baseY = std::max(baseY, cmpWaterManager->GetWaterLevel(m_X, m_Z));
		}
		return m_Y + baseY;
	}

	virtual bool IsHeightRelative()
	{
		return m_RelativeToGround;
	}

	virtual void SetHeightRelative(bool relative)
	{
		// move y to use the right offset (from terrain or from map origin)
		m_Y = relative ? GetHeightOffset() : GetHeightFixed();
		m_RelativeToGround = relative;
		m_LastYDifference = entity_pos_t::Zero();
	}

	virtual bool IsFloating()
	{
		return m_Floating;
	}

	virtual void SetFloating(bool flag)
	{
		m_Floating = flag;
	}

	virtual CFixedVector3D GetPosition()
	{
		if (!m_InWorld)
		{
			LOGERROR(L"CCmpPosition::GetPosition called on entity when IsInWorld is false");
			return CFixedVector3D();
		}

		return CFixedVector3D(m_X, GetHeightFixed(), m_Z);
	}

	virtual CFixedVector2D GetPosition2D()
	{
		if (!m_InWorld)
		{
			LOGERROR(L"CCmpPosition::GetPosition2D called on entity when IsInWorld is false");
			return CFixedVector2D();
		}

		return CFixedVector2D(m_X, m_Z);
	}

	virtual CFixedVector3D GetPreviousPosition() 
	{ 
		if (!m_InWorld) 
		{ 
			LOGERROR(L"CCmpPosition::GetPreviousPosition called on entity when IsInWorld is false"); 
			return CFixedVector3D(); 
		} 

		return CFixedVector3D(m_PrevX, GetHeightFixed(), m_PrevZ); 
	} 

	virtual CFixedVector2D GetPreviousPosition2D() 
	{ 
		if (!m_InWorld) 
		{ 
			LOGERROR(L"CCmpPosition::GetPreviousPosition2D called on entity when IsInWorld is false"); 
			return CFixedVector2D(); 
		} 

		return CFixedVector2D(m_PrevX, m_PrevZ); 
	}

	virtual void TurnTo(entity_angle_t y)
	{
		m_RotY = y;

		AdvertisePositionChanges();
	}

	virtual void SetYRotation(entity_angle_t y)
	{
		m_RotY = y;
		m_InterpolatedRotY = m_RotY.ToFloat();

		if (m_InWorld)
		{
			UpdateXZRotation();

			m_LastInterpolatedRotX = m_InterpolatedRotX;
			m_LastInterpolatedRotZ = m_InterpolatedRotZ;
		}

		AdvertisePositionChanges();
	}

	virtual void SetXZRotation(entity_angle_t x, entity_angle_t z)
	{
		m_RotX = x;
		m_RotZ = z;

		if (m_InWorld)
		{
			UpdateXZRotation();

			m_LastInterpolatedRotX = m_InterpolatedRotX;
			m_LastInterpolatedRotZ = m_InterpolatedRotZ;
		}

		AdvertisePositionChanges();
	}

	virtual CFixedVector3D GetRotation()
	{
		return CFixedVector3D(m_RotX, m_RotY, m_RotZ);
	}

	virtual fixed GetDistanceTravelled()
	{
		if (!m_InWorld)
		{
			LOGERROR(L"CCmpPosition::GetDistanceTravelled called on entity when IsInWorld is false");
			return fixed::Zero();
		}

		return CFixedVector2D(m_X - m_LastX, m_Z - m_LastZ).Length();
	}

	virtual void GetInterpolatedPosition2D(float frameOffset, float& x, float& z, float& rotY)
	{
		if (!m_InWorld)
		{
			LOGERROR(L"CCmpPosition::GetInterpolatedPosition2D called on entity when IsInWorld is false");
			return;
		}

		x = Interpolate(m_LastX.ToFloat(), m_X.ToFloat(), frameOffset);
		z = Interpolate(m_LastZ.ToFloat(), m_Z.ToFloat(), frameOffset);

		rotY = m_InterpolatedRotY;
	}

	virtual CMatrix3D GetInterpolatedTransform(float frameOffset, bool forceFloating)
	{
		if (!m_InWorld)
		{
			LOGERROR(L"CCmpPosition::GetInterpolatedTransform called on entity when IsInWorld is false");
			CMatrix3D m;
			m.SetIdentity();
			return m;
		}

		float x, z, rotY;
		GetInterpolatedPosition2D(frameOffset, x, z, rotY);

	
		float baseY = 0;
		if (m_RelativeToGround)
		{
			CmpPtr<ICmpTerrain> cmpTerrain(GetSystemEntity());
			if (cmpTerrain)
				baseY = cmpTerrain->GetExactGroundLevel(x, z);

			if (m_Floating || forceFloating)
			{
				CmpPtr<ICmpWaterManager> cmpWaterManager(GetSystemEntity());
				if (cmpWaterManager)
					baseY = std::max(baseY, cmpWaterManager->GetExactWaterLevel(x, z));
			}
		}

		float y = baseY + m_Y.ToFloat() + Interpolate(m_LastYDifference.ToFloat(), 0.f, frameOffset);

		CMatrix3D m;
		
		// linear interpolation is good enough (for RotX/Z). 
		// As you always stay close to zero angle.	
		m.SetXRotation(Interpolate(m_LastInterpolatedRotX, m_InterpolatedRotX, frameOffset));
		m.RotateZ(Interpolate(m_LastInterpolatedRotZ, m_InterpolatedRotZ, frameOffset));
	
		m.RotateY(rotY + (float)M_PI);
		m.Translate(CVector3D(x, y, z));
		
		return m;
	}

	virtual void HandleMessage(const CMessage& msg, bool UNUSED(global))
	{
		switch (msg.GetType())
		{
		case MT_Interpolate:
		{
			const CMessageInterpolate& msgData = static_cast<const CMessageInterpolate&> (msg);

			float rotY = m_RotY.ToFloat();

			if (rotY != m_InterpolatedRotY)
			{
				float delta = rotY - m_InterpolatedRotY;
				// Wrap delta to -M_PI..M_PI
				delta = fmodf(delta + (float)M_PI, 2*(float)M_PI); // range -2PI..2PI
				if (delta < 0) delta += 2*(float)M_PI; // range 0..2PI
				delta -= (float)M_PI; // range -M_PI..M_PI
				// Clamp to max rate
				float deltaClamped = clamp(delta, -m_RotYSpeed*msgData.deltaSimTime, +m_RotYSpeed*msgData.deltaSimTime);
				// Calculate new orientation, in a peculiar way in order to make sure the
				// result gets close to m_orientation (rather than being n*2*M_PI out)
				m_InterpolatedRotY = rotY + deltaClamped - delta;
				
				// update the visual XZ rotation
				if (m_InWorld)
				{
					m_LastInterpolatedRotX = m_InterpolatedRotX;
					m_LastInterpolatedRotZ = m_InterpolatedRotZ;

					UpdateXZRotation();
				}
			}
			
			if (m_InWorld && m_NeedInitialXZRotation)
			{
				// the terrain probably wasn't loaded last time we tried, so update the XZ rotation without interpolation
				UpdateXZRotation();

				m_LastInterpolatedRotX = m_InterpolatedRotX;
				m_LastInterpolatedRotZ = m_InterpolatedRotZ;
			}

			break;
		}
		case MT_TurnStart:
		{
			m_LastInterpolatedRotX = m_InterpolatedRotX;
			m_LastInterpolatedRotZ = m_InterpolatedRotZ;

			if (m_InWorld && (m_LastX != m_X || m_LastZ != m_Z))
				UpdateXZRotation();

			// Store the positions from the turn before
			m_PrevX = m_LastX;
			m_PrevZ = m_LastZ;

			m_LastX = m_X;
			m_LastZ = m_Z;
			m_LastYDifference = entity_pos_t::Zero();


			// warn when a position change also causes a territory change under the entity
			if (m_InWorld)
			{
				player_id_t newTerritory;
				CmpPtr<ICmpTerritoryManager> cmpTerritoryManager(GetSystemEntity());
				if (cmpTerritoryManager)
					newTerritory = cmpTerritoryManager->GetOwner(m_X, m_Z);
				else
					newTerritory = INVALID_PLAYER;
				if (newTerritory != m_Territory)
				{
					m_Territory = newTerritory;
					CMessageTerritoryPositionChanged msg(GetEntityId(), m_Territory);
					GetSimContext().GetComponentManager().PostMessage(GetEntityId(), msg);
				}
			}
			else if (m_Territory != INVALID_PLAYER)
			{
				CMessageTerritoryPositionChanged msg(GetEntityId(), m_Territory);
				GetSimContext().GetComponentManager().PostMessage(GetEntityId(), msg);
			}
			break;
		}
	}
};

private:
	void AdvertisePositionChanges()
	{
		if (m_InWorld)
		{
			CMessagePositionChanged msg(GetEntityId(), true, m_X, m_Z, m_RotY);
			GetSimContext().GetComponentManager().PostMessage(GetEntityId(), msg);
		}
		else
		{
			CMessagePositionChanged msg(GetEntityId(), false, entity_pos_t::Zero(), entity_pos_t::Zero(), entity_angle_t::Zero());
			GetSimContext().GetComponentManager().PostMessage(GetEntityId(), msg);
		}
	}

	void UpdateXZRotation()
	{
		if (!m_InWorld)
		{
			LOGERROR(L"CCmpPosition::UpdateXZRotation called on entity when IsInWorld is false");
			return;
		}

		if (m_AnchorType == UPRIGHT || !m_RotZ.IsZero() || !m_RotX.IsZero())
		{
			// set the visual rotations to the ones fixed by the interface
			m_InterpolatedRotX = m_RotX.ToFloat();
			m_InterpolatedRotZ = m_RotZ.ToFloat();
			return;
		}

		CmpPtr<ICmpTerrain> cmpTerrain(GetSystemEntity());
		if (!cmpTerrain || !cmpTerrain->IsLoaded())
		{
			// try again when terrain is loaded
			m_NeedInitialXZRotation = true;
			return;
		}

		// TODO: average normal (average all the tiles?) for big units or for buildings?
		CVector3D normal = cmpTerrain->CalcExactNormal(m_X.ToFloat(), m_Z.ToFloat());

		// rotate the normal so the positive x direction is in the direction of the unit
		CVector2D projected = CVector2D(normal.X, normal.Z);
		projected.Rotate(m_InterpolatedRotY);

		normal.X = projected.X;
		normal.Z = projected.Y;

		// project and calculate the angles
		if (m_AnchorType == PITCH || m_AnchorType == PITCH_ROLL)
			m_InterpolatedRotX = -atan2(normal.Z, normal.Y);

		if (m_AnchorType == ROLL || m_AnchorType == PITCH_ROLL)
			m_InterpolatedRotZ = atan2(normal.X, normal.Y);

		m_NeedInitialXZRotation = false;
		
		return;
	}
};

REGISTER_COMPONENT_TYPE(Position)
