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

#include "scriptinterface/ScriptInterface.h"

#include "graphics/Camera.h"
#include "graphics/GameView.h"
#include "graphics/MapReader.h"
#include "gui/GUIManager.h"
#include "lib/timer.h"
#include "lib/sysdep/sysdep.h"
#include "maths/FixedVector3D.h"
#include "network/NetClient.h"
#include "network/NetServer.h"
#include "network/NetTurnManager.h"
#include "ps/CLogger.h"
#include "ps/CConsole.h"
#include "ps/Errors.h"
#include "ps/Game.h"
#include "ps/Overlay.h"
#include "ps/Pyrogenesis.h"
#include "ps/GameSetup/Atlas.h"
#include "ps/GameSetup/Config.h"

#include "simulation2/Simulation2.h"
#include "simulation2/components/ICmpCommandQueue.h"
#include "simulation2/components/ICmpGuiInterface.h"
#include "simulation2/components/ICmpRangeManager.h"
#include "simulation2/components/ICmpTemplateManager.h"
#include "simulation2/helpers/Selection.h"

#include "js/jsapi.h"

/*
 * This file defines a set of functions that are available to GUI scripts, to allow
 * interaction with the rest of the engine.
 * Functions are exposed to scripts within the global object 'Engine', so
 * scripts should call "Engine.FunctionName(...)" etc.
 */

extern void restart_mainloop_in_atlas(); // from main.cpp

namespace {

CScriptVal GetActiveGui(void* UNUSED(cbdata))
{
	return OBJECT_TO_JSVAL(g_GUI->GetScriptObject());
}

void PushGuiPage(void* UNUSED(cbdata), std::wstring name, CScriptVal initData)
{
	g_GUI->PushPage(name, initData);
}

void SwitchGuiPage(void* UNUSED(cbdata), std::wstring name, CScriptVal initData)
{
	g_GUI->SwitchPage(name, initData);
}

void PopGuiPage(void* UNUSED(cbdata))
{
	g_GUI->PopPage();
}

CScriptVal GuiInterfaceCall(void* cbdata, std::wstring name, CScriptVal data)
{
	CGUIManager* guiManager = static_cast<CGUIManager*> (cbdata);

	if (!g_Game)
		return JSVAL_VOID;
	CSimulation2* sim = g_Game->GetSimulation2();
	debug_assert(sim);

	CmpPtr<ICmpGuiInterface> gui(*sim, SYSTEM_ENTITY);
	if (gui.null())
		return JSVAL_VOID;

	int player = -1;
	if (g_Game)
		player = g_Game->GetPlayerID();

	CScriptValRooted arg (sim->GetScriptInterface().GetContext(), sim->GetScriptInterface().CloneValueFromOtherContext(guiManager->GetScriptInterface(), data.get()));
	CScriptVal ret (gui->ScriptCall(player, name, arg.get()));
	return guiManager->GetScriptInterface().CloneValueFromOtherContext(sim->GetScriptInterface(), ret.get());
}

void PostNetworkCommand(void* cbdata, CScriptVal cmd)
{
	CGUIManager* guiManager = static_cast<CGUIManager*> (cbdata);

	if (!g_Game)
		return;
	CSimulation2* sim = g_Game->GetSimulation2();
	debug_assert(sim);

	CmpPtr<ICmpCommandQueue> queue(*sim, SYSTEM_ENTITY);
	if (queue.null())
		return;

	jsval cmd2 = sim->GetScriptInterface().CloneValueFromOtherContext(guiManager->GetScriptInterface(), cmd.get());

	queue->PostNetworkCommand(cmd2);
}

std::vector<entity_id_t> PickEntitiesAtPoint(void* UNUSED(cbdata), int x, int y)
{
	return EntitySelection::PickEntitiesAtPoint(*g_Game->GetSimulation2(), *g_Game->GetView()->GetCamera(), x, y, g_Game->GetPlayerID());
}

std::vector<entity_id_t> PickFriendlyEntitiesInRect(void* UNUSED(cbdata), int x0, int y0, int x1, int y1, int player)
{
	return EntitySelection::PickEntitiesInRect(*g_Game->GetSimulation2(), *g_Game->GetView()->GetCamera(), x0, y0, x1, y1, player);
}

CFixedVector3D GetTerrainAtPoint(void* UNUSED(cbdata), int x, int y)
{
	CVector3D pos = g_Game->GetView()->GetCamera()->GetWorldCoordinates(x, y, true);
	return CFixedVector3D(fixed::FromFloat(pos.X), fixed::FromFloat(pos.Y), fixed::FromFloat(pos.Z));
}

std::wstring SetCursor(void* UNUSED(cbdata), std::wstring name)
{
	std::wstring old = g_CursorName;
	g_CursorName = name;
	return old;
}

int GetPlayerID(void* UNUSED(cbdata))
{
	if (g_Game)
		return g_Game->GetPlayerID();
	return -1;
}

std::wstring GetDefaultPlayerName(void* UNUSED(cbdata))
{
	// TODO: this should come from a config file or something
	std::wstring name = sys_get_user_name();
	if (name.empty())
		name = L"anonymous";
	return name;
}

void StartNetworkGame(void* UNUSED(cbdata))
{
	debug_assert(g_NetServer);
	g_NetServer->StartGame();
}

void StartGame(void* cbdata, CScriptVal attribs, int playerID)
{
	CGUIManager* guiManager = static_cast<CGUIManager*> (cbdata);

	debug_assert(!g_NetServer);
	debug_assert(!g_NetClient);

	debug_assert(!g_Game);
	g_Game = new CGame();

	// Convert from GUI script context to sim script context
	CSimulation2* sim = g_Game->GetSimulation2();
	CScriptValRooted gameAttribs (sim->GetScriptInterface().GetContext(),
			sim->GetScriptInterface().CloneValueFromOtherContext(guiManager->GetScriptInterface(), attribs.get()));

	g_Game->SetPlayerID(playerID);
	g_Game->StartGame(gameAttribs);
}

void SetNetworkGameAttributes(void* cbdata, CScriptVal attribs)
{
	CGUIManager* guiManager = static_cast<CGUIManager*> (cbdata);

	debug_assert(g_NetServer);

	// Convert from GUI script context to net server script context
	CScriptValRooted gameAttribs (g_NetServer->GetScriptInterface().GetContext(),
			g_NetServer->GetScriptInterface().CloneValueFromOtherContext(guiManager->GetScriptInterface(), attribs.get()));

	g_NetServer->UpdateGameAttributes(gameAttribs);
}

void StartNetworkHost(void* cbdata, std::wstring playerName)
{
	CGUIManager* guiManager = static_cast<CGUIManager*> (cbdata);

	debug_assert(!g_NetClient);
	debug_assert(!g_NetServer);
	debug_assert(!g_Game);

	g_NetServer = new CNetServer();
	if (!g_NetServer->SetupConnection())
	{
		guiManager->GetScriptInterface().ReportError("Failed to start server");
		SAFE_DELETE(g_NetServer);
		return;
	}

	g_Game = new CGame();
	g_NetClient = new CNetClient(g_Game);
	g_NetClient->SetUserName(playerName);

	if (!g_NetClient->SetupConnection("127.0.0.1"))
	{
		guiManager->GetScriptInterface().ReportError("Failed to connect to server");
		SAFE_DELETE(g_NetClient);
		SAFE_DELETE(g_Game);
	}
}

void StartNetworkJoin(void* cbdata, std::wstring playerName, std::string serverAddress)
{
	CGUIManager* guiManager = static_cast<CGUIManager*> (cbdata);

	debug_assert(!g_NetClient);
	debug_assert(!g_NetServer);
	debug_assert(!g_Game);

	g_Game = new CGame();
	g_NetClient = new CNetClient(g_Game);
	g_NetClient->SetUserName(playerName);
	if (!g_NetClient->SetupConnection(serverAddress))
	{
		guiManager->GetScriptInterface().ReportError("Failed to connect to server");
		SAFE_DELETE(g_NetClient);
		SAFE_DELETE(g_Game);
	}
}

void DisconnectNetworkGame(void* UNUSED(cbdata))
{
	// TODO: we ought to do async reliable disconnections

	SAFE_DELETE(g_NetServer);
	SAFE_DELETE(g_NetClient);
	SAFE_DELETE(g_Game);
}

CScriptVal PollNetworkClient(void* cbdata)
{
	CGUIManager* guiManager = static_cast<CGUIManager*> (cbdata);

	if (!g_NetClient)
		return CScriptVal();

	CScriptValRooted poll = g_NetClient->GuiPoll();

	// Convert from net client context to GUI script context
	return guiManager->GetScriptInterface().CloneValueFromOtherContext(g_NetClient->GetScriptInterface(), poll.get());
}

void AssignNetworkPlayer(void* UNUSED(cbdata), int playerID, std::string guid)
{
	debug_assert(g_NetServer);

	g_NetServer->AssignPlayer(playerID, guid);
}

void SendNetworkChat(void* UNUSED(cbdata), std::wstring message)
{
	debug_assert(g_NetClient);

	g_NetClient->SendChatMessage(message);
}

void OpenURL(void* UNUSED(cbdata), std::string url)
{
	sys_open_url(url);
}

void RestartInAtlas(void* UNUSED(cbdata))
{
	restart_mainloop_in_atlas();
}

bool AtlasIsAvailable(void* UNUSED(cbdata))
{
	return ATLAS_IsAvailable();
}

CScriptVal LoadMapSettings(void* cbdata, std::wstring pathname)
{
	CGUIManager* guiManager = static_cast<CGUIManager*> (cbdata);

	CMapSummaryReader reader;

	if (reader.LoadMap(VfsPath(pathname + L".xml")) != PSRETURN_OK)
		return CScriptVal();

	return reader.GetMapSettings(guiManager->GetScriptInterface()).get();
}

void SetRevealMap(void* UNUSED(cbdata), bool enabled)
{
	if (!g_Game)
		return;

	CSimulation2* sim = g_Game->GetSimulation2();
	debug_assert(sim);
	CmpPtr<ICmpRangeManager> cmpRangeManager(*sim, SYSTEM_ENTITY);
	if (!cmpRangeManager.null())
		cmpRangeManager->SetLosRevealAll(enabled);
}

/**
 * Start / stop camera following mode
 * @param entityid unit id to follow. If zero, stop following mode
 */
void CameraFollow(void* UNUSED(cbdata), entity_id_t entityid)
{
	if (g_Game && g_Game->GetView())
		g_Game->GetView()->CameraFollow(entityid);
}


void SetSimRate(void* UNUSED(cbdata), float rate)
{
	g_Game->SetSimRate(rate);
}

void SetTurnLength(void* UNUSED(cbdata), int length)
{
	if (g_NetServer)
		g_NetServer->SetTurnLength(length);
	else
		LOGERROR(L"Only network host can change turn length");
}

// Focus the game camera on a given position.
void SetCameraTarget(void* UNUSED(cbdata), float x, float y, float z)
{
	g_Game->GetView()->ResetCameraTarget(CVector3D(x, y, z));
}

// Deliberately cause the game to crash.
// Currently implemented via access violation (read of address 0).
// Useful for testing the crashlog/stack trace code.
int Crash(void* UNUSED(cbdata))
{
	MICROLOG(L"Crashing at user's request.");
	return *(int*)0;
}

// Force a JS garbage collection cycle to take place immediately.
// Writes an indication of how long this took to the console.
void ForceGC(void* cbdata)
{
	CGUIManager* guiManager = static_cast<CGUIManager*> (cbdata);

	double time = timer_Time();
	JS_GC(guiManager->GetScriptInterface().GetContext());
	time = timer_Time() - time;
	g_Console->InsertMessage(L"Garbage collection completed in: %f", time);
}

} // namespace

