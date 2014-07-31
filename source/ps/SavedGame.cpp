/* Copyright (C) 2014 Wildfire Games.
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

#include "SavedGame.h"

#include "graphics/GameView.h"
#include "gui/GUIManager.h"
#include "lib/allocators/shared_ptr.h"
#include "lib/file/archive/archive_zip.h"
#include "i18n/L10n.h"
#include "lib/utf8.h"
#include "ps/CLogger.h"
#include "ps/Filesystem.h"
#include "ps/Game.h"
#include "scriptinterface/ScriptInterface.h"
#include "simulation2/Simulation2.h"

static const int SAVED_GAME_VERSION_MAJOR = 1; // increment on incompatible changes to the format
static const int SAVED_GAME_VERSION_MINOR = 0; // increment on compatible changes to the format
std::vector<std::string> g_modsLoaded; // list of mods loaded

// TODO: we ought to check version numbers when loading files


Status SavedGames::SavePrefix(const std::wstring& prefix, const std::wstring& description, CSimulation2& simulation, shared_ptr<ScriptInterface::StructuredClone> guiMetadataClone, int playerID)
{
	// Determine the filename to save under
	const VfsPath basenameFormat(L"saves/" + prefix + L"-%04d");
	const VfsPath filenameFormat = basenameFormat.ChangeExtension(L".0adsave");
	VfsPath filename;

	// Don't make this a static global like NextNumberedFilename expects, because
	// that wouldn't work when 'prefix' changes, and because it's not thread-safe
	size_t nextSaveNumber = 0;
	vfs::NextNumberedFilename(g_VFS, filenameFormat, nextSaveNumber, filename);

	return Save(filename.Filename().string(), description, simulation, guiMetadataClone, playerID);
}

Status SavedGames::Save(const std::wstring& name, const std::wstring& description, CSimulation2& simulation, shared_ptr<ScriptInterface::StructuredClone> guiMetadataClone, int playerID)
{
	JSContext* cx = simulation.GetScriptInterface().GetContext();
	JSAutoRequest rq(cx);
	// Determine the filename to save under
	const VfsPath basenameFormat(L"saves/" + name);
	const VfsPath filename = basenameFormat.ChangeExtension(L".0adsave");

	// ArchiveWriter_Zip can only write to OsPaths, not VfsPaths,
	// but we'd like to handle saved games via VFS.
	// To avoid potential confusion from writing with non-VFS then
	// reading the same file with VFS, we'll just write to a temporary
	// non-VFS path and then load and save again via VFS,
	// which is kind of a hack.

	OsPath tempSaveFileRealPath;
	WARN_RETURN_STATUS_IF_ERR(g_VFS->GetDirectoryRealPath("cache/", tempSaveFileRealPath));
	tempSaveFileRealPath = tempSaveFileRealPath / "temp.0adsave";

	time_t now = time(NULL);

	// Construct the serialized state to be saved

	std::stringstream simStateStream;
	if (!simulation.SerializeState(simStateStream))
		WARN_RETURN(ERR::FAIL);

	JS::RootedValue metadata(cx);
	simulation.GetScriptInterface().Eval("({})", &metadata);
	simulation.GetScriptInterface().SetProperty(metadata, "version_major", SAVED_GAME_VERSION_MAJOR);
	simulation.GetScriptInterface().SetProperty(metadata, "version_minor", SAVED_GAME_VERSION_MINOR);
	simulation.GetScriptInterface().SetProperty(metadata, "mods", g_modsLoaded);
	simulation.GetScriptInterface().SetProperty(metadata, "time", (double)now);
	simulation.GetScriptInterface().SetProperty(metadata, "player", playerID);
	simulation.GetScriptInterface().SetProperty(metadata, "initAttributes", simulation.GetInitAttributes());

	JS::RootedValue guiMetadata(cx, simulation.GetScriptInterface().ReadStructuredClone(guiMetadataClone));

	// get some camera data
	JS::RootedValue cameraMetadata(cx);
	simulation.GetScriptInterface().Eval("({})", &cameraMetadata);
	simulation.GetScriptInterface().SetProperty(cameraMetadata, "PosX", g_Game->GetView()->GetCameraPosX());
	simulation.GetScriptInterface().SetProperty(cameraMetadata, "PosY", g_Game->GetView()->GetCameraPosY());
	simulation.GetScriptInterface().SetProperty(cameraMetadata, "PosZ", g_Game->GetView()->GetCameraPosZ());
	simulation.GetScriptInterface().SetProperty(cameraMetadata, "RotX", g_Game->GetView()->GetCameraRotX());
	simulation.GetScriptInterface().SetProperty(cameraMetadata, "RotY", g_Game->GetView()->GetCameraRotY());
	simulation.GetScriptInterface().SetProperty(cameraMetadata, "Zoom", g_Game->GetView()->GetCameraZoom());
	simulation.GetScriptInterface().SetProperty(guiMetadata, "camera", cameraMetadata);
	simulation.GetScriptInterface().SetProperty(metadata, "gui", guiMetadata);
	
	simulation.GetScriptInterface().SetProperty(metadata, "description", description);
	
	std::string metadataString = simulation.GetScriptInterface().StringifyJSON(metadata, true);
	
	// Write the saved game as zip file containing the various components
	PIArchiveWriter archiveWriter = CreateArchiveWriter_Zip(tempSaveFileRealPath, false);
	if (!archiveWriter)
		WARN_RETURN(ERR::FAIL);

	WARN_RETURN_STATUS_IF_ERR(archiveWriter->AddMemory((const u8*)metadataString.c_str(), metadataString.length(), now, "metadata.json"));
	WARN_RETURN_STATUS_IF_ERR(archiveWriter->AddMemory((const u8*)simStateStream.str().c_str(), simStateStream.str().length(), now, "simulation.dat"));
	archiveWriter.reset(); // close the file

	WriteBuffer buffer;
	CFileInfo tempSaveFile;
	WARN_RETURN_STATUS_IF_ERR(GetFileInfo(tempSaveFileRealPath, &tempSaveFile));
	buffer.Reserve(tempSaveFile.Size());
	WARN_RETURN_STATUS_IF_ERR(io::Load(tempSaveFileRealPath, buffer.Data().get(), buffer.Size()));
	WARN_RETURN_STATUS_IF_ERR(g_VFS->CreateFile(filename, buffer.Data(), buffer.Size()));

	OsPath realPath;
	WARN_RETURN_STATUS_IF_ERR(g_VFS->GetRealPath(filename, realPath));
	LOGMESSAGERENDER(wstring_from_utf8(L10n::Instance().Translate("Saved game to '%ls'") + "\n").c_str(), realPath.string().c_str());

	return INFO::OK;
}

/**
 * Helper class for retrieving data from saved game archives
 */
class CGameLoader
{
	NONCOPYABLE(CGameLoader);
public:
	
	/**
	 * @param scriptInterface the ScriptInterface used for loading metadata.
	 * @param[out] savedState serialized simulation state stored as string of bytes,
	 *	loaded from simulation.dat inside the archive.
	 *
	 * Note: We use a different approach for returning the string and the metadata JS::Value.
	 * We use a pointer for the string to avoid copies (efficiency). We don't use this approach 
	 * for the metadata because it would be error prone with rooting and the stack-based rooting 
	 * types and confusing (a chain of pointers pointing to other pointers).
	 */
	CGameLoader(ScriptInterface& scriptInterface, std::string* savedState) :
		m_ScriptInterface(scriptInterface), m_SavedState(savedState)
	{
	}

	static void ReadEntryCallback(const VfsPath& pathname, const CFileInfo& fileInfo, PIArchiveFile archiveFile, uintptr_t cbData)
	{
		((CGameLoader*)cbData)->ReadEntry(pathname, fileInfo, archiveFile);
	}

	void ReadEntry(const VfsPath& pathname, const CFileInfo& fileInfo, PIArchiveFile archiveFile)
	{
		if (pathname == L"metadata.json")
		{
			std::string buffer;
			buffer.resize(fileInfo.Size());
			WARN_IF_ERR(archiveFile->Load("", DummySharedPtr((u8*)buffer.data()), buffer.size()));
			m_Metadata = m_ScriptInterface.ParseJSON(buffer);
		}
		else if (pathname == L"simulation.dat" && m_SavedState)
		{
			m_SavedState->resize(fileInfo.Size());
			WARN_IF_ERR(archiveFile->Load("", DummySharedPtr((u8*)m_SavedState->data()), m_SavedState->size()));
		}
	}
	
