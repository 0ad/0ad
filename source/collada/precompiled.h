#ifndef PRECOMPILED_H__
#define PRECOMPILED_H__

#define COLLADA_DLL
#include "DLL.h"

extern void Log(int severity, const char* fmt, ...);

#ifdef _WIN32
# define WIN32
# define WIN32_LEAN_AND_MEAN
# pragma warning(disable: 4996)
#endif

#include "FCollada.h"
#include "FCDocument/FCDAsset.h"
#include "FCDocument/FCDocument.h"
#include "FCDocument/FCDAnimated.h"
#include "FCDocument/FCDAnimationCurve.h"
#include "FCDocument/FCDController.h"
#include "FCDocument/FCDControllerInstance.h"
#include "FCDocument/FCDExtra.h"
#include "FCDocument/FCDGeometry.h"
#include "FCDocument/FCDGeometryMesh.h"
#include "FCDocument/FCDGeometryPolygons.h"
#include "FCDocument/FCDGeometryPolygonsTools.h"
#include "FCDocument/FCDGeometrySource.h"
#include "FCDocument/FCDSceneNode.h"
#include "FCDocument/FCDSkinController.h"
#include <cassert>
#include <string>

// FCollada pollutes the global namespace by defining these
// to std::{min,max}, so undo its macros
#undef min
#undef max

#endif // PRECOMPILED_H__
