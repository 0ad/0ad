/* Copyright (C) 2022 Wildfire Games.
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

	bool MoveToPointRange(entity_pos_t x, entity_pos_t z, entity_pos_t minRange, entity_pos_t maxRange) override
	{
		return m_Script.Call<bool>("MoveToPointRange", x, z, minRange, maxRange);
	}

	bool MoveToTargetRange(entity_id_t target, entity_pos_t minRange, entity_pos_t maxRange) override
	{
		return m_Script.Call<bool>("MoveToTargetRange", target, minRange, maxRange);
	}

	void MoveToFormationOffset(entity_id_t target, entity_pos_t x, entity_pos_t z) override
	{
		m_Script.CallVoid("MoveToFormationOffset", target, x, z);
	}

	void SetMemberOfFormation(entity_id_t controller) override
	{
		m_Script.CallVoid("SetMemberOfFormation", controller);
	}

	bool IsTargetRangeReachable(entity_id_t target, entity_pos_t minRange, entity_pos_t maxRange) override
	{
		return m_Script.Call<bool>("IsTargetRangeReachable", target, minRange, maxRange);
	}

	void FaceTowardsPoint(entity_pos_t x, entity_pos_t z) override
	{
		m_Script.CallVoid("FaceTowardsPoint", x, z);
	}

	void StopMoving() override
	{
		m_Script.CallVoid("StopMoving");
	}

	fixed GetCurrentSpeed() const override
	{
		return m_Script.Call<fixed>("GetCurrentSpeed");
	}

	bool IsMoveRequested() const override
	{
		return m_Script.Call<bool>("IsMoveRequested");
	}

	fixed GetSpeed() const override
	{
		return m_Script.Call<fixed>("GetSpeed");
	}

	fixed GetWalkSpeed() const override
	{
		return m_Script.Call<fixed>("GetWalkSpeed");
	}

	fixed GetRunMultiplier() const override
	{
		return m_Script.Call<fixed>("GetRunMultiplier");
	}

	void SetSpeedMultiplier(fixed multiplier) override
	{
		m_Script.CallVoid("SetSpeedMultiplier", multiplier);
	}

	fixed GetSpeedMultiplier() const override
	{
		return m_Script.Call<fixed>("GetSpeedMultiplier");
	}

	CFixedVector2D EstimateFuturePosition(const fixed dt) const override
	{
		return m_Script.Call<CFixedVector2D>("EstimateFuturePosition", dt);
	}

	fixed GetAcceleration() const override
	{
		return m_Script.Call<fixed>("GetAcceleration");
	}

	void SetAcceleration(fixed acceleration) override
	{
		m_Script.CallVoid("SetAcceleration", acceleration);
	}

	void SetFacePointAfterMove(bool facePointAfterMove) override
	{
		m_Script.CallVoid("SetFacePointAfterMove", facePointAfterMove);
	}

	bool GetFacePointAfterMove() const override
	{
		return m_Script.Call<bool>("GetFacePointAfterMove");
	}

	pass_class_t GetPassabilityClass() const override
	{
		return m_Script.Call<pass_class_t>("GetPassabilityClass");
	}

	std::string GetPassabilityClassName() const override
	{
		return m_Script.Call<std::string>("GetPassabilityClassName");
	}

	entity_pos_t GetUnitClearance() const override
	{
		return m_Script.Call<entity_pos_t>("GetUnitClearance");
	}

	void SetDebugOverlay(bool enabled) override
	{
		m_Script.CallVoid("SetDebugOverlay", enabled);
	}

};

REGISTER_COMPONENT_SCRIPT_WRAPPER(UnitMotionScripted)
