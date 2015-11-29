/* Copyright (C) 2015 Wildfire Games.
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

#ifndef INCLUDED_REPlAY
#define INCLUDED_REPlAY

#include "scriptinterface/ScriptInterface.h"
class CSimulation2;
class CGUIManager;

/**
 * Contains functions for visually replaying past games.
 */
namespace VisualReplay
{

/**
 * Returns the path to the sim-log directory (that contains the directories with the replay files.
 *
 * @param scriptInterface the ScriptInterface in which to create the return data.
 * @return OsPath the absolute file path
 */
OsPath GetDirectoryName();

/**
 * Replays the commands.txt file in the given subdirectory visually.
 */
void StartVisualReplay(CStrW directory);

/**
 * Get a list of replays to display in the GUI.
 *
 * @param scriptInterface the ScriptInterface in which to create the return data.
 * @return array of objects containing replay data
 */
JS::Value GetReplays(ScriptInterface& scriptInterface);

/**
 * Parses a commands.txt file and extracts metadata.
 * Works similarly to CGame::LoadReplayData().
 */
JS::Value LoadReplayData(ScriptInterface& scriptInterface, OsPath& directory);

/**
 * Permanently deletes the visual replay (including the parent directory)
 *
 * @param replayFile path to commands.txt, whose parent directory will be deleted
 * @return true if deletion was successful, false on error
 */
bool DeleteReplay(const CStrW& replayFile);

/**
 * Returns the parsed header of the replay file (commands.txt).
 */
JS::Value GetReplayAttributes(ScriptInterface::CxPrivate* pCxPrivate, const CStrW& directoryName);

/**
 * Returns the metadata of a replay.
 */
JS::Value GetReplayMetadata(ScriptInterface::CxPrivate* pCxPrivate, const CStrW& directoryName);

/**
 * Saves the metadata from the session to metadata.json
 */
void SaveReplayMetadata(const CStrW& data);

}

#endif
