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

#include "scriptinterface/ScriptConversions.h"

#include "graphics/Color.h"
#include "maths/Fixed.h"
#include "maths/FixedVector2D.h"
#include "maths/FixedVector3D.h"
#include "maths/Rect.h"
#include "ps/CLogger.h"
#include "ps/utf16string.h"
#include "simulation2/helpers/CinemaPath.h"
#include "simulation2/helpers/Grid.h"
#include "simulation2/system/IComponent.h"
#include "simulation2/system/ParamNode.h"

#define FAIL(msg) STMT(LOGERROR(msg); return false)
#define FAIL_VOID(msg) STMT(ScriptException::Raise(rq, msg); return)

template<> void ScriptInterface::ToJSVal<IComponent*>(const ScriptRequest& rq,  JS::MutableHandleValue ret, IComponent* const& val)
{
	if (val == NULL)
	{
		ret.setNull();
		return;
	}

	// If this is a scripted component, just return the JS object directly
	JS::RootedValue instance(rq.cx, val->GetJSInstance());
	if (!instance.isNull())
	{
		ret.set(instance);
		return;
	}

	// Otherwise we need to construct a wrapper object
	// (TODO: cache wrapper objects?)
	JS::RootedObject obj(rq.cx);
	if (!val->NewJSObject(*ScriptInterface::GetScriptInterfaceAndCBData(rq.cx)->pScriptInterface, &obj))
	{
		// Report as an error, since scripts really shouldn't try to use unscriptable interfaces
		LOGERROR("IComponent does not have a scriptable interface");
		ret.setUndefined();
		return;
	}

	JS_SetPrivate(obj, static_cast<void*>(val));
	ret.setObject(*obj);
}

template<> void ScriptInterface::ToJSVal<CParamNode>(const ScriptRequest& rq,  JS::MutableHandleValue ret, CParamNode const& val)
{
	val.ToJSVal(rq, true, ret);

	// Prevent modifications to the object, so that it's safe to share between
	// components and to reconstruct on deserialization
	if (ret.isObject())
	{
		JS::RootedObject obj(rq.cx, &ret.toObject());
		JS_DeepFreezeObject(rq.cx, obj);
	}
}

template<> void ScriptInterface::ToJSVal<const CParamNode*>(const ScriptRequest& rq,  JS::MutableHandleValue ret, const CParamNode* const& val)
{
	if (val)
		ToJSVal(rq, ret, *val);
	else
		ret.setUndefined();
}

template<> bool ScriptInterface::FromJSVal<CColor>(const ScriptRequest& rq,  JS::HandleValue v, CColor& out)
{
	if (!v.isObject())
		FAIL("CColor has to be an object");

	JS::RootedObject obj(rq.cx, &v.toObject());

	JS::RootedValue r(rq.cx);
	JS::RootedValue g(rq.cx);
	JS::RootedValue b(rq.cx);
	JS::RootedValue a(rq.cx);
	if (!JS_GetProperty(rq.cx, obj, "r", &r) || !FromJSVal(rq, r, out.r))
		FAIL("Failed to get property CColor.r");
	if (!JS_GetProperty(rq.cx, obj, "g", &g) || !FromJSVal(rq, g, out.g))
		FAIL("Failed to get property CColor.g");
	if (!JS_GetProperty(rq.cx, obj, "b", &b) || !FromJSVal(rq, b, out.b))
		FAIL("Failed to get property CColor.b");
	if (!JS_GetProperty(rq.cx, obj, "a", &a) || !FromJSVal(rq, a, out.a))
		FAIL("Failed to get property CColor.a");

	return true;
}

template<> void ScriptInterface::ToJSVal<CColor>(const ScriptRequest& rq,  JS::MutableHandleValue ret, CColor const& val)
{
	CreateObject(
		rq,
		ret,
		"r", val.r,
		"g", val.g,
		"b", val.b,
		"a", val.a);
}

template<> bool ScriptInterface::FromJSVal<fixed>(const ScriptRequest& rq,  JS::HandleValue v, fixed& out)
{
	double ret;
	if (!JS::ToNumber(rq.cx, v, &ret))
		return false;
	out = fixed::FromDouble(ret);
	// double can precisely represent the full range of fixed, so this is a non-lossy conversion

	return true;
}

