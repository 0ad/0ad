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

#include "ICmpPosition.h"

#include "simulation2/system/InterfaceScripted.h"

BEGIN_INTERFACE_WRAPPER(Position)
DEFINE_INTERFACE_METHOD("SetTurretParent", ICmpPosition, SetTurretParent)
DEFINE_INTERFACE_METHOD("GetTurretParent", ICmpPosition, GetTurretParent)
DEFINE_INTERFACE_METHOD("IsInWorld", ICmpPosition, IsInWorld)
DEFINE_INTERFACE_METHOD("MoveOutOfWorld", ICmpPosition, MoveOutOfWorld)
DEFINE_INTERFACE_METHOD("MoveTo", ICmpPosition, MoveTo)
DEFINE_INTERFACE_METHOD("MoveAndTurnTo", ICmpPosition, MoveAndTurnTo)
DEFINE_INTERFACE_METHOD("JumpTo", ICmpPosition, JumpTo)
DEFINE_INTERFACE_METHOD("SetHeightOffset", ICmpPosition, SetHeightOffset)
DEFINE_INTERFACE_METHOD("GetHeightOffset", ICmpPosition, GetHeightOffset)
DEFINE_INTERFACE_METHOD("SetHeightFixed", ICmpPosition, SetHeightFixed)
DEFINE_INTERFACE_METHOD("GetHeightAt", ICmpPosition, GetHeightAtFixed)
DEFINE_INTERFACE_METHOD("IsHeightRelative", ICmpPosition, IsHeightRelative)
DEFINE_INTERFACE_METHOD("SetHeightRelative", ICmpPosition, SetHeightRelative)
DEFINE_INTERFACE_METHOD("CanFloat", ICmpPosition, CanFloat)
DEFINE_INTERFACE_METHOD("SetFloating", ICmpPosition, SetFloating)
DEFINE_INTERFACE_METHOD("SetConstructionProgress", ICmpPosition, SetConstructionProgress)
DEFINE_INTERFACE_METHOD("GetPosition", ICmpPosition, GetPosition)
DEFINE_INTERFACE_METHOD("GetPosition2D", ICmpPosition, GetPosition2D)
DEFINE_INTERFACE_METHOD("GetPreviousPosition", ICmpPosition, GetPreviousPosition)
DEFINE_INTERFACE_METHOD("GetPreviousPosition2D", ICmpPosition, GetPreviousPosition2D)
DEFINE_INTERFACE_METHOD("GetTurnRate", ICmpPosition, GetTurnRate)
DEFINE_INTERFACE_METHOD("TurnTo", ICmpPosition, TurnTo)
DEFINE_INTERFACE_METHOD("SetYRotation", ICmpPosition, SetYRotation)
DEFINE_INTERFACE_METHOD("SetXZRotation", ICmpPosition, SetXZRotation)
DEFINE_INTERFACE_METHOD("GetRotation", ICmpPosition, GetRotation)
// Excluded: GetInterpolatedTransform (not safe for scripts)
END_INTERFACE_WRAPPER(Position)
