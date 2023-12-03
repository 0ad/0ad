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

#include "precompiled.h"

#include "ICmpFootprint.h"

#include "simulation2/system/InterfaceScripted.h"

#include "simulation2/system/SimContext.h"
#include "maths/FixedVector3D.h"

JS::Value ICmpFootprint::GetShape_wrapper() const
{
	EShape shape;
	entity_pos_t size0, size1, height;
	GetShape(shape, size0, size1, height);

	ScriptRequest rq(GetSimContext().GetScriptInterface());

	JS::RootedObject obj(rq.cx, JS_NewPlainObject(rq.cx));
	if (!obj)
		return JS::UndefinedValue();

	if (shape == CIRCLE)
	{
		JS::RootedValue ptype(rq.cx);
		JS::RootedValue pradius(rq.cx);
		JS::RootedValue pheight(rq.cx);
		Script::ToJSVal<std::string>(rq, &ptype, "circle");
		Script::ToJSVal(rq, &pradius, size0);
		Script::ToJSVal(rq, &pheight, height);
		JS_SetProperty(rq.cx, obj, "type", ptype);
		JS_SetProperty(rq.cx, obj, "radius", pradius);
		JS_SetProperty(rq.cx, obj, "height", pheight);
	}
	else
	{
		JS::RootedValue ptype(rq.cx);
		JS::RootedValue pwidth(rq.cx);
		JS::RootedValue pdepth(rq.cx);
		JS::RootedValue pheight(rq.cx);
		Script::ToJSVal<std::string>(rq, &ptype, "square");
		Script::ToJSVal(rq, &pwidth, size0);
		Script::ToJSVal(rq, &pdepth, size1);
		Script::ToJSVal(rq, &pheight, height);
		JS_SetProperty(rq.cx, obj, "type", ptype);
		JS_SetProperty(rq.cx, obj, "width", pwidth);
		JS_SetProperty(rq.cx, obj, "depth", pdepth);
		JS_SetProperty(rq.cx, obj, "height", pheight);
	}

	return JS::ObjectValue(*obj);
}

BEGIN_INTERFACE_WRAPPER(Footprint)
DEFINE_INTERFACE_METHOD("PickSpawnPoint", ICmpFootprint, PickSpawnPoint)
DEFINE_INTERFACE_METHOD("PickSpawnPointBothPass", ICmpFootprint, PickSpawnPointBothPass)
DEFINE_INTERFACE_METHOD("GetShape", ICmpFootprint, GetShape_wrapper)
END_INTERFACE_WRAPPER(Footprint)
