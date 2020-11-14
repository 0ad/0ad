/* Copyright (C) 2018 Wildfire Games.
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

#ifndef INCLUDED_JSI_MODIO
#define INCLUDED_JSI_MODIO

#include "scriptinterface/ScriptInterface.h"

namespace JSI_ModIo
{
	void RegisterScriptFunctions(const ScriptInterface& scriptInterface);

	void StartGetGameId(ScriptInterface::CmptPrivate* pCmptPrivate);
	void StartListMods(ScriptInterface::CmptPrivate* pCmptPrivate);
	void StartDownloadMod(ScriptInterface::CmptPrivate* pCmptPrivate, uint32_t idx);
	bool AdvanceRequest(ScriptInterface::CmptPrivate* pCmptPrivate);
	void CancelRequest(ScriptInterface::CmptPrivate* pCmptPrivate);
	JS::Value GetMods(ScriptInterface::CmptPrivate* pCmptPrivate);
	JS::Value GetDownloadProgress(ScriptInterface::CmptPrivate* pCmptPrivate);
}

#endif // INCLUDED_JSI_MODIO
