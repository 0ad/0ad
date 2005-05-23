#ifndef __OBJPARSE_H__
#define __OBJPARSE_H__

#include "map.h"
#include "entity.h"
#include "simpleconstraints.h"
#include "simplepainters.h"
#include "rectplacer.h"

// Object type constants

const int 
TYPE_RECTPLACER = 1,
TYPE_TERRAINPAINTER = 2,
TYPE_NULLCONSTRAINT = 3;

// Helper functions to parse objects from array versions

JSObject* GetRaw(JSContext* cx, jsval val);

bool ParseFields(JSContext* cx, JSObject* array, const char* format, ...);

AreaPainter* ParsePainter(JSContext* cx, jsval val);
AreaPlacer* ParsePlacer(JSContext* cx, jsval val);
Constraint* ParseConstraint(JSContext* cx, jsval val);

#endif