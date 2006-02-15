/**
 * =========================================================================
 * File        : JSInterface_LightEnv.h
 * Project     : Pyrogenesis
 * Description : Provide the LightEnv object type for JavaScript
 *
 * @author Nicolai Hähnle <nicolai@wildfiregames.com>
 * =========================================================================
 */

#ifndef JSI_LIGHTENV_INCLUDED
#define JSI_LIGHTENV_INCLUDED

#include "scripting/ScriptingHost.h"

namespace JSI_LightEnv
{
	void init();
	JSBool getLightEnv( JSContext* cx, JSObject* obj, jsval id, jsval* vp );
	JSBool setLightEnv( JSContext* cx, JSObject* obj, jsval id, jsval* vp );
};

#endif
