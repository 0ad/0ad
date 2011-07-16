/* Copyright (C) 2009 Wildfire Games.
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

#include "JSInterface_Vector3D.h"
#include "scripting/JSConversions.h"
#include "scripting/ScriptingHost.h"

JSClass JSI_Vector3D::JSI_class = {
	"Vector3D", JSCLASS_HAS_PRIVATE,
	JS_PropertyStub, JS_PropertyStub,
	JSI_Vector3D::getProperty, JSI_Vector3D::setProperty,
	JS_EnumerateStub, JS_ResolveStub,
	JS_ConvertStub, JSI_Vector3D::finalize,
	NULL, NULL, NULL, NULL 
};

JSPropertySpec JSI_Vector3D::JSI_props[] = 
{
	{ "x", JSI_Vector3D::component_x, JSPROP_ENUMERATE },
	{ "y", JSI_Vector3D::component_y, JSPROP_ENUMERATE },
	{ "z", JSI_Vector3D::component_z, JSPROP_ENUMERATE },
	{ 0 }
};

JSFunctionSpec JSI_Vector3D::JSI_methods[] = 
{
	{ "toString", JSI_Vector3D::toString, 0, 0 },
	{ 0 }
};

void JSI_Vector3D::init()
{
	g_ScriptingHost.DefineCustomObjectType(&JSI_class, JSI_Vector3D::construct, 0, JSI_props, JSI_methods, NULL, NULL);
}

JSI_Vector3D::Vector3D_Info::Vector3D_Info()
{
	owner = NULL;
	vector = new CVector3D();
}

JSI_Vector3D::Vector3D_Info::Vector3D_Info(float x, float y, float z)
{
	owner = NULL;
	vector = new CVector3D(x, y, z);
}

JSI_Vector3D::Vector3D_Info::Vector3D_Info(const CVector3D& copy)
{
	owner = NULL;
	vector = new CVector3D(copy.X, copy.Y, copy.Z);
}

JSI_Vector3D::Vector3D_Info::Vector3D_Info(CVector3D* attach, IPropertyOwner* _owner)
{
	owner = _owner;
	updateFn = NULL;
	freshenFn = NULL;
	vector = attach;
}

JSI_Vector3D::Vector3D_Info::Vector3D_Info(CVector3D* attach, IPropertyOwner* _owner, void(IPropertyOwner::*_updateFn)(void))
{
	owner = _owner;
	updateFn = _updateFn;
	freshenFn = NULL;
	vector = attach;
}

JSI_Vector3D::Vector3D_Info::Vector3D_Info(CVector3D* attach, IPropertyOwner* _owner, void(IPropertyOwner::*_updateFn)(void),
		void(IPropertyOwner::*_freshenFn)(void))
{
	owner = _owner;
	updateFn = _updateFn;
	freshenFn = _freshenFn;
	vector = attach;
}

JSI_Vector3D::Vector3D_Info::~Vector3D_Info()
{
	if (!owner)
		delete (vector);
}

JSBool JSI_Vector3D::getProperty(JSContext* cx, JSObject* obj, jsid id, jsval* vp)
{
	if (!JSID_IS_INT(id))
		return JS_TRUE;

	Vector3D_Info* vectorInfo = (Vector3D_Info*)JS_GetInstancePrivate(cx, obj, &JSI_Vector3D::JSI_class, NULL);
	if (!vectorInfo)
		return JS_FALSE;

	CVector3D* vectorData = vectorInfo->vector;

	if (vectorInfo->owner && vectorInfo->freshenFn)
		((vectorInfo->owner)->*(vectorInfo->freshenFn))();

	switch (JSID_TO_INT(id))
	{
	case component_x:
		return JS_NewNumberValue(cx, vectorData->X, vp);
	case component_y:
		return JS_NewNumberValue(cx, vectorData->Y, vp);
	case component_z:
		return JS_NewNumberValue(cx, vectorData->Z, vp);
	}

	return JS_FALSE;
}

JSBool JSI_Vector3D::setProperty(JSContext* cx, JSObject* obj, jsid id, JSBool UNUSED(strict), jsval* vp)
{
	if (!JSID_IS_INT(id))
		return JS_TRUE;

	Vector3D_Info* vectorInfo = (Vector3D_Info*)JS_GetInstancePrivate(cx, obj, &JSI_Vector3D::JSI_class, NULL);
	if (!vectorInfo)
		return JS_FALSE;

	CVector3D* vectorData = vectorInfo->vector;

	if (vectorInfo->owner && vectorInfo->freshenFn)
		((vectorInfo->owner)->*(vectorInfo->freshenFn))();

	switch (JSID_TO_INT(id))
	{
	case component_x:
		vectorData->X = ToPrimitive<float> (*vp);
		break;
	case component_y:
		vectorData->Y = ToPrimitive<float> (*vp);
		break;
	case component_z:
		vectorData->Z = ToPrimitive<float> (*vp);
		break;
	}

	if (vectorInfo->owner && vectorInfo->updateFn)
		((vectorInfo->owner)->*(vectorInfo->updateFn))();

	return JS_TRUE;
}

JSBool JSI_Vector3D::construct(JSContext* cx, uintN argc, jsval* vp)
{
	JSObject* vector = JS_NewObject(cx, &JSI_Vector3D::JSI_class, NULL, NULL);

	if (argc == 0)
	{
		JS_SetPrivate(cx, vector, new Vector3D_Info());
		JS_SET_RVAL(cx, vp, OBJECT_TO_JSVAL(vector));
		return JS_TRUE;
	}

	JSU_REQUIRE_PARAMS(3);
	try
	{
		float x = ToPrimitive<float> (JS_ARGV(cx, vp)[0]);
		float y = ToPrimitive<float> (JS_ARGV(cx, vp)[1]);
		float z = ToPrimitive<float> (JS_ARGV(cx, vp)[2]);
		JS_SetPrivate(cx, vector, new Vector3D_Info(x, y, z));
		JS_SET_RVAL(cx, vp, OBJECT_TO_JSVAL(vector));
		return JS_TRUE;
	}
	catch (PSERROR_Scripting_ConversionFailed)
	{
		// Invalid input (i.e. can't be coerced into doubles) - fail
		JS_ReportError(cx, "Invalid parameters to Vector3D constructor");
		return JS_FALSE;
	}
}

void JSI_Vector3D::finalize(JSContext* cx, JSObject* obj)
{
	delete ((Vector3D_Info*)JS_GetPrivate(cx, obj));
}

JSBool JSI_Vector3D::toString(JSContext* cx, uintN UNUSED(argc), jsval* vp)
{
	char buffer[256];
	Vector3D_Info* vectorInfo = (Vector3D_Info*)JS_GetInstancePrivate(cx, JS_THIS_OBJECT(cx, vp), &JSI_Vector3D::JSI_class, NULL);
	if (!vectorInfo)
		return JS_FALSE;

	if (vectorInfo->owner && vectorInfo->freshenFn)
		((vectorInfo->owner)->*(vectorInfo->freshenFn))();

	CVector3D* vectorData = vectorInfo->vector;
	sprintf_s(buffer, ARRAY_SIZE(buffer), "[object Vector3D: ( %f, %f, %f )]", vectorData->X, vectorData->Y, vectorData->Z);
	JS_SET_RVAL(cx, vp, STRING_TO_JSVAL(JS_NewStringCopyZ(cx, buffer)));
	return JS_TRUE;
}
