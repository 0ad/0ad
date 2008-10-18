#include "stdafx.h"
#include "rmgen.h"
#include "map.h"
#include "output.h"
#include "api.h"
#include "scripting.h"
#include "random.h"
#include "noise.h"

using namespace std;
using namespace js;

JSRuntime *rt = 0;
JSContext *cx = 0;
JSObject *global = 0;

Map* theMap = 0;

// JS support functions

template<typename T> JSClass Class<T>::jsClass = {
	"T", JSCLASS_HAS_PRIVATE,
	JS_PropertyStub, JS_PropertyStub, Class<T>::getProperty, Class<T>::setProperty,
	JS_EnumerateStub,   JS_ResolveStub,  JS_ConvertStub, JS_FinalizeStub
	// Note: the finalize stub should really be replaced by some function which
	// deletes the object if it must be deleted; this requires that we use some
	// kind of smart handles instead of just pointers though, so we don't delete
	// objects that we actually want C++ to use (which is most objects)
};
template<typename T> JSNative Class<T>::constructor;
template<typename T> vector<JSFunctionSpec> Class<T>::methodSpecs;
template<typename T> vector<JSPropertySpec> Class<T>::propertySpecs;
template<typename T> vector<AbstractProperty*> Class<T>::properties;

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
	
	Class<Noise2D>::addMethod(Method<Noise2D, float, float, float, &Noise2D::operator()>, 2, "eval");
	Class<Noise2D>::init("Noise2D", Constructor<Noise2D, float>);
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
	const string LIBRARY_FILE = "../data/mods/public/maps/rmlibrary.js";
	const string RMS_PATH = "../data/mods/public/maps/random/";
	const string SCENARIO_PATH = "../data/mods/public/maps/scenarios/";

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
