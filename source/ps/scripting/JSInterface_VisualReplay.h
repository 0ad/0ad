/* Copyright (C) 2017 Wildfire Games.
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

#ifndef INCLUDED_JSI_VISUALREPLAY
#define INCLUDED_JSI_VISUALREPLAY

#include "scriptinterface/ScriptInterface.h"

namespace JSI_VisualReplay
{
	bool StartVisualReplay(ScriptInterface::CmptPrivate* pCmptPrivate, const CStrW& directory);
	bool DeleteReplay(ScriptInterface::CmptPrivate* pCmptPrivate, const CStrW& replayFile);
	JS::Value GetReplays(ScriptInterface::CmptPrivate* pCmptPrivate, bool compareFiles);
	JS::Value GetReplayAttributes(ScriptInterface::CmptPrivate* pCmptPrivate, const CStrW& directoryName);
	bool HasReplayMetadata(ScriptInterface::CmptPrivate* pCmptPrivate, const CStrW& directoryName);
	JS::Value GetReplayMetadata(ScriptInterface::CmptPrivate* pCmptPrivate, const CStrW& directoryName);
	void AddReplayToCache(ScriptInterface::CmptPrivate* pCmptPrivate, const CStrW& directoryName);
	void RegisterScriptFunctions(const ScriptInterface& scriptInterface);
	CStrW GetReplayDirectoryName(ScriptInterface::CmptPrivate* pCmptPrivate, const CStrW& directoryName);
}

#endif // INCLUDED_JSI_VISUALREPLAY
