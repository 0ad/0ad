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

#include "JSInterface_GUIProxy_impl.h"

#include "gui/ObjectBases/IGUIObject.h"
#include "gui/ObjectTypes/CButton.h"
#include "gui/ObjectTypes/CList.h"
#include "gui/ObjectTypes/CMiniMap.h"
#include "gui/ObjectTypes/CText.h"

// Called for every specialization - adds the common interface.
template<>
void JSI_GUIProxy<IGUIObject>::CreateFunctions(const ScriptRequest& rq, GUIProxyProps* cache)
{
	CreateFunction<&IGUIObject::GetName>(rq, cache, "toString");
	CreateFunction<&IGUIObject::GetName>(rq, cache, "toSource");
	CreateFunction<&IGUIObject::SetFocus>(rq, cache, "focus");
	CreateFunction<&IGUIObject::ReleaseFocus>(rq, cache, "blur");
	CreateFunction<&IGUIObject::GetComputedSize>(rq, cache, "getComputedSize");
}
DECLARE_GUIPROXY(IGUIObject);

// Implement derived types below.

// CButton
template<> void JSI_GUIProxy<CButton>::CreateFunctions(const ScriptRequest& rq, GUIProxyProps* cache)
{
	CreateFunction<&CButton::GetTextSize>(rq, cache, "getTextSize");
}
DECLARE_GUIPROXY(CButton);

// CText
template<> void JSI_GUIProxy<CText>::CreateFunctions(const ScriptRequest& rq, GUIProxyProps* cache)
{
	CreateFunction<&CText::GetTextSize>(rq, cache, "getTextSize");
}
DECLARE_GUIPROXY(CText);

// CList
template<> void JSI_GUIProxy<CList>::CreateFunctions(const ScriptRequest& rq, GUIProxyProps* cache)
{
	CreateFunction<static_cast<void(CList::*)(const CGUIString&)>(&CList::AddItem)>(rq, cache, "addItem");
}
DECLARE_GUIPROXY(CList);

// CMiniMap
template<> void JSI_GUIProxy<CMiniMap>::CreateFunctions(const ScriptRequest& rq, GUIProxyProps* cache)
{
	CreateFunction<&CMiniMap::Flare>(rq, cache, "flare");
}
DECLARE_GUIPROXY(CMiniMap);
