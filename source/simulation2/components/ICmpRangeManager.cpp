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

BEGIN_INTERFACE_WRAPPER(RangeManager)
DEFINE_INTERFACE_METHOD_4("ExecuteQuery", std::vector<entity_id_t>, ICmpRangeManager, ExecuteQuery, entity_id_t, entity_pos_t, std::vector<int>, int)
DEFINE_INTERFACE_METHOD_4("CreateActiveQuery", ICmpRangeManager::tag_t, ICmpRangeManager, CreateActiveQuery, entity_id_t, entity_pos_t, std::vector<int>, int)
DEFINE_INTERFACE_METHOD_1("DestroyActiveQuery", void, ICmpRangeManager, DestroyActiveQuery, ICmpRangeManager::tag_t)
DEFINE_INTERFACE_METHOD_1("EnableActiveQuery", void, ICmpRangeManager, EnableActiveQuery, ICmpRangeManager::tag_t)
DEFINE_INTERFACE_METHOD_1("DisableActiveQuery", void, ICmpRangeManager, DisableActiveQuery, ICmpRangeManager::tag_t)
DEFINE_INTERFACE_METHOD_1("ResetActiveQuery", std::vector<entity_id_t>, ICmpRangeManager, ResetActiveQuery, ICmpRangeManager::tag_t)
DEFINE_INTERFACE_METHOD_1("SetDebugOverlay", void, ICmpRangeManager, SetDebugOverlay, bool)
DEFINE_INTERFACE_METHOD_1("SetLosRevealAll", void, ICmpRangeManager, SetLosRevealAll, bool)
END_INTERFACE_WRAPPER(RangeManager)
