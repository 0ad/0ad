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

#include "precompiled.h"

#include "JSInterface_Simulation.h"

#include "graphics/GameView.h"
#include "ps/ConfigDB.h"
#include "ps/Game.h"
#include "ps/GameSetup/Config.h"
#include "ps/Pyrogenesis.h"
#include "scriptinterface/ScriptInterface.h"
#include "simulation2/Simulation2.h"
#include "simulation2/system/Entity.h"
#include "simulation2/components/ICmpAIManager.h"
#include "simulation2/components/ICmpCommandQueue.h"
#include "simulation2/components/ICmpGuiInterface.h"
#include "simulation2/components/ICmpPosition.h"
#include "simulation2/components/ICmpSelectable.h"
#include "simulation2/helpers/Selection.h"

#include <array>
#include <fstream>

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

std::vector<entity_id_t> JSI_Simulation::GetEntitiesWithStaticObstructionOnScreen(ScriptInterface::CxPrivate* UNUSED(pCxPrivate))
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

JS::Value JSI_Simulation::GetEdgesOfStaticObstructionsOnScreenNearTo(ScriptInterface::CxPrivate* pCxPrivate, entity_pos_t x, entity_pos_t z)
{
	if (!g_Game)
		return JS::UndefinedValue();

	CSimulation2* sim = g_Game->GetSimulation2();
	ENSURE(sim);

	JSContext* cx = pCxPrivate->pScriptInterface->GetContext();
	JSAutoRequest rq(cx);
	JS::RootedValue edgeList(cx);
	ScriptInterface::CreateArray(cx, &edgeList);
	int edgeListIndex = 0;

	float distanceThreshold = 10.0f;
	CFG_GET_VAL("gui.session.snaptoedgesdistancethreshold", distanceThreshold);
	CFixedVector2D entityPos(x, z);

	std::vector<entity_id_t> entities = GetEntitiesWithStaticObstructionOnScreen(pCxPrivate);
	for (entity_id_t entity : entities)
	{
		CmpPtr<ICmpObstruction> cmpObstruction(sim->GetSimContext(), entity);
		if (!cmpObstruction)
			continue;

		CmpPtr<ICmpPosition> cmpPosition(sim->GetSimContext(), entity);
		if (!cmpPosition || !cmpPosition->IsInWorld())
			continue;
		
		CFixedVector2D halfSize = cmpObstruction->GetStaticSize() / 2;
		if (halfSize.X.IsZero() || halfSize.Y.IsZero() || std::max(halfSize.X, halfSize.Y) <= fixed::FromInt(2))
			continue;

		std::array<CFixedVector2D, 4> corners = {
			CFixedVector2D(-halfSize.X, -halfSize.Y),
			CFixedVector2D(-halfSize.X, halfSize.Y),
			halfSize,
			CFixedVector2D(halfSize.X, -halfSize.Y)
		};
		fixed angle = cmpPosition->GetRotation().Y;
		for (CFixedVector2D& corner : corners)
			corner = corner.Rotate(angle) + cmpPosition->GetPosition2D();

		for (size_t i = 0; i < corners.size(); ++i)
		{
			JS::RootedValue edge(cx);
			const CFixedVector2D& corner = corners[i];
			const CFixedVector2D& nextCorner = corners[(i + 1) % corners.size()];

			// TODO: calculate real distance;
			fixed distanceToEdge = std::min(
				(corner - entityPos).Length(),
				(nextCorner - entityPos).Length());
			if (distanceToEdge.ToFloat() > distanceThreshold)
				continue;

			CFixedVector2D normal = -(nextCorner - corner).Perpendicular();
			normal.Normalize();
			ScriptInterface::CreateObject(
				cx,
				&edge,
				"begin", corner,
				"end", nextCorner,
				"angle", angle,
				"normal", normal,
				"order", "cw");

			pCxPrivate->pScriptInterface->SetPropertyInt(edgeList, edgeListIndex++, edge);
		}
	}
	return edgeList;
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
	scriptInterface.RegisterFunction<JS::Value, std::wstring, JS::HandleValue, &GuiInterfaceCall>("GuiInterfaceCall");
	scriptInterface.RegisterFunction<void, JS::HandleValue, &PostNetworkCommand>("PostNetworkCommand");
	scriptInterface.RegisterFunction<void, &DumpSimState>("DumpSimState");
	scriptInterface.RegisterFunction<JS::Value, &GetAIs>("GetAIs");
	scriptInterface.RegisterFunction<entity_id_t, int, int, &PickEntityAtPoint>("PickEntityAtPoint");
	scriptInterface.RegisterFunction<std::vector<entity_id_t>, int, int, int, int, int, &PickPlayerEntitiesInRect>("PickPlayerEntitiesInRect");
	scriptInterface.RegisterFunction<std::vector<entity_id_t>, int, &PickPlayerEntitiesOnScreen>("PickPlayerEntitiesOnScreen");
	scriptInterface.RegisterFunction<std::vector<entity_id_t>, &PickNonGaiaEntitiesOnScreen>("PickNonGaiaEntitiesOnScreen");
	scriptInterface.RegisterFunction<std::vector<entity_id_t>, &GetEntitiesWithStaticObstructionOnScreen>("GetEntitiesWithStaticObstructionOnScreen");
	scriptInterface.RegisterFunction<JS::Value, entity_pos_t, entity_pos_t, &GetEdgesOfStaticObstructionsOnScreenNearTo>("GetEdgesOfStaticObstructionsOnScreenNearTo");
	scriptInterface.RegisterFunction<std::vector<entity_id_t>, std::string, bool, bool, bool, &PickSimilarPlayerEntities>("PickSimilarPlayerEntities");
	scriptInterface.RegisterFunction<void, bool, &SetBoundingBoxDebugOverlay>("SetBoundingBoxDebugOverlay");
}
