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

#include "ICmpRangeManager.h"

#include "simulation2/system/InterfaceScripted.h"

namespace {
	std::string VisibilityToString(ICmpRangeManager::ELosVisibility visibility)
	{
		switch (visibility)
		{
		case ICmpRangeManager::VIS_HIDDEN: return "hidden";
		case ICmpRangeManager::VIS_FOGGED: return "fogged";
		case ICmpRangeManager::VIS_VISIBLE: return "visible";
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
DEFINE_INTERFACE_METHOD_5("ExecuteQuery", std::vector<entity_id_t>, ICmpRangeManager, ExecuteQuery, entity_id_t, entity_pos_t, entity_pos_t, std::vector<int>, int)
DEFINE_INTERFACE_METHOD_5("ExecuteQueryAroundPos", std::vector<entity_id_t>, ICmpRangeManager, ExecuteQueryAroundPos, CFixedVector2D, entity_pos_t, entity_pos_t, std::vector<int>, int)
DEFINE_INTERFACE_METHOD_6("CreateActiveQuery", ICmpRangeManager::tag_t, ICmpRangeManager, CreateActiveQuery, entity_id_t, entity_pos_t, entity_pos_t, std::vector<int>, int, u8)
DEFINE_INTERFACE_METHOD_7("CreateActiveParabolicQuery", ICmpRangeManager::tag_t, ICmpRangeManager, CreateActiveParabolicQuery, entity_id_t, entity_pos_t, entity_pos_t, entity_pos_t, std::vector<int>, int, u8)
DEFINE_INTERFACE_METHOD_1("DestroyActiveQuery", void, ICmpRangeManager, DestroyActiveQuery, ICmpRangeManager::tag_t)
DEFINE_INTERFACE_METHOD_1("EnableActiveQuery", void, ICmpRangeManager, EnableActiveQuery, ICmpRangeManager::tag_t)
DEFINE_INTERFACE_METHOD_1("DisableActiveQuery", void, ICmpRangeManager, DisableActiveQuery, ICmpRangeManager::tag_t)
DEFINE_INTERFACE_METHOD_CONST_1("IsActiveQueryEnabled", bool, ICmpRangeManager, IsActiveQueryEnabled, ICmpRangeManager::tag_t)
DEFINE_INTERFACE_METHOD_1("ResetActiveQuery", std::vector<entity_id_t>, ICmpRangeManager, ResetActiveQuery, ICmpRangeManager::tag_t)
DEFINE_INTERFACE_METHOD_3("SetEntityFlag", void, ICmpRangeManager, SetEntityFlag, entity_id_t, std::string, bool)
DEFINE_INTERFACE_METHOD_CONST_1("GetEntityFlagMask", u8, ICmpRangeManager, GetEntityFlagMask, std::string)
DEFINE_INTERFACE_METHOD_CONST_1("GetEntitiesByPlayer", std::vector<entity_id_t>, ICmpRangeManager, GetEntitiesByPlayer, player_id_t)
DEFINE_INTERFACE_METHOD_CONST_0("GetNonGaiaEntities", std::vector<entity_id_t>, ICmpRangeManager, GetNonGaiaEntities)
DEFINE_INTERFACE_METHOD_CONST_0("GetGaiaAndNonGaiaEntities", std::vector<entity_id_t>, ICmpRangeManager, GetGaiaAndNonGaiaEntities)
DEFINE_INTERFACE_METHOD_1("SetDebugOverlay", void, ICmpRangeManager, SetDebugOverlay, bool)
DEFINE_INTERFACE_METHOD_1("ExploreAllTiles", void, ICmpRangeManager, ExploreAllTiles, player_id_t)
DEFINE_INTERFACE_METHOD_0("ExploreTerritories", void, ICmpRangeManager, ExploreTerritories)
DEFINE_INTERFACE_METHOD_2("SetLosRevealAll", void, ICmpRangeManager, SetLosRevealAll, player_id_t, bool)
DEFINE_INTERFACE_METHOD_CONST_1("GetLosRevealAll", bool, ICmpRangeManager, GetLosRevealAll, player_id_t)
DEFINE_INTERFACE_METHOD_CONST_5("GetElevationAdaptedRange", entity_pos_t, ICmpRangeManager, GetElevationAdaptedRange, CFixedVector3D, CFixedVector3D, entity_pos_t, entity_pos_t, entity_pos_t)
DEFINE_INTERFACE_METHOD_2("ActivateScriptedVisibility", void, ICmpRangeManager, ActivateScriptedVisibility, entity_id_t, bool)
DEFINE_INTERFACE_METHOD_CONST_2("GetLosVisibility", std::string, ICmpRangeManager, GetLosVisibility_wrapper, entity_id_t, player_id_t)
DEFINE_INTERFACE_METHOD_CONST_3("GetLosVisibilityPosition", std::string, ICmpRangeManager, GetLosVisibilityPosition_wrapper, entity_pos_t, entity_pos_t, player_id_t)
DEFINE_INTERFACE_METHOD_1("RequestVisibilityUpdate", void, ICmpRangeManager, RequestVisibilityUpdate, entity_id_t)
DEFINE_INTERFACE_METHOD_1("SetLosCircular", void, ICmpRangeManager, SetLosCircular, bool)
DEFINE_INTERFACE_METHOD_CONST_0("GetLosCircular", bool, ICmpRangeManager, GetLosCircular)
DEFINE_INTERFACE_METHOD_2("SetSharedLos", void, ICmpRangeManager, SetSharedLos, player_id_t, std::vector<player_id_t>)
DEFINE_INTERFACE_METHOD_CONST_1("GetPercentMapExplored", u8, ICmpRangeManager, GetPercentMapExplored, player_id_t)
DEFINE_INTERFACE_METHOD_CONST_1("GetUnionPercentMapExplored", u8, ICmpRangeManager, GetUnionPercentMapExplored, std::vector<player_id_t>)
END_INTERFACE_WRAPPER(RangeManager)
