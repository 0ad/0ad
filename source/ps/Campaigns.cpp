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

#include "precompiled.h"

#include "Campaigns.h"

#include "graphics/GameView.h"
#include "gui/GUIManager.h"
#include "lib/allocators/shared_ptr.h"
#include "i18n/L10n.h"
#include "lib/utf8.h"
#include "ps/CLogger.h"
#include "ps/FileIo.h"
#include "ps/Filesystem.h"
#include "ps/Game.h"
#include "ps/Mod.h"
#include "ps/Pyrogenesis.h"
#include "ps/XML/Xeromyces.h"
#include "scriptinterface/ScriptInterface.h"
#include "simulation2/Simulation2.h"

static const int CAMPAIGN_VERSION = 1; // increment on incompatible changes to the format

// TODO: we ought to check version numbers when loading files

Status Campaigns::Save(ScriptInterface& scriptInterface, const CStrW& name, const shared_ptr<ScriptInterface::StructuredClone>& metadataClone)
{
	JSContext* cx = scriptInterface.GetContext();
	JSAutoRequest rq(cx);

	// Determine the filename to save under
	const VfsPath basenameFormat(L"campaignsaves/" + name);
	const VfsPath filename = basenameFormat.ChangeExtension(L".0adcampaign");

	time_t now = time(NULL);

	JS::RootedValue metadata(cx);
	scriptInterface.Eval("({})", &metadata);
	scriptInterface.SetProperty(metadata, "engine_version", std::string(engine_version));
	scriptInterface.SetProperty(metadata, "mods", g_modsLoaded);
	scriptInterface.SetProperty(metadata, "time", (double)now);

	JS::RootedValue campaignState(cx);
	scriptInterface.ReadStructuredClone(metadataClone, &campaignState);

	scriptInterface.SetProperty(metadata, "campaign_state", campaignState);

	std::string dataString = scriptInterface.StringifyJSON(&metadata, true);

	// ensure we won't crash if the save somehow fails.
	try
	{
		CFilePacker packer(CAMPAIGN_VERSION, "CSST");
		packer.PackString(dataString);
		packer.Write(filename);
	}
	catch (PSERROR_File_WriteFailed&)
	{
		LOGERROR("Failed to write campaign '%s'", filename.string8());
		return ERR::FAIL;
	}

	LOGMESSAGERENDER(g_L10n.Translate("Saved campaign to '%s'"), filename.string8());
	debug_printf("Saved campaign to '%s'\n", filename.string8().c_str());

	return INFO::OK;
}

Status Campaigns::Load(ScriptInterface& scriptInterface, const std::wstring& name, JS::MutableHandleValue campaignData)
{
	// Determine the filename to load
	const VfsPath basename(L"campaignsaves/" + name);
	const VfsPath filename = basename.ChangeExtension(L".0adcampaign");

	if (!VfsFileExists(filename))
		return ERR::FILE_NOT_FOUND;

	CFileUnpacker unpacker;
	unpacker.Read(filename, "CSST");

	if (unpacker.GetVersion() < CAMPAIGN_VERSION)
	{
		LOGWARNING("Campaign file is too old, version %i, current version %i", unpacker.GetVersion(), CAMPAIGN_VERSION);
		return ERR::FAIL;
	}

	CStr datastream;
	unpacker.UnpackString(datastream);


	if (!scriptInterface.ParseJSON(datastream, campaignData))
	{
		LOGERROR("Error parsing campaign JSON.");
		return ERR::FAIL;
	}

	return INFO::OK;
}

