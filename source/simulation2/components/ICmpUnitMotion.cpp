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

#include "ICmpUnitMotion.h"

#include "simulation2/system/InterfaceScripted.h"

BEGIN_INTERFACE_WRAPPER(UnitMotion)
DEFINE_INTERFACE_METHOD_4("MoveToPointRange", bool, ICmpUnitMotion, MoveToPointRange, entity_pos_t, entity_pos_t, entity_pos_t, entity_pos_t)
DEFINE_INTERFACE_METHOD_3("IsInTargetRange", bool, ICmpUnitMotion, IsInTargetRange, entity_id_t, entity_pos_t, entity_pos_t)
DEFINE_INTERFACE_METHOD_3("MoveToTargetRange", bool, ICmpUnitMotion, MoveToTargetRange, entity_id_t, entity_pos_t, entity_pos_t)
DEFINE_INTERFACE_METHOD_3("MoveToFormationOffset", void, ICmpUnitMotion, MoveToFormationOffset, entity_id_t, entity_pos_t, entity_pos_t)
DEFINE_INTERFACE_METHOD_0("StopMoving", void, ICmpUnitMotion, StopMoving)
DEFINE_INTERFACE_METHOD_1("SetSpeed", void, ICmpUnitMotion, SetSpeed, fixed)
DEFINE_INTERFACE_METHOD_0("GetWalkSpeed", fixed, ICmpUnitMotion, GetWalkSpeed)
DEFINE_INTERFACE_METHOD_0("GetRunSpeed", fixed, ICmpUnitMotion, GetRunSpeed)
DEFINE_INTERFACE_METHOD_1("SetUnitRadius", void, ICmpUnitMotion, SetUnitRadius, fixed)
DEFINE_INTERFACE_METHOD_1("SetDebugOverlay", void, ICmpUnitMotion, SetDebugOverlay, bool)
END_INTERFACE_WRAPPER(UnitMotion)
