//***********************************************************
//
// Name:		Types.H
// Last Update:	25/1/02
// Author:		Poya Manouchehri
//
// Description: The basic types used by the engine
//
//***********************************************************

#ifndef TYPES_H
#define TYPES_H

#include <windows.h>
#include <stdio.h>

//basic return types
enum FRESULT
{
	R_OK = 0,
	R_FAIL,				//use if nothing else matches the return type
	
	R_BADPARAMS,		//one or more of the parameters were invalid

	R_NOMEMORY,			//not enough memory for an operation

	R_FILE_NOOPEN,		//file could not be opened
	R_FILE_NOREAD,		//file could not be read
	R_FILE_INVALID		//file is corrupt or not supported
};

//string related
#define MAX_NAME_LENGTH		(50)
#define MAX_PATH_LENGTH		(100)

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
class CBitmap;

class CCamera;

class CDiesel3DVertex;

class CGameResource;

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
class CTexture;

class CVector3D;

class CWorld;


#endif