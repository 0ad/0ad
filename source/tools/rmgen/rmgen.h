#pragma once

extern JSRuntime *rt;
extern JSContext *cx;
extern JSObject *global;

void Shutdown(int status);

char* ValToString(jsval val);

JSBool print(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval);
JSBool init(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval);