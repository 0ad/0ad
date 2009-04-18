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

/**
 * =========================================================================
 * File        : JSInterface_LightEnv.cpp
 * Project     : Pyrogenesis
 * Description : Provide the LightEnv object type for JavaScript
 * =========================================================================
 */

#include "precompiled.h"

#include "maths/scripting/JSInterface_Vector3D.h"

#include "graphics/scripting/JSInterface_LightEnv.h"
#include "graphics/LightEnv.h"

#include "ps/World.h"

#include "scripting/JSConversions.h"


namespace JSI_LightEnv {

namespace {

extern JSClass JSI_class;

/**
 * This enumeration is used to index properties with the JavaScript implementation.
 */
enum
{
	lightenv_elevation,
	lightenv_rotation,
	lightenv_terrainShadowTransparency,
	lightenv_sun,
	lightenv_terrainAmbient,
	lightenv_unitsAmbient
};

///////////////////////////////////////////////////////////////////////////////////////////////
// LightEnv_Info, the private structure that holds data for individual LightEnv objects

struct LightEnv_Info : public IPropertyOwner
{
	CLightEnv* m_Data;
	bool m_EngineOwned;

	// Create a new LightEnv that will only be used by JavaScript
	LightEnv_Info()
	{
		m_Data = new CLightEnv;
		m_EngineOwned = false;
	}

	// Use the given CLightEnv from the engine. The caller must guarantee that
	// the copy object will not be deleted.
	LightEnv_Info(CLightEnv* copy)
	{
		m_Data = copy;
		m_EngineOwned = true;
	}

