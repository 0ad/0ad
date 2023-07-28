/* Copyright (C) 2021 Wildfire Games.
 * This file is part of 0 A.D.
 *
 * 0 A.D. is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * 0 A.D. is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with 0 A.D.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "precompiled.h"

#include "JSInterface_VisualReplay.h"

#include "ps/CStr.h"
#include "ps/VisualReplay.h"
#include "scriptinterface/FunctionWrapper.h"
#include "scriptinterface/ScriptRequest.h"

namespace JSI_VisualReplay
{
CStrW GetReplayDirectoryName(const CStrW& directoryName)
{
	// The string conversion is added to account for non-latin characters.
	return wstring_from_utf8(OsPath(VisualReplay::GetDirectoryPath() / directoryName).string8());
}

void RegisterScriptFunctions(const ScriptRequest& rq)
{
	ScriptFunction::Register<&VisualReplay::GetReplays>(rq, "GetReplays");
	ScriptFunction::Register<&VisualReplay::DeleteReplay>(rq, "DeleteReplay");
	ScriptFunction::Register<&VisualReplay::StartVisualReplay>(rq, "StartVisualReplay");
	ScriptFunction::Register<&VisualReplay::GetReplayAttributes>(rq, "GetReplayAttributes");
	ScriptFunction::Register<&VisualReplay::GetReplayMetadata>(rq, "GetReplayMetadata");
	ScriptFunction::Register<&VisualReplay::HasReplayMetadata>(rq, "HasReplayMetadata");
	ScriptFunction::Register<&VisualReplay::AddReplayToCache>(rq, "AddReplayToCache");
	ScriptFunction::Register<&GetReplayDirectoryName>(rq, "GetReplayDirectoryName");
}
}
