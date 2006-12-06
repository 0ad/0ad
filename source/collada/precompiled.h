#define COLLADA_DLL
#include "DLL.h"

extern void Log(int severity, const char* fmt, ...);

#ifdef _WIN32
# define WIN32
# define WIN32_LEAN_AND_MEAN
# pragma warning(disable: 4996)
#endif

#include "FCollada.h"
#include "FCDocument/FCDocument.h"
#include "FCDocument/FCDSceneNode.h"
#include "FCDocument/FCDGeometry.h"
#include "FCDocument/FCDGeometryMesh.h"
#include "FCDocument/FCDGeometryPolygons.h"
#include "FCDocument/FCDGeometrySource.h"
