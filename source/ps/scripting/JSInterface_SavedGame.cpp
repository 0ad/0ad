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

#include "JSInterface_SavedGame.h"

#include "network/NetClient.h"
#include "network/NetServer.h"
#include "ps/CLogger.h"
#include "ps/Game.h"
#include "ps/SavedGame.h"
#include "scriptinterface/FunctionWrapper.h"
#include "scriptinterface/ScriptInterface.h"
#include "simulation2/Simulation2.h"
#include "simulation2/system/TurnManager.h"

namespace JSI_SavedGame
{
JS::Value GetSavedGames(const ScriptInterface& scriptInterface)
{
	return SavedGames::GetSavedGames(scriptInterface);
}

bool DeleteSavedGame(const std::wstring& name)
{
	return SavedGames::DeleteSavedGame(name);
}

void SaveGame(const ScriptInterface& scriptInterface, const std::wstring& filename, const std::wstring& description, JS::HandleValue GUIMetadata)
{
	ScriptInterface::StructuredClone GUIMetadataClone = scriptInterface.WriteStructuredClone(GUIMetadata);
	if (SavedGames::Save(filename, description, *g_Game->GetSimulation2(), GUIMetadataClone) < 0)
		LOGERROR("Failed to save game");
}

void SaveGamePrefix(const ScriptInterface& scriptInterface, const std::wstring& prefix, const std::wstring& description, JS::HandleValue GUIMetadata)
{
	ScriptInterface::StructuredClone GUIMetadataClone = scriptInterface.WriteStructuredClone(GUIMetadata);
	if (SavedGames::SavePrefix(prefix, description, *g_Game->GetSimulation2(), GUIMetadataClone) < 0)
		LOGERROR("Failed to save game");
}

void QuickSave(JS::HandleValue GUIMetadata)
{
	if (g_NetServer || g_NetClient)
		LOGERROR("Can't store quicksave during multiplayer!");
	else if (g_Game)
		g_Game->GetTurnManager()->QuickSave(GUIMetadata);
	else
		LOGERROR("Can't store quicksave if game is not running!");
}

void QuickLoad()
{
	if (g_NetServer || g_NetClient)
		LOGERROR("Can't load quicksave during multiplayer!");
	else if (g_Game)
		g_Game->GetTurnManager()->QuickLoad();
	else
		LOGERROR("Can't load quicksave if game is not running!");
}

JS::Value StartSavedGame(const ScriptInterface& scriptInterface, const std::wstring& name)
{
	// We need to be careful with different compartments and contexts.
	// The GUI calls this function from the GUI context and expects the return value in the same context.
	// The game we start from here creates another context and expects data in this context.

	ScriptRequest rqGui(scriptInterface);

	ENSURE(!g_NetServer);
	ENSURE(!g_NetClient);

	ENSURE(!g_Game);

	// Load the saved game data from disk
	JS::RootedValue guiContextMetadata(rqGui.cx);
	std::string savedState;
	Status err = SavedGames::Load(name, scriptInterface, &guiContextMetadata, savedState);
	if (err < 0)
		return JS::UndefinedValue();

	g_Game = new CGame(true);

	{
		CSimulation2* sim = g_Game->GetSimulation2();
		ScriptRequest rqGame(sim->GetScriptInterface());

		JS::RootedValue gameContextMetadata(rqGame.cx,
			sim->GetScriptInterface().CloneValueFromOtherCompartment(scriptInterface, guiContextMetadata));
		JS::RootedValue gameInitAttributes(rqGame.cx);
		sim->GetScriptInterface().GetProperty(gameContextMetadata, "initAttributes", &gameInitAttributes);

		int playerID;
		sim->GetScriptInterface().GetProperty(gameContextMetadata, "playerID", playerID);

		g_Game->SetPlayerID(playerID);
		g_Game->StartGame(&gameInitAttributes, savedState);
	}

	return guiContextMetadata;
}

void ActivateRejoinTest()
{
	if (!g_Game || !g_Game->GetSimulation2() || !g_Game->GetTurnManager())
		return;
	g_Game->GetSimulation2()->ActivateRejoinTest(g_Game->GetTurnManager()->GetCurrentTurn() + 1);
}

void RegisterScriptFunctions(const ScriptRequest& rq)
{
	ScriptFunction::Register<&GetSavedGames>(rq, "GetSavedGames");
	ScriptFunction::Register<&DeleteSavedGame>(rq, "DeleteSavedGame");
	ScriptFunction::Register<&SaveGame>(rq, "SaveGame");
	ScriptFunction::Register<&SaveGamePrefix>(rq, "SaveGamePrefix");
	ScriptFunction::Register<&QuickSave>(rq, "QuickSave");
	ScriptFunction::Register<&QuickLoad>(rq, "QuickLoad");
	ScriptFunction::Register<&ActivateRejoinTest>(rq, "ActivateRejoinTest");
	ScriptFunction::Register<&StartSavedGame>(rq, "StartSavedGame");
}
}
