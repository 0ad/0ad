/* Copyright (C) 2016 Wildfire Games.
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

#ifndef INCLUDED_CAMPAIGNS
#define INCLUDED_CAMPAIGNS

#include "scriptinterface/ScriptInterface.h"
class CSimulation2;
class CGUIManager;

/**
 * @file
 * Contains functions for managing campaign archives.
 *
 * A saved game is a simple *.0adcampaign file
 * which is a binary JSON file containing the campaign metadata
 */

namespace Campaigns
{

/**
 * Create new campaign file with given name and metadata
 *
 * @param scriptInterface
 * @param name Name of the campaign
 * @param MetadataClone Actual campaign Metadata
 * @return INFO::OK if successfully saved, else an error Status
 */
Status Save(ScriptInterface& scriptInterface, const CStrW& name, const shared_ptr<ScriptInterface::StructuredClone>& metadataClone);

/**
 * Load campaign with the given name
 *
 * @param scriptInterface
 * @param name filename of campaign game (without path or extension)
 * @param[out] metadata object containing metadata associated with saved game,
 *	parsed from metadata.json inside the archive.
 * @return INFO::OK if successfully loaded, else an error Status
 */
Status Load(ScriptInterface& scriptInterface, const std::wstring& name, JS::MutableHandleValue campaignData);

/**
 * Permanently deletes the saved game archive with the given name
 *
 * @param name filename of saved game (without path or extension)
 * @return true if deletion was successful, or false on error
 *
bool DeleteSavedGame(const std::wstring& name);

/**
 * Gets info (version and mods loaded) on the running engine
 *
 * @param scriptInterface the ScriptInterface in which to create the return data.
 * @return list of objects containing saved game data
 *
JS::Value GetEngineInfo(ScriptInterface& scriptInterface);
*/
}

#endif // INCLUDED_CAMPAIGNS
