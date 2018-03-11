// Slightly modified version of weldmesh, by Wildfire Games, for 0 A.D.
// 
// Motivation for changes:
//  * Fix build on *BSD (including malloc.h produces an error)

#include "precompiled.h"

#ifdef _MSC_VER
#if _MSC_VER > 1800
# pragma warning(disable:4456) // hides previous local declaration
#endif
#endif

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


#include "weldmesh.h"
#include <string.h>
#include <assert.h>

#if defined(__APPLE__) || defined(__FreeBSD__) || defined(__OpenBSD__)
#include <stdlib.h>  /* BSD-based OSes get their malloc stuff through here */
#else
#include <malloc.h> 
#endif

static void MergeVertsFast(int * piCurNrUniqueVertices, int * piRemapTable, float * pfVertexDataOut, int * piVertexIDs,
			  const float pfVertexDataIn[], const int iNrVerticesIn, const int iFloatsPerVert,
			  const int iL_in, const int iR_in, const int iChannelNum);

int WeldMesh(int * piRemapTable, float * pfVertexDataOut,
			  const float pfVertexDataIn[], const int iNrVerticesIn, const int iFloatsPerVert)
{
	int iUniqueVertices = 0, i=0;
	int * piVertexIDs = NULL;
	if(iNrVerticesIn<=0) return 0;


	iUniqueVertices = 0;
	piVertexIDs = (int *) malloc(sizeof(int)*iNrVerticesIn);
	if(piVertexIDs!=NULL)
	{
		for(i=0; i<iNrVerticesIn; i++)
		{
			piRemapTable[i] = -1;
			piVertexIDs[i] = i;
		}

		MergeVertsFast(&iUniqueVertices, piRemapTable, pfVertexDataOut, piVertexIDs,
										 pfVertexDataIn, iNrVerticesIn, iFloatsPerVert, 0, iNrVerticesIn-1, 0);

		free(piVertexIDs);

		// debug check
		for(i=0; i<iUniqueVertices; i++)
			assert(piRemapTable[i]>=0);
	}

	return iUniqueVertices;
}





static void MergeVertsFast(int * piCurNrUniqueVertices, int * piRemapTable, float * pfVertexDataOut, int * piVertexIDs,
			  const float pfVertexDataIn[], const int iNrVerticesIn, const int iFloatsPerVert,
			  const int iL_in, const int iR_in, const int iChannelNum)
{
	const int iCount = iR_in-iL_in+1;
	int l=0;
	float fMin, fMax, fAvg;
	assert(iCount>0);
	// make bbox
	fMin = pfVertexDataIn[ piVertexIDs[iL_in]*iFloatsPerVert + iChannelNum]; fMax = fMin;
	for(l=(iL_in+1); l<=iR_in; l++)
	{
		const int index = piVertexIDs[l]*iFloatsPerVert + iChannelNum;
		const float fVal = pfVertexDataIn[index];
		if(fMin>fVal) fMin=fVal;
		else if(fMax<fVal) fMax=fVal;
	}

	// terminate recursion when the separation/average value
	// is no longer strictly between fMin and fMax values.
	fAvg = 0.5f*(fMax + fMin);
	if(fAvg<=fMin || fAvg>=fMax || iCount==1)
	{
		if((iChannelNum+1) == iFloatsPerVert || iCount==1)	// we are done, weld by hand
		{
			int iUniqueNewVertices = 0;
			float * pfNewUniVertsOut = &pfVertexDataOut[ piCurNrUniqueVertices[0]*iFloatsPerVert ];

			for(l=iL_in; l<=iR_in; l++)
			{
				const int index = piVertexIDs[l]*iFloatsPerVert;

				int iFound = 0;	// didn't find copy yet.
				int l2=0;
				while(l2<iUniqueNewVertices && iFound==0)
				{
					const int index2 = l2*iFloatsPerVert;

					int iAllSame = 1;
					int c=0;
					while(iAllSame!=0 && c<iFloatsPerVert)
					{
						iAllSame &= (pfVertexDataIn[index+c] == pfNewUniVertsOut[index2+c] ? 1 : 0);
						++c;
					}

					iFound = iAllSame;
					if(iFound==0) ++l2;
				}
				
				// generate new entry
				if(iFound==0)
				{
					memcpy(pfNewUniVertsOut+iUniqueNewVertices*iFloatsPerVert, pfVertexDataIn+index, sizeof(float)*iFloatsPerVert);
					++iUniqueNewVertices;
				}

				assert(piRemapTable[piVertexIDs[l]] == -1);	// has not yet been assigned
				piRemapTable[piVertexIDs[l]] = piCurNrUniqueVertices[0] + l2;
			}

			piCurNrUniqueVertices[0] += iUniqueNewVertices;
		}
		else
		{
			MergeVertsFast(piCurNrUniqueVertices, piRemapTable, pfVertexDataOut, piVertexIDs,
						   pfVertexDataIn, iNrVerticesIn, iFloatsPerVert,
							iL_in, iR_in, iChannelNum+1);
		}
	}
	else
	{
		int iL=iL_in, iR=iR_in, index;

		// seperate (by fSep) all points between iL_in and iR_in in pTmpVert[]
		while(iL < iR)
		{
			int iReadyLeftSwap = 0;
			int iReadyRightSwap = 0;
			while(iReadyLeftSwap==0 && iL<iR)
			{
				assert(iL>=iL_in && iL<=iR_in);
				index = piVertexIDs[iL]*iFloatsPerVert+iChannelNum;
				iReadyLeftSwap = !(pfVertexDataIn[index]<fAvg) ? 1 : 0;
				if(iReadyLeftSwap==0) ++iL;
			}
			while(iReadyRightSwap==0 && iL<iR)
			{
				assert(iR>=iL_in && iR<=iR_in);
				index = piVertexIDs[iR]*iFloatsPerVert+iChannelNum;
				iReadyRightSwap = pfVertexDataIn[index]<fAvg ? 1 : 0;
				if(iReadyRightSwap==0) --iR;
			}
			assert( (iL<iR) || (iReadyLeftSwap==0 || iReadyRightSwap==0));

			if(iReadyLeftSwap!=0 && iReadyRightSwap!=0)
			{
				int iID=0;
				assert(iL<iR);
				iID = piVertexIDs[iL];
				piVertexIDs[iL] = piVertexIDs[iR];
				piVertexIDs[iR] = iID;
				++iL; --iR;
			}
		}

		assert(iL==(iR+1) || (iL==iR));
		if(iL==iR)
		{
			const int index = piVertexIDs[iR]*iFloatsPerVert+iChannelNum;
			const int iReadyRightSwap = pfVertexDataIn[index]<fAvg ? 1 : 0;
			if(iReadyRightSwap!=0) ++iL;
			else --iR;
		}

		// recurse
		if(iL_in <= iR)
			MergeVertsFast(piCurNrUniqueVertices, piRemapTable, pfVertexDataOut, piVertexIDs,
						   pfVertexDataIn, iNrVerticesIn, iFloatsPerVert, iL_in, iR, iChannelNum);	// weld all left of fSep
		if(iL <= iR_in)
			MergeVertsFast(piCurNrUniqueVertices, piRemapTable, pfVertexDataOut, piVertexIDs,
						   pfVertexDataIn, iNrVerticesIn, iFloatsPerVert, iL, iR_in, iChannelNum);	// weld all right of (or equal to) fSep
	}
}

