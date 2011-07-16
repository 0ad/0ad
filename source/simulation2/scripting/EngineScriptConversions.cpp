/* Copyright (C) 2011 Wildfire Games.
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
#include "scriptinterface/ScriptExtraHeaders.h" // for typed arrays

#include "maths/Fixed.h"
#include "maths/FixedVector2D.h"
#include "maths/FixedVector3D.h"
#include "ps/CLogger.h"
#include "ps/Overlay.h"
#include "ps/utf16string.h"
#include "simulation2/helpers/Grid.h"
#include "simulation2/system/IComponent.h"
#include "simulation2/system/ParamNode.h"

#define FAIL(msg) STMT(JS_ReportError(cx, msg); return false)

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

template<> jsval ScriptInterface::ToJSVal<CParamNode>(JSContext* cx, CParamNode const& val)
{
	jsval rval = val.ToJSVal(cx, true);

	// Prevent modifications to the object, so that it's safe to share between
	// components and to reconstruct on deserialization
	if (JSVAL_IS_OBJECT(rval))
		JS_DeepFreezeObject(cx, JSVAL_TO_OBJECT(rval));

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
		FAIL("jsval not an object");

	JSObject* obj = JSVAL_TO_OBJECT(v);

	jsval r, g, b, a;
	if (!JS_GetProperty(cx, obj, "r", &r) || !FromJSVal(cx, r, out.r))
		FAIL("Failed to get property CColor.r");
	if (!JS_GetProperty(cx, obj, "g", &g) || !FromJSVal(cx, g, out.g))
		FAIL("Failed to get property CColor.g");
	if (!JS_GetProperty(cx, obj, "b", &b) || !FromJSVal(cx, b, out.b))
		FAIL("Failed to get property CColor.b");
	if (!JS_GetProperty(cx, obj, "a", &a) || !FromJSVal(cx, a, out.a))
		FAIL("Failed to get property CColor.a");
	// TODO: this probably has GC bugs if a getter returns an unrooted value

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

template<> jsval ScriptInterface::ToJSVal<CFixedVector2D>(JSContext* cx, const CFixedVector2D& val)
{
	JSObject* obj = JS_NewObject(cx, NULL, NULL, NULL);
	if (!obj)
		return JSVAL_VOID;

	jsval x = ToJSVal(cx, val.X);
	jsval y = ToJSVal(cx, val.Y);

	JS_SetProperty(cx, obj, "x", &x);
	JS_SetProperty(cx, obj, "y", &y);

	return OBJECT_TO_JSVAL(obj);
}

template<> jsval ScriptInterface::ToJSVal<Grid<u16> >(JSContext* cx, const Grid<u16>& val)
{
	JSObject* obj = JS_NewObject(cx, NULL, NULL, NULL);
	if (!obj)
		return JSVAL_VOID;

	size_t len = val.m_W * val.m_H;
	JSObject *darray = js_CreateTypedArray(cx, js::TypedArray::TYPE_UINT16, len);
	if (!darray)
		return JSVAL_VOID;

	js::TypedArray *tdest = js::TypedArray::fromJSObject(darray);
	ENSURE(tdest->byteLength == len*sizeof(u16));

	memcpy(tdest->data, val.m_Data, tdest->byteLength);

	jsval w = ToJSVal(cx, val.m_W);
	jsval h = ToJSVal(cx, val.m_H);
	jsval data = OBJECT_TO_JSVAL(darray);

	JS_SetProperty(cx, obj, "width", &w);
	JS_SetProperty(cx, obj, "height", &h);
	JS_SetProperty(cx, obj, "data", &data);

	return OBJECT_TO_JSVAL(obj);
}
