#ifndef __CONVERT_H__
#define __CONVERT_H__

#include "map.h"
#include "object.h"
#include "constraint.h"
#include "areapainter.h"
#include "areaplacer.h"
#include "terrain.h"

// Object type constants

const int 
	TYPE_RECT_PLACER = 1,
	TYPE_TERRAIN_PAINTER = 2,
	TYPE_NULL_CONSTRAINT = 3,
	TYPE_LAYERED_PAINTER = 4,
	TYPE_AVOID_AREA_CONSTRAINT = 5,
	TYPE_CLUMP_PLACER = 6,
	TYPE_AVOID_TEXTURE_CONSTRAINT = 7,
	TYPE_ELEVATION_PAINTER = 8,
	TYPE_SMOOTH_ELEVATION_PAINTER = 9,
	TYPE_SIMPLE_GROUP = 10,
	TYPE_AVOID_TILE_CLASS_CONSTRAINT = 11,
	TYPE_TILE_CLASS_PAINTER = 12;

// Helper functions to convert objects from JS versions

int GetType(JSContext* cx, jsval val);

bool GetIntField(JSContext* cx, jsval obj, const char* name, int& ret);
bool GetBoolField(JSContext* cx, jsval obj, const char* name, int& ret);
bool GetFloatField(JSContext* cx, jsval obj, const char* name, float& ret);
bool GetStringField(JSContext* cx, jsval obj, const char* name, std::string& ret);
bool GetArrayField(JSContext* cx, jsval obj, const char* name, std::vector<jsval>& ret);
bool GetJsvalField(JSContext* cx, jsval obj, const char* name, jsval& ret);

bool ParseArray(JSContext* cx, jsval val, std::vector<jsval>& ret);

Terrain* ParseTerrain(JSContext* cx, jsval val);
AreaPainter* ParseAreaPainter(JSContext* cx, jsval val);
AreaPlacer* ParseAreaPlacer(JSContext* cx, jsval val);
ObjectGroupPlacer* ParseObjectGroupPlacer(JSContext* cx, jsval val);
Constraint* ParseConstraint(JSContext* cx, jsval val);

#endif
