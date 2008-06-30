/**
 * =========================================================================
 * File        : Color.h
 * Project     : 0 A.D.
 * Description : Convert float RGB(A) colors to unsigned byte
 * =========================================================================
 */

#ifndef INCLUDED_COLOR
#define INCLUDED_COLOR

#include "SColor.h"
#include "maths/Vector3D.h"
#include "maths/Vector4D.h"

// simple defines for 3 and 4 component floating point colors - just map to
// corresponding vector types
typedef CVector3D RGBColor;
typedef CVector4D RGBAColor;

// exposed as function pointer because it is set at init-time to
// one of several implementations depending on CPU caps.
extern SColor4ub (*ConvertRGBColorTo4ub)(const RGBColor& src);

// call once ia32_Init has run; detects CPU caps and activates the best
// possible codepath.
extern void ColorActivateFastImpl();

#endif
