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

#include "ICmpObstruction.h"

#include "simulation2/system/InterfaceScripted.h"

#include "simulation2/system/SimContext.h"

std::string ICmpObstruction::CheckFoundation_wrapper(const std::string& className, bool onlyCenterPoint) const
{
	EFoundationCheck check = CheckFoundation(className, onlyCenterPoint);

	switch (check)
	{
	case FOUNDATION_CHECK_SUCCESS:
		return "success";
	case FOUNDATION_CHECK_FAIL_ERROR:
		return "fail_error";
	case FOUNDATION_CHECK_FAIL_NO_OBSTRUCTION:
		return "fail_no_obstruction";
	case FOUNDATION_CHECK_FAIL_OBSTRUCTS_FOUNDATION:
		return "fail_obstructs_foundation";
	case FOUNDATION_CHECK_FAIL_TERRAIN_CLASS:
		return "fail_terrain_class";
	default:
		debug_warn(L"Unexpected result from CheckFoundation");
		return "";
	}
}

BEGIN_INTERFACE_WRAPPER(Obstruction)
DEFINE_INTERFACE_METHOD("GetSize", ICmpObstruction, GetSize)
DEFINE_INTERFACE_METHOD("CheckShorePlacement", ICmpObstruction, CheckShorePlacement)
DEFINE_INTERFACE_METHOD("CheckFoundation", ICmpObstruction, CheckFoundation_wrapper)
DEFINE_INTERFACE_METHOD("CheckDuplicateFoundation", ICmpObstruction, CheckDuplicateFoundation)
DEFINE_INTERFACE_METHOD("GetEntitiesBlockingMovement", ICmpObstruction, GetEntitiesBlockingMovement)
DEFINE_INTERFACE_METHOD("GetEntitiesBlockingConstruction", ICmpObstruction, GetEntitiesBlockingConstruction)
DEFINE_INTERFACE_METHOD("GetEntitiesDeletedUponConstruction", ICmpObstruction, GetEntitiesDeletedUponConstruction)
DEFINE_INTERFACE_METHOD("SetActive", ICmpObstruction, SetActive)
DEFINE_INTERFACE_METHOD("SetDisableBlockMovementPathfinding", ICmpObstruction, SetDisableBlockMovementPathfinding)
DEFINE_INTERFACE_METHOD("GetBlockMovementFlag", ICmpObstruction, GetBlockMovementFlag)
DEFINE_INTERFACE_METHOD("SetControlGroup", ICmpObstruction, SetControlGroup)
DEFINE_INTERFACE_METHOD("GetControlGroup", ICmpObstruction, GetControlGroup)
DEFINE_INTERFACE_METHOD("SetControlGroup2", ICmpObstruction, SetControlGroup2)
DEFINE_INTERFACE_METHOD("GetControlGroup2", ICmpObstruction, GetControlGroup2)
END_INTERFACE_WRAPPER(Obstruction)
