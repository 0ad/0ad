#include "stdafx.h"
#include "objparse.h"
#include "rmgen.h"

using namespace std;

bool GetRaw(JSContext* cx, jsval val, JSObject** retObj, int* retType) {
	if(!JSVAL_IS_OBJECT(val)) return 0;
	JSObject* obj = JSVAL_TO_OBJECT(val);
	jsval ret;
	if(!JS_CallFunctionName(cx, obj, "raw", 0, 0, &ret)) {
		return false;
	}
	if(!JSVAL_IS_OBJECT(ret)) return false;
	*retObj = JSVAL_TO_OBJECT(ret);
	if(!JS_IsArrayObject(cx, *retObj)) return false;
	jsuint len;
	JS_GetArrayLength(cx, *retObj, &len);
	if(len==0) return false;
	jsval rval;
	JS_GetElement(cx, *retObj, 0, &rval);
	if(!JSVAL_IS_INT(rval)) return 0;
	*retType = JSVAL_TO_INT(rval);
	return true;
}

bool ParseFields(JSContext* cx, JSObject* array, const char* format, ...) {
	int len = strlen(format);
	jsuint arLen;
	JS_GetArrayLength(cx, array, &arLen);
	if(arLen != len+1) return false;

	va_list ap;
	va_start(ap, format);   // start at next arg after format

	for(int i=0; i<len; i++)
	{
		jsval val;
		JS_GetElement(cx, array, i+1, &val);

		if(format[i] == 'i') {
			int* r = va_arg(ap, int*);
			if(!JSVAL_IS_INT(val)) return false;
			*r = JSVAL_TO_INT(val);
		}
		else if(format[i] == 'n') {
			float* r = va_arg(ap, float*);
			if(!JSVAL_IS_NUMBER(val)) return false;
			jsdouble jsd;
			JS_ValueToNumber(cx, val, &jsd);
			*r = jsd;
		}
		else if(format[i] == 's') {
			string* r = va_arg(ap, string*);
			if(!JSVAL_IS_STRING(val)) return false;
			*r = JS_GetStringBytes(JS_ValueToString(cx, val));
		}
		else if(format[i] == '*') {
			jsval* r = va_arg(ap, jsval*);
			*r = val;
		}
		else 
		{
			cerr << "Internal Error: unsupported type '" << format[i] << "' for ParseArgs!\n";
			Shutdown(1);
			return false;
		}
	}
	va_end(ap);
	return true;
}

AreaPlacer* ParsePlacer(JSContext* cx, jsval val) {
	JSObject* obj; int type;
	if(!GetRaw(cx, val, &obj, &type)) return 0;

	int x1, y1, x2, y2;

	switch(type) {
		case TYPE_RECTPLACER:
			ParseFields(cx, obj, "iiii", &x1, &y1, &x2, &y2);
			return new RectPlacer(x1, y1, x2, y2);
		default:
			return 0;
	}
}

AreaPainter* ParsePainter(JSContext* cx, jsval val) {
	JSObject* obj; int type;
	if(!GetRaw(cx, val, &obj, &type)) return 0;

	string terrain;

	switch(type) {
		case TYPE_TERRAINPAINTER:
			ParseFields(cx, obj, "s", &terrain);
			return new TerrainPainter(terrain);
		default:
			return 0;
	}
}

Constraint* ParseConstraint(JSContext* cx, jsval val) {
	if(JSVAL_IS_NULL(val)) return new NullConstraint();

	JSObject* obj; int type;
	if(!GetRaw(cx, val, &obj, &type)) return 0;

	switch(type) {
		case TYPE_NULLCONSTRAINT:
			ParseFields(cx, obj, "");
			return new NullConstraint();
		default:
			return 0;
	}
}