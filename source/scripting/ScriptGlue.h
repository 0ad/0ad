
#ifndef _SCRIPTGLUE_H_
#define _SCRIPTGLUE_H_

#include "ScriptingHost.h"

JSBool WriteLog(JSContext * context, JSObject * globalObject, unsigned int argc, jsval *argv, jsval *rval);

JSBool writeConsole( JSContext* context, JSObject* globalObject, unsigned int argc, jsval* argv, jsval* rval );
JSBool getEntityByHandle( JSContext* context, JSObject* globalObject, unsigned int argc, jsval* argv, jsval* rval );
JSBool getEntityTemplate( JSContext* context, JSObject* globalObject, unsigned int argc, jsval* argv, jsval* rval );

extern JSFunctionSpec ScriptFunctionTable[];

#endif
