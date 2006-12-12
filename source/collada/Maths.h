#ifndef MATHS_H__
#define MATHS_H__

extern FMMatrix44 operator+ (const FMMatrix44& a, const FMMatrix44& b);
extern FMMatrix44 operator/ (const FMMatrix44& a, const float b);
extern FMMatrix44 QuatToMatrix(float x, float y, float z, float w);

#endif // MATHS_H__
