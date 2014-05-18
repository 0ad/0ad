/* Copyright (C) 2011 Wildfire Games.
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
DEFINE_INTERFACE_METHOD_4("IsInPointRange", bool, ICmpUnitMotion, IsInPointRange, entity_pos_t, entity_pos_t, entity_pos_t, entity_pos_t)
DEFINE_INTERFACE_METHOD_3("IsInTargetRange", bool, ICmpUnitMotion, IsInTargetRange, entity_id_t, entity_pos_t, entity_pos_t)
DEFINE_INTERFACE_METHOD_3("MoveToTargetRange", bool, ICmpUnitMotion, MoveToTargetRange, entity_id_t, entity_pos_t, entity_pos_t)
DEFINE_INTERFACE_METHOD_3("MoveToFormationOffset", void, ICmpUnitMotion, MoveToFormationOffset, entity_id_t, entity_pos_t, entity_pos_t)
DEFINE_INTERFACE_METHOD_2("FaceTowardsPoint", void, ICmpUnitMotion, FaceTowardsPoint, entity_pos_t, entity_pos_t)
DEFINE_INTERFACE_METHOD_0("StopMoving", void, ICmpUnitMotion, StopMoving)
DEFINE_INTERFACE_METHOD_0("GetCurrentSpeed", fixed, ICmpUnitMotion, GetCurrentSpeed)
DEFINE_INTERFACE_METHOD_1("SetSpeed", void, ICmpUnitMotion, SetSpeed, fixed)
DEFINE_INTERFACE_METHOD_0("IsMoving", bool, ICmpUnitMotion, IsMoving)
DEFINE_INTERFACE_METHOD_0("GetWalkSpeed", fixed, ICmpUnitMotion, GetWalkSpeed)
DEFINE_INTERFACE_METHOD_0("GetRunSpeed", fixed, ICmpUnitMotion, GetRunSpeed)
DEFINE_INTERFACE_METHOD_0("GetPassabilityClassName", std::string, ICmpUnitMotion, GetPassabilityClassName)
DEFINE_INTERFACE_METHOD_1("SetPassabilityClassName", void, ICmpUnitMotion, SetPassabilityClassName, std::string)
DEFINE_INTERFACE_METHOD_1("SetFacePointAfterMove", void, ICmpUnitMotion, SetFacePointAfterMove, bool)
DEFINE_INTERFACE_METHOD_1("SetUnitRadius", void, ICmpUnitMotion, SetUnitRadius, fixed)
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

	virtual bool IsInPointRange(entity_pos_t x, entity_pos_t z, entity_pos_t minRange, entity_pos_t maxRange)
	{
		return m_Script.Call<bool>("IsInPointRange", x, z, minRange, maxRange);
	}

	virtual bool IsInTargetRange(entity_id_t target, entity_pos_t minRange, entity_pos_t maxRange)
	{
		return m_Script.Call<bool>("IsInTargetRange", target, minRange, maxRange);
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

	virtual fixed GetCurrentSpeed()
	{
		return m_Script.Call<fixed>("GetCurrentSpeed");
	}

	virtual void SetSpeed(fixed speed)
	{
		m_Script.CallVoid("SetSpeed", speed);
	}

	virtual bool IsMoving()
	{
		return m_Script.Call<bool>("IsMoving");
	}

	virtual fixed GetWalkSpeed()
	{
		return m_Script.Call<fixed>("GetWalkSpeed");
	}

	virtual fixed GetRunSpeed()
	{
		return m_Script.Call<fixed>("GetRunSpeed");
	}

	virtual void SetFacePointAfterMove(bool facePointAfterMove)
	{
		m_Script.CallVoid("SetFacePointAfterMove", facePointAfterMove);
	}

	virtual ICmpPathfinder::pass_class_t GetPassabilityClass()
	{
		return m_Script.Call<ICmpPathfinder::pass_class_t>("GetPassabilityClass");
	}

	virtual std::string GetPassabilityClassName()
	{
		return m_Script.Call<std::string>("GetPassabilityClassName");
	}

	virtual void SetPassabilityClassName(std::string passClassName)
	{
		m_Script.CallVoid("SetPassabilityClassName", passClassName);
	}

	virtual void SetUnitRadius(fixed radius)
	{
		m_Script.CallVoid("SetUnitRadius", radius);
	}

	virtual void SetDebugOverlay(bool enabled)
	{
		m_Script.CallVoid("SetDebugOverlay", enabled);
	}

};

REGISTER_COMPONENT_SCRIPT_WRAPPER(UnitMotionScripted)
