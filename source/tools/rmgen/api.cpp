#include "stdafx.h"
#include "api.h"
#include "rmgen.h"
#include "map.h"
#include "random.h"

using namespace std;

// Function specs

JSFunctionSpec globalFunctions[] = {
//  {name, native, args}
    {"init", init, 3},
    {"print", print, 1},
    {"error", error, 1},
    {"getTerrain", getTerrain, 2},
    {"setTerrain", setTerrain, 3},
    {"getHeight", getHeight, 2},
    {"setHeight", setHeight, 3},
    {"randInt", randInt, 1},
    {"randFloat", randFloat, 0},
    {0}
};

// JS API implementation

JSBool print(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
    if(argc != 1) {
        JS_ReportError(cx, "print: expected 1 argument but got %d", argc);
    }
    
    cout << JS_GetStringBytes(JS_ValueToString(cx, argv[0]));
    return JS_TRUE;
}

JSBool error(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
    if(argc != 1) {
        // wow, you made an error calling the error() function!
        JS_ReportError(cx, "error: expected 1 argument but got %d", argc);
    }
    
    JS_ReportError(cx, JS_GetStringBytes(JS_ValueToString(cx, argv[0])));
    return JS_TRUE;
}

JSBool init(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
    if(argc != 3) {
        JS_ReportError(cx, "init: expected 3 arguments but got %d", argc);
    }
    if(!JSVAL_IS_INT(argv[0])) {
        JS_ReportError(cx, "init: first argument must be an integer");
    }
    if(!JSVAL_IS_STRING(argv[1])) {
        JS_ReportError(cx, "init: second argument must be a string");
    }
    if(!JSVAL_IS_NUMBER(argv[2])) {
        JS_ReportError(cx, "init: third argument must be a number");
    }
    if(theMap != 0) {
        JS_ReportError(cx, "init: cannot be called twice");
    }

    int size = JSVAL_TO_INT(argv[0]);
    char* baseTerrain = JS_GetStringBytes(JSVAL_TO_STRING(argv[1]));
    jsdouble baseHeight;
    JS_ValueToNumber(cx, argv[2], &baseHeight);

    theMap = new Map(size, baseTerrain, (float) baseHeight);
    return JS_TRUE;
}

JSBool getTerrain(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
    if(argc != 2) {
        JS_ReportError(cx, "getTerrain: expected 2 arguments but got %d", argc);
    }
    if(theMap == 0) {
        JS_ReportError(cx, "getTerrain: cannot be called before init()");
    }
    if(!JSVAL_IS_INT(argv[0])) {
        JS_ReportError(cx, "getTerrain: first argument must be an integer");
    }
    if(!JSVAL_IS_INT(argv[1])) {
        JS_ReportError(cx, "getTerrain: second argument must be an integer");
    }
    
    int x = JSVAL_TO_INT(argv[0]);
    int y = JSVAL_TO_INT(argv[1]);
    string terrain = theMap->getTerrain(x, y);
    *rval = NewJSString(terrain);
    return JS_TRUE;
}

JSBool setTerrain(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
    if(argc != 3) {
        JS_ReportError(cx, "setTerrain: expected 3 arguments but got %d", argc);
    }
    if(theMap == 0) {
        JS_ReportError(cx, "setTerrain: cannot be called before init()");
    }
    if(!JSVAL_IS_INT(argv[0])) {
        JS_ReportError(cx, "setTerrain: first argument must be an integer");
    }
    if(!JSVAL_IS_INT(argv[1])) {
        JS_ReportError(cx, "setTerrain: second argument must be an integer");
    }
    if(!JSVAL_IS_STRING(argv[2])) {
        JS_ReportError(cx, "setTerrain: third argument must be a string");
    }
    
    int x = JSVAL_TO_INT(argv[0]);
    int y = JSVAL_TO_INT(argv[1]);
    char* terrain = JS_GetStringBytes(JSVAL_TO_STRING(argv[2]));
    theMap->setTerrain(x, y, terrain);
    return JS_TRUE;
}

JSBool getHeight(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
    if(argc != 2) {
        JS_ReportError(cx, "getHeight: expected 2 arguments but got %d", argc);
    }
    if(theMap == 0) {
        JS_ReportError(cx, "getHeight: cannot be called before init()");
    }
    if(!JSVAL_IS_INT(argv[0])) {
        JS_ReportError(cx, "getHeight: first argument must be an integer");
    }
    if(!JSVAL_IS_INT(argv[1])) {
        JS_ReportError(cx, "getHeight: second argument must be an integer");
    }
    
    int x = JSVAL_TO_INT(argv[0]);
    int y = JSVAL_TO_INT(argv[1]);
    jsdouble height = theMap->getHeight(x, y);
    JS_NewDoubleValue(cx, height, rval);
    return JS_TRUE;
}

JSBool setHeight(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
    if(argc != 3) {
        JS_ReportError(cx, "setHeight: expected 3 arguments but got %d", argc);
    }
    if(theMap == 0) {
        JS_ReportError(cx, "setHeight: cannot be called before init()");
    }
    if(!JSVAL_IS_INT(argv[0])) {
        JS_ReportError(cx, "setHeight: first argument must be an integer");
    }
    if(!JSVAL_IS_INT(argv[1])) {
        JS_ReportError(cx, "setHeight: second argument must be an integer");
    }
    if(!JSVAL_IS_NUMBER(argv[2])) {
        JS_ReportError(cx, "setHeight: third argument must be a number");
    }
    
    int x = JSVAL_TO_INT(argv[0]);
    int y = JSVAL_TO_INT(argv[1]);
    jsdouble height;
    JS_ValueToNumber(cx, argv[2], &height);
    theMap->setHeight(x, y, (float) height);
    return JS_TRUE;
}

JSBool randInt(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
    if(argc != 1) {
        JS_ReportError(cx, "randInt: expected 1 argument but got %d", argc);
    }
    if(!JSVAL_IS_INT(argv[0])) {
        JS_ReportError(cx, "randInt: first argument must be an integer");
    }
    
    int x = JSVAL_TO_INT(argv[0]);
    if(x<=0) {
        JS_ReportError(cx, "randInt: argument must be positive");
    }
    int r = RandInt(x);
    *rval = INT_TO_JSVAL(r);
    return JS_TRUE;
}

JSBool randFloat(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
    if(argc != 0) {
        JS_ReportError(cx, "randFloat: expected 0 arguments but got %d", argc);
    }
    
    jsdouble r = RandFloat();
    JS_NewDoubleValue(cx, r, rval);
    return JS_TRUE;
}