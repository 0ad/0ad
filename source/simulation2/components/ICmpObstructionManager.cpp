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

#include "ICmpObstructionManager.h"

#include "simulation2/system/InterfaceScripted.h"

BEGIN_INTERFACE_WRAPPER(ObstructionManager)
DEFINE_INTERFACE_METHOD("SetPassabilityCircular", ICmpObstructionManager, SetPassabilityCircular)
DEFINE_INTERFACE_METHOD("SetDebugOverlay", ICmpObstructionManager, SetDebugOverlay)
DEFINE_INTERFACE_METHOD("DistanceToPoint", ICmpObstructionManager, DistanceToPoint)
DEFINE_INTERFACE_METHOD("MaxDistanceToPoint", ICmpObstructionManager, MaxDistanceToPoint)
DEFINE_INTERFACE_METHOD("DistanceToTarget", ICmpObstructionManager, DistanceToTarget)
DEFINE_INTERFACE_METHOD("MaxDistanceToTarget", ICmpObstructionManager, MaxDistanceToTarget)
DEFINE_INTERFACE_METHOD("IsInPointRange", ICmpObstructionManager, IsInPointRange)
DEFINE_INTERFACE_METHOD("IsInTargetRange", ICmpObstructionManager, IsInTargetRange)
DEFINE_INTERFACE_METHOD("IsInTargetParabolicRange", ICmpObstructionManager, IsInTargetParabolicRange)
DEFINE_INTERFACE_METHOD("IsPointInPointRange", ICmpObstructionManager, IsPointInPointRange)
END_INTERFACE_WRAPPER(ObstructionManager)
