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
 * Contains functions for deleting a campaign save
 */

namespace Campaigns
{
/**
 * Permanently deletes the saved campaign run with the given name
 *
 * @param name filename of saved campaign (without path or extension)
 * @return true if deletion was successful, or false on error
 */
bool DeleteGame(const std::wstring& name)
{
	const VfsPath basename(L"campaignsaves/" + name);
	const VfsPath filename = basename.ChangeExtension(L".0adcampaign");
	OsPath realpath;

	// Make sure it exists in VFS and find its real path
	if (!VfsFileExists(filename) || g_VFS->GetRealPath(filename, realpath) != INFO::OK)
		return false; // Error

	// Remove from VFS
	if (g_VFS->RemoveFile(filename) != INFO::OK)
		return false; // Error

	// Delete actual file
	if (wunlink(realpath) != 0)
		return false; // Error

	// Successfully deleted file
	return true;
}
}

#endif // INCLUDED_CAMPAIGNS
