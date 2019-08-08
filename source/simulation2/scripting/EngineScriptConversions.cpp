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

#include "scriptinterface/ScriptConversions.h"

#include "graphics/Color.h"
#include "maths/Fixed.h"
#include "maths/FixedVector2D.h"
#include "maths/FixedVector3D.h"
#include "ps/CLogger.h"
#include "ps/Shapes.h"
#include "ps/utf16string.h"
#include "simulation2/helpers/CinemaPath.h"
#include "simulation2/helpers/Grid.h"
#include "simulation2/system/IComponent.h"
#include "simulation2/system/ParamNode.h"

#define FAIL(msg) STMT(JS_ReportError(cx, msg); return false)
#define FAIL_VOID(msg) STMT(JS_ReportError(cx, msg); return)

template<> void ScriptInterface::ToJSVal<IComponent*>(JSContext* cx, JS::MutableHandleValue ret, IComponent* const& val)
{
	JSAutoRequest rq(cx);
	if (val == NULL)
	{
		ret.setNull();
		return;
	}

	// If this is a scripted component, just return the JS object directly
	JS::RootedValue instance(cx, val->GetJSInstance());
	if (!instance.isNull())
	{
		ret.set(instance);
		return;
	}

	// Otherwise we need to construct a wrapper object
	// (TODO: cache wrapper objects?)
	JS::RootedObject obj(cx);
	if (!val->NewJSObject(*ScriptInterface::GetScriptInterfaceAndCBData(cx)->pScriptInterface, &obj))
	{
		// Report as an error, since scripts really shouldn't try to use unscriptable interfaces
		LOGERROR("IComponent does not have a scriptable interface");
		ret.setUndefined();
		return;
	}

	JS_SetPrivate(obj, static_cast<void*>(val));
	ret.setObject(*obj);
}

template<> void ScriptInterface::ToJSVal<CParamNode>(JSContext* cx, JS::MutableHandleValue ret, CParamNode const& val)
{
	JSAutoRequest rq(cx);
	val.ToJSVal(cx, true, ret);

	// Prevent modifications to the object, so that it's safe to share between
	// components and to reconstruct on deserialization
	if (ret.isObject())
	{
		JS::RootedObject obj(cx, &ret.toObject());
		JS_DeepFreezeObject(cx, obj);
	}
}

template<> void ScriptInterface::ToJSVal<const CParamNode*>(JSContext* cx, JS::MutableHandleValue ret, const CParamNode* const& val)
{
	if (val)
		ToJSVal(cx, ret, *val);
	else
		ret.setUndefined();
}

template<> bool ScriptInterface::FromJSVal<CColor>(JSContext* cx, JS::HandleValue v, CColor& out)
{
	if (!v.isObject())
		FAIL("JS::HandleValue not an object");

	JSAutoRequest rq(cx);
	JS::RootedObject obj(cx, &v.toObject());

	JS::RootedValue r(cx);
	JS::RootedValue g(cx);
	JS::RootedValue b(cx);
	JS::RootedValue a(cx);
	if (!JS_GetProperty(cx, obj, "r", &r) || !FromJSVal(cx, r, out.r))
		FAIL("Failed to get property CColor.r");
	if (!JS_GetProperty(cx, obj, "g", &g) || !FromJSVal(cx, g, out.g))
		FAIL("Failed to get property CColor.g");
	if (!JS_GetProperty(cx, obj, "b", &b) || !FromJSVal(cx, b, out.b))
		FAIL("Failed to get property CColor.b");
	if (!JS_GetProperty(cx, obj, "a", &a) || !FromJSVal(cx, a, out.a))
		FAIL("Failed to get property CColor.a");

	return true;
}

template<> void ScriptInterface::ToJSVal<CColor>(JSContext* cx, JS::MutableHandleValue ret, CColor const& val)
{
	JSAutoRequest rq(cx);
	JS::RootedObject obj(cx, JS_NewPlainObject(cx));
	if (!obj)
	{
		ret.setUndefined();
		return;
	}

	JS::RootedValue r(cx);
	JS::RootedValue g(cx);
	JS::RootedValue b(cx);
	JS::RootedValue a(cx);
	ToJSVal(cx, &r, val.r);
	ToJSVal(cx, &g, val.g);
	ToJSVal(cx, &b, val.b);
	ToJSVal(cx, &a, val.a);

	JS_SetProperty(cx, obj, "r", r);
	JS_SetProperty(cx, obj, "g", g);
	JS_SetProperty(cx, obj, "b", b);
	JS_SetProperty(cx, obj, "a", a);

	ret.setObject(*obj);
}

template<> bool ScriptInterface::FromJSVal<fixed>(JSContext* cx, JS::HandleValue v, fixed& out)
{
	JSAutoRequest rq(cx);
	double ret;
	if (!JS::ToNumber(cx, v, &ret))
		return false;
	out = fixed::FromDouble(ret);
	// double can precisely represent the full range of fixed, so this is a non-lossy conversion

	return true;
}

template<> void ScriptInterface::ToJSVal<fixed>(JSContext* UNUSED(cx), JS::MutableHandleValue ret, const fixed& val)
{
	ret.set(JS::NumberValue(val.ToDouble()));
}

template<> bool ScriptInterface::FromJSVal<CFixedVector3D>(JSContext* cx, JS::HandleValue v, CFixedVector3D& out)
{
	if (!v.isObject())
		return false; // TODO: report type error

	JSAutoRequest rq(cx);
	JS::RootedObject obj(cx, &v.toObject());
	JS::RootedValue p(cx);

	if (!JS_GetProperty(cx, obj, "x", &p)) return false; // TODO: report type errors
	if (!FromJSVal(cx, p, out.X)) return false;

	if (!JS_GetProperty(cx, obj, "y", &p)) return false;
	if (!FromJSVal(cx, p, out.Y)) return false;

	if (!JS_GetProperty(cx, obj, "z", &p)) return false;
	if (!FromJSVal(cx, p, out.Z)) return false;

	return true;
}

template<> void ScriptInterface::ToJSVal<CFixedVector3D>(JSContext* cx, JS::MutableHandleValue ret, const CFixedVector3D& val)
{
	JSAutoRequest rq(cx);

 	ScriptInterface::CxPrivate* pCxPrivate = ScriptInterface::GetScriptInterfaceAndCBData(cx);
	JS::RootedObject global(cx, &pCxPrivate->pScriptInterface->GetGlobalObject().toObject());
	JS::RootedValue valueVector3D(cx);
	if (!JS_GetProperty(cx, global, "Vector3D", &valueVector3D))
		FAIL_VOID("Failed to get Vector3D constructor");

	JS::AutoValueArray<3> args(cx);
	args[0].setNumber(val.X.ToDouble());
	args[1].setNumber(val.Y.ToDouble());
	args[2].setNumber(val.Z.ToDouble());

	if (!JS::Construct(cx, valueVector3D, args, ret))
		FAIL_VOID("Failed to construct Vector3D object");
}

template<> bool ScriptInterface::FromJSVal<CFixedVector2D>(JSContext* cx, JS::HandleValue v, CFixedVector2D& out)
{
	JSAutoRequest rq(cx);
	if (!v.isObject())
		return false; // TODO: report type error
	JS::RootedObject obj(cx, &v.toObject());

	JS::RootedValue p(cx);

	if (!JS_GetProperty(cx, obj, "x", &p)) return false; // TODO: report type errors
	if (!FromJSVal(cx, p, out.X)) return false;

	if (!JS_GetProperty(cx, obj, "y", &p)) return false;
	if (!FromJSVal(cx, p, out.Y)) return false;

	return true;
}

template<> void ScriptInterface::ToJSVal<CFixedVector2D>(JSContext* cx, JS::MutableHandleValue ret, const CFixedVector2D& val)
{
	JSAutoRequest rq(cx);

 	ScriptInterface::CxPrivate* pCxPrivate = ScriptInterface::GetScriptInterfaceAndCBData(cx);
	JS::RootedObject global(cx, &pCxPrivate->pScriptInterface->GetGlobalObject().toObject());
	JS::RootedValue valueVector2D(cx);
	if (!JS_GetProperty(cx, global, "Vector2D", &valueVector2D))
		FAIL_VOID("Failed to get Vector2D constructor");

	JS::AutoValueArray<2> args(cx);
	args[0].setNumber(val.X.ToDouble());
	args[1].setNumber(val.Y.ToDouble());

	if (!JS::Construct(cx, valueVector2D, args, ret))
		FAIL_VOID("Failed to construct Vector2D object");
}

template<> void ScriptInterface::ToJSVal<Grid<u8> >(JSContext* cx, JS::MutableHandleValue ret, const Grid<u8>& val)
{
	JSAutoRequest rq(cx);
	u32 length = (u32)(val.m_W * val.m_H);
	u32 nbytes = (u32)(length * sizeof(u8));
	JS::RootedObject objArr(cx, JS_NewUint8Array(cx, length));
	// Copy the array data and then remove the no-GC check to allow further changes to the JS data
	{
		JS::AutoCheckCannotGC nogc;
		bool sharedMemory;
		memcpy((void*)JS_GetUint8ArrayData(objArr, &sharedMemory, nogc), val.m_Data, nbytes);
	}

	JS::RootedValue data(cx, JS::ObjectValue(*objArr));
	JS::RootedValue w(cx);
	JS::RootedValue h(cx);
	ScriptInterface::ToJSVal(cx, &w, val.m_W);
	ScriptInterface::ToJSVal(cx, &h, val.m_H);

	JS::RootedObject obj(cx, JS_NewPlainObject(cx));
	JS_SetProperty(cx, obj, "width", w);
	JS_SetProperty(cx, obj, "height", h);
	JS_SetProperty(cx, obj, "data", data);

	ret.setObject(*obj);
}

template<> void ScriptInterface::ToJSVal<Grid<u16> >(JSContext* cx, JS::MutableHandleValue ret, const Grid<u16>& val)
 {
	JSAutoRequest rq(cx);
	u32 length = (u32)(val.m_W * val.m_H);
	u32 nbytes = (u32)(length * sizeof(u16));
	JS::RootedObject objArr(cx, JS_NewUint16Array(cx, length));
	// Copy the array data and then remove the no-GC check to allow further changes to the JS data
	{
		JS::AutoCheckCannotGC nogc;
		bool sharedMemory;
		memcpy((void*)JS_GetUint16ArrayData(objArr, &sharedMemory, nogc), val.m_Data, nbytes);
	}

	JS::RootedValue data(cx, JS::ObjectValue(*objArr));
	JS::RootedValue w(cx);
	JS::RootedValue h(cx);
	ScriptInterface::ToJSVal(cx, &w, val.m_W);
	ScriptInterface::ToJSVal(cx, &h, val.m_H);

	JS::RootedObject obj(cx, JS_NewPlainObject(cx));
	JS_SetProperty(cx, obj, "width", w);
	JS_SetProperty(cx, obj, "height", h);
	JS_SetProperty(cx, obj, "data", data);

	ret.setObject(*obj);
}

template<> bool ScriptInterface::FromJSVal<TNSpline>(JSContext* cx, JS::HandleValue v, TNSpline& out)
{
	if (!v.isObject())
		FAIL("Argument must be an object");

	JSAutoRequest rq(cx);
	JS::RootedObject obj(cx, &v.toObject());
	bool isArray;
	if (!JS_IsArrayObject(cx, obj, &isArray) || !isArray)
		FAIL("Argument must be an array");

	u32 numberOfNodes = 0;
	if (!JS_GetArrayLength(cx, obj, &numberOfNodes))
		FAIL("Failed to get array length");

	for (u32 i = 0; i < numberOfNodes; ++i)
	{
		JS::RootedValue node(cx);
		if (!JS_GetElement(cx, obj, i, &node))
			FAIL("Failed to read array element");

		fixed deltaTime;
		if (!FromJSProperty(cx, node, "deltaTime", deltaTime))
			FAIL("Failed to read Spline.deltaTime property");

		CFixedVector3D position;
		if (!FromJSProperty(cx, node, "position", position))
			FAIL("Failed to read Spline.position property");

		out.AddNode(position, CFixedVector3D(), deltaTime);
	}

	if (out.GetAllNodes().empty())
		FAIL("Spline must contain at least one node");

	return true;
}

template<> bool ScriptInterface::FromJSVal<CCinemaPath>(JSContext* cx, JS::HandleValue v, CCinemaPath& out)
{
	if (!v.isObject())
		FAIL("Argument must be an object");

	JSAutoRequest rq(cx);
	JS::RootedObject obj(cx, &v.toObject());

	CCinemaData pathData;
	TNSpline positionSpline, targetSpline;

	if (!FromJSProperty(cx, v, "name", pathData.m_Name))
		FAIL("Failed to get CCinemaPath.name property");

	if (!FromJSProperty(cx, v, "orientation", pathData.m_Orientation))
		FAIL("Failed to get CCinemaPath.orientation property");

	if (!FromJSProperty(cx, v, "positionNodes", positionSpline))
		FAIL("Failed to get CCinemaPath.positionNodes property");

	if (pathData.m_Orientation == L"target" && !FromJSProperty(cx, v, "targetNodes", targetSpline))
		FAIL("Failed to get CCinemaPath.targetNodes property");

	// Other properties are not necessary to be defined
	if (!FromJSProperty(cx, v, "timescale", pathData.m_Timescale))
		pathData.m_Timescale = fixed::FromInt(1);

	if (!FromJSProperty(cx, v, "mode", pathData.m_Mode))
		pathData.m_Mode = L"ease_inout";

	if (!FromJSProperty(cx, v, "style", pathData.m_Style))
		pathData.m_Style = L"default";

	out = CCinemaPath(pathData, positionSpline, targetSpline);

	return true;
}

// define vectors
JSVAL_VECTOR(CFixedVector2D)

#undef FAIL
#undef FAIL_VOID
