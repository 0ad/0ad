#include "scripting/ScriptingHost.h"

#ifndef JSI_SELECTION_INCLUDED
#define JSI_SELECTION_INCLUDED

namespace JSI_Selected
{
	JSBool getProperty( JSContext* context, JSObject* objt, jsval id, jsval* vp );
	JSBool setProperty( JSContext* context, JSObject* obj, jsval id, jsval* vp );
};

namespace JSI_Selection
{
	JSBool getProperty( JSContext* context, JSObject* obj, jsval id, jsval* vp );
	JSBool setProperty( JSContext* context, JSObject* obj, jsval id, jsval* vp );
	JSBool isValidContextOrder( JSContext* context, JSObject* obj, unsigned int argc, jsval* argv, jsval* rval );
	JSBool getContextOrder( JSContext* context, JSObject* obj, jsval id, jsval* vp );
	JSBool setContextOrder( JSContext* context, JSObject* obj, jsval id, jsval* vp );
};

#endif