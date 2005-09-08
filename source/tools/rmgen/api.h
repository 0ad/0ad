#ifndef __API_H__
#define __API_H__

// Function specs (registered by rmgen.cpp)
extern JSFunctionSpec globalFunctions[];

// JS API implementation

// Map creation functions
JSBool init(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval);
JSBool initFromScenario(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval);

// Utility functions
JSBool error(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval);
JSBool print(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval);
JSBool randInt(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval);
JSBool randFloat(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval);

// Low-level access to map data
JSBool getTexture(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval);
JSBool setTexture(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval);
JSBool getHeight(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval);
JSBool setHeight(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval);
JSBool getMapSize(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval);

// Low-level placement functions
JSBool placeTerrain(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval);
JSBool placeObject(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval);

// Medium-level placement functions (high level JS API sits on top of these)
JSBool createArea(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval);
JSBool createObjectGroup(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval);

// Tile class functions
JSBool createTileClass(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval);

#endif