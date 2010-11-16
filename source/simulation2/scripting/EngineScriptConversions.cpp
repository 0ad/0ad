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

#include "scriptinterface/ScriptInterface.h"

#include "maths/Fixed.h"
#include "maths/FixedVector2D.h"
#include "maths/FixedVector3D.h"
#include "ps/CLogger.h"
#include "ps/Overlay.h"
#include "ps/utf16string.h"
#include "simulation2/system/IComponent.h"
#include "simulation2/system/ParamNode.h"

#include "js/jsapi.h"

template<> jsval ScriptInterface::ToJSVal<IComponent*>(JSContext* cx, IComponent* const& val)
{
	if (val == NULL)
		return JSVAL_NULL;

	// If this is a scripted component, just return the JS object directly
	jsval instance = val->GetJSInstance();
	if (!JSVAL_IS_NULL(instance))
		return instance;

	// Otherwise we need to construct a wrapper object
	// (TODO: cache wrapper objects?)
	JSClass* cls = val->GetJSClass();
	if (!cls)
	{
		// Report as an error, since scripts really shouldn't try to use unscriptable interfaces
		LOGERROR(L"IComponent does not have a scriptable interface");
		return JSVAL_VOID;
	}

	JSObject* obj = JS_NewObject(cx, cls, NULL, NULL);
	if (!obj)
	{
		LOGERROR(L"Failed to construct IComponent script object");
		return JSVAL_VOID;
	}
	JS_SetPrivate(cx, obj, static_cast<void*>(val));

	return OBJECT_TO_JSVAL(obj);
}

static jsval ConvertCParamNode(JSContext* cx, CParamNode const& val)
{
	const std::map<std::string, CParamNode>& children = val.GetChildren();
	if (children.empty())
	{
		// Empty node - map to undefined
		if (val.ToString().empty())
			return JSVAL_VOID;

		// Just a string
		utf16string text(val.ToString().begin(), val.ToString().end());
		JSString* str = JS_InternUCStringN(cx, reinterpret_cast<const jschar*>(text.data()), text.length());
		if (str)
			return STRING_TO_JSVAL(str);
		// TODO: report error
		return JSVAL_VOID;
	}

	// Got child nodes - convert this node into a hash-table-style object:

	JSObject* obj = JS_NewObject(cx, NULL, NULL, NULL);
	if (!obj)
		return JSVAL_VOID; // TODO: report error
	CScriptValRooted objRoot(cx, OBJECT_TO_JSVAL(obj));

	for (std::map<std::string, CParamNode>::const_iterator it = children.begin(); it != children.end(); ++it)
	{
		jsval childVal = ConvertCParamNode(cx, it->second);
		if (!JS_SetProperty(cx, obj, it->first.c_str(), &childVal))
			return JSVAL_VOID; // TODO: report error
	}

	// If the node has a string too, add that as an extra property
	if (!val.ToString().empty())
	{
		utf16string text(val.ToString().begin(), val.ToString().end());
		JSString* str = JS_InternUCStringN(cx, reinterpret_cast<const jschar*>(text.data()), text.length());
		if (!str)
			return JSVAL_VOID; // TODO: report error
		jsval childVal = STRING_TO_JSVAL(str);
		if (!JS_SetProperty(cx, obj, "_string", &childVal))
			return JSVAL_VOID; // TODO: report error
	}

	// Prevent modifications to the object, so that it's safe to share between
	// components and to reconstruct on deserialization
	//JS_SealObject(cx, obj, JS_TRUE);
	// TODO: need to re-enable this when it works safely (e.g. it doesn't seal the
	// global object too (via the parent chain))

	return OBJECT_TO_JSVAL(obj);
}

template<> jsval ScriptInterface::ToJSVal<CParamNode>(JSContext* cx, CParamNode const& val)
{
	// If there's a cached value, then return it
	if (!JSVAL_IS_VOID(val.GetScriptVal().get()))
		return val.GetScriptVal().get();

	jsval rval = ConvertCParamNode(cx, val);
	val.SetScriptVal(CScriptValRooted(cx, rval));
	return rval;
}

template<> jsval ScriptInterface::ToJSVal<const CParamNode*>(JSContext* cx, const CParamNode* const& val)
{
	if (val)
		return ToJSVal(cx, *val);
	else
		return JSVAL_VOID;
}

