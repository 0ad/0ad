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

#include "ps/VisualReplay.h"
#include "ps/scripting/JSInterface_VisualReplay.h"

void JSI_VisualReplay::StartVisualReplay(ScriptInterface::CxPrivate* UNUSED(pCxPrivate), const CStrW& directory)
{
	VisualReplay::StartVisualReplay(directory);
}

bool JSI_VisualReplay::DeleteReplay(ScriptInterface::CxPrivate* UNUSED(pCxPrivate), const CStrW& replayFile)
{
	return VisualReplay::DeleteReplay(replayFile);
}

JS::Value JSI_VisualReplay::GetReplays(ScriptInterface::CxPrivate* pCxPrivate)
{
	return VisualReplay::GetReplays(*(pCxPrivate->pScriptInterface));
}

JS::Value JSI_VisualReplay::GetReplayAttributes(ScriptInterface::CxPrivate* pCxPrivate, const CStrW& directoryName)
{
	return VisualReplay::GetReplayAttributes(pCxPrivate, directoryName);
}

bool JSI_VisualReplay::HasReplayMetadata(ScriptInterface::CxPrivate* UNUSED(pCxPrivate), const CStrW& directoryName)
{
	return VisualReplay::HasReplayMetadata(directoryName);
}

JS::Value JSI_VisualReplay::GetReplayMetadata(ScriptInterface::CxPrivate* pCxPrivate, const CStrW& directoryName)
{
	return VisualReplay::GetReplayMetadata(pCxPrivate, directoryName);
}

void JSI_VisualReplay::SaveReplayMetadata(ScriptInterface::CxPrivate* UNUSED(pCxPrivate), const CStrW& data)
{
	VisualReplay::SaveReplayMetadata(data);
}

void JSI_VisualReplay::RegisterScriptFunctions(ScriptInterface& scriptInterface)
{
	scriptInterface.RegisterFunction<JS::Value, &GetReplays>("GetReplays");
	scriptInterface.RegisterFunction<bool, CStrW, &DeleteReplay>("DeleteReplay");
	scriptInterface.RegisterFunction<void, CStrW, &StartVisualReplay>("StartVisualReplay");
	scriptInterface.RegisterFunction<JS::Value, CStrW, &GetReplayAttributes>("GetReplayAttributes");
	scriptInterface.RegisterFunction<JS::Value, CStrW, &GetReplayMetadata>("GetReplayMetadata");
	scriptInterface.RegisterFunction<bool, CStrW, &HasReplayMetadata>("HasReplayMetadata");
	scriptInterface.RegisterFunction<void, CStrW, &SaveReplayMetadata>("SaveReplayMetadata");
}
