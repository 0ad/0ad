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

#include "JSInterface_Simulation.h"

#include "graphics/GameView.h"
#include "ps/Game.h"
#include "ps/GameSetup/Config.h"
#include "ps/Pyrogenesis.h"
#include "scriptinterface/ScriptInterface.h"
#include "simulation2/Simulation2.h"
#include "simulation2/system/Entity.h"
#include "simulation2/components/ICmpAIManager.h"
#include "simulation2/components/ICmpCommandQueue.h"
#include "simulation2/components/ICmpGuiInterface.h"
#include "simulation2/components/ICmpSelectable.h"
#include "simulation2/helpers/Selection.h"

#include <fstream>

JS::Value JSI_Simulation::GetInitAttributes(ScriptInterface::CxPrivate* pCxPrivate)
{
	if (!g_Game)
		return JS::UndefinedValue();

	JSContext* cx = g_Game->GetSimulation2()->GetScriptInterface().GetContext();
	JSAutoRequest rq(cx);

	JS::RootedValue initAttribs(cx);
	g_Game->GetSimulation2()->GetInitAttributes(&initAttribs);

	return pCxPrivate->pScriptInterface->CloneValueFromOtherContext(
		g_Game->GetSimulation2()->GetScriptInterface(),
		initAttribs);
}

JS::Value JSI_Simulation::GuiInterfaceCall(ScriptInterface::CxPrivate* pCxPrivate, const std::wstring& name, JS::HandleValue data)
{
	if (!g_Game)
		return JS::UndefinedValue();

	CSimulation2* sim = g_Game->GetSimulation2();
	ENSURE(sim);

	CmpPtr<ICmpGuiInterface> cmpGuiInterface(*sim, SYSTEM_ENTITY);
	if (!cmpGuiInterface)
		return JS::UndefinedValue();

	JSContext* cxSim = sim->GetScriptInterface().GetContext();
	JSAutoRequest rqSim(cxSim);
	JS::RootedValue arg(cxSim, sim->GetScriptInterface().CloneValueFromOtherContext(*(pCxPrivate->pScriptInterface), data));
	JS::RootedValue ret(cxSim);
	cmpGuiInterface->ScriptCall(g_Game->GetViewedPlayerID(), name, arg, &ret);

	return pCxPrivate->pScriptInterface->CloneValueFromOtherContext(sim->GetScriptInterface(), ret);
}

void JSI_Simulation::PostNetworkCommand(ScriptInterface::CxPrivate* pCxPrivate, JS::HandleValue cmd)
{
	if (!g_Game)
		return;

	CSimulation2* sim = g_Game->GetSimulation2();
	ENSURE(sim);

	CmpPtr<ICmpCommandQueue> cmpCommandQueue(*sim, SYSTEM_ENTITY);
	if (!cmpCommandQueue)
		return;

	JSContext* cxSim = sim->GetScriptInterface().GetContext();
	JSAutoRequest rqSim(cxSim);
	JS::RootedValue cmd2(cxSim, sim->GetScriptInterface().CloneValueFromOtherContext(*(pCxPrivate->pScriptInterface), cmd));

	cmpCommandQueue->PostNetworkCommand(cmd2);
}

void JSI_Simulation::DumpSimState(ScriptInterface::CxPrivate* UNUSED(pCxPrivate))
{
	OsPath path = psLogDir()/"sim_dump.txt";
	std::ofstream file (OsString(path).c_str(), std::ofstream::out | std::ofstream::trunc);
	g_Game->GetSimulation2()->DumpDebugState(file);
}

entity_id_t JSI_Simulation::PickEntityAtPoint(ScriptInterface::CxPrivate* UNUSED(pCxPrivate), int x, int y)
{
	return EntitySelection::PickEntityAtPoint(*g_Game->GetSimulation2(), *g_Game->GetView()->GetCamera(), x, y, g_Game->GetViewedPlayerID(), false);
}

std::vector<entity_id_t> JSI_Simulation::PickPlayerEntitiesInRect(ScriptInterface::CxPrivate* UNUSED(pCxPrivate), int x0, int y0, int x1, int y1, int player)
{
	return EntitySelection::PickEntitiesInRect(*g_Game->GetSimulation2(), *g_Game->GetView()->GetCamera(), x0, y0, x1, y1, player, false);
}

