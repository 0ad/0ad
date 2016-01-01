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

#include "precompiled.h"

#include "VisualReplay.h"
#include "graphics/GameView.h"
#include "gui/GUIManager.h"
#include "lib/allocators/shared_ptr.h"
#include "lib/utf8.h"
#include "network/NetClient.h"
#include "network/NetServer.h"
#include "ps/CLogger.h"
#include "ps/Filesystem.h"
#include "ps/Game.h"
#include "ps/Pyrogenesis.h"
#include "ps/Replay.h"
#include "scriptinterface/ScriptInterface.h"

/**
 * Filter too short replays (value in seconds).
 */
const u8 minimumReplayDuration = 3;

OsPath VisualReplay::GetDirectoryName()
{
	return OsPath(psLogDir() / L"sim_log");
}

void VisualReplay::StartVisualReplay(const CStrW& directory)
{
	ENSURE(!g_NetServer);
	ENSURE(!g_NetClient);
	ENSURE(!g_Game);

	const OsPath replayFile = VisualReplay::GetDirectoryName() / directory / L"commands.txt";

	if (!FileExists(replayFile))
		return;

	g_Game = new CGame(false, false);
	g_Game->StartVisualReplay(replayFile.string8());
}

JS::Value VisualReplay::GetReplays(ScriptInterface& scriptInterface)
{
	TIMER(L"GetReplays");
	JSContext* cx = scriptInterface.GetContext();
	JSAutoRequest rq(cx);

	u32 i = 0;
	DirectoryNames directories;
	JS::RootedObject replays(cx, JS_NewArrayObject(cx, 0));
	if (GetDirectoryEntries(GetDirectoryName(), NULL, &directories) == INFO::OK)
		for (OsPath& directory : directories)
		{
			JS::RootedValue replayData(cx, LoadReplayData(scriptInterface, directory));
			if (!replayData.isNull())
				JS_SetElement(cx, replays, i++, replayData);
		}
	return JS::ObjectValue(*replays);
}

/**
 * Move the cursor backwards until a newline was read or the beginning of the file was found.
 * Either way the cursor points to the beginning of a newline.
 *
 * @return The current cursor position or -1 on error.
 */
inline int goBackToLineBeginning(std::istream* replayStream, const CStr& fileName, const u64& fileSize)
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
			LOGERROR("Unknown error when returning to the last line (%i of %lu) of %s", currentPos, fileSize, fileName.c_str());
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

	LOGERROR("Infinite loop when going back to a line beginning in %s", fileName.c_str());
	return -1;
}

/**
 * Compute game duration. Assume constant turn length.
 * Find the last line that starts with "turn" by reading the file backwards.
 *
 * @return seconds or -1 on error
 */
inline int getReplayDuration(std::istream *replayStream, const CStr& fileName, const u64& fileSize)
{
	CStr type;

	// Move one character before the file-end
	replayStream->seekg(-2, std::ios_base::end);

	// Infinite loop protection, should never occur.
	// There should be about 5 lines to read until a turn is found.
	for (int linesRead = 1; linesRead < 1000; ++linesRead)
	{
		int currentPosition = goBackToLineBeginning(replayStream, fileName, fileSize);

		// Read error or reached file beginning. No turns exist.
		if (currentPosition < 1)
			return -1;

		if (!replayStream->good())
		{
			LOGERROR("Read error when determining replay duration at %i of %llu in %s", currentPosition - 2, fileSize, fileName.c_str());
			return -1;
		}

		// Found last turn, compute duration.
		if ((u64) currentPosition + 4 < fileSize && (*replayStream >> type).good() && type == "turn")
		{
			u32 turn = 0, turnLength = 0;
			*replayStream >> turn >> turnLength;
			return (turn+1) * turnLength / 1000; // add +1 as turn numbers starts with 0
		}

		// Otherwise move cursor back to the character before the last newline
		replayStream->seekg(currentPosition - 2, std::ios_base::beg);
	}

	LOGERROR("Infinite loop when determining replay duration for %s", fileName.c_str());
	return -1;
}

