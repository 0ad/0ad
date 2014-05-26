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

#include "ICmpPosition.h"

#include "simulation2/system/InterfaceScripted.h"

BEGIN_INTERFACE_WRAPPER(Position)
DEFINE_INTERFACE_METHOD_0("IsInWorld", bool, ICmpPosition, IsInWorld)
DEFINE_INTERFACE_METHOD_0("MoveOutOfWorld", void, ICmpPosition, MoveOutOfWorld)
DEFINE_INTERFACE_METHOD_2("MoveTo", void, ICmpPosition, MoveTo, entity_pos_t, entity_pos_t)
DEFINE_INTERFACE_METHOD_3("MoveAndTurnTo", void, ICmpPosition, MoveAndTurnTo, entity_pos_t, entity_pos_t, entity_pos_t)
DEFINE_INTERFACE_METHOD_2("JumpTo", void, ICmpPosition, JumpTo, entity_pos_t, entity_pos_t)
DEFINE_INTERFACE_METHOD_1("SetHeightOffset", void, ICmpPosition, SetHeightOffset, entity_pos_t)
DEFINE_INTERFACE_METHOD_0("GetHeightOffset", entity_pos_t, ICmpPosition, GetHeightOffset)
DEFINE_INTERFACE_METHOD_1("SetHeightFixed", void, ICmpPosition, SetHeightFixed, entity_pos_t)
DEFINE_INTERFACE_METHOD_0("GetHeightFixed", entity_pos_t, ICmpPosition, GetHeightFixed)
DEFINE_INTERFACE_METHOD_0("IsHeightRelative", bool, ICmpPosition, IsHeightRelative)
DEFINE_INTERFACE_METHOD_1("SetHeightRelative", void, ICmpPosition, SetHeightRelative, bool)
DEFINE_INTERFACE_METHOD_0("IsFloating", bool, ICmpPosition, IsFloating)
DEFINE_INTERFACE_METHOD_1("SetFloating", void, ICmpPosition, SetFloating, bool)
DEFINE_INTERFACE_METHOD_0("GetPosition", CFixedVector3D, ICmpPosition, GetPosition)
DEFINE_INTERFACE_METHOD_0("GetPosition2D", CFixedVector2D, ICmpPosition, GetPosition2D)
DEFINE_INTERFACE_METHOD_0("GetPreviousPosition", CFixedVector3D, ICmpPosition, GetPreviousPosition) 
DEFINE_INTERFACE_METHOD_0("GetPreviousPosition2D", CFixedVector2D, ICmpPosition, GetPreviousPosition2D) 
DEFINE_INTERFACE_METHOD_1("TurnTo", void, ICmpPosition, TurnTo, entity_angle_t)
DEFINE_INTERFACE_METHOD_1("SetYRotation", void, ICmpPosition, SetYRotation, entity_angle_t)
DEFINE_INTERFACE_METHOD_2("SetXZRotation", void, ICmpPosition, SetXZRotation, entity_angle_t, entity_angle_t)
DEFINE_INTERFACE_METHOD_0("GetRotation", CFixedVector3D, ICmpPosition, GetRotation)
// Excluded: GetInterpolatedTransform (not safe for scripts)
END_INTERFACE_WRAPPER(Position)
