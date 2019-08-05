/* Copyright (C) 2019 Wildfire Games.
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
DEFINE_INTERFACE_METHOD_4("MoveToPointRange", bool, ICmpUnitMotion, MoveToPointRange, entity_pos_t, entity_pos_t, entity_pos_t, entity_pos_t)
DEFINE_INTERFACE_METHOD_3("MoveToTargetRange", bool, ICmpUnitMotion, MoveToTargetRange, entity_id_t, entity_pos_t, entity_pos_t)
DEFINE_INTERFACE_METHOD_3("MoveToFormationOffset", void, ICmpUnitMotion, MoveToFormationOffset, entity_id_t, entity_pos_t, entity_pos_t)
DEFINE_INTERFACE_METHOD_2("FaceTowardsPoint", void, ICmpUnitMotion, FaceTowardsPoint, entity_pos_t, entity_pos_t)
DEFINE_INTERFACE_METHOD_0("StopMoving", void, ICmpUnitMotion, StopMoving)
DEFINE_INTERFACE_METHOD_CONST_0("GetCurrentSpeed", fixed, ICmpUnitMotion, GetCurrentSpeed)
DEFINE_INTERFACE_METHOD_CONST_0("IsMoveRequested", bool, ICmpUnitMotion, IsMoveRequested)
DEFINE_INTERFACE_METHOD_CONST_0("GetSpeed", fixed, ICmpUnitMotion, GetSpeed)
DEFINE_INTERFACE_METHOD_CONST_0("GetWalkSpeed", fixed, ICmpUnitMotion, GetWalkSpeed)
DEFINE_INTERFACE_METHOD_CONST_0("GetRunMultiplier", fixed, ICmpUnitMotion, GetRunMultiplier)
DEFINE_INTERFACE_METHOD_1("SetSpeedMultiplier", void, ICmpUnitMotion, SetSpeedMultiplier, fixed)
DEFINE_INTERFACE_METHOD_CONST_0("GetPassabilityClassName", std::string, ICmpUnitMotion, GetPassabilityClassName)
DEFINE_INTERFACE_METHOD_CONST_0("GetUnitClearance", entity_pos_t, ICmpUnitMotion, GetUnitClearance)
DEFINE_INTERFACE_METHOD_1("SetFacePointAfterMove", void, ICmpUnitMotion, SetFacePointAfterMove, bool)
DEFINE_INTERFACE_METHOD_1("SetDebugOverlay", void, ICmpUnitMotion, SetDebugOverlay, bool)
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

	virtual void SetFacePointAfterMove(bool facePointAfterMove)
	{
		m_Script.CallVoid("SetFacePointAfterMove", facePointAfterMove);
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
