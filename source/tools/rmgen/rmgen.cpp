#include "stdafx.h"
#include "rmgen.h"
#include "map.h"
#include "output.h"

using namespace std;

JSRuntime *rt = 0;
JSContext *cx = 0;
JSObject *global = 0;

Map* theMap = 0;

// JS support functions

void ErrorReporter(JSContext *cx, const char *message, JSErrorReport *report) {
	cerr << "Error at " << report->filename << ":" << report->lineno << ":\n\t"
		<< message << endl;
	Shutdown(1);
}

JSFunctionSpec globalFunctions[] = {
//  {name, native, args}
	{"init", init, 3},
	{"print", print, 1},
	{"getTerrain", getTerrain, 2},
	{"setTerrain", setTerrain, 3},
	{"getHeight", getHeight, 2},
	{"setHeight", setHeight, 3},
	{0}
};

void InitJS() {
    rt = JS_NewRuntime(8L * 1024L * 1024L);
    cx = JS_NewContext(rt, 8192);

	JS_SetErrorReporter(cx, ErrorReporter);
	
	static JSClass globalClass = {
		"global",0,
		JS_PropertyStub,JS_PropertyStub,JS_PropertyStub,JS_PropertyStub,
		JS_EnumerateStub,JS_ResolveStub,JS_ConvertStub,JS_FinalizeStub
	};
    global = JS_NewObject(cx, &globalClass, NULL, NULL);
    JS_InitStandardClasses(cx, global);

	JS_DefineFunctions(cx, global, globalFunctions);
}

void Shutdown(int status) {
	JS_DestroyContext(cx);
	JS_DestroyRuntime(rt);
	JS_ShutDown();
	system("pause");
	exit(status);
}

char* ValToString(jsval val) {
	return JS_GetStringBytes(JS_ValueToString(cx, val));
}

jsval NewJSString(const string& str) {
	char* buf = (char*) JS_malloc(cx, str.length());
	memcpy(buf, str.c_str(), str.length());
	return STRING_TO_JSVAL(JS_NewString(cx, buf, str.length()));
}

// JS API implementation

JSBool print(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
	if(argc != 1) {
		JS_ReportError(cx, "print: expected 1 argument but got %d", argc);
	}
	
	cout << JS_GetStringBytes(JS_ValueToString(cx, argv[0]));
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
// Program entry point

int main(int argc, char* argv[])
{
	InitJS();

	if(argc!=3) {
		cerr << "Usage: rmgen <script> <output name without extension>" << endl;
		Shutdown(1);
	}

	FILE* scriptFile = fopen(argv[1], "r");
	if(!scriptFile) {
		cerr << "Cannot open " << scriptFile << endl;
		Shutdown(1);
	}
	string script;
	char buf[1025];
	while(fgets(buf, 1024, scriptFile)) {
		script += buf;
	}

	jsval rval;
	JSBool ok = JS_EvaluateScript(cx, global, script.c_str(), script.length(), argv[1], 1, &rval);
	if(!ok) Shutdown(1);

	if(!theMap) {
		cerr << "Error:\n\tScript never called init!" << endl;
		Shutdown(1);
	}

	string outputName = argv[2];
	OutputMap(theMap, outputName);

	Shutdown(0);
}
