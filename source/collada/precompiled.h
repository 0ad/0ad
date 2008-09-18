#ifndef INCLUDED_COLLADA_PRECOMPILED
#define INCLUDED_COLLADA_PRECOMPILED

#define COLLADA_DLL
#include "DLL.h"

extern void Log(int severity, const char* fmt, ...);

#ifdef _WIN32
# define WIN32
# define WIN32_LEAN_AND_MEAN
# pragma warning(disable: 4996)
#endif

#include <climits>

#include "FCollada.h"
#include "FCDocument/FCDAsset.h"
#include "FCDocument/FCDocument.h"
#include "FCDocument/FCDocumentTools.h"
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
#include "FUtils/FUDaeSyntax.h"
#include "FUtils/FUFileManager.h"
#include "FUtils/FUXmlParser.h"

#include <cassert>
#include <cstdarg>
#include <string>

// FCollada pollutes the global namespace by defining these
// to std::{min,max}, so undo its macros
#undef min
#undef max

#define UNUSED(paramName)

#endif // INCLUDED_COLLADA_PRECOMPILED
