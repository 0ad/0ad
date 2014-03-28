/* Copyright (C) 2010 Wildfire Games.
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

#include "ICmpFootprint.h"

#include "simulation2/system/InterfaceScripted.h"

#include "simulation2/system/SimContext.h"
#include "maths/FixedVector3D.h"

CScriptVal ICmpFootprint::GetShape_wrapper()
{
	EShape shape;
	entity_pos_t size0, size1, height;
	GetShape(shape, size0, size1, height);

	JSContext* cx = GetSimContext().GetScriptInterface().GetContext();

	JSObject* obj = JS_NewObject(cx, NULL, NULL, NULL);
	if (!obj)
		return JSVAL_VOID;

	if (shape == CIRCLE)
	{
		JS::RootedValue ptype(cx);
		JS::RootedValue pradius(cx);
		JS::RootedValue pheight(cx);
		ScriptInterface::ToJSVal<std::string>(cx, ptype.get(), "circle");
		ScriptInterface::ToJSVal(cx, pradius.get(), size0);
		ScriptInterface::ToJSVal(cx, pheight.get(), height);
		JS_SetProperty(cx, obj, "type", ptype.address());
		JS_SetProperty(cx, obj, "radius", pradius.address());
		JS_SetProperty(cx, obj, "height", pheight.address());
	}
	else
	{
		JS::RootedValue ptype(cx);
		JS::RootedValue pwidth(cx);
		JS::RootedValue pdepth(cx);
		JS::RootedValue pheight(cx);
		ScriptInterface::ToJSVal<std::string>(cx, ptype.get(), "square");
		ScriptInterface::ToJSVal(cx, pwidth.get(), size0);
		ScriptInterface::ToJSVal(cx, pdepth.get(), size1);
		ScriptInterface::ToJSVal(cx, pheight.get(), height);
		JS_SetProperty(cx, obj, "type", ptype.address());
		JS_SetProperty(cx, obj, "width", pwidth.address());
		JS_SetProperty(cx, obj, "depth", pdepth.address());
		JS_SetProperty(cx, obj, "height", pheight.address());
	}

	return OBJECT_TO_JSVAL(obj);
}

BEGIN_INTERFACE_WRAPPER(Footprint)
DEFINE_INTERFACE_METHOD_1("PickSpawnPoint", CFixedVector3D, ICmpFootprint, PickSpawnPoint, entity_id_t)
DEFINE_INTERFACE_METHOD_1("PickSpawnPointBothPass", CFixedVector3D, ICmpFootprint, PickSpawnPointBothPass, entity_id_t)
DEFINE_INTERFACE_METHOD_0("GetShape", CScriptVal, ICmpFootprint, GetShape_wrapper)
END_INTERFACE_WRAPPER(Footprint)
