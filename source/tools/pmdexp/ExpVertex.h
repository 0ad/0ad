#ifndef __EXPVERTEX_H
#define __EXPVERTEX_H

#include "lib\types.h"
#include "Vector3D.h"
#include "ModelDef.h"

////////////////////////////////////////////////////////////////////////
// ExpVertex: vertex type used in building mesh geometry
struct ExpVertex {
	// index into original meshes point list
	unsigned int m_Index;
	// object space position
	CVector3D m_Pos;
	// object space normal
	CVector3D m_Normal;
	// uv coordinates
	float m_UVs[2];
};

#endif

