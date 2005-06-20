#include "stdafx.h"
#include "rmgen.h"
#include "map.h"
#include "output.h"
#include "api.h"
#include "random.h"

using namespace std;

JSRuntime *rt = 0;
JSContext *cx = 0;
JSObject *global = 0;

Map* theMap = 0;

// JS support functions

void ErrorReporter(JSContext *cx, const char *message, JSErrorReport *report) {
	string path = string(report->filename);
	int lastSlash = -1;
	for(int i = ((int) path.size()) - 1; i >= 0; i--) {
		if(path[i]=='/' || path[i]=='\\') {
			lastSlash = i;
			break;
		}
	}
	string filename = path.substr(lastSlash+1);
	cerr << "Error at " << filename << ":" << report->lineno << ":\n\t"
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

jsval NewJSString(const string& str) {
	char* buf = (char*) JS_malloc(cx, str.length());
	memcpy(buf, str.c_str(), str.length());
	return STRING_TO_JSVAL(JS_NewString(cx, buf, str.length()));
}

void ExecuteFile(const string& fileName) {
	FILE* f = fopen(fileName.c_str(), "r");
	if(!f) {
		cerr << "Cannot open " << fileName << endl;
		Shutdown(1);
	}
	
	string code;
	char buf[1025];
	while(fgets(buf, 1024, f)) {
		code += buf;
	}

	jsval rval;
	JSBool ok = JS_EvaluateScript(cx, global, code.c_str(), code.length(), fileName.c_str(), 1, &rval);
	if(!ok) Shutdown(1);
}

// Program entry point

int main(int argc, char* argv[])
{
	const string LIBRARY_FILE = "../data/mods/official/maps/rmlibrary.js";
	const string RMS_PATH = "../data/mods/official/maps/random/";
	const string SCENARIO_PATH = "../data/mods/official/maps/scenarios/";

	clock_t start = clock();

	InitJS();

	if(argc!=3 && argc!=4) {
		cerr << "Usage: rmgen <script> <output map> [<seed>] (no file extensions)" << endl;
		Shutdown(1);
	}

	unsigned long seed;
	if(argc==4) {
		sscanf(argv[3], "%u", &seed);
	}
	else {
		seed = time(0);
	}
	SeedRand(seed);

	// Load map settings (things like game type and player info)
	ostringstream out;
	out << "const SEED=" << seed << ";\n";
	string setts = out.str();
	jsval rval;
	JSBool ok = JS_EvaluateScript(cx, global, setts.c_str(), setts.length(), 
		"map settings script", 1, &rval);
	if(!ok) Shutdown(1);

	// Load library
	ExecuteFile(LIBRARY_FILE);

	// Run the script
	ExecuteFile(RMS_PATH + argv[1] + ".js");

	if(!theMap) {
		cerr << "Error:\n\tScript never called init!" << endl;
		Shutdown(1);
	}

	string outputName = SCENARIO_PATH + argv[2];
	OutputMap(theMap, outputName);

	clock_t end = clock();

	printf("Took %0.3f seconds.\n", float(end-start) / CLOCKS_PER_SEC);

	Shutdown(0);
}
