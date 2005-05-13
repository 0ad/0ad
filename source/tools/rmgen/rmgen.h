#ifndef __RMGEN_H__
#define __RMGEN_H__

extern JSRuntime *rt;
extern JSContext *cx;
extern JSObject *global;

// Utility functions
void Shutdown(int status);
char* ValToString(jsval val);
jsval NewJSString(const std::string& str);

// JS API implementation
JSBool init(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval);
JSBool print(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval);
JSBool getTerrain(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval);
JSBool setTerrain(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval);
JSBool getHeight(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval);
JSBool setHeight(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval);

#endif