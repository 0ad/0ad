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

#include "ICmpRangeManager.h"

#include "simulation2/system/InterfaceScripted.h"

namespace {
	std::string VisibilityToString(LosVisibility visibility)
	{
		switch (visibility)
		{
		case LosVisibility::HIDDEN: return "hidden";
		case LosVisibility::FOGGED: return "fogged";
		case LosVisibility::VISIBLE: return "visible";
		default: return "error"; // should never happen
		}
	}
}

std::string ICmpRangeManager::GetLosVisibility_wrapper(entity_id_t ent, int player) const
{
	return VisibilityToString(GetLosVisibility(ent, player));
}

std::string ICmpRangeManager::GetLosVisibilityPosition_wrapper(entity_pos_t x, entity_pos_t z, int player) const
{
	return VisibilityToString(GetLosVisibilityPosition(x, z, player));
}

BEGIN_INTERFACE_WRAPPER(RangeManager)
DEFINE_INTERFACE_METHOD("ExecuteQuery", ICmpRangeManager, ExecuteQuery)
DEFINE_INTERFACE_METHOD("ExecuteQueryAroundPos", ICmpRangeManager, ExecuteQueryAroundPos)
DEFINE_INTERFACE_METHOD("CreateActiveQuery", ICmpRangeManager, CreateActiveQuery)
DEFINE_INTERFACE_METHOD("CreateActiveParabolicQuery", ICmpRangeManager, CreateActiveParabolicQuery)
DEFINE_INTERFACE_METHOD("DestroyActiveQuery", ICmpRangeManager, DestroyActiveQuery)
DEFINE_INTERFACE_METHOD("EnableActiveQuery", ICmpRangeManager, EnableActiveQuery)
DEFINE_INTERFACE_METHOD("DisableActiveQuery", ICmpRangeManager, DisableActiveQuery)
DEFINE_INTERFACE_METHOD("IsActiveQueryEnabled", ICmpRangeManager, IsActiveQueryEnabled)
DEFINE_INTERFACE_METHOD("ResetActiveQuery", ICmpRangeManager, ResetActiveQuery)
DEFINE_INTERFACE_METHOD("SetEntityFlag", ICmpRangeManager, SetEntityFlag)
DEFINE_INTERFACE_METHOD("GetEntityFlagMask", ICmpRangeManager, GetEntityFlagMask)
DEFINE_INTERFACE_METHOD("GetEntitiesByPlayer", ICmpRangeManager, GetEntitiesByPlayer)
DEFINE_INTERFACE_METHOD("GetNonGaiaEntities", ICmpRangeManager, GetNonGaiaEntities)
DEFINE_INTERFACE_METHOD("GetGaiaAndNonGaiaEntities", ICmpRangeManager, GetGaiaAndNonGaiaEntities)
DEFINE_INTERFACE_METHOD("SetDebugOverlay", ICmpRangeManager, SetDebugOverlay)
DEFINE_INTERFACE_METHOD("ExploreMap", ICmpRangeManager, ExploreMap)
DEFINE_INTERFACE_METHOD("ExploreTerritories", ICmpRangeManager, ExploreTerritories)
DEFINE_INTERFACE_METHOD("SetLosRevealAll", ICmpRangeManager, SetLosRevealAll)
DEFINE_INTERFACE_METHOD("GetLosRevealAll", ICmpRangeManager, GetLosRevealAll)
DEFINE_INTERFACE_METHOD("GetEffectiveParabolicRange", ICmpRangeManager, GetEffectiveParabolicRange)
DEFINE_INTERFACE_METHOD("GetElevationAdaptedRange", ICmpRangeManager, GetElevationAdaptedRange)
DEFINE_INTERFACE_METHOD("ActivateScriptedVisibility", ICmpRangeManager, ActivateScriptedVisibility)
DEFINE_INTERFACE_METHOD("GetLosVisibility", ICmpRangeManager, GetLosVisibility_wrapper)
DEFINE_INTERFACE_METHOD("GetLosVisibilityPosition", ICmpRangeManager, GetLosVisibilityPosition_wrapper)
DEFINE_INTERFACE_METHOD("RequestVisibilityUpdate", ICmpRangeManager, RequestVisibilityUpdate)
DEFINE_INTERFACE_METHOD("SetLosCircular", ICmpRangeManager, SetLosCircular)
DEFINE_INTERFACE_METHOD("GetLosCircular", ICmpRangeManager, GetLosCircular)
DEFINE_INTERFACE_METHOD("SetSharedLos", ICmpRangeManager, SetSharedLos)
DEFINE_INTERFACE_METHOD("GetPercentMapExplored", ICmpRangeManager, GetPercentMapExplored)
DEFINE_INTERFACE_METHOD("GetUnionPercentMapExplored", ICmpRangeManager, GetUnionPercentMapExplored)
END_INTERFACE_WRAPPER(RangeManager)
