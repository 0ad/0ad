/**
 *  Copyright (C) 2011 by Morten S. Mikkelsen
 *
 *  This software is provided 'as-is', without any express or implied
 *  warranty.  In no event will the authors be held liable for any damages
 *  arising from the use of this software.
 *
 *  Permission is granted to anyone to use this software for any purpose,
 *  including commercial applications, and to alter it and redistribute it
 *  freely, subject to the following restrictions:
 *
 *  1. The origin of this software must not be misrepresented; you must not
 *     claim that you wrote the original software. If you use this software
 *     in a product, an acknowledgment in the product documentation would be
 *     appreciated but is not required.
 *  2. Altered source versions must be plainly marked as such, and must not be
 *     misrepresented as being the original software.
 *  3. This notice may not be removed or altered from any source distribution.
 */


#ifndef __WELDMESH_H__
#define __WELDMESH_H__


#ifdef __cplusplus
extern "C" {
#endif

// piRemapTable must be initialized and point to an area in memory
// with the byte size: iNrVerticesIn * sizeof(int).
// pfVertexDataOut must be initialized and point to an area in memory
// with the byte size: iNrVerticesIn * iFloatsPerVert * sizeof(float).
// At the end of the WeldMesh() call the array pfVertexDataOut will contain
// unique vertices only. Each entry in piRemapTable contains the index to
// the new location of the vertex in pfVertexDataOut in units of iFloatsPerVert.
// Note that this code is suitable for welding both unindexed meshes but also
// indexed meshes which need to have duplicates removed. In the latter case
// one simply uses the remap table to convert the old index list to the new vertex array.
// Finally, the return value is the number of unique vertices found.
int WeldMesh(int * piRemapTable, float * pfVertexDataOut,
			  const float pfVertexDataIn[], const int iNrVerticesIn, const int iFloatsPerVert);

#ifdef __cplusplus
}
#endif


#endif