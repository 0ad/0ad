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

#include "JSInterface_IGUITextOwner.h"

#include "gui/IGUITextOwner.h"
#include "scriptinterface/ScriptInterface.h"

JSFunctionSpec JSI_IGUITextOwner::JSI_methods[] =
{
	JS_FN("getTextSize", GetTextSize, 0, 0),
	JS_FS_END
};

void JSI_IGUITextOwner::RegisterScriptFunctions(JSContext* cx, JS::HandleObject obj)
{
	JS_DefineFunctions(cx, obj, JSI_methods);
}

bool JSI_IGUITextOwner::GetTextSize(JSContext* cx, uint UNUSED(argc), JS::Value* vp)
{
	JSAutoRequest rq(cx);
	JS::CallReceiver rec = JS::CallReceiverFromVp(vp);
	JS::RootedObject thisObj(cx, &rec.thisv().toObject());

	IGUIObject* obj = static_cast<IGUIObject*>(JS_GetInstancePrivate(cx, thisObj, &JSI_IGUIObject::JSI_class, nullptr));
	if (!obj)
	{
		JS_ReportError(cx, "This is not an IGUIObject!");
		return false;
	}

	// Avoid dynamic_cast for performance reasons
	IGUITextOwner* objText = static_cast<IGUITextOwner*>(obj->GetTextOwner());
	if (!objText)
	{
		JS_ReportError(cx, "This IGUIObject is not an IGUITextOwner!");
		return false;
	}

	ScriptInterface::ToJSVal(cx, rec.rval(), objText->CalculateTextSize());
	return true;
}
