#include "stdafx.h"
#include "rmgen.h"
#include "map.h"
#include "output.h"

using namespace std;

JSRuntime *rt = 0;
JSContext *cx = 0;
JSObject *global = 0;

Map* theMap = 0;

JSFunctionSpec globalFunctions[] = {
/*    name          native          args    */
    {"print",       print,          1},
    {"init",        init,           3},
    {0}
};

void ErrorReporter(JSContext *cx, const char *message, JSErrorReport *report) {
	cerr << "Error at " << report->filename << ":" << report->lineno << ":\n\t"
		<< message << endl;
	Shutdown(1);
}

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

JSBool print(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
	if(argc != 1) {
		JS_ReportError(cx, "print: expected 1 argument but got %d", argc);
		 return JS_FALSE;
	}
	
	cout << JS_GetStringBytes(JS_ValueToString(cx, argv[0]));
	return JS_TRUE;
}

JSBool init(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
	if(argc != 2) {
		JS_ReportError(cx, "init: expected 2 arguments but got %d", argc);
	}
	if(!JSVAL_IS_INT(argv[0])) {
		JS_ReportError(cx, "init: first argument must be an integer");
	}
	if(!JSVAL_IS_STRING(argv[1])) {
		JS_ReportError(cx, "init: second argument must be a string");
	}
	if(theMap != 0) {
		JS_ReportError(cx, "init: cannot be called twice");
	}

	int size = JSVAL_TO_INT(argv[0]);
	char* baseTerrain = JS_GetStringBytes(JSVAL_TO_STRING(argv[1]));

	theMap = new Map(size, baseTerrain);
	return JS_TRUE;
}

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
	JS_EvaluateScript(cx, global, script.c_str(), script.length(),
	                                  argv[1], 1, &rval);

	if(!theMap) {
		cerr << "Error:\n\tScript never called init!" << endl;
		Shutdown(1);
	}

	string outputName = argv[2];
	OutputMap(theMap, outputName);

	Shutdown(0);
}

