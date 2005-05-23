#include "stdafx.h"
#include "api.h"
#include "rmgen.h"
#include "random.h"
#include "map.h"
#include "entity.h"
#include "objparse.h"

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
	{"addEntity", addEntity, 5},
	{"createArea", createArea, 3},
	{0}
};

// Some global variables used for the API

map<Area*, int> areaToId;
vector<Area*> areas;

// Helper function to validate argument types; the types string can contain the following:
// i (integers), s (strings), n (numbers), . (anything); for example ValidateArgs("iin",...)
// would check that arguments 1 and 2 are integers while the third is a number.
void ValidateArgs(const char* types, JSContext* cx, uintN argc, jsval* argv, const char* function) {
	int num = strlen(types);
	if(argc != num) {
		JS_ReportError(cx, "%s: expected %d arguments but got %d", function, num, argc);
	}
	JSObject* obj;
	for(int i=0; i<num; i++) {
		switch(types[i]) {
			case 'i':
				if(!JSVAL_IS_INT(argv[i])) {
					JS_ReportError(cx, "%s: argument %d must be an integer", function, i+1);
				}
				break;
			case 's':
				if(!JSVAL_IS_STRING(argv[i])) {
					JS_ReportError(cx, "%s: argument %d must be an integer", function, i+1);
				}
				break;
			case 'n':
				if(!JSVAL_IS_NUMBER(argv[i])) {
					JS_ReportError(cx, "%s: argument %d must be an integer", function, i+1);
				}
				break;
			case 'a':
				if(!JSVAL_IS_OBJECT(argv[i])) {
					JS_ReportError(cx, "%s: argument %d must be an array", function, i+1);
				}
				obj = JSVAL_TO_OBJECT(argv[i]);
				if(!JS_IsArrayObject(cx, obj)) {
					JS_ReportError(cx, "%s: argument %d must be an array", function, i+1);
				}
				break;
			case '*':
				break;
			default:
				cerr << "Internal Error: Invalid type passed to ValidateArgs: " << types[i] << "." << endl;
				Shutdown(1);
		}
	}
}

// JS API implementation

JSBool print(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
	ValidateArgs("*", cx, argc, argv, __FUNCTION__);
	
	cout << JS_GetStringBytes(JS_ValueToString(cx, argv[0]));
	return JS_TRUE;
}

JSBool error(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
	ValidateArgs("*", cx, argc, argv, __FUNCTION__);
	
	JS_ReportError(cx, JS_GetStringBytes(JS_ValueToString(cx, argv[0])));
	return JS_TRUE;
}

JSBool init(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
	ValidateArgs("isn", cx, argc, argv, __FUNCTION__);
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
	ValidateArgs("ii", cx, argc, argv, __FUNCTION__);
	if(theMap == 0) {
		JS_ReportError(cx, "getTerrain: cannot be called before init()");
	}
	
	int x = JSVAL_TO_INT(argv[0]);
	int y = JSVAL_TO_INT(argv[1]);
	string terrain = theMap->getTerrain(x, y);
	*rval = NewJSString(terrain);
	return JS_TRUE;
}

JSBool setTerrain(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
	ValidateArgs("iis", cx, argc, argv, __FUNCTION__);
	if(theMap == 0) {
		JS_ReportError(cx, "setTerrain: cannot be called before init()");
	}
	
	int x = JSVAL_TO_INT(argv[0]);
	int y = JSVAL_TO_INT(argv[1]);
	char* terrain = JS_GetStringBytes(JSVAL_TO_STRING(argv[2]));
	theMap->setTerrain(x, y, terrain);
	return JS_TRUE;
}

JSBool getHeight(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
	ValidateArgs("ii", cx, argc, argv, __FUNCTION__);
	if(theMap == 0) {
		JS_ReportError(cx, "getHeight: cannot be called before init()");
	}
	
	int x = JSVAL_TO_INT(argv[0]);
	int y = JSVAL_TO_INT(argv[1]);
	jsdouble height = theMap->getHeight(x, y);
	JS_NewDoubleValue(cx, height, rval);
	return JS_TRUE;
}

JSBool setHeight(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
	ValidateArgs("iin", cx, argc, argv, __FUNCTION__);
	if(theMap == 0) {
		JS_ReportError(cx, "setHeight: cannot be called before init()");
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
	ValidateArgs("i", cx, argc, argv, __FUNCTION__);
	
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
	ValidateArgs("", cx, argc, argv, __FUNCTION__);
	
	jsdouble r = RandFloat();
	JS_NewDoubleValue(cx, r, rval);
	return JS_TRUE;
}

JSBool addEntity(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
	ValidateArgs("sinnn", cx, argc, argv, __FUNCTION__);
	if(theMap == 0) {
		JS_ReportError(cx, "addEntity: cannot be called before init()");
	}

	string type = JS_GetStringBytes(JS_ValueToString(cx, argv[0]));
	int player = JSVAL_TO_INT(argv[1]);
	jsdouble x, y, orientation;
	JS_ValueToNumber(cx, argv[2], &x);
	JS_ValueToNumber(cx, argv[3], &y);
	JS_ValueToNumber(cx, argv[5], &orientation);

	theMap->addEntity(new Entity(type, player, x,0,y, orientation));
	
	return JS_TRUE;
}

JSBool createArea(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval) 
{
	if(argc != 2 && argc != 3) {
		JS_ReportError(cx, "createArea: expected 2 or 3 arguments but got %d", argc);
	}
	if(theMap == 0) {
		JS_ReportError(cx, "createArea: cannot be called before init()");
	}

	AreaPlacer* placer;
	AreaPainter* painter;
	Constraint* constr;

	if(!(placer = ParsePlacer(cx, argv[0]))) {
		JS_ReportError(cx, "createArea: argument 1 must be an area placer");
	}
	if(!(painter = ParsePainter(cx, argv[1]))) {
		JS_ReportError(cx, "createArea: argument 2 must be an area paint");
	}
	if(argc == 3) {
		if(!(constr = ParseConstraint(cx, argv[2]))) {
			JS_ReportError(cx, "createArea: argument 3 must be a constraint");
		}
	}
	else {
		constr = new NullConstraint();
	}

	Area* area = theMap->createArea(placer, painter, constr);

	delete placer;
	delete painter;
	delete constr;

	if(!area) {
		*rval = INT_TO_JSVAL(0);
	}
	else {
		areas.push_back(area);
		int id = areas.size();
		areaToId[area] = id;
		*rval = INT_TO_JSVAL(id);
	}
	return JS_TRUE;
}