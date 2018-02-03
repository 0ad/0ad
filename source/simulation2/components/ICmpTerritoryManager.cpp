/* Copyright (C) 2018 Wildfire Games.
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

#include "ICmpTerritoryManager.h"

#include "simulation2/system/InterfaceScripted.h"

BEGIN_INTERFACE_WRAPPER(TerritoryManager)
DEFINE_INTERFACE_METHOD_2("GetOwner", player_id_t, ICmpTerritoryManager, GetOwner, entity_pos_t, entity_pos_t)
DEFINE_INTERFACE_METHOD_3("GetNeighbours", std::vector<u32>, ICmpTerritoryManager, GetNeighbours, entity_pos_t, entity_pos_t, bool)
DEFINE_INTERFACE_METHOD_2("IsConnected", bool, ICmpTerritoryManager, IsConnected, entity_pos_t, entity_pos_t)
DEFINE_INTERFACE_METHOD_3("SetTerritoryBlinking", void, ICmpTerritoryManager, SetTerritoryBlinking, entity_pos_t, entity_pos_t, bool)
DEFINE_INTERFACE_METHOD_2("IsTerritoryBlinking", bool, ICmpTerritoryManager, IsTerritoryBlinking, entity_pos_t, entity_pos_t)
DEFINE_INTERFACE_METHOD_1("GetTerritoryPercentage", u8, ICmpTerritoryManager, GetTerritoryPercentage, player_id_t)
DEFINE_INTERFACE_METHOD_0("UpdateColors", void, ICmpTerritoryManager, UpdateColors)
END_INTERFACE_WRAPPER(TerritoryManager)