template<> bool ScriptInterface::FromJSVal<CColor>(JSContext* cx, jsval v, CColor& out)
{
	if (!JSVAL_IS_OBJECT(v))
		return false; // TODO: report type error
	JSObject* obj = JSVAL_TO_OBJECT(v);

	jsval r, g, b, a;
	if (!JS_GetProperty(cx, obj, "r", &r)) return false; // TODO: report type errors
	if (!JS_GetProperty(cx, obj, "g", &g)) return false;
	if (!JS_GetProperty(cx, obj, "b", &b)) return false;
	if (!JS_GetProperty(cx, obj, "a", &a)) return false;
	// TODO: this probably has GC bugs if a getter returns an unrooted value

	if (!FromJSVal(cx, r, out.r)) return false;
	if (!FromJSVal(cx, g, out.g)) return false;
	if (!FromJSVal(cx, b, out.b)) return false;
	if (!FromJSVal(cx, a, out.a)) return false;

	return true;
}

template<> jsval ScriptInterface::ToJSVal<CColor>(JSContext* cx, CColor const& val)
{
	JSObject* obj = JS_NewObject(cx, NULL, NULL, NULL);
	if (!obj)
		return JSVAL_VOID;

	jsval r = ToJSVal(cx, val.r);
	jsval g = ToJSVal(cx, val.g);
	jsval b = ToJSVal(cx, val.b);
	jsval a = ToJSVal(cx, val.a);

	JS_SetProperty(cx, obj, "r", &r);
	JS_SetProperty(cx, obj, "g", &g);
	JS_SetProperty(cx, obj, "b", &b);
	JS_SetProperty(cx, obj, "a", &a);

	return OBJECT_TO_JSVAL(obj);
}

template<> bool ScriptInterface::FromJSVal<fixed>(JSContext* cx, jsval v, fixed& out)
{
	jsdouble ret;
	if (!JS_ValueToNumber(cx, v, &ret))
		return false;
	out = fixed::FromDouble(ret);
	// double can precisely represent the full range of fixed, so this is a non-lossy conversion

	return true;
}

template<> jsval ScriptInterface::ToJSVal<fixed>(JSContext* cx, const fixed& val)
{
	jsval rval = JSVAL_VOID;
	JS_NewNumberValue(cx, val.ToDouble(), &rval); // ignore return value
	return rval;
}

template<> bool ScriptInterface::FromJSVal<CFixedVector3D>(JSContext* cx, jsval v, CFixedVector3D& out)
{
	if (!JSVAL_IS_OBJECT(v))
		return false; // TODO: report type error
	JSObject* obj = JSVAL_TO_OBJECT(v);

	jsval p;

	if (!JS_GetProperty(cx, obj, "x", &p)) return false; // TODO: report type errors
	if (!FromJSVal(cx, p, out.X)) return false;

	if (!JS_GetProperty(cx, obj, "y", &p)) return false;
	if (!FromJSVal(cx, p, out.Y)) return false;

	if (!JS_GetProperty(cx, obj, "z", &p)) return false;
	if (!FromJSVal(cx, p, out.Z)) return false;

	return true;
}

template<> jsval ScriptInterface::ToJSVal<CFixedVector3D>(JSContext* cx, const CFixedVector3D& val)
{
	JSObject* obj = JS_NewObject(cx, NULL, NULL, NULL);
	if (!obj)
		return JSVAL_VOID;

	jsval x = ToJSVal(cx, val.X);
	jsval y = ToJSVal(cx, val.Y);
	jsval z = ToJSVal(cx, val.Z);

	JS_SetProperty(cx, obj, "x", &x);
	JS_SetProperty(cx, obj, "y", &y);
	JS_SetProperty(cx, obj, "z", &z);

	return OBJECT_TO_JSVAL(obj);
}

template<> bool ScriptInterface::FromJSVal<CFixedVector2D>(JSContext* cx, jsval v, CFixedVector2D& out)
{
	if (!JSVAL_IS_OBJECT(v))
		return false; // TODO: report type error
	JSObject* obj = JSVAL_TO_OBJECT(v);

	jsval p;

	if (!JS_GetProperty(cx, obj, "x", &p)) return false; // TODO: report type errors
	if (!FromJSVal(cx, p, out.X)) return false;

	if (!JS_GetProperty(cx, obj, "y", &p)) return false;
	if (!FromJSVal(cx, p, out.Y)) return false;

	return true;
}
