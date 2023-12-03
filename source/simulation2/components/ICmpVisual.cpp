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

#include "ICmpVisual.h"

#include "simulation2/system/InterfaceScripted.h"

BEGIN_INTERFACE_WRAPPER(Visual)
DEFINE_INTERFACE_METHOD("SetVariant", ICmpVisual, SetVariant)
DEFINE_INTERFACE_METHOD("GetAnimationName", ICmpVisual, GetAnimationName)
DEFINE_INTERFACE_METHOD("GetProjectileActor", ICmpVisual, GetProjectileActor)
DEFINE_INTERFACE_METHOD("GetProjectileLaunchPoint", ICmpVisual, GetProjectileLaunchPoint)
DEFINE_INTERFACE_METHOD("SelectAnimation", ICmpVisual, SelectAnimation)
DEFINE_INTERFACE_METHOD("SetAnimationSyncRepeat", ICmpVisual, SetAnimationSyncRepeat)
DEFINE_INTERFACE_METHOD("SetAnimationSyncOffset", ICmpVisual, SetAnimationSyncOffset)
DEFINE_INTERFACE_METHOD("SetShadingColor", ICmpVisual, SetShadingColor)
DEFINE_INTERFACE_METHOD("SetVariable", ICmpVisual, SetVariable)
DEFINE_INTERFACE_METHOD("GetActorSeed", ICmpVisual, GetActorSeed)
DEFINE_INTERFACE_METHOD("SetActorSeed", ICmpVisual, SetActorSeed)
DEFINE_INTERFACE_METHOD("RecomputeActorName", ICmpVisual, RecomputeActorName)
DEFINE_INTERFACE_METHOD("HasConstructionPreview", ICmpVisual, HasConstructionPreview)
END_INTERFACE_WRAPPER(Visual)
