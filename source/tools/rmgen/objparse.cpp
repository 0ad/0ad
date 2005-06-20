#include "stdafx.h"
#include "objparse.h"
#include "rmgen.h"
#include "simpleconstraints.h"
#include "simplepainters.h"
#include "rectplacer.h"
#include "layeredpainter.h"
#include "clumpplacer.h"

using namespace std;

int GetType(JSContext* cx, jsval val) {
	int ret;
	if(!GetIntField(cx, val, "TYPE", ret)) return 0;
	return ret;
}

bool GetIntField(JSContext* cx, jsval obj, const char* name, int& ret) {
	jsval val;
	if(!GetJsvalField(cx, obj, name, val)) return false;
	if(!JSVAL_IS_INT(val)) return false;
	ret = JSVAL_TO_INT(val);
	return true;
}

bool GetBoolField(JSContext* cx, jsval obj, const char* name, int& ret) {
	jsval val;
	if(!GetJsvalField(cx, obj, name, val)) return false;
	if(!JSVAL_IS_BOOLEAN(val)) return false;
	ret = JSVAL_TO_BOOLEAN(val);
	return true;
}

bool GetFloatField(JSContext* cx, jsval obj, const char* name, float& ret) {
	jsval val;
	if(!GetJsvalField(cx, obj, name, val)) return false;
	if(!JSVAL_IS_NUMBER(val)) return false;
	jsdouble jsdbl;
	JS_ValueToNumber(cx, val, &jsdbl);
	ret = jsdbl;
	return true;
}

bool GetStringField(JSContext* cx, jsval obj, const char* name, std::string& ret) {
	jsval val;
	if(!GetJsvalField(cx, obj, name, val)) return false;
	if(!JSVAL_IS_STRING(val)) return false;
	ret = JS_GetStringBytes(JSVAL_TO_STRING(val));
	return true;
}

bool GetArrayField(JSContext* cx, jsval obj, const char* name, std::vector<jsval>& ret) {
	ret.clear();
	jsval val;
	if(!GetJsvalField(cx, obj, name, val)) return false;
	if(!ParseArray(cx, val, ret)) return false;
	return true;
}

bool GetJsvalField(JSContext* cx, jsval obj, const char* name, jsval& ret) {
	if(!JSVAL_IS_OBJECT(obj)) return false;
	JSObject* fieldObj = JSVAL_TO_OBJECT(obj);
	return JS_GetProperty(cx, fieldObj, name, &ret);
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
	vector<jsval> array;
	jsval jsv, jsv2;
	Terrain* terrain = 0;
	float elevation;
	vector<Terrain*> terrains;
	vector<int> widths;

	if(ParseArray(cx, val, array)) {
		// MultiPainter is encoded as an array of painters
		vector<AreaPainter*> painters(array.size());
		for(int i=0; i<array.size(); i++) {
			painters[i] = ParsePainter(cx, array[i]);
			if(painters[i]==0) return 0;
		}
		return new MultiPainter(painters);
	}

	switch(GetType(cx, val)) {
		case TYPE_TERRAINPAINTER:
			if(!GetJsvalField(cx, val, "terrain", jsv)) return 0;
			terrain = ParseTerrain(cx, jsv);
			if(terrain==0) return 0;
			return new TerrainPainter(terrain);

		case TYPE_ELEVATIONPAINTER:
			if(!GetFloatField(cx, val, "elevation", elevation)) return 0;
			return new ElevationPainter(elevation);

		case TYPE_LAYEREDPAINTER:
			if(!GetJsvalField(cx, val, "widths", jsv)) return 0;
			if(!GetJsvalField(cx, val, "terrains", jsv2)) return 0;
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
	jsval jsv;
	int x, y, x1, y1, x2, y2, num, maxFail;
	float size, coherence, smoothness;

	switch(GetType(cx, val)) {
		case TYPE_RECTPLACER:
			if(!GetIntField(cx, val, "x1", x1)) return 0;
			if(!GetIntField(cx, val, "y1", y1)) return 0;
			if(!GetIntField(cx, val, "x2", x2)) return 0;
			if(!GetIntField(cx, val, "y2", y2)) return 0;
			return new RectPlacer(x1, y1, x2, y2);

		case TYPE_CLUMPPLACER:
			if(!GetFloatField(cx, val, "size", size)) return 0;
			if(!GetFloatField(cx, val, "coherence", coherence)) return 0;
			if(!GetFloatField(cx, val, "smoothness", smoothness)) return 0;
			if(!GetIntField(cx, val, "x", x)) return 0;
			if(!GetIntField(cx, val, "y", y)) return 0;
			return new ClumpPlacer(size, coherence, smoothness, x, y);

		default:
			return 0;
	}
}

Constraint* ParseConstraint(JSContext* cx, jsval val) {
	vector<jsval> array;
	int areaId;
	string texture;
	jsval jsv, jsv2;

	if(JSVAL_IS_NULL(val)) {
		// convenience way of specifying a NullConstraint
		return new NullConstraint();
	}

	if(ParseArray(cx, val, array)) {
		// AndConstraint is encoded as an array of constraints
		vector<Constraint*> constraints(array.size());
		for(int i=0; i<array.size(); i++) {
			constraints[i] = ParseConstraint(cx, array[i]);
			if(constraints[i]==0) return 0;
		}
		return new AndConstraint(constraints);
	}

	switch(GetType(cx, val)) {
		case TYPE_NULLCONSTRAINT:
			return new NullConstraint();

		case TYPE_AVOIDAREACONSTRAINT:
			if(!GetIntField(cx, val, "area", areaId)) return 0;
			if(areaId <= 0 || areaId > theMap->areas.size()) return 0;
			return new AvoidAreaConstraint(theMap->areas[areaId-1]);

		case TYPE_AVOIDTEXTURECONSTRAINT:
			if(!GetStringField(cx, val, "texture", texture)) return 0;
			return new AvoidTextureConstraint(theMap->getId(texture));

		default:
			return 0;
	}
}

Terrain* ParseTerrain(JSContext* cx, jsval val) {
	vector<jsval> array;

	if(JSVAL_IS_STRING(val)) {
		// simple terrains are just encoded as strings
		string str = JS_GetStringBytes(JS_ValueToString(cx, val));
		return SimpleTerrain::parse(str);
	}

	if(ParseArray(cx, val, array)) {
		// RandomTerrain is encoded as an array of terrains
		vector<Terrain*> terrains(array.size());
		for(int i=0; i<array.size(); i++) {
			terrains[i] = ParseTerrain(cx, array[i]);
			if(terrains[i]==0) return 0;
		}
		return new RandomTerrain(terrains);
	}

	// so far these are the only ways of specifying a terrain
	return 0;
}