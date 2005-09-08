#include "stdafx.h"
#include "api.h"
#include "rmgen.h"
#include "random.h"
#include "map.h"
#include "object.h"
#include "convert.h"
#include "terrain.h"
#include "simpleconstraints.h"

using namespace std;

// Function specs

JSFunctionSpec globalFunctions[] = {
//  {name, native, args}
	{"init", init, 3},
	{"initFromScenario", initFromScenario, 2},
	{"print", print, 1},
	{"error", error, 1},
	{"getTexture", getTexture, 2},
	{"setTexture", setTexture, 3},
	{"getHeight", getHeight, 2},
	{"setHeight", setHeight, 3},
	{"getMapSize", getMapSize, 0},
	{"randInt", randInt, 1},
	{"randFloat", randFloat, 0},
	{"placeObject", placeObject, 4},
	{"createArea", createArea, 3},
	{"createObjectGroup", createObjectGroup, 3},
	{"createTileClass", createTileClass, 0},
	{0, 0, 0}
};

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
					JS_ReportError(cx, "%s: argument %d must be a string", function, i+1);
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

// Helper function to check whether map was initialized
void CheckInit(bool shouldBeInitialized, JSContext* cx, const char* function) {
	if(shouldBeInitialized) {
		if(theMap == 0) {
			JS_ReportError(cx, "%s: cannot be called before map is initialized", function);
		}
	}
	else {
		if(theMap != 0) {
			JS_ReportError(cx, "%s: map was already initialized by an earlier call", function);
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
	CheckInit(false, cx, __FUNCTION__);
	ValidateArgs("i*n", cx, argc, argv, __FUNCTION__);

	int size = JSVAL_TO_INT(argv[0]);
	Terrain* baseTerrain = ParseTerrain(cx, argv[1]);
	if(!baseTerrain) {
		JS_ReportError(cx, "init: argument 2 must be a terrain");
	}
	jsdouble baseHeight;
	JS_ValueToNumber(cx, argv[2], &baseHeight);

	theMap = new Map(size, baseTerrain, (float) baseHeight);
	return JS_TRUE;
}

JSBool initFromScenario(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
	CheckInit(false, cx, __FUNCTION__);
	ValidateArgs("si", cx, argc, argv, __FUNCTION__);

	string fileName = JS_GetStringBytes(JSVAL_TO_STRING(argv[0]));
	int loadObjectLevel = JSVAL_TO_INT(argv[1]);

	theMap = new Map(fileName, loadObjectLevel);

	return JS_TRUE;
}

JSBool getTexture(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
	CheckInit(true, cx, __FUNCTION__);
	ValidateArgs("ii", cx, argc, argv, __FUNCTION__);
	
	int x = JSVAL_TO_INT(argv[0]);
	int y = JSVAL_TO_INT(argv[1]);
	string texture = theMap->getTexture(x, y);
	*rval = NewJSString(texture);
	return JS_TRUE;
}

JSBool setTexture(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
	CheckInit(true, cx, __FUNCTION__);
	ValidateArgs("iis", cx, argc, argv, __FUNCTION__);
	
	int x = JSVAL_TO_INT(argv[0]);
	int y = JSVAL_TO_INT(argv[1]);
	char* texture = JS_GetStringBytes(JSVAL_TO_STRING(argv[2]));
	theMap->setTexture(x, y, texture);
	return JS_TRUE;
}

JSBool getHeight(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
	CheckInit(true, cx, __FUNCTION__);
	ValidateArgs("ii", cx, argc, argv, __FUNCTION__);
	
	int x = JSVAL_TO_INT(argv[0]);
	int y = JSVAL_TO_INT(argv[1]);
	jsdouble height = theMap->getHeight(x, y);
	JS_NewDoubleValue(cx, height, rval);
	return JS_TRUE;
}

JSBool setHeight(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
	CheckInit(true, cx, __FUNCTION__);
	ValidateArgs("iin", cx, argc, argv, __FUNCTION__);
	
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

JSBool placeObject(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
	CheckInit(true, cx, __FUNCTION__);
	ValidateArgs("sinnn", cx, argc, argv, __FUNCTION__);

	string type = JS_GetStringBytes(JS_ValueToString(cx, argv[0]));
	int player = JSVAL_TO_INT(argv[1]);
	jsdouble x, y, orientation;
	JS_ValueToNumber(cx, argv[2], &x);
	JS_ValueToNumber(cx, argv[3], &y);
	JS_ValueToNumber(cx, argv[4], &orientation);

	theMap->addObject(new Object(type, player, x,y, orientation));
	
	return JS_TRUE;
}

JSBool placeTerrain(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
	CheckInit(true, cx, __FUNCTION__);
	ValidateArgs("ii*", cx, argc, argv, __FUNCTION__);

	int x = JSVAL_TO_INT(argv[0]);
	int y = JSVAL_TO_INT(argv[1]);
	Terrain* terrain = ParseTerrain(cx, argv[2]);
	if(!terrain) {
		JS_ReportError(cx, "placeTerrain: invalid terrain argument");
	}
	theMap->placeTerrain(x, y, terrain);
	return JS_TRUE;
}

JSBool createArea(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval) 
{
	CheckInit(true, cx, __FUNCTION__);
	if(argc != 2 && argc != 3) {
		JS_ReportError(cx, "createArea: expected 2 or 3 arguments but got %d", argc);
	}

	AreaPlacer* placer;
	AreaPainter* painter;
	Constraint* constr;

	if(!(placer = ParseAreaPlacer(cx, argv[0]))) {
		JS_ReportError(cx, "createArea: argument 1 must be a valid area placer");
	}
	if(!(painter = ParseAreaPainter(cx, argv[1]))) {
		JS_ReportError(cx, "createArea: argument 2 must be a valid area painter");
	}
	if(argc == 3) {
		if(!(constr = ParseConstraint(cx, argv[2]))) {
			JS_ReportError(cx, "createArea: argument 3 must be a valid constraint");
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
		*rval = INT_TO_JSVAL(theMap->areas.size());
	}
	return JS_TRUE;
}

JSBool createObjectGroup(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval) 
{
	CheckInit(true, cx, __FUNCTION__);
	if(argc != 2 && argc != 3) {
		JS_ReportError(cx, "createObjectGroup: expected 1 or 2 arguments but got %d", argc);
	}

	ObjectGroupPlacer* placer;
	Constraint* constr;

	if(!(placer = ParseObjectGroupPlacer(cx, argv[0]))) {
		JS_ReportError(cx, "createObjectGroup: argument 1 must be a valid object group placer");
	}
	if(!JSVAL_IS_INT(argv[1])) {
		JS_ReportError(cx, "createObjectGroup: argument 2 must be a valid player number");
	}

	if(argc == 3) {
		if(!(constr = ParseConstraint(cx, argv[2]))) {
			JS_ReportError(cx, "createObjectGroup: argument 3 must be a valid constraint");
		}
	}
	else {
		constr = new NullConstraint();
	}

	int player = JSVAL_TO_INT(argv[1]);

	bool ret = theMap->createObjectGroup(placer, player, constr);

	delete placer;
	delete constr;

	*rval = ret;
	return JS_TRUE;
}

JSBool createTileClass(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval) 
{
	CheckInit(true, cx, __FUNCTION__);
	if(argc != 0) {
		JS_ReportError(cx, "createTileClass: expected 0 arguments but got %d", argc);
	}

	int id = theMap->createTileClass();

	*rval = INT_TO_JSVAL(id);
	return JS_TRUE;
}

JSBool getMapSize(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval) 
{
	CheckInit(true, cx, __FUNCTION__);
	if(argc != 0) {
		JS_ReportError(cx, "getMapSize: expected 0 arguments but got %d", argc);
	}

	*rval = INT_TO_JSVAL(theMap->size);
	return JS_TRUE;
}
