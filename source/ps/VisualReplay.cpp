/* Copyright (C) 2019 Wildfire Games.
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

#include "VisualReplay.h"
#include "graphics/GameView.h"
#include "gui/GUIManager.h"
#include "lib/allocators/shared_ptr.h"
#include "lib/external_libraries/libsdl.h"
#include "lib/utf8.h"
#include "network/NetClient.h"
#include "network/NetServer.h"
#include "ps/CLogger.h"
#include "ps/Filesystem.h"
#include "ps/Game.h"
#include "ps/GameSetup/Paths.h"
#include "ps/Mod.h"
#include "ps/Pyrogenesis.h"
#include "ps/Replay.h"
#include "ps/Util.h"
#include "scriptinterface/ScriptInterface.h"

/**
 * Filter too short replays (value in seconds).
 */
const u8 minimumReplayDuration = 3;

OsPath VisualReplay::GetDirectoryPath()
{
	return Paths(g_args).UserData() / "replays" / engine_version;
}

OsPath VisualReplay::GetCacheFilePath()
{
	return GetDirectoryPath() / L"replayCache.json";
}

OsPath VisualReplay::GetTempCacheFilePath()
{
	return GetDirectoryPath() / L"replayCache_temp.json";
}

bool VisualReplay::StartVisualReplay(const OsPath& directory)
{
	ENSURE(!g_NetServer);
	ENSURE(!g_NetClient);
	ENSURE(!g_Game);

	const OsPath replayFile = VisualReplay::GetDirectoryPath() / directory / L"commands.txt";

	if (!FileExists(replayFile))
		return false;

	g_Game = new CGame(false, false);
	return g_Game->StartVisualReplay(replayFile);
}

bool VisualReplay::ReadCacheFile(const ScriptInterface& scriptInterface, JS::MutableHandleObject cachedReplaysObject)
{
	JSContext* cx = scriptInterface.GetContext();
	JSAutoRequest rq(cx);

	if (!FileExists(GetCacheFilePath()))
		return false;

	std::ifstream cacheStream(OsString(GetCacheFilePath()).c_str());
	CStr cacheStr((std::istreambuf_iterator<char>(cacheStream)), std::istreambuf_iterator<char>());
	cacheStream.close();

	JS::RootedValue cachedReplays(cx);
	if (scriptInterface.ParseJSON(cacheStr, &cachedReplays))
	{
		cachedReplaysObject.set(&cachedReplays.toObject());
		bool isArray;
		if (JS_IsArrayObject(cx, cachedReplaysObject, &isArray) && isArray)
			return true;
	}

	LOGWARNING("The replay cache file is corrupted, it will be deleted");
	wunlink(GetCacheFilePath());
	return false;
}

void VisualReplay::StoreCacheFile(const ScriptInterface& scriptInterface, JS::HandleObject replays)
{
	JSContext* cx = scriptInterface.GetContext();
	JSAutoRequest rq(cx);

	JS::RootedValue replaysRooted(cx, JS::ObjectValue(*replays));
	std::ofstream cacheStream(OsString(GetTempCacheFilePath()).c_str(), std::ofstream::out | std::ofstream::trunc);
	cacheStream << scriptInterface.StringifyJSON(&replaysRooted);
	cacheStream.close();

	wunlink(GetCacheFilePath());
	if (wrename(GetTempCacheFilePath(), GetCacheFilePath()))
		LOGERROR("Could not store the replay cache");
}

