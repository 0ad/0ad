#ifndef __OBJPARSE_H__
#define __OBJPARSE_H__

#include "map.h"
#include "entity.h"
#include "constraint.h"
#include "areapainter.h"
#include "areaplacer.h"
#include "centeredplacer.h"
#include "terrain.h"

// Object type constants

const int 
TYPE_RECTPLACER = 1,
TYPE_TERRAINPAINTER = 2,
TYPE_NULLCONSTRAINT = 3,
TYPE_RANDOMTERRAIN = 4,
TYPE_LAYEREDPAINTER = 5,
TYPE_AVOIDAREACONSTRAINT = 6,
TYPE_CLUMPPLACER = 7,
TYPE_EXACTPLACER = 8,
TYPE_AVOIDTERRAINCONSTRAINT = 9,
TYPE_ANDCONSTRAINT = 10,
TYPE_MULTIPLACER = 11;

// Helper functions to parse objects from array versions

JSObject* GetRaw(JSContext* cx, jsval val);

bool ParseFields(JSContext* cx, JSObject* array, const char* format, ...);
bool ParseArray(JSContext* cx, jsval val, std::vector<jsval>& ret);

AreaPainter* ParsePainter(JSContext* cx, jsval val);
AreaPlacer* ParsePlacer(JSContext* cx, jsval val);
Constraint* ParseConstraint(JSContext* cx, jsval val);
Terrain* ParseTerrain(JSContext* cx, jsval val);
CenteredPlacer* ParseCenteredPlacer(JSContext* cx, jsval val);

#endif