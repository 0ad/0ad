//***********************************************************
//
// Name:		TerrGlobals.H
// Last Update: 27/2/02
// Author:		Poya Manouchehri
//
// Description: Some globals and macros, used by the CTerrain
//				and CPatch
//
//***********************************************************

#ifndef TERRGLOBALS_H
#define TERRGLOBALS_H

const int PATCH_SIZE = 16;
const int  CELL_SIZE = 4;	//horizontal scale of the patches
const float HEIGHT_SCALE = 0.35f;

//only 3x3 patches loaded at a time
const int NUM_PATCHES_PER_SIDE = 20;

//must be odd number of patches
//#define TERRAIN_CHUNK_SIZE		(PATCH_SIZE*NUM_PATCHES_PER_SIDE)
const int MAP_SIZE = ( (NUM_PATCHES_PER_SIDE*PATCH_SIZE)+1 );




#endif