JS::HandleObject VisualReplay::ReloadReplayCache(const ScriptInterface& scriptInterface, bool compareFiles)
{
	TIMER(L"ReloadReplayCache");
	JSContext* cx = scriptInterface.GetContext();
	JSAutoRequest rq(cx);

	// Maps the filename onto the index and size
	typedef std::map<OsPath, std::pair<u32, off_t>> replayCacheMap;

	replayCacheMap fileList;

	JS::RootedObject cachedReplaysObject(cx);
	if (ReadCacheFile(scriptInterface, &cachedReplaysObject))
	{
		// Create list of files included in the cache
		u32 cacheLength = 0;
		JS_GetArrayLength(cx, cachedReplaysObject, &cacheLength);
		for (u32 j = 0; j < cacheLength; ++j)
		{
			JS::RootedValue replay(cx);
			JS_GetElement(cx, cachedReplaysObject, j, &replay);

			JS::RootedValue file(cx);
			OsPath fileName;
			double fileSize;
			scriptInterface.GetProperty(replay, "directory", fileName);
			scriptInterface.GetProperty(replay, "fileSize", fileSize);

			fileList[fileName] = std::make_pair(j, fileSize);
		}
	}

	JS::RootedObject replays(cx, JS_NewArrayObject(cx, 0));
	DirectoryNames directories;

	if (GetDirectoryEntries(GetDirectoryPath(), nullptr, &directories) != INFO::OK)
		return replays;

	bool newReplays = false;
	std::vector<u32> copyFromOldCache;
	// Specifies where the next replay should be kept
	u32 i = 0;

	for (const OsPath& directory : directories)
	{
		// This cannot use IsQuitRequested(), because the current loop and that function both run in the main thread.
		// So SDL events are not processed unless called explicitly here.
		if (SDL_QuitRequested())
			// Don't return, because we want to save our progress
			break;

		const OsPath replayFile = GetDirectoryPath() / directory / L"commands.txt";

		bool isNew = true;
		replayCacheMap::iterator it = fileList.find(directory);
		if (it != fileList.end())
		{
			if (compareFiles)
			{
				if (!FileExists(replayFile))
					continue;
				CFileInfo fileInfo;
				GetFileInfo(replayFile, &fileInfo);
				if (fileInfo.Size() == it->second.second)
					isNew = false;
			}
			else
				isNew = false;
		}

		if (isNew)
		{
			JS::RootedValue replayData(cx, LoadReplayData(scriptInterface, directory));
			if (replayData.isNull())
			{
				if (!FileExists(replayFile))
					continue;
				CFileInfo fileInfo;
				GetFileInfo(replayFile, &fileInfo);

				scriptInterface.CreateObject(
					&replayData,
					"directory", directory.string(),
					"fileSize", static_cast<double>(fileInfo.Size()));
			}
			JS_SetElement(cx, replays, i++, replayData);
			newReplays = true;
		}
		else
			copyFromOldCache.push_back(it->second.first);
	}

	debug_printf(
		"Loading %lu cached replays, removed %lu outdated entries, loaded %i new entries\n",
		(unsigned long)fileList.size(), (unsigned long)(fileList.size() - copyFromOldCache.size()), i);

	if (!newReplays && fileList.empty())
		return replays;

	// No replay was changed, so just return the cache
	if (!newReplays && fileList.size() == copyFromOldCache.size())
		return cachedReplaysObject;

	{
		// Copy the replays from the old cache that are not deleted
		if (!copyFromOldCache.empty())
			for (u32 j : copyFromOldCache)
			{
				JS::RootedValue replay(cx);
				JS_GetElement(cx, cachedReplaysObject, j, &replay);
				JS_SetElement(cx, replays, i++, replay);
			}
	}
	StoreCacheFile(scriptInterface, replays);
	return replays;
}

JS::Value VisualReplay::GetReplays(const ScriptInterface& scriptInterface, bool compareFiles)
{
	TIMER(L"GetReplays");
	JSContext* cx = scriptInterface.GetContext();
	JSAutoRequest rq(cx);
	JS::RootedObject replays(cx, ReloadReplayCache(scriptInterface, compareFiles));
	// Only take entries with data
	JS::RootedValue replaysWithoutNullEntries(cx);
	scriptInterface.CreateArray(&replaysWithoutNullEntries);

	u32 replaysLength = 0;
	JS_GetArrayLength(cx, replays, &replaysLength);
	for (u32 j = 0, i = 0; j < replaysLength; ++j)
	{
		JS::RootedValue replay(cx);
		JS_GetElement(cx, replays, j, &replay);
		if (scriptInterface.HasProperty(replay, "attribs"))
			scriptInterface.SetPropertyInt(replaysWithoutNullEntries, i++, replay);
	}
	return replaysWithoutNullEntries;
}

/**
 * Move the cursor backwards until a newline was read or the beginning of the file was found.
 * Either way the cursor points to the beginning of a newline.
 *
 * @return The current cursor position or -1 on error.
 */
