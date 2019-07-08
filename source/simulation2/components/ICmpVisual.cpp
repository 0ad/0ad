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

#include "ICmpVisual.h"

#include "simulation2/system/InterfaceScripted.h"

BEGIN_INTERFACE_WRAPPER(Visual)
DEFINE_INTERFACE_METHOD_2("SetVariant", void, ICmpVisual, SetVariant, CStr, CStr)
DEFINE_INTERFACE_METHOD_CONST_0("GetAnimationName", std::string, ICmpVisual, GetAnimationName)
DEFINE_INTERFACE_METHOD_CONST_0("GetProjectileActor", std::wstring, ICmpVisual, GetProjectileActor)
DEFINE_INTERFACE_METHOD_CONST_0("GetProjectileLaunchPoint", CFixedVector3D, ICmpVisual, GetProjectileLaunchPoint)
DEFINE_INTERFACE_METHOD_3("SelectAnimation", void, ICmpVisual, SelectAnimation, std::string, bool, fixed)
DEFINE_INTERFACE_METHOD_1("SelectMovementAnimation", void, ICmpVisual, SelectMovementAnimation, fixed)
DEFINE_INTERFACE_METHOD_1("SetAnimationSyncRepeat", void, ICmpVisual, SetAnimationSyncRepeat, fixed)
DEFINE_INTERFACE_METHOD_1("SetAnimationSyncOffset", void, ICmpVisual, SetAnimationSyncOffset, fixed)
DEFINE_INTERFACE_METHOD_4("SetShadingColor", void, ICmpVisual, SetShadingColor, fixed, fixed, fixed, fixed)
DEFINE_INTERFACE_METHOD_2("SetVariable", void, ICmpVisual, SetVariable, std::string, float)
DEFINE_INTERFACE_METHOD_CONST_0("GetActorSeed", u32, ICmpVisual, GetActorSeed)
DEFINE_INTERFACE_METHOD_1("SetActorSeed", void, ICmpVisual, SetActorSeed, u32)
DEFINE_INTERFACE_METHOD_CONST_0("HasConstructionPreview", bool, ICmpVisual, HasConstructionPreview)
END_INTERFACE_WRAPPER(Visual)
