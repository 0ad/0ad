// This code is in the public domain -- Ignacio Castaño <castano@gmail.com>

#include "PackedFloat.h"
#include "Vector.inl"
#include "ftoi.h"

using namespace nv;

Vector3 nv::rgb9e5_to_vector3(FloatRGB9E5 v) {
}

FloatRGB9E5 nv::vector3_to_rgb9e5(const Vector3 & v) {
}


float nv::float11_to_float32(uint v) {
}

float nv::float10_to_float32(uint v) {
}

Vector3 nv::r11g11b10_to_vector3(FloatR11G11B10 v) {
}

FloatR11G11B10 nv::vector3_to_r11g11b10(const Vector3 & v) {
}

// These are based on: 
// http://www.graphics.cornell.edu/~bjw/rgbe/rgbe.c
// While this may not be the best way to encode/decode RGBE8, I'm not making any changes to maintain compatibility.
FloatRGBE8 nv::vector3_to_rgbe8(const Vector3 & v) {

    float m = max3(v.x, v.y, v.z);

    FloatRGBE8 rgbe;

    if (m < 1e-32) {
        rgbe.v = 0;
    }
    else {
        int e;
        float scale = frexpf(m, &e) * 256.0f / m;
        rgbe.r = U8(ftoi_round(v.x * scale));
        rgbe.g = U8(ftoi_round(v.y * scale));
        rgbe.b = U8(ftoi_round(v.z * scale));
        rgbe.e = U8(e + 128);
    }

    return rgbe;
}


Vector3 nv::rgbe8_to_vector3(FloatRGBE8 v) {
    if (v.e != 0) {
        float scale = ldexpf(1.0f, v.e-(int)(128+8));             // +8 to divide by 256. @@ Shouldn't we divide by 255 instead?
        return scale * Vector3(float(v.r), float(v.g), float(v.b));
    }
    
    return Vector3(0);
}