inline off_t goBackToLineBeginning(std::istream* replayStream, const OsPath& fileName, off_t fileSize)
{
	int currentPos;
	char character;
	for (int characters = 0; characters < 10000; ++characters)
	{
		currentPos = (int) replayStream->tellg();

		// Stop when reached the beginning of the file
		if (currentPos == 0)
			return currentPos;

		if (!replayStream->good())
		{
			LOGERROR("Unknown error when returning to the last line (%i of %lu) of %s", currentPos, fileSize, fileName.string8().c_str());
			return -1;
		}

		// Stop when reached newline
		replayStream->get(character);
		if (character == '\n')
			return currentPos;

		// Otherwise go back one character.
		// Notice: -1 will set the cursor back to the most recently read character.
		replayStream->seekg(-2, std::ios_base::cur);
	}

	LOGERROR("Infinite loop when going back to a line beginning in %s", fileName.string8().c_str());
	return -1;
}

/**
 * Compute game duration in seconds. Assume constant turn length.
 * Find the last line that starts with "turn" by reading the file backwards.
 *
 * @return seconds or -1 on error
 */
inline int getReplayDuration(std::istream* replayStream, const OsPath& fileName, off_t fileSize)
{
	CStr type;

	// Move one character before the file-end
	replayStream->seekg(-2, std::ios_base::end);

	// Infinite loop protection, should never occur.
	// There should be about 5 lines to read until a turn is found.
	for (int linesRead = 1; linesRead < 1000; ++linesRead)
	{
		off_t currentPosition = goBackToLineBeginning(replayStream, fileName, fileSize);

		// Read error or reached file beginning. No turns exist.
		if (currentPosition < 1)
			return -1;

		if (!replayStream->good())
		{
			LOGERROR("Read error when determining replay duration at %i of %llu in %s", currentPosition - 2, fileSize, fileName.string8().c_str());
			return -1;
		}

		// Found last turn, compute duration.
		if (currentPosition + 4 < fileSize && (*replayStream >> type).good() && type == "turn")
		{
			u32 turn = 0, turnLength = 0;
			*replayStream >> turn >> turnLength;
			return (turn+1) * turnLength / 1000; // add +1 as turn numbers starts with 0
		}

		// Otherwise move cursor back to the character before the last newline
		replayStream->seekg(currentPosition - 2, std::ios_base::beg);
	}

	LOGERROR("Infinite loop when determining replay duration for %s", fileName.string8().c_str());
	return -1;
}

JS::Value VisualReplay::LoadReplayData(const ScriptInterface& scriptInterface, const OsPath& directory)
{
	// The directory argument must not be constant, otherwise concatenating will fail
	const OsPath replayFile = GetDirectoryPath() / directory / L"commands.txt";

	if (!FileExists(replayFile))
		return JS::NullValue();

	// Get file size and modification date
	CFileInfo fileInfo;
	GetFileInfo(replayFile, &fileInfo);
	const off_t fileSize = fileInfo.Size();

	if (fileSize == 0)
		return JS::NullValue();

	std::ifstream* replayStream = new std::ifstream(OsString(replayFile).c_str());

	CStr type;
	if (!(*replayStream >> type).good())
	{
		LOGERROR("Couldn't open %s.", replayFile.string8().c_str());
		SAFE_DELETE(replayStream);
		return JS::NullValue();
	}

	if (type != "start")
	{
		LOGWARNING("The replay %s doesn't begin with 'start'!", replayFile.string8().c_str());
		SAFE_DELETE(replayStream);
		return JS::NullValue();
	}

	// Parse header / first line
	CStr header;
	std::getline(*replayStream, header);
	JSContext* cx = scriptInterface.GetContext();
	JSAutoRequest rq(cx);
	JS::RootedValue attribs(cx);
	if (!scriptInterface.ParseJSON(header, &attribs))
	{
		LOGERROR("Couldn't parse replay header of %s", replayFile.string8().c_str());
		SAFE_DELETE(replayStream);
		return JS::NullValue();
	}

	// Ensure "turn" after header
	if (!(*replayStream >> type).good() || type != "turn")
	{
		SAFE_DELETE(replayStream);
		return JS::NullValue(); // there are no turns at all
	}

	// Don't process files of rejoined clients
	u32 turn = 1;
	*replayStream >> turn;
	if (turn != 0)
	{
		SAFE_DELETE(replayStream);
		return JS::NullValue();
	}

	int duration = getReplayDuration(replayStream, replayFile, fileSize);

	SAFE_DELETE(replayStream);

	// Ensure minimum duration
	if (duration < minimumReplayDuration)
		return JS::NullValue();

	// Return the actual data
	JS::RootedValue replayData(cx);

	scriptInterface.CreateObject(
		&replayData,
		"directory", directory.string(),
		"fileSize", static_cast<double>(fileSize),
		"duration", static_cast<u32>(duration));

	scriptInterface.SetProperty(replayData, "attribs", attribs);

	return replayData;
}

