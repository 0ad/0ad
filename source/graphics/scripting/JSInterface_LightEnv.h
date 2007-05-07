/**
 * =========================================================================
 * File        : JSInterface_LightEnv.h
 * Project     : Pyrogenesis
 * Description : Provide the LightEnv object type for JavaScript
 * =========================================================================
 */

#ifndef INCLUDED_JSI_LIGHTENV
#define INCLUDED_JSI_LIGHTENV

#include "scripting/ScriptingHost.h"

namespace JSI_LightEnv
{
	void init();
	JSBool getLightEnv( JSContext* cx, JSObject* obj, jsval id, jsval* vp );
	JSBool setLightEnv( JSContext* cx, JSObject* obj, jsval id, jsval* vp );
}

#endif
