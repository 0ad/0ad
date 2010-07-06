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
#include "gui/GUIManager.h"
#include "lib/sysdep/sysdep.h"
#include "maths/FixedVector3D.h"
#include "network/NetClient.h"
#include "network/NetServer.h"
#include "network/NetTurnManager.h"
#include "ps/CLogger.h"
#include "ps/Game.h"
#include "ps/Overlay.h"
#include "ps/GameSetup/Config.h"
#include "simulation2/Simulation2.h"
#include "simulation2/components/ICmpCommandQueue.h"
#include "simulation2/components/ICmpGuiInterface.h"
#include "simulation2/components/ICmpTemplateManager.h"
#include "simulation2/helpers/Selection.h"

#include "js/jsapi.h"

/*
 * This file defines a set of functions that are available to GUI scripts, to allow
 * interaction with the rest of the engine.
 * Functions are exposed to scripts within the global object 'Engine', so
 * scripts should call "Engine.FunctionName(...)" etc.
 */

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

bool IsNewSimulation(void* UNUSED(cbdata))
{
	return true; // XXX: delete this function
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
	return EntitySelection::PickEntitiesAtPoint(*g_Game->GetSimulation2(), *g_Game->GetView()->GetCamera(), x, y);
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


} // namespace

void GuiScriptingInit(ScriptInterface& scriptInterface)
{
	// GUI manager functions:
	scriptInterface.RegisterFunction<CScriptVal, &GetActiveGui>("GetActiveGui");
	scriptInterface.RegisterFunction<void, std::wstring, CScriptVal, &PushGuiPage>("PushGuiPage");
	scriptInterface.RegisterFunction<void, std::wstring, CScriptVal, &SwitchGuiPage>("SwitchGuiPage");
	scriptInterface.RegisterFunction<void, &PopGuiPage>("PopGuiPage");

	// Simulation<->GUI interface functions:
	scriptInterface.RegisterFunction<bool, &IsNewSimulation>("IsNewSimulation");
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
}