	~LightEnv_Info()
	{
		if (!m_EngineOwned)
			delete m_Data;
	}
};


///////////////////////////////////////////////////////////////////////////////////////////////
// Construction and finalization of LightEnvs

/**
 * construct: the LightEnv constructor has been called from JavaScript, so create a new
 * LightEnv object
 */
JSBool construct(JSContext* cx, JSObject* UNUSED(obj), uintN argc, jsval* argv, jsval* rval)
{
	JSObject* lightenv = JS_NewObject(cx, &JSI_class, NULL, NULL);

	JSU_REQUIRE_NO_PARAMS();

	JS_SetPrivate(cx, lightenv, new LightEnv_Info);
	*rval = OBJECT_TO_JSVAL(lightenv);
	return JS_TRUE;
}

/**
 * finalize: callback from the JS engine to indicate we should free our private data
 */
void finalize(JSContext* cx, JSObject* obj)
{
	delete (LightEnv_Info*)JS_GetPrivate(cx, obj);
}


///////////////////////////////////////////////////////////////////////////////////////////////
// Accessing properties of a LightEnv object

// Can't use ToJSVal here because we need live updates from the vectors
JSBool getVectorProperty(JSContext* cx, LightEnv_Info* lightenvInfo, CVector3D* vec, jsval* vp)
{
	JSObject* vector3d = JS_NewObject(cx, &JSI_Vector3D::JSI_class, NULL, NULL);
	JS_SetPrivate(cx, vector3d, new JSI_Vector3D::Vector3D_Info(vec, lightenvInfo));
	*vp = OBJECT_TO_JSVAL(vector3d);
	return JS_TRUE;
}


JSBool getProperty(JSContext* cx, JSObject* obj, jsval id, jsval* vp)
{
	if (!JSVAL_IS_INT(id))
		return JS_TRUE ;

	LightEnv_Info* lightenvInfo = (LightEnv_Info*)JS_GetPrivate(cx, obj);
	if (!lightenvInfo)
	{
		JS_ReportError(cx, "[LightEnv] Invalid Reference");
		return JS_TRUE;
	}

	CLightEnv* lightenv = lightenvInfo->m_Data;

	switch(ToPrimitive<int>(id)) {
	case lightenv_elevation: *vp = ToJSVal(lightenv->GetElevation()); break;
	case lightenv_rotation: *vp = ToJSVal(lightenv->GetRotation()); break;
	case lightenv_terrainShadowTransparency: *vp = ToJSVal(lightenv->GetTerrainShadowTransparency()); break;
	case lightenv_sun: return getVectorProperty(cx, lightenvInfo, &lightenv->m_SunColor, vp);
	case lightenv_terrainAmbient: return getVectorProperty(cx, lightenvInfo, &lightenv->m_TerrainAmbientColor, vp);
	case lightenv_unitsAmbient: return getVectorProperty(cx, lightenvInfo, &lightenv->m_UnitsAmbientColor, vp);
	default: break;
	}

	return JS_TRUE;
}


JSBool setProperty(JSContext* cx, JSObject* obj, jsval id, jsval* vp)
{
	if (!JSVAL_IS_INT(id))
		return( JS_TRUE );

	LightEnv_Info* lightenvInfo = (LightEnv_Info*)JS_GetPrivate(cx, obj);
	if (!lightenvInfo)
	{
		JS_ReportError(cx, "[LightEnv] Invalid reference");
		return JS_TRUE;
	}

	CLightEnv* lightenv = lightenvInfo->m_Data;

	switch(ToPrimitive<int>(id)) {
	case lightenv_elevation: lightenv->SetElevation(ToPrimitive<float>(*vp)); break;
	case lightenv_rotation: lightenv->SetRotation(ToPrimitive<float>(*vp)); break;
	case lightenv_terrainShadowTransparency: lightenv->SetTerrainShadowTransparency(ToPrimitive<float>(*vp)); break;
	case lightenv_sun: lightenv->m_SunColor = ToPrimitive<CVector3D>(*vp); break;
	case lightenv_terrainAmbient: lightenv->m_TerrainAmbientColor = ToPrimitive<CVector3D>(*vp); break;
	case lightenv_unitsAmbient: lightenv->m_UnitsAmbientColor = ToPrimitive<CVector3D>(*vp); break;
	default: break;
	}

	return JS_TRUE;
}



///////////////////////////////////////////////////////////////////////////////////////////////
// Registration of LightEnv class with JavaScript

JSClass JSI_class = {
	"LightEnv", JSCLASS_HAS_PRIVATE,
	JS_PropertyStub, JS_PropertyStub,
	getProperty, setProperty,
	JS_EnumerateStub, JS_ResolveStub,
	JS_ConvertStub, finalize,
	NULL, NULL, NULL, NULL
};

JSPropertySpec JSI_props[] =
{
	{ "elevation", lightenv_elevation, JSPROP_ENUMERATE },
	{ "rotation", lightenv_rotation, JSPROP_ENUMERATE },
	{ "terrainShadowTransparency", lightenv_terrainShadowTransparency, JSPROP_ENUMERATE },
	{ "sun", lightenv_sun, JSPROP_ENUMERATE },
	{ "terrainAmbient", lightenv_terrainAmbient, JSPROP_ENUMERATE },
	{ "unitsAmbient", lightenv_unitsAmbient, JSPROP_ENUMERATE },
	{ 0 },
};

JSFunctionSpec JSI_methods[] =
{
	{ 0 }
};

} // anonymous namespace

/**
 * init: called by GameSetup to register the LightEnv class with the JS engine.
 */
void init()
{
	g_ScriptingHost.DefineCustomObjectType( &JSI_class, construct, 0, JSI_props, JSI_methods, NULL, NULL );
}


///////////////////////////////////////////////////////////////////////////////////////////////
// Accessing the global lightenv

JSBool getLightEnv(JSContext* cx, JSObject* UNUSED(obj), jsval UNUSED(id), jsval* vp)
{
	JSObject* lightenv = JS_NewObject(cx, &JSI_class, NULL, NULL);
	JS_SetPrivate(cx, lightenv, new LightEnv_Info(&g_LightEnv));
	*vp = OBJECT_TO_JSVAL(lightenv);
	return JS_TRUE;
}

JSBool setLightEnv(JSContext* cx, JSObject* UNUSED(obj), jsval UNUSED(id), jsval* vp)
{
	JSObject* lightenv = JSVAL_TO_OBJECT(*vp);
	LightEnv_Info* lightenvInfo;

	if (!JSVAL_IS_OBJECT(*vp) || NULL == (lightenvInfo = (LightEnv_Info*)JS_GetInstancePrivate(cx, lightenv, &JSI_class, NULL)))
	{
		JS_ReportError( cx, "[LightEnv] Invalid object" );
	}
	else
	{
		g_LightEnv = *lightenvInfo->m_Data;
	}

	return JS_TRUE;
}

} // namespace JSI_LightEnv


