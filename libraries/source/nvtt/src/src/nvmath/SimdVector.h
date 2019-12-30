// This code is in the public domain -- Ignacio Castaño <castano@gmail.com>

#include "Vector.h" // Vector3, Vector4


#if NV_USE_ALTIVEC
#   include "SimdVector_VE.h"
#endif

#if NV_USE_SSE
#   include "SimdVector_SSE.h"
#endif