JS::Value VisualReplay::LoadReplayData(ScriptInterface& scriptInterface, OsPath& directory)
{
	// The directory argument must not be constant, otherwise concatenating will fail
	const OsPath replayFile = GetDirectoryName() / directory / L"commands.txt";

	if (!FileExists(replayFile))
		return JSVAL_NULL;

	// Get file size and modification date
	CFileInfo fileInfo;
	GetFileInfo(replayFile, &fileInfo);
	const u64 fileTime = (u64)fileInfo.MTime() & ~1; // skip lowest bit, since zip and FAT don't preserve it (according to CCacheLoader::LooseCachePath)
	const u64 fileSize = (u64)fileInfo.Size();

	if (fileSize == 0)
		return JSVAL_NULL;

	// Open file
	const CStr fileName = replayFile.string8();
	std::ifstream* replayStream = new std::ifstream(fileName.c_str());

	// File must begin with "start"
	CStr type;
	if (!(*replayStream >> type).good())
	{
		LOGERROR("Couldn't open %s. Non-latin characters are not supported yet.", fileName.c_str());
		SAFE_DELETE(replayStream);
		return JSVAL_NULL;
	}
	if (type != "start")
	{
		LOGWARNING("The replay %s is broken!", fileName.c_str());
		SAFE_DELETE(replayStream);
		return JSVAL_NULL;
	}

	// Parse header / first line
	CStr header;
	std::getline(*replayStream, header);
	JSContext* cx = scriptInterface.GetContext();
	JSAutoRequest rq(cx);
	JS::RootedValue attribs(cx);
	if (!scriptInterface.ParseJSON(header, &attribs))
	{
		LOGERROR("Couldn't parse replay header of %s", fileName.c_str());
		SAFE_DELETE(replayStream);
		return JSVAL_NULL;
	}

	// Ensure "turn" after header
	if (!(*replayStream >> type).good() || type != "turn")
	{
		SAFE_DELETE(replayStream);
		return JSVAL_NULL; // there are no turns at all
	}

	// Don't process files of rejoined clients
	u32 turn = 1;
	*replayStream >> turn;
	if (turn != 0)
	{
		SAFE_DELETE(replayStream);
		return JSVAL_NULL;
	}

	int duration = getReplayDuration(replayStream, fileName, fileSize);

	SAFE_DELETE(replayStream);

	// Ensure minimum duration
	if (duration < minimumReplayDuration)
		return JSVAL_NULL;

	// Return the actual data
	JS::RootedValue replayData(cx);
	scriptInterface.Eval("({})", &replayData);
	scriptInterface.SetProperty(replayData, "file", replayFile);
	scriptInterface.SetProperty(replayData, "directory", directory);
	scriptInterface.SetProperty(replayData, "filemod_timestamp", std::to_string(fileTime));
	scriptInterface.SetProperty(replayData, "attribs", attribs);
	scriptInterface.SetProperty(replayData, "duration", duration);
	return replayData;
}

bool VisualReplay::DeleteReplay(const CStrW& replayDirectory)
{
	if (replayDirectory.empty())
		return false;

	const OsPath directory = GetDirectoryName() / replayDirectory;
	return DirectoryExists(directory) && DeleteDirectory(directory) == INFO::OK;
}


JS::Value VisualReplay::GetReplayAttributes(ScriptInterface::CxPrivate* pCxPrivate, const CStrW& directoryName)
{
	// Create empty JS object
	JSContext* cx = pCxPrivate->pScriptInterface->GetContext();
	JSAutoRequest rq(cx);
	JS::RootedValue attribs(cx);
	pCxPrivate->pScriptInterface->Eval("({})", &attribs);

	// Return empty object if file doesn't exist
	const OsPath replayFile = GetDirectoryName() / directoryName / L"commands.txt";
	if (!FileExists(replayFile))
		return attribs;

	// Open file
	std::istream* replayStream = new std::ifstream(replayFile.string8().c_str());
	CStr type, line;
	ENSURE((*replayStream >> type).good() && type == "start");

	// Read and return first line
	std::getline(*replayStream, line);
	pCxPrivate->pScriptInterface->ParseJSON(line, &attribs);
	SAFE_DELETE(replayStream);;
	return attribs;
}

// TODO: enhancement: how to save the data if the process is killed? (case SDL_QUIT in main.cpp)
void VisualReplay::SaveReplayMetadata(const CStrW& data)
{
	// TODO: enhancement: use JS::HandleValue similar to SaveGame
	if (!g_Game)
		return;

	// Get the directory of the currently active replay
	const OsPath fileName = g_Game->GetReplayLogger().GetDirectory() / L"metadata.json";
	CreateDirectories(fileName.Parent(), 0700);

	std::ofstream stream (fileName.string8().c_str(), std::ofstream::out | std::ofstream::trunc);
	stream << utf8_from_wstring(data);
	stream.close();
}

bool VisualReplay::HasReplayMetadata(const CStrW& directoryName)
{
	return FileExists(GetDirectoryName() / directoryName / L"metadata.json");
}

JS::Value VisualReplay::GetReplayMetadata(ScriptInterface::CxPrivate* pCxPrivate, const CStrW& directoryName)
{
	if (!HasReplayMetadata(directoryName))
		return JSVAL_NULL;

	JSContext* cx = pCxPrivate->pScriptInterface->GetContext();
	JSAutoRequest rq(cx);
	JS::RootedValue metadata(cx);

	std::ifstream* stream = new std::ifstream(OsPath(GetDirectoryName() / directoryName / L"metadata.json").string8());
	ENSURE(stream->good());
	CStr line;
	std::getline(*stream, line);
	stream->close();
	SAFE_DELETE(stream);
	pCxPrivate->pScriptInterface->ParseJSON(line, &metadata);

	return metadata;
}
