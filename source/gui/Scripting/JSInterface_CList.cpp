/* Copyright (C) 2020 Wildfire Games.
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

#include "JSInterface_GUIProxy_impl.h"

#include "gui/ObjectTypes/CList.h"

bool CList_AddItem(JSContext* cx, uint argc, JS::Value* vp)
{
	JS::CallArgs args = JS::CallArgsFromVp(argc, vp);
	CList* e = static_cast<CList*>(js::GetProxyPrivate(args.thisv().toObjectOrNull()).toPrivate());
	if (!e)
		return false;

	ScriptInterface& scriptInterface = *ScriptInterface::GetScriptInterfaceAndCBData(cx)->pScriptInterface;
	ScriptRequest rq(scriptInterface);
	CGUIString text;
	if (!ScriptInterface::FromJSVal(rq, args.get(0), text))
		return false;
	e->AddItem(text, text);
	return true;
}

using GUIObjectType = CList;

template<>
void JSI_GUIProxy<GUIObjectType>::CreateFunctions(const ScriptRequest& rq, GUIProxyProps* cache)
{
#define func(class, func) &JSInterface_GUIProxy::apply_to<GUIObjectType, class, &class::func>
	cache->setFunction(rq, "toString", func(IGUIObject, toString), 0);
	cache->setFunction(rq, "focus", func(IGUIObject, focus), 0);
	cache->setFunction(rq, "blur", func(IGUIObject, blur), 0);
	cache->setFunction(rq, "getComputedSize", func(IGUIObject, getComputedSize), 0);
#undef func
	cache->setFunction(rq, "addItem", CList_AddItem, 1);
}

template class JSI_GUIProxy<GUIObjectType>;
