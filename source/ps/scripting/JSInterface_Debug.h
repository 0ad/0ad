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

#ifndef INCLUDED_JSI_DEBUG
#define INCLUDED_JSI_DEBUG

#include "scriptinterface/ScriptInterface.h"

#include <string>

namespace JSI_Debug
{
	int Crash(ScriptInterface::CmptPrivate* UNUSED(pCmptPrivate));
	void DebugWarn(ScriptInterface::CmptPrivate* UNUSED(pCmptPrivate));
	void DisplayErrorDialog(ScriptInterface::CmptPrivate* UNUSED(pCmptPrivate), const std::wstring& msg);
	std::wstring GetBuildDate(ScriptInterface::CmptPrivate* UNUSED(pCmptPrivate));
	double GetBuildTimestamp(ScriptInterface::CmptPrivate* UNUSED(pCmptPrivate));
	std::wstring GetBuildRevision(ScriptInterface::CmptPrivate* UNUSED(pCmptPrivate));
	double GetMicroseconds(ScriptInterface::CmptPrivate* UNUSED(pCmptPrivate));

	void RegisterScriptFunctions(const ScriptInterface& ScriptInterface);
}

#endif // INCLUDED_JSI_DEBUG