template<> void ScriptInterface::ToJSVal<fixed>(const ScriptRequest& UNUSED(rq), JS::MutableHandleValue ret, const fixed& val)
{
	ret.set(JS::NumberValue(val.ToDouble()));
}

template<> bool ScriptInterface::FromJSVal<CFixedVector3D>(const ScriptRequest& rq,  JS::HandleValue v, CFixedVector3D& out)
{
	if (!v.isObject())
		return false; // TODO: report type error

	JS::RootedObject obj(rq.cx, &v.toObject());
	JS::RootedValue p(rq.cx);

	if (!JS_GetProperty(rq.cx, obj, "x", &p)) return false; // TODO: report type errors
	if (!FromJSVal(rq, p, out.X)) return false;

	if (!JS_GetProperty(rq.cx, obj, "y", &p)) return false;
	if (!FromJSVal(rq, p, out.Y)) return false;

	if (!JS_GetProperty(rq.cx, obj, "z", &p)) return false;
	if (!FromJSVal(rq, p, out.Z)) return false;

	return true;
}

template<> void ScriptInterface::ToJSVal<CFixedVector3D>(const ScriptRequest& rq,  JS::MutableHandleValue ret, const CFixedVector3D& val)
{
	JS::RootedObject global(rq.cx, rq.glob);
	JS::RootedValue valueVector3D(rq.cx);
	if (!ScriptInterface::GetGlobalProperty(rq, "Vector3D", &valueVector3D))
		FAIL_VOID("Failed to get Vector3D constructor");

	JS::RootedValueArray<3> args(rq.cx);
	args[0].setNumber(val.X.ToDouble());
	args[1].setNumber(val.Y.ToDouble());
	args[2].setNumber(val.Z.ToDouble());

	JS::RootedObject objVec(rq.cx);
	if (!JS::Construct(rq.cx, valueVector3D, args, &objVec))
		FAIL_VOID("Failed to construct Vector3D object");

	ret.setObject(*objVec);
}

template<> bool ScriptInterface::FromJSVal<CFixedVector2D>(const ScriptRequest& rq,  JS::HandleValue v, CFixedVector2D& out)
{
	if (!v.isObject())
		return false; // TODO: report type error
	JS::RootedObject obj(rq.cx, &v.toObject());

	JS::RootedValue p(rq.cx);

	if (!JS_GetProperty(rq.cx, obj, "x", &p)) return false; // TODO: report type errors
	if (!FromJSVal(rq, p, out.X)) return false;

	if (!JS_GetProperty(rq.cx, obj, "y", &p)) return false;
	if (!FromJSVal(rq, p, out.Y)) return false;

	return true;
}

template<> void ScriptInterface::ToJSVal<CFixedVector2D>(const ScriptRequest& rq,  JS::MutableHandleValue ret, const CFixedVector2D& val)
{
	JS::RootedObject global(rq.cx, rq.glob);
	JS::RootedValue valueVector2D(rq.cx);
	if (!ScriptInterface::GetGlobalProperty(rq, "Vector2D", &valueVector2D))
		FAIL_VOID("Failed to get Vector2D constructor");

	JS::RootedValueArray<2> args(rq.cx);
	args[0].setNumber(val.X.ToDouble());
	args[1].setNumber(val.Y.ToDouble());

	JS::RootedObject objVec(rq.cx);
	if (!JS::Construct(rq.cx, valueVector2D, args, &objVec))
		FAIL_VOID("Failed to construct Vector2D object");

	ret.setObject(*objVec);
}

template<> void ScriptInterface::ToJSVal<Grid<u8> >(const ScriptRequest& rq,  JS::MutableHandleValue ret, const Grid<u8>& val)
{
	u32 length = (u32)(val.m_W * val.m_H);
	u32 nbytes = (u32)(length * sizeof(u8));
	JS::RootedObject objArr(rq.cx, JS_NewUint8Array(rq.cx, length));
	// Copy the array data and then remove the no-GC check to allow further changes to the JS data
	{
		JS::AutoCheckCannotGC nogc;
		bool sharedMemory;
		memcpy((void*)JS_GetUint8ArrayData(objArr, &sharedMemory, nogc), val.m_Data, nbytes);
	}

	JS::RootedValue data(rq.cx, JS::ObjectValue(*objArr));
	CreateObject(
		rq,
		ret,
		"width", val.m_W,
		"height", val.m_H,
		"data", data);
}

