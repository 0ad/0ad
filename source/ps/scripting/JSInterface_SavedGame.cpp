/* Copyright (C) 2017 Wildfire Games.
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

#include "ps/scripting/JSInterface_SavedGame.h"

#include "network/NetClient.h"
#include "network/NetServer.h"
#include "ps/CLogger.h"
#include "ps/Game.h"
#include "ps/SavedGame.h"
#include "simulation2/Simulation2.h"
#include "simulation2/system/TurnManager.h"

JS::Value JSI_SavedGame::GetSavedGames(ScriptInterface::CxPrivate* pCxPrivate)
{
	return SavedGames::GetSavedGames(*(pCxPrivate->pScriptInterface));
}

bool JSI_SavedGame::DeleteSavedGame(ScriptInterface::CxPrivate* UNUSED(pCxPrivate), const std::wstring& name)
{
	return SavedGames::DeleteSavedGame(name);
}

void JSI_SavedGame::SaveGame(ScriptInterface::CxPrivate* pCxPrivate, const std::wstring& filename, const std::wstring& description, JS::HandleValue GUIMetadata)
{
	shared_ptr<ScriptInterface::StructuredClone> GUIMetadataClone = pCxPrivate->pScriptInterface->WriteStructuredClone(GUIMetadata);
	if (SavedGames::Save(filename, description, *g_Game->GetSimulation2(), GUIMetadataClone) < 0)
		LOGERROR("Failed to save game");
}

void JSI_SavedGame::SaveGamePrefix(ScriptInterface::CxPrivate* pCxPrivate, const std::wstring& prefix, const std::wstring& description, JS::HandleValue GUIMetadata)
{
	shared_ptr<ScriptInterface::StructuredClone> GUIMetadataClone = pCxPrivate->pScriptInterface->WriteStructuredClone(GUIMetadata);
	if (SavedGames::SavePrefix(prefix, description, *g_Game->GetSimulation2(), GUIMetadataClone) < 0)
		LOGERROR("Failed to save game");
}

void JSI_SavedGame::QuickSave(ScriptInterface::CxPrivate* UNUSED(pCxPrivate))
{
	g_Game->GetTurnManager()->QuickSave();
}

void JSI_SavedGame::QuickLoad(ScriptInterface::CxPrivate* UNUSED(pCxPrivate))
{
	g_Game->GetTurnManager()->QuickLoad();
}

JS::Value JSI_SavedGame::StartSavedGame(ScriptInterface::CxPrivate* pCxPrivate, const std::wstring& name)
{
	// We need to be careful with different compartments and contexts.
	// The GUI calls this function from the GUI context and expects the return value in the same context.
	// The game we start from here creates another context and expects data in this context.

	JSContext* cxGui = pCxPrivate->pScriptInterface->GetContext();
	JSAutoRequest rq(cxGui);

	ENSURE(!g_NetServer);
	ENSURE(!g_NetClient);

	ENSURE(!g_Game);

	// Load the saved game data from disk
	JS::RootedValue guiContextMetadata(cxGui);
	std::string savedState;
	Status err = SavedGames::Load(name, *(pCxPrivate->pScriptInterface), &guiContextMetadata, savedState);
	if (err < 0)
		return JS::UndefinedValue();

	g_Game = new CGame();

	{
		CSimulation2* sim = g_Game->GetSimulation2();
		JSContext* cxGame = sim->GetScriptInterface().GetContext();
		JSAutoRequest rq(cxGame);

		JS::RootedValue gameContextMetadata(cxGame,
			sim->GetScriptInterface().CloneValueFromOtherContext(*(pCxPrivate->pScriptInterface), guiContextMetadata));
		JS::RootedValue gameInitAttributes(cxGame);
		sim->GetScriptInterface().GetProperty(gameContextMetadata, "initAttributes", &gameInitAttributes);

		int playerID;
		sim->GetScriptInterface().GetProperty(gameContextMetadata, "playerID", playerID);

		g_Game->SetPlayerID(playerID);
		g_Game->StartGame(&gameInitAttributes, savedState);
	}

	return guiContextMetadata;
}

void JSI_SavedGame::RegisterScriptFunctions(const ScriptInterface& scriptInterface)
{
	scriptInterface.RegisterFunction<JS::Value, &GetSavedGames>("GetSavedGames");
	scriptInterface.RegisterFunction<bool, std::wstring, &DeleteSavedGame>("DeleteSavedGame");
	scriptInterface.RegisterFunction<void, std::wstring, std::wstring, JS::HandleValue, &SaveGame>("SaveGame");
	scriptInterface.RegisterFunction<void, std::wstring, std::wstring, JS::HandleValue, &SaveGamePrefix>("SaveGamePrefix");
	scriptInterface.RegisterFunction<void, &QuickSave>("QuickSave");
	scriptInterface.RegisterFunction<void, &QuickLoad>("QuickLoad");
	scriptInterface.RegisterFunction<JS::Value, std::wstring, &StartSavedGame>("StartSavedGame");
}
