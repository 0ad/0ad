#include "scripting/ScriptingHost.h"

#ifndef JSI_GUITYPES_INCLUDED
#define JSI_GUITYPES_INCLUDED

#define GUISTDTYPE(x)							\
	namespace JSI_GUI##x						\
	{											\
		extern JSClass JSI_class;				\
		extern JSPropertySpec JSI_props[];		\
		extern JSFunctionSpec JSI_methods[];	\
		JSBool construct( JSContext* cx, JSObject* obj, uint argc, jsval* argv, jsval* rval );	\
		JSBool getByName( JSContext* cx, JSObject* obj, uint argc, jsval* argv, jsval* rval );	\
		JSBool toString( JSContext* cx, JSObject* obj, uint argc, jsval* argv, jsval* rval );	\
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