bool VisualReplay::DeleteReplay(const OsPath& replayDirectory)
{
	if (replayDirectory.empty())
		return false;

	const OsPath directory = GetDirectoryPath() / replayDirectory;
	return DirectoryExists(directory) && DeleteDirectory(directory) == INFO::OK;
}

JS::Value VisualReplay::GetReplayAttributes(ScriptInterface::CxPrivate* pCxPrivate, const OsPath& directoryName)
{
	// Create empty JS object
	JSContext* cx = pCxPrivate->pScriptInterface->GetContext();
	JSAutoRequest rq(cx);
	JS::RootedValue attribs(cx);
	pCxPrivate->pScriptInterface->CreateObject(&attribs);

	// Return empty object if file doesn't exist
	const OsPath replayFile = GetDirectoryPath() / directoryName / L"commands.txt";
	if (!FileExists(replayFile))
		return attribs;

	// Open file
	std::istream* replayStream = new std::ifstream(OsString(replayFile).c_str());
	CStr type, line;
	ENSURE((*replayStream >> type).good() && type == "start");

	// Read and return first line
	std::getline(*replayStream, line);
	pCxPrivate->pScriptInterface->ParseJSON(line, &attribs);
	SAFE_DELETE(replayStream);;
	return attribs;
}

void VisualReplay::AddReplayToCache(const ScriptInterface& scriptInterface, const CStrW& directoryName)
{
	TIMER(L"AddReplayToCache");
	JSContext* cx = scriptInterface.GetContext();
	JSAutoRequest rq(cx);

	JS::RootedValue replayData(cx, LoadReplayData(scriptInterface, OsPath(directoryName)));
	if (replayData.isNull())
		return;

	JS::RootedObject cachedReplaysObject(cx);
	if (!ReadCacheFile(scriptInterface, &cachedReplaysObject))
		cachedReplaysObject = JS_NewArrayObject(cx, 0);

	u32 cacheLength = 0;
	JS_GetArrayLength(cx, cachedReplaysObject, &cacheLength);
	JS_SetElement(cx, cachedReplaysObject, cacheLength, replayData);

	StoreCacheFile(scriptInterface, cachedReplaysObject);
}

void VisualReplay::SaveReplayMetadata(ScriptInterface* scriptInterface)
{
	JSContext* cx = scriptInterface->GetContext();
	JSAutoRequest rq(cx);

	JS::RootedValue metadata(cx);
	JS::RootedValue global(cx, scriptInterface->GetGlobalObject());

	if (!scriptInterface->CallFunction(global, "getReplayMetadata", &metadata))
	{
		LOGERROR("Could not save replay metadata!");
		return;
	}

	// Get the directory of the currently active replay
	const OsPath fileName = g_Game->GetReplayLogger().GetDirectory() / L"metadata.json";
	CreateDirectories(fileName.Parent(), 0700);

	std::ofstream stream (OsString(fileName).c_str(), std::ofstream::out | std::ofstream::trunc);
	stream << scriptInterface->StringifyJSON(&metadata, false);
	stream.close();
	debug_printf("Saved replay metadata to %s\n", fileName.string8().c_str());
}

bool VisualReplay::HasReplayMetadata(const OsPath& directoryName)
{
	const OsPath filePath(GetDirectoryPath() / directoryName / L"metadata.json");

	if (!FileExists(filePath))
		return false;

	CFileInfo fileInfo;
	GetFileInfo(filePath, &fileInfo);

	return fileInfo.Size() > 0;
}

JS::Value VisualReplay::GetReplayMetadata(ScriptInterface::CxPrivate* pCxPrivate, const OsPath& directoryName)
{
	if (!HasReplayMetadata(directoryName))
		return JS::NullValue();

	JSContext* cx = pCxPrivate->pScriptInterface->GetContext();
	JSAutoRequest rq(cx);
	JS::RootedValue metadata(cx);

	std::ifstream* stream = new std::ifstream(OsString(GetDirectoryPath() / directoryName / L"metadata.json").c_str());
	ENSURE(stream->good());
	CStr line;
	std::getline(*stream, line);
	stream->close();
	SAFE_DELETE(stream);
	pCxPrivate->pScriptInterface->ParseJSON(line, &metadata);

	return metadata;
}
