#include "stdafx.h"
#include "objparse.h"
#include "rmgen.h"
#include "simpleconstraints.h"
#include "simplepainters.h"
#include "rectplacer.h"
#include "layeredpainter.h"
#include "clumpplacer.h"

using namespace std;

bool GetRaw(JSContext* cx, jsval val, JSObject** retObj, int* retType) {
	if(!JSVAL_IS_OBJECT(val)) return false;
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
			cerr << "Internal Error: unsupported type '" << format[i] << "' for ParseFields!\n";
			Shutdown(1);
			return false;
		}
	}
	va_end(ap);
	return true;
}

bool ParseArray(JSContext* cx, jsval val, vector<jsval>& ret) {
	ret.clear();
	if(!JSVAL_IS_OBJECT(val)) return false;
	JSObject* obj = JSVAL_TO_OBJECT(val);
	if(!JS_IsArrayObject(cx, obj)) return false;
	jsuint len;
	JS_GetArrayLength(cx, obj, &len);
	for(int i=0; i<len; i++) {
		jsval rval;
		JS_GetElement(cx, obj, i, &rval);
		ret.push_back(rval);
	}
	return true;
}

AreaPainter* ParsePainter(JSContext* cx, jsval val) {
	JSObject* obj; int type;
	if(!GetRaw(cx, val, &obj, &type)) return 0;

	jsval jsv, jsv2;
	Terrain* terrain = 0;
	vector<jsval> array;
	vector<Terrain*> terrains;
	vector<int> widths;

	switch(type) {
		case TYPE_TERRAINPAINTER:
			if(!ParseFields(cx, obj, "*", &jsv)) return 0;
			terrain = ParseTerrain(cx, jsv);
			if(terrain==0) return 0;
			return new TerrainPainter(terrain);

		case TYPE_LAYEREDPAINTER:
			if(!ParseFields(cx, obj, "**", &jsv, &jsv2)) return 0;
			if(!ParseArray(cx, jsv, array)) return 0;
			for(int i=0; i<array.size(); i++) {
				if(!JSVAL_IS_INT(array[i])) return 0;
				widths.push_back(JSVAL_TO_INT(array[i]));
			}
			if(!ParseArray(cx, jsv2, array)) return 0;
			for(int i=0; i<array.size(); i++) {
				terrain = ParseTerrain(cx, array[i]);
				if(terrain==0) return 0;
				terrains.push_back(terrain);
			}
			if(terrains.size() != 1+widths.size()) return 0;
			return new LayeredPainter(terrains, widths);

		default:
			return 0;
	}
}

AreaPlacer* ParsePlacer(JSContext* cx, jsval val) {
	JSObject* obj; int type;
	if(!GetRaw(cx, val, &obj, &type)) return 0;

	jsval jsv;
	int x, y, x1, y1, x2, y2, num, maxFail;
	float size, coherence, smoothness;

	switch(type) {
		case TYPE_RECTPLACER:
			if(!ParseFields(cx, obj, "iiii", &x1, &y1, &x2, &y2)) return 0;
			return new RectPlacer(x1, y1, x2, y2);

		case TYPE_CLUMPPLACER:
			if(!ParseFields(cx, obj, "nnnii", &size, &coherence, &smoothness, &x, &y)) return 0;
			return new ClumpPlacer(size, coherence, smoothness, x, y);

		default:
			return 0;
	}
}

Constraint* ParseConstraint(JSContext* cx, jsval val) {
	if(JSVAL_IS_NULL(val)) return new NullConstraint();

	JSObject* obj; int type;
	if(!GetRaw(cx, val, &obj, &type)) return 0;

	int areaId;
	string texture;
	jsval jsv, jsv2;
	Constraint* c1, *c2;

	switch(type) {
		case TYPE_NULLCONSTRAINT:
			if(!ParseFields(cx, obj, "")) return 0;
			return new NullConstraint();

		case TYPE_AVOIDAREACONSTRAINT:
			if(!ParseFields(cx, obj, "i", &areaId)) return 0;
			if(areaId <= 0 || areaId > theMap->areas.size()) return 0;
			return new AvoidAreaConstraint(theMap->areas[areaId-1]);

		case TYPE_AVOIDTERRAINCONSTRAINT:
			if(!ParseFields(cx, obj, "s", &texture)) return 0;
			return new AvoidTerrainConstraint(theMap->getId(texture));

		case TYPE_ANDCONSTRAINT:
			if(!ParseFields(cx, obj, "**", &jsv, &jsv2)) return 0;
			if(!(c1 = ParseConstraint(cx, jsv))) return 0;
			if(!(c2 = ParseConstraint(cx, jsv2))) return 0;
			return new AndConstraint(c1, c2);

		default:
			return 0;
	}
}

Terrain* ParseTerrain(JSContext* cx, jsval val) {
	if(JSVAL_IS_STRING(val)) {
		// simple terrains are just encoded as strings
		string str = JS_GetStringBytes(JS_ValueToString(cx, val));
		return SimpleTerrain::parse(str);
	}
	else {
		// complex terrain type
		JSObject* obj; int type;
		if(!GetRaw(cx, val, &obj, &type)) return 0;

		jsval jsv;
		Terrain* terrain = 0;
		vector<jsval> array;
		vector<Terrain*> terrains;

		switch(type) {
			case TYPE_RANDOMTERRAIN:
				if(!ParseFields(cx, obj, "*", &jsv)) return 0;
				if(!ParseArray(cx, jsv, array)) return 0;
				for(int i=0; i<array.size(); i++) {
					terrain = ParseTerrain(cx, array[i]);
					if(terrain==0) return 0;
					terrains.push_back(terrain);
				}
				return new RandomTerrain(terrains);

			default:
				return 0;
		}
		return 0;
	}
}