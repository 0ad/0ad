/* Copyright (C) 2020 Wildfire Games.
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

#ifndef INCLUDED_JSI_SIMULATION
#define INCLUDED_JSI_SIMULATION

#include "scriptinterface/ScriptInterface.h"
#include "simulation2/helpers/Position.h"
#include "simulation2/system/Entity.h"

namespace JSI_Simulation
{
	JS::Value GuiInterfaceCall(ScriptInterface::RealmPrivate* pRealmPrivate, const std::wstring& name, JS::HandleValue data);
	void PostNetworkCommand(ScriptInterface::RealmPrivate* pRealmPrivate, JS::HandleValue cmd);
	entity_id_t PickEntityAtPoint(ScriptInterface::RealmPrivate* pRealmPrivate, int x, int y);
	void DumpSimState(ScriptInterface::RealmPrivate* pRealmPrivate);
	std::vector<entity_id_t> PickPlayerEntitiesInRect(ScriptInterface::RealmPrivate* pRealmPrivate, int x0, int y0, int x1, int y1, int player);
	std::vector<entity_id_t> PickPlayerEntitiesOnScreen(ScriptInterface::RealmPrivate* pRealmPrivate, int player);
	std::vector<entity_id_t> PickNonGaiaEntitiesOnScreen(ScriptInterface::RealmPrivate* pRealmPrivate);
	std::vector<entity_id_t> GetEntitiesWithStaticObstructionOnScreen(ScriptInterface::RealmPrivate* pRealmPrivate);
	JS::Value GetEdgesOfStaticObstructionsOnScreenNearTo(ScriptInterface::RealmPrivate* pRealmPrivate, entity_pos_t x, entity_pos_t z);
	std::vector<entity_id_t> PickSimilarPlayerEntities(ScriptInterface::RealmPrivate* pRealmPrivate, const std::string& templateName, bool includeOffScreen, bool matchRank, bool allowFoundations);
	JS::Value GetAIs(ScriptInterface::RealmPrivate* pRealmPrivate);
	void SetBoundingBoxDebugOverlay(ScriptInterface::RealmPrivate* pRealmPrivate, bool enabled);

	void RegisterScriptFunctions(const ScriptInterface& ScriptInterface);
}

#endif // INCLUDED_JSI_SIMULATION
