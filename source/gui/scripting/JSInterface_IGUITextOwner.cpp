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
	JS_FN("getTextSize", JSI_IGUITextOwner::GetTextSize, 0, 0),
	JS_FS_END
};

void JSI_IGUITextOwner::RegisterScriptFunctions(JSContext* cx, JS::HandleObject obj)
{
	JS_DefineFunctions(cx, obj, JSI_methods);
}

bool JSI_IGUITextOwner::GetTextSize(JSContext* cx, uint argc, JS::Value* vp)
{
	// No JSAutoRequest needed for these calls
	JS::CallArgs args = JS::CallArgsFromVp(argc, vp);
	IGUIObject* obj = ScriptInterface::GetPrivate<IGUIObject>(cx, args, &JSI_IGUIObject::JSI_class);
	if (!obj)
		return false;

	// Avoid dynamic_cast for performance reasons
	IGUITextOwner* objText = static_cast<IGUITextOwner*>(obj->GetTextOwner());
	if (!objText)
	{
		JSAutoRequest rq(cx);
		JS_ReportError(cx, "This IGUIObject is not an IGUITextOwner!");
		return false;
	}

	ScriptInterface::ToJSVal(cx, args.rval(), objText->CalculateTextSize());
	return true;
}
