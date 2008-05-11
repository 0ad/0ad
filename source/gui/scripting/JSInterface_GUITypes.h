#include "scripting/ScriptingHost.h"

#ifndef INCLUDED_JSI_GUITYPES
#define INCLUDED_JSI_GUITYPES

#define GUISTDTYPE(x)							\
	namespace JSI_GUI##x						\
	{											\
		extern JSClass JSI_class;				\
		extern JSPropertySpec JSI_props[];		\
		extern JSFunctionSpec JSI_methods[];	\
		JSBool construct( JSContext* cx, JSObject* obj, uintN argc, jsval* argv, jsval* rval );	\
		JSBool getByName( JSContext* cx, JSObject* obj, uintN argc, jsval* argv, jsval* rval );	\
		JSBool toString( JSContext* cx, JSObject* obj, uintN argc, jsval* argv, jsval* rval );	\
	}

GUISTDTYPE(Size)
GUISTDTYPE(Color)
GUISTDTYPE(Mouse)

#undef GUISTDTYPE // avoid unnecessary pollution

namespace JSI_GUITypes
{
	void init();
}

#endif