void GuiScriptingInit(ScriptInterface& scriptInterface)
{
	// GUI manager functions:
	scriptInterface.RegisterFunction<CScriptVal, &GetActiveGui>("GetActiveGui");
	scriptInterface.RegisterFunction<void, std::wstring, CScriptVal, &PushGuiPage>("PushGuiPage");
	scriptInterface.RegisterFunction<void, std::wstring, CScriptVal, &SwitchGuiPage>("SwitchGuiPage");
	scriptInterface.RegisterFunction<void, &PopGuiPage>("PopGuiPage");

	// Simulation<->GUI interface functions:
	scriptInterface.RegisterFunction<CScriptVal, std::wstring, CScriptVal, &GuiInterfaceCall>("GuiInterfaceCall");
	scriptInterface.RegisterFunction<void, CScriptVal, &PostNetworkCommand>("PostNetworkCommand");

	// Entity picking
	scriptInterface.RegisterFunction<std::vector<entity_id_t>, int, int, &PickEntitiesAtPoint>("PickEntitiesAtPoint");
	scriptInterface.RegisterFunction<std::vector<entity_id_t>, int, int, int, int, int, &PickFriendlyEntitiesInRect>("PickFriendlyEntitiesInRect");
	scriptInterface.RegisterFunction<CFixedVector3D, int, int, &GetTerrainAtPoint>("GetTerrainAtPoint");

	// Network / game setup functions
	scriptInterface.RegisterFunction<void, &StartNetworkGame>("StartNetworkGame");
	scriptInterface.RegisterFunction<void, CScriptVal, int, &StartGame>("StartGame");
	scriptInterface.RegisterFunction<void, std::wstring, &StartNetworkHost>("StartNetworkHost");
	scriptInterface.RegisterFunction<void, std::wstring, std::string, &StartNetworkJoin>("StartNetworkJoin");
	scriptInterface.RegisterFunction<void, &DisconnectNetworkGame>("DisconnectNetworkGame");
	scriptInterface.RegisterFunction<CScriptVal, &PollNetworkClient>("PollNetworkClient");
	scriptInterface.RegisterFunction<void, CScriptVal, &SetNetworkGameAttributes>("SetNetworkGameAttributes");
	scriptInterface.RegisterFunction<void, int, std::string, &AssignNetworkPlayer>("AssignNetworkPlayer");
	scriptInterface.RegisterFunction<void, std::wstring, &SendNetworkChat>("SendNetworkChat");

	// Misc functions
	scriptInterface.RegisterFunction<std::wstring, std::wstring, &SetCursor>("SetCursor");
	scriptInterface.RegisterFunction<int, &GetPlayerID>("GetPlayerID");
	scriptInterface.RegisterFunction<std::wstring, &GetDefaultPlayerName>("GetDefaultPlayerName");
	scriptInterface.RegisterFunction<void, std::string, &OpenURL>("OpenURL");
	scriptInterface.RegisterFunction<void, &RestartInAtlas>("RestartInAtlas");
	scriptInterface.RegisterFunction<bool, &AtlasIsAvailable>("AtlasIsAvailable");
	scriptInterface.RegisterFunction<CScriptVal, std::wstring, &LoadMapSettings>("LoadMapSettings");
	scriptInterface.RegisterFunction<void, bool, &SetRevealMap>("SetRevealMap");
	scriptInterface.RegisterFunction<void, entity_id_t, &CameraFollow>("CameraFollow");

	// Development/debugging functions
	scriptInterface.RegisterFunction<void, float, &SetSimRate>("SetSimRate");
	scriptInterface.RegisterFunction<void, int, &SetTurnLength>("SetTurnLength");
	scriptInterface.RegisterFunction<void, float, float, float, &SetCameraTarget>("SetCameraTarget");
	scriptInterface.RegisterFunction<int, &Crash>("Crash");
	scriptInterface.RegisterFunction<void, &ForceGC>("ForceGC");
}
