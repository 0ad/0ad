#include "stdafx.h"
#include "convert.h"
#include "rmgen.h"
#include "simpleconstraints.h"
#include "simplepainters.h"
#include "rectplacer.h"
#include "layeredpainter.h"
#include "clumpplacer.h"
#include "smoothelevationpainter.h"
#include "simplegroup.h"

using namespace std;

bool GetTileClassField(JSContext* cx, jsval obj, const char* name, TileClass*& ret) {
	jsval val;
	if(!GetJsvalField(cx, obj, name, val)) return false;
	if(JSVAL_IS_NULL(val)) {
		ret = 0;
		return true;
	}
	if(!JSVAL_IS_INT(val)) return false;
	int id = JSVAL_TO_INT(val);
	if(id < 0 || id > theMap->tileClasses.size()) return false;
	ret = (id==0? 0 : theMap->tileClasses[id-1]);
	return true;
}

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

bool GetBoolField(JSContext* cx, jsval obj, const char* name, bool& ret) {
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

AreaPainter* ParseAreaPainter(JSContext* cx, jsval val) {
	vector<jsval> array;
	jsval jsv, jsv2;
	Terrain* terrain = 0;
	float elevation;
	int type;
	int blendRadius;
	TileClass* tileClass;
	vector<Terrain*> terrains;
	vector<int> widths;

	if(ParseArray(cx, val, array)) {
		// MultiPainter is encoded as an array of painters
		vector<AreaPainter*> painters(array.size());
		for(int i=0; i<array.size(); i++) {
			painters[i] = ParseAreaPainter(cx, array[i]);
			if(painters[i]==0) return 0;
		}
		return new MultiPainter(painters);
	}

	switch(GetType(cx, val)) {
		case TYPE_TERRAIN_PAINTER:
			if(!GetJsvalField(cx, val, "terrain", jsv)) return 0;
			terrain = ParseTerrain(cx, jsv);
			if(terrain==0) return 0;
			return new TerrainPainter(terrain);

		case TYPE_ELEVATION_PAINTER:
			if(!GetFloatField(cx, val, "elevation", elevation)) return 0;
			return new ElevationPainter(elevation);

		case TYPE_TILE_CLASS_PAINTER:
			if(!GetTileClassField(cx, val, "tileClass", tileClass)) return 0;
			return new TileClassPainter(tileClass);

		case TYPE_SMOOTH_ELEVATION_PAINTER:
			if(!GetIntField(cx, val, "type", type)) return 0;
			if(!GetFloatField(cx, val, "elevation", elevation)) return 0;
			if(!GetIntField(cx, val, "blendRadius", blendRadius)) return 0;
			return new SmoothElevationPainter(type, elevation, blendRadius);

		case TYPE_LAYERED_PAINTER:
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

AreaPlacer* ParseAreaPlacer(JSContext* cx, jsval val) {
	int x, y, x1, y1, x2, y2, num, maxFail;
	float size, coherence, smoothness, failFraction;

	switch(GetType(cx, val)) {
		case TYPE_RECT_PLACER:
			if(!GetIntField(cx, val, "x1", x1)) return 0;
			if(!GetIntField(cx, val, "y1", y1)) return 0;
			if(!GetIntField(cx, val, "x2", x2)) return 0;
			if(!GetIntField(cx, val, "y2", y2)) return 0;
			return new RectPlacer(x1, y1, x2, y2);

		case TYPE_CLUMP_PLACER:
			if(!GetFloatField(cx, val, "size", size)) return 0;
			if(!GetFloatField(cx, val, "coherence", coherence)) return 0;
			if(!GetFloatField(cx, val, "smoothness", smoothness)) return 0;
			if(!GetFloatField(cx, val, "failFraction", failFraction)) return 0;
			if(!GetIntField(cx, val, "x", x)) return 0;
			if(!GetIntField(cx, val, "y", y)) return 0;
			return new ClumpPlacer(size, coherence, smoothness, failFraction, x, y);

		default:
			return 0;
	}
}

ObjectGroupPlacer* ParseObjectGroupPlacer(JSContext* cx, jsval val) {
	jsval jsv;
	vector<jsval> array;
	int x, y;
	bool avoidSelf;
	TileClass* tileClass;
	vector<SimpleGroup::Element*> elements;

	switch(GetType(cx, val)) {
		case TYPE_SIMPLE_GROUP:
			// convert x and y
			if(!GetIntField(cx, val, "x", x)) return 0;
			if(!GetIntField(cx, val, "y", y)) return 0;
			if(!GetBoolField(cx, val, "avoidSelf", avoidSelf)) return 0;
			if(!GetTileClassField(cx, val, "tileClass", tileClass)) return 0;
			// convert the elements (which will be JS SimpleElement objects)
			if(!GetJsvalField(cx, val, "elements", jsv)) return 0;
			if(!ParseArray(cx, jsv, array)) return 0;
			elements.resize(array.size());
			for(int i=0; i<array.size(); i++) {
				string type;
				int minCount, maxCount;
				float minDistance, maxDistance;
				if(!GetStringField(cx, array[i], "type", type)) return 0;
				if(!GetIntField(cx, array[i], "minCount", minCount)) return 0;
				if(!GetIntField(cx, array[i], "maxCount", maxCount)) return 0;
				if(!GetFloatField(cx, array[i], "minDistance", minDistance)) return 0;
				if(!GetFloatField(cx, array[i], "maxDistance", maxDistance)) return 0;
				elements[i] = new SimpleGroup::Element(type, minCount, maxCount, 
					minDistance, maxDistance);
			}
			return new SimpleGroup(elements, tileClass, avoidSelf, x, y);

		default:
			return 0;
	}
}

Constraint* ParseConstraint(JSContext* cx, jsval val) {
	vector<jsval> array;
	int areaId;
	TileClass* tileClass;
	float distance;
	float distance2;
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
		case TYPE_NULL_CONSTRAINT:
			return new NullConstraint();

		case TYPE_AVOID_AREA_CONSTRAINT:
			if(!GetIntField(cx, val, "area", areaId)) return 0;
			if(areaId <= 0 || areaId > theMap->areas.size()) return 0;
			return new AvoidAreaConstraint(theMap->areas[areaId-1]);

		case TYPE_AVOID_TEXTURE_CONSTRAINT:
			if(!GetStringField(cx, val, "texture", texture)) return 0;
			return new AvoidTextureConstraint(theMap->getId(texture));

		case TYPE_AVOID_TILE_CLASS_CONSTRAINT:
			if(!GetFloatField(cx, val, "distance", distance)) return 0;
			if(!GetTileClassField(cx, val, "tileClass", tileClass)) return 0;
			return new AvoidTileClassConstraint(tileClass, distance);

		case TYPE_STAY_IN_TILE_CLASS_CONSTRAINT:
			if(!GetFloatField(cx, val, "distance", distance)) return 0;
			if(!GetTileClassField(cx, val, "tileClass", tileClass)) return 0;
			return new StayInTileClassConstraint(tileClass, distance);

		case TYPE_BORDER_TILE_CLASS_CONSTRAINT:
			if(!GetFloatField(cx, val, "distanceInside", distance)) return 0;
			if(!GetFloatField(cx, val, "distanceOutside", distance2)) return 0;
			if(!GetTileClassField(cx, val, "tileClass", tileClass)) return 0;
			return new BorderTileClassConstraint(tileClass, distance, distance2);

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