/* Copyright (C) 2021 Wildfire Games.
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

#include "ICmpUnitMotion.h"

#include "simulation2/system/InterfaceScripted.h"
#include "simulation2/scripting/ScriptComponent.h"

BEGIN_INTERFACE_WRAPPER(UnitMotion)
DEFINE_INTERFACE_METHOD("MoveToPointRange", ICmpUnitMotion, MoveToPointRange)
DEFINE_INTERFACE_METHOD("MoveToTargetRange", ICmpUnitMotion, MoveToTargetRange)
DEFINE_INTERFACE_METHOD("MoveToFormationOffset", ICmpUnitMotion, MoveToFormationOffset)
DEFINE_INTERFACE_METHOD("SetMemberOfFormation", ICmpUnitMotion, SetMemberOfFormation)
DEFINE_INTERFACE_METHOD("IsTargetRangeReachable", ICmpUnitMotion, IsTargetRangeReachable)
DEFINE_INTERFACE_METHOD("FaceTowardsPoint", ICmpUnitMotion, FaceTowardsPoint)
DEFINE_INTERFACE_METHOD("StopMoving", ICmpUnitMotion, StopMoving)
DEFINE_INTERFACE_METHOD("GetCurrentSpeed", ICmpUnitMotion, GetCurrentSpeed)
DEFINE_INTERFACE_METHOD("IsMoveRequested", ICmpUnitMotion, IsMoveRequested)
DEFINE_INTERFACE_METHOD("GetSpeed", ICmpUnitMotion, GetSpeed)
DEFINE_INTERFACE_METHOD("GetWalkSpeed", ICmpUnitMotion, GetWalkSpeed)
DEFINE_INTERFACE_METHOD("GetRunMultiplier", ICmpUnitMotion, GetRunMultiplier)
DEFINE_INTERFACE_METHOD("EstimateFuturePosition", ICmpUnitMotion, EstimateFuturePosition)
DEFINE_INTERFACE_METHOD("SetSpeedMultiplier", ICmpUnitMotion, SetSpeedMultiplier)
DEFINE_INTERFACE_METHOD("GetAcceleration", ICmpUnitMotion, GetAcceleration)
DEFINE_INTERFACE_METHOD("SetAcceleration", ICmpUnitMotion, SetAcceleration)
DEFINE_INTERFACE_METHOD("GetPassabilityClassName", ICmpUnitMotion, GetPassabilityClassName)
DEFINE_INTERFACE_METHOD("GetUnitClearance", ICmpUnitMotion, GetUnitClearance)
DEFINE_INTERFACE_METHOD("SetFacePointAfterMove", ICmpUnitMotion, SetFacePointAfterMove)
DEFINE_INTERFACE_METHOD("GetFacePointAfterMove", ICmpUnitMotion, GetFacePointAfterMove)
DEFINE_INTERFACE_METHOD("SetDebugOverlay", ICmpUnitMotion, SetDebugOverlay)
END_INTERFACE_WRAPPER(UnitMotion)

class CCmpUnitMotionScripted : public ICmpUnitMotion
{
public:
	DEFAULT_SCRIPT_WRAPPER(UnitMotionScripted)

	virtual bool MoveToPointRange(entity_pos_t x, entity_pos_t z, entity_pos_t minRange, entity_pos_t maxRange)
	{
		return m_Script.Call<bool>("MoveToPointRange", x, z, minRange, maxRange);
	}

	virtual bool MoveToTargetRange(entity_id_t target, entity_pos_t minRange, entity_pos_t maxRange)
	{
		return m_Script.Call<bool>("MoveToTargetRange", target, minRange, maxRange);
	}

	virtual void MoveToFormationOffset(entity_id_t target, entity_pos_t x, entity_pos_t z)
	{
		m_Script.CallVoid("MoveToFormationOffset", target, x, z);
	}

	virtual void SetMemberOfFormation(entity_id_t controller)
	{
		m_Script.CallVoid("SetMemberOfFormation", controller);
	}

	virtual bool IsTargetRangeReachable(entity_id_t target, entity_pos_t minRange, entity_pos_t maxRange)
	{
		return m_Script.Call<bool>("IsTargetRangeReachable", target, minRange, maxRange);
	}

	virtual void FaceTowardsPoint(entity_pos_t x, entity_pos_t z)
	{
		m_Script.CallVoid("FaceTowardsPoint", x, z);
	}

	virtual void StopMoving()
	{
		m_Script.CallVoid("StopMoving");
	}

	virtual fixed GetCurrentSpeed() const
	{
		return m_Script.Call<fixed>("GetCurrentSpeed");
	}

	virtual bool IsMoveRequested() const
	{
		return m_Script.Call<bool>("IsMoveRequested");
	}

	virtual fixed GetSpeed() const
	{
		return m_Script.Call<fixed>("GetSpeed");
	}

	virtual fixed GetWalkSpeed() const
	{
		return m_Script.Call<fixed>("GetWalkSpeed");
	}

	virtual fixed GetRunMultiplier() const
	{
		return m_Script.Call<fixed>("GetRunMultiplier");
	}

	virtual void SetSpeedMultiplier(fixed multiplier)
	{
		m_Script.CallVoid("SetSpeedMultiplier", multiplier);
	}

	virtual fixed GetSpeedMultiplier() const
	{
		return m_Script.Call<fixed>("GetSpeedMultiplier");
	}

	virtual CFixedVector2D EstimateFuturePosition(const fixed dt) const
	{
		return m_Script.Call<CFixedVector2D>("EstimateFuturePosition", dt);
	}

	virtual fixed GetAcceleration() const
	{
		return m_Script.Call<fixed>("GetAcceleration");
	}

	virtual void SetAcceleration(fixed acceleration)
	{
		m_Script.CallVoid("SetAcceleration", acceleration);
	}

	virtual void SetFacePointAfterMove(bool facePointAfterMove)
	{
		m_Script.CallVoid("SetFacePointAfterMove", facePointAfterMove);
	}

	virtual bool GetFacePointAfterMove() const
	{
		return m_Script.Call<bool>("GetFacePointAfterMove");
	}

	virtual pass_class_t GetPassabilityClass() const
	{
		return m_Script.Call<pass_class_t>("GetPassabilityClass");
	}

	virtual std::string GetPassabilityClassName() const
	{
		return m_Script.Call<std::string>("GetPassabilityClassName");
	}

	virtual entity_pos_t GetUnitClearance() const
	{
		return m_Script.Call<entity_pos_t>("GetUnitClearance");
	}

	virtual void SetDebugOverlay(bool enabled)
	{
		m_Script.CallVoid("SetDebugOverlay", enabled);
	}

};

REGISTER_COMPONENT_SCRIPT_WRAPPER(UnitMotionScripted)