	JS::Value GetMetadata()
	{
		return m_Metadata.get();
	}
	
private:

	ScriptInterface& m_ScriptInterface;
	CScriptValRooted m_Metadata;
	std::string* m_SavedState;
};

Status SavedGames::Load(const std::wstring& name, ScriptInterface& scriptInterface, JS::MutableHandleValue metadata, std::string& savedState)
{
	// Determine the filename to load
	const VfsPath basename(L"saves/" + name);
	const VfsPath filename = basename.ChangeExtension(L".0adsave");

	// Don't crash just because file isn't found, this can happen if the file is deleted from the OS
	if (!VfsFileExists(filename))
		return ERR::FILE_NOT_FOUND;

	OsPath realPath;
	WARN_RETURN_STATUS_IF_ERR(g_VFS->GetRealPath(filename, realPath));

	PIArchiveReader archiveReader = CreateArchiveReader_Zip(realPath);
	if (!archiveReader)
		WARN_RETURN(ERR::FAIL);

	CGameLoader loader(scriptInterface, &savedState);
	WARN_RETURN_STATUS_IF_ERR(archiveReader->ReadEntries(CGameLoader::ReadEntryCallback, (uintptr_t)&loader));
	metadata.set(loader.GetMetadata());

	return INFO::OK;	
}

std::vector<CScriptValRooted> SavedGames::GetSavedGames(ScriptInterface& scriptInterface)
{
	TIMER(L"GetSavedGames");
	JSContext* cx = scriptInterface.GetContext();
	JSAutoRequest rq(cx);
	
	std::vector<CScriptValRooted> games;

	Status err;

	VfsPaths pathnames;
	err = vfs::GetPathnames(g_VFS, "saves/", L"*.0adsave", pathnames);
	WARN_IF_ERR(err);

	for (size_t i = 0; i < pathnames.size(); ++i)
	{
		OsPath realPath;
		err = g_VFS->GetRealPath(pathnames[i], realPath);
		if (err < 0)
		{
			DEBUG_WARN_ERR(err);
			continue; // skip this file
		}

		PIArchiveReader archiveReader = CreateArchiveReader_Zip(realPath);
		if (!archiveReader)
		{
			// Triggered by e.g. the file being open in another program
			LOGWARNING(L"Failed to read saved game '%ls'", realPath.string().c_str());
			continue; // skip this file
		}

		CGameLoader loader(scriptInterface, NULL);
		err = archiveReader->ReadEntries(CGameLoader::ReadEntryCallback, (uintptr_t)&loader);
		if (err < 0)
		{
			DEBUG_WARN_ERR(err);
			continue; // skip this file
		}
		JS::RootedValue metadata(cx, loader.GetMetadata());
		
		JS::RootedValue game(cx);
		scriptInterface.Eval("({})", &game);
		scriptInterface.SetProperty(game, "id", pathnames[i].Basename());
		scriptInterface.SetProperty(game, "metadata", metadata);
		games.push_back(CScriptValRooted(cx, game));
	}

	return games;
}

bool SavedGames::DeleteSavedGame(const std::wstring& name)
{
	const VfsPath basename(L"saves/" + name);
	const VfsPath filename = basename.ChangeExtension(L".0adsave");
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

CScriptValRooted SavedGames::GetEngineInfo(ScriptInterface& scriptInterface) 
{ 
	JSContext* cx = scriptInterface.GetContext();
	JSAutoRequest rq(cx);
	
	JS::RootedValue metainfo(cx); 
	scriptInterface.Eval("({})", &metainfo); 
	scriptInterface.SetProperty(metainfo, "version_major", SAVED_GAME_VERSION_MAJOR); 
	scriptInterface.SetProperty(metainfo, "version_minor", SAVED_GAME_VERSION_MINOR); 
	scriptInterface.SetProperty(metainfo, "mods"         , g_modsLoaded);
	return CScriptValRooted(cx, metainfo); 
}

