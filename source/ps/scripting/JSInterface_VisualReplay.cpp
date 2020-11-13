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

#include "JSInterface_VisualReplay.h"

#include "ps/CStr.h"
#include "ps/VisualReplay.h"
#include "scriptinterface/ScriptInterface.h"

bool JSI_VisualReplay::StartVisualReplay(ScriptInterface::CmptPrivate* UNUSED(pCmptPrivate), const CStrW& directory)
{
	return VisualReplay::StartVisualReplay(directory);
}

bool JSI_VisualReplay::DeleteReplay(ScriptInterface::CmptPrivate* UNUSED(pCmptPrivate), const CStrW& replayFile)
{
	return VisualReplay::DeleteReplay(replayFile);
}

JS::Value JSI_VisualReplay::GetReplays(ScriptInterface::CmptPrivate* pCmptPrivate, bool compareFiles)
{
	return VisualReplay::GetReplays(*(pCmptPrivate->pScriptInterface), compareFiles);
}

JS::Value JSI_VisualReplay::GetReplayAttributes(ScriptInterface::CmptPrivate* pCmptPrivate, const CStrW& directoryName)
{
	return VisualReplay::GetReplayAttributes(pCmptPrivate, directoryName);
}

bool JSI_VisualReplay::HasReplayMetadata(ScriptInterface::CmptPrivate* UNUSED(pCmptPrivate), const CStrW& directoryName)
{
	return VisualReplay::HasReplayMetadata(directoryName);
}

JS::Value JSI_VisualReplay::GetReplayMetadata(ScriptInterface::CmptPrivate* pCmptPrivate, const CStrW& directoryName)
{
	return VisualReplay::GetReplayMetadata(pCmptPrivate, directoryName);
}

void JSI_VisualReplay::AddReplayToCache(ScriptInterface::CmptPrivate* pCmptPrivate, const CStrW& directoryName)
{
	VisualReplay::AddReplayToCache(*(pCmptPrivate->pScriptInterface), directoryName);
}

CStrW JSI_VisualReplay::GetReplayDirectoryName(ScriptInterface::CmptPrivate* UNUSED(pCmptPrivate), const CStrW& directoryName)
{
	return wstring_from_utf8(OsPath(VisualReplay::GetDirectoryPath() / directoryName).string8());
}

void JSI_VisualReplay::RegisterScriptFunctions(const ScriptInterface& scriptInterface)
{
	scriptInterface.RegisterFunction<JS::Value, bool, &GetReplays>("GetReplays");
	scriptInterface.RegisterFunction<bool, CStrW, &DeleteReplay>("DeleteReplay");
	scriptInterface.RegisterFunction<bool, CStrW, &StartVisualReplay>("StartVisualReplay");
	scriptInterface.RegisterFunction<JS::Value, CStrW, &GetReplayAttributes>("GetReplayAttributes");
	scriptInterface.RegisterFunction<JS::Value, CStrW, &GetReplayMetadata>("GetReplayMetadata");
	scriptInterface.RegisterFunction<bool, CStrW, &HasReplayMetadata>("HasReplayMetadata");
	scriptInterface.RegisterFunction<void, CStrW, &AddReplayToCache>("AddReplayToCache");
	scriptInterface.RegisterFunction<CStrW, CStrW, &GetReplayDirectoryName>("GetReplayDirectoryName");
}
