#ifndef INCLUDED_MATHS
#define INCLUDED_MATHS

class FMMatrix44;

extern void DumpMatrix(const FMMatrix44& m);
extern FMMatrix44 DecomposeToScaleMatrix(const FMMatrix44& m);

// (None of these are used any more)
// extern FMMatrix44 operator+ (const FMMatrix44& a, const FMMatrix44& b);
// extern FMMatrix44 operator/ (const FMMatrix44& a, const float b);
// extern FMMatrix44 QuatToMatrix(float x, float y, float z, float w);

#endif // INCLUDED_MATHS