template<> void ScriptInterface::ToJSVal<Grid<u16> >(const ScriptRequest& rq,  JS::MutableHandleValue ret, const Grid<u16>& val)
 {
	u32 length = (u32)(val.m_W * val.m_H);
	u32 nbytes = (u32)(length * sizeof(u16));
	JS::RootedObject objArr(rq.cx, JS_NewUint16Array(rq.cx, length));
	// Copy the array data and then remove the no-GC check to allow further changes to the JS data
	{
		JS::AutoCheckCannotGC nogc;
		bool sharedMemory;
		memcpy((void*)JS_GetUint16ArrayData(objArr, &sharedMemory, nogc), val.m_Data, nbytes);
	}

	JS::RootedValue data(rq.cx, JS::ObjectValue(*objArr));
	CreateObject(
		rq,
		ret,
		"width", val.m_W,
		"height", val.m_H,
		"data", data);
}

template<> bool ScriptInterface::FromJSVal<TNSpline>(const ScriptRequest& rq,  JS::HandleValue v, TNSpline& out)
{
	if (!v.isObject())
		FAIL("Argument must be an object");

	JS::RootedObject obj(rq.cx, &v.toObject());
	bool isArray;
	if (!JS::IsArrayObject(rq.cx, obj, &isArray) || !isArray)
		FAIL("Argument must be an array");

	u32 numberOfNodes = 0;
	if (!JS::GetArrayLength(rq.cx, obj, &numberOfNodes))
		FAIL("Failed to get array length");

	for (u32 i = 0; i < numberOfNodes; ++i)
	{
		JS::RootedValue node(rq.cx);
		if (!JS_GetElement(rq.cx, obj, i, &node))
			FAIL("Failed to read array element");

		fixed deltaTime;
		if (!FromJSProperty(rq, node, "deltaTime", deltaTime))
			FAIL("Failed to read Spline.deltaTime property");

		CFixedVector3D position;
		if (!FromJSProperty(rq, node, "position", position))
			FAIL("Failed to read Spline.position property");

		out.AddNode(position, CFixedVector3D(), deltaTime);
	}

	if (out.GetAllNodes().empty())
		FAIL("Spline must contain at least one node");

	return true;
}

template<> bool ScriptInterface::FromJSVal<CCinemaPath>(const ScriptRequest& rq,  JS::HandleValue v, CCinemaPath& out)
{
	if (!v.isObject())
		FAIL("Argument must be an object");

	JS::RootedObject obj(rq.cx, &v.toObject());

	CCinemaData pathData;
	TNSpline positionSpline, targetSpline;

	if (!FromJSProperty(rq, v, "name", pathData.m_Name))
		FAIL("Failed to get CCinemaPath.name property");

	if (!FromJSProperty(rq, v, "orientation", pathData.m_Orientation))
		FAIL("Failed to get CCinemaPath.orientation property");

	if (!FromJSProperty(rq, v, "positionNodes", positionSpline))
		FAIL("Failed to get CCinemaPath.positionNodes property");

	if (pathData.m_Orientation == L"target" && !FromJSProperty(rq, v, "targetNodes", targetSpline))
		FAIL("Failed to get CCinemaPath.targetNodes property");

	// Other properties are not necessary to be defined
	if (!FromJSProperty(rq, v, "timescale", pathData.m_Timescale))
		pathData.m_Timescale = fixed::FromInt(1);

	if (!FromJSProperty(rq, v, "mode", pathData.m_Mode))
		pathData.m_Mode = L"ease_inout";

	if (!FromJSProperty(rq, v, "style", pathData.m_Style))
		pathData.m_Style = L"default";

	out = CCinemaPath(pathData, positionSpline, targetSpline);

	return true;
}

// define vectors
JSVAL_VECTOR(CFixedVector2D)

#undef FAIL
#undef FAIL_VOID
