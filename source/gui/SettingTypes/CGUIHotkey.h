/* Copyright (C) 2021 Wildfire Games.
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

#ifndef INCLUDED_CGUIHOTKEY
#define INCLUDED_CGUIHOTKEY

#include "gui/CGUISetting.h"
#include "scriptinterface/ScriptForward.h"
#include "ps/CStr.h"

class IGUIObject;
class ScriptRequest;

/**
 * Manages a hotkey setting for a GUI object.
 */
class CGUIHotkey : public CGUISimpleSetting<CStr>
{
public:
	CGUIHotkey(IGUIObject* pObject, const CStr& Name) : CGUISimpleSetting<CStr>(pObject, Name)
	{}
	NONCOPYABLE(CGUIHotkey);
	MOVABLE(CGUIHotkey);

	bool DoFromString(const CStrW& value) override;
	bool DoFromJSVal(const ScriptRequest& rq, JS::HandleValue value) override;
	void OnSettingChange(const CStr& setting, bool sendMessage) override;
};

#endif // INCLUDED_CGUIHOTKEY
