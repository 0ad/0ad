
#ifndef _SCRIPTCUSTOMTYPES_H_
#define _SCRIPTCUSTOMTYPES_H_

// Custom object types

// Whilst Point2d is fully coded, it is never registered so is not available in script
// This is mostly as a demonstration of what you need to code to add a new type

// VECTOR2D
extern JSClass Point2dClass;
extern JSPropertySpec Point2dProperties[];
JSBool Point2d_Constructor(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval);



#endif
