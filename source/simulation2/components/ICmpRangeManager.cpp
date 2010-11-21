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

#include "ICmpRangeManager.h"

#include "simulation2/system/InterfaceScripted.h"

std::string ICmpRangeManager::GetLosVisibility_wrapper(entity_id_t ent, int player)
{
	ELosVisibility visibility = GetLosVisibility(ent, player);
	switch (visibility)
	{
	case VIS_HIDDEN: return "hidden";
	case VIS_FOGGED: return "fogged";
	case VIS_VISIBLE: return "visible";
	default: return "error"; // should never happen
	}
}

BEGIN_INTERFACE_WRAPPER(RangeManager)
DEFINE_INTERFACE_METHOD_5("ExecuteQuery", std::vector<entity_id_t>, ICmpRangeManager, ExecuteQuery, entity_id_t, entity_pos_t, entity_pos_t, std::vector<int>, int)
DEFINE_INTERFACE_METHOD_5("CreateActiveQuery", ICmpRangeManager::tag_t, ICmpRangeManager, CreateActiveQuery, entity_id_t, entity_pos_t, entity_pos_t, std::vector<int>, int)
DEFINE_INTERFACE_METHOD_1("DestroyActiveQuery", void, ICmpRangeManager, DestroyActiveQuery, ICmpRangeManager::tag_t)
DEFINE_INTERFACE_METHOD_1("EnableActiveQuery", void, ICmpRangeManager, EnableActiveQuery, ICmpRangeManager::tag_t)
DEFINE_INTERFACE_METHOD_1("DisableActiveQuery", void, ICmpRangeManager, DisableActiveQuery, ICmpRangeManager::tag_t)
DEFINE_INTERFACE_METHOD_1("ResetActiveQuery", std::vector<entity_id_t>, ICmpRangeManager, ResetActiveQuery, ICmpRangeManager::tag_t)
DEFINE_INTERFACE_METHOD_1("GetEntitiesByPlayer", std::vector<entity_id_t>, ICmpRangeManager, GetEntitiesByPlayer, int)
DEFINE_INTERFACE_METHOD_1("SetDebugOverlay", void, ICmpRangeManager, SetDebugOverlay, bool)
DEFINE_INTERFACE_METHOD_1("SetLosRevealAll", void, ICmpRangeManager, SetLosRevealAll, bool)
DEFINE_INTERFACE_METHOD_2("GetLosVisibility", std::string, ICmpRangeManager, GetLosVisibility_wrapper, entity_id_t, int)
DEFINE_INTERFACE_METHOD_1("SetLosCircular", void, ICmpRangeManager, SetLosCircular, bool)
DEFINE_INTERFACE_METHOD_0("GetLosCircular", bool, ICmpRangeManager, GetLosCircular)
DEFINE_INTERFACE_METHOD_1("GetPercentMapExplored", i32, ICmpRangeManager, GetPercentMapExplored, player_id_t)
END_INTERFACE_WRAPPER(RangeManager)