std::vector<entity_id_t> JSI_Simulation::PickPlayerEntitiesOnScreen(ScriptInterface::CxPrivate* UNUSED(pCxPrivate), int player)
{
	return EntitySelection::PickEntitiesInRect(*g_Game->GetSimulation2(), *g_Game->GetView()->GetCamera(), 0, 0, g_xres, g_yres, player, false);
}

std::vector<entity_id_t> JSI_Simulation::PickNonGaiaEntitiesOnScreen(ScriptInterface::CxPrivate* UNUSED(pCxPrivate))
{
	return EntitySelection::PickNonGaiaEntitiesInRect(*g_Game->GetSimulation2(), *g_Game->GetView()->GetCamera(), 0, 0, g_xres, g_yres, false);
}

std::vector<entity_id_t> JSI_Simulation::GetEntitiesWithStaticObstructionOnScreen(ScriptInterface::CxPrivate* pCxPrivate)
{
	struct StaticObstructionFilter
	{
		bool operator()(IComponent* cmp)
		{
			ICmpObstruction* cmpObstruction = static_cast<ICmpObstruction*>(cmp);
			return cmpObstruction->GetObstructionType() == ICmpObstruction::STATIC;
		}
	};
	return EntitySelection::GetEntitiesWithComponentInRect<StaticObstructionFilter>(*g_Game->GetSimulation2(), IID_Obstruction, *g_Game->GetView()->GetCamera(), 0, 0, g_xres, g_yres);
}

std::vector<entity_id_t> JSI_Simulation::PickSimilarPlayerEntities(ScriptInterface::CxPrivate* UNUSED(pCxPrivate), const std::string& templateName, bool includeOffScreen, bool matchRank, bool allowFoundations)
{
	return EntitySelection::PickSimilarEntities(*g_Game->GetSimulation2(), *g_Game->GetView()->GetCamera(), templateName, g_Game->GetViewedPlayerID(), includeOffScreen, matchRank, false, allowFoundations);
}

JS::Value JSI_Simulation::GetAIs(ScriptInterface::CxPrivate* pCxPrivate)
{
	return ICmpAIManager::GetAIs(*(pCxPrivate->pScriptInterface));
}

void JSI_Simulation::SetBoundingBoxDebugOverlay(ScriptInterface::CxPrivate* UNUSED(pCxPrivate), bool enabled)
{
	ICmpSelectable::ms_EnableDebugOverlays = enabled;
}

void JSI_Simulation::RegisterScriptFunctions(const ScriptInterface& scriptInterface)
{
	scriptInterface.RegisterFunction<JS::Value, &GetInitAttributes>("GetInitAttributes");
	scriptInterface.RegisterFunction<JS::Value, std::wstring, JS::HandleValue, &GuiInterfaceCall>("GuiInterfaceCall");
	scriptInterface.RegisterFunction<void, JS::HandleValue, &PostNetworkCommand>("PostNetworkCommand");
	scriptInterface.RegisterFunction<void, &DumpSimState>("DumpSimState");
	scriptInterface.RegisterFunction<JS::Value, &GetAIs>("GetAIs");
	scriptInterface.RegisterFunction<entity_id_t, int, int, &PickEntityAtPoint>("PickEntityAtPoint");
	scriptInterface.RegisterFunction<std::vector<entity_id_t>, int, int, int, int, int, &PickPlayerEntitiesInRect>("PickPlayerEntitiesInRect");
	scriptInterface.RegisterFunction<std::vector<entity_id_t>, int, &PickPlayerEntitiesOnScreen>("PickPlayerEntitiesOnScreen");
	scriptInterface.RegisterFunction<std::vector<entity_id_t>, &PickNonGaiaEntitiesOnScreen>("PickNonGaiaEntitiesOnScreen");
	scriptInterface.RegisterFunction<std::vector<entity_id_t>, &GetEntitiesWithStaticObstructionOnScreen>("GetEntitiesWithStaticObstructionOnScreen");
	scriptInterface.RegisterFunction<std::vector<entity_id_t>, std::string, bool, bool, bool, &PickSimilarPlayerEntities>("PickSimilarPlayerEntities");
	scriptInterface.RegisterFunction<void, bool, &SetBoundingBoxDebugOverlay>("SetBoundingBoxDebugOverlay");
}
