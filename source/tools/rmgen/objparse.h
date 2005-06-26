#ifndef __OBJPARSE_H__
#define __OBJPARSE_H__

#include "map.h"
#include "entity.h"
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
TYPE_SMOOTH_ELEVATION_PAINTER = 9;

// Helper functions to parse objects from JS versions

int GetType(JSContext* cx, jsval val);
bool GetIntField(JSContext* cx, jsval obj, const char* name, int& ret);
bool GetBoolField(JSContext* cx, jsval obj, const char* name, int& ret);
bool GetFloatField(JSContext* cx, jsval obj, const char* name, float& ret);
bool GetStringField(JSContext* cx, jsval obj, const char* name, std::string& ret);
bool GetArrayField(JSContext* cx, jsval obj, const char* name, std::vector<jsval>& ret);
bool GetJsvalField(JSContext* cx, jsval obj, const char* name, jsval& ret);

bool ParseArray(JSContext* cx, jsval val, std::vector<jsval>& ret);

AreaPainter* ParsePainter(JSContext* cx, jsval val);
AreaPlacer* ParsePlacer(JSContext* cx, jsval val);
Constraint* ParseConstraint(JSContext* cx, jsval val);
Terrain* ParseTerrain(JSContext* cx, jsval val);

#endif