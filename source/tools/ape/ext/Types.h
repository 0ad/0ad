//***********************************************************
//
// Name:		Types.h
//
// Description: The basic types used by the engine
//
//***********************************************************

#ifndef TYPES_H
#define TYPES_H

#include <stdio.h>

//color structures
struct SColor4ub
{
	unsigned char R;
	unsigned char G;
	unsigned char B;
	unsigned char A;
};

struct SColor4f
{
	float R;
	float G;
	float B;
	float A;
};

//all the major classes:

class CCamera;

class CEngine;
class CEntity;

class CFrustum;

class CMatrix3D;
class CMesh;
class CMeshPoly;
class CShadyMesh;
class CShadyMeshPoly;

class CNode;

class CPatch;
class CPlane;

class CRenderer;

class CTerrain;

class CVector3D;

class CWorld;


#endif
