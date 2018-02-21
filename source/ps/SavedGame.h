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

#ifndef INCLUDED_SAVEDGAME
#define INCLUDED_SAVEDGAME

#include "scriptinterface/ScriptInterface.h"
class CSimulation2;
class CGUIManager;

/**
 * @file
 * Contains functions for managing saved game archives.
 *
 * A saved game is simply a zip archive with the extension '0adsave'
 * and containing two files:
 * <ul>
 *  <li>metadata.json - JSON data file containing the game metadata</li>
 *	<li>simulation.dat - the serialized simulation state data</li>
 * </ul>
 */

namespace SavedGames
{
	/**
	 * Create new saved game archive with given name and simulation data
	 *
	 * @param name Name to save the game with
	 * @param description A user-given description of the save
	 * @param simulation
	 * @param guiMetadataClone if not NULL, store some UI-related data with the saved game
	 * @return INFO::OK if successfully saved, else an error Status
	 */
	Status Save(const CStrW& name, const CStrW& description, CSimulation2& simulation, const shared_ptr<ScriptInterface::StructuredClone>& guiMetadataClone);

	/**
	 * Create new saved game archive with given prefix and simulation data
	 *
	 * @param prefix Create new numbered file starting with this prefix
	 * @param description A user-given description of the save
	 * @param simulation
	 * @param guiMetadataClone if not NULL, store some UI-related data with the saved game
	 * @return INFO::OK if successfully saved, else an error Status
	 */
	Status SavePrefix(const CStrW& prefix, const CStrW& description, CSimulation2& simulation, const shared_ptr<ScriptInterface::StructuredClone>& guiMetadataClone);

	/**
	 * Load saved game archive with the given name
	 *
	 * @param name filename of saved game (without path or extension)
	 * @param scriptInterface
	 * @param[out] metadata object containing metadata associated with saved game,
	 *	parsed from metadata.json inside the archive.
	 * @param[out] savedState serialized simulation state stored as string of bytes,
	 *	loaded from simulation.dat inside the archive.
	 * @return INFO::OK if successfully loaded, else an error Status
	 */
	Status Load(const std::wstring& name, const ScriptInterface& scriptInterface, JS::MutableHandleValue metadata, std::string& savedState);

	/**
	 * Get list of saved games for GUI script usage
	 *
	 * @param scriptInterface the ScriptInterface in which to create the return data.
	 * @return array of objects containing saved game data
	 */
	JS::Value GetSavedGames(const ScriptInterface& scriptInterface);

	/**
	 * Permanently deletes the saved game archive with the given name
	 *
	 * @param name filename of saved game (without path or extension)
	 * @return true if deletion was successful, or false on error
	 */
	bool DeleteSavedGame(const std::wstring& name);
}

#endif // INCLUDED_SAVEDGAME
