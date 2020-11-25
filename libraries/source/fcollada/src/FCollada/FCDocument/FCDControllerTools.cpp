/*
	Copyright (C) 2005-2007 Feeling Software Inc.
	Portions of the code are:
	Copyright (C) 2005-2007 Sony Computer Entertainment America
	
	MIT License: http://www.opensource.org/licenses/mit-license.php
*/

#include "StdAfx.h"
#include "FCDocument/FCDControllerTools.h"
#include "FCDocument/FCDSkinController.h"
#include "FCDocument/FCDLibrary.h"
#include "FCDocument/FCDEntity.h"
#include "FCDocument/FCDGeometry.h"
#include "FCDocument/FCDController.h"
#include "FCDocument/FCDMorphController.h"
#include "FCDocument/FCDGeometryMesh.h"
#include "FCDocument/FCDGeometrySource.h"
#include "FCDocument/FCDGeometryPolygons.h"

namespace FCDControllerTools
{
	void ApplyTranslationMap(const FCDSkinController* controller, const FCDGeometryIndexTranslationMap& translationMap, const UInt16List& packingMap, fm::pvector<const FCDSkinControllerVertex>& skinInfluences)
	{
		const FCDSkinControllerVertex* influences = controller->GetVertexInfluences();
		uint32 influenceCount = (uint32)controller->GetInfluenceCount();

		// make a copy of the influences
		//FCDSkinControllerVertex* copiedInfluences = new FCDSkinControllerVertex[influenceCount];
		//for (size_t i = 0; i < influenceCount; i++)
		//{
		//	copiedInfluences[i] = influences[i];
		//}

		// Alright, this map contains pointers to the different weights
		uint16 largestIdx = 0;
		for (size_t i = 0; i < packingMap.size(); ++i)
		{
			if (packingMap[i] > largestIdx && packingMap[i] != (uint16)-1) 
			{
				largestIdx = packingMap[i];
			}
		}

		// This is how many vertices we are packing here!
		skinInfluences.resize(largestIdx + 1);
		FUAssert(largestIdx < influenceCount,);

		// Now iterate over all the vertices that we have used and come up with something
		for (uint32 i = 0; i < influenceCount; ++i)
		{
			UInt32List uniqueIndices = translationMap[i];
			for (UInt32List::iterator uItr = uniqueIndices.begin(); uItr != uniqueIndices.end(); ++uItr)
			{
				if (packingMap[*uItr] != (uint16)-1)
				{
					skinInfluences[packingMap[*uItr]] = influences + i;
				}
			}
		}
		/* Failing on this assertion is probably because the translationMap was
		   not created by GenerateUniqueIndices for the target of the given
		   controller. It can also be caused by calling this method multiple
		   times for the same translationMap. *
		FUAssert(translationMap.size() <= influenceCount, return;);
		FUAssert(influenceCount > 0, return;);

		// find the largest index
		uint32 largest = 0;
		for (FCDGeometryIndexTranslationMap::const_iterator it = translationMap.begin(), itEnd = translationMap.end(); it != itEnd; ++it)
		{
			const UInt32List& curList = it->second;
			for (UInt32List::const_iterator it = curList.begin(); it != curList.end(); ++it)
			{
				largest = max(largest, *it);
			}
		}

		// This function will create havok when re-initializing meshes
		// The problem is because the skin has already been remapped,
		// but the mesh has not.  Condition is making assumption that
		// the skin already maps to the mesh perfectly.
		uint32 newInfluenceCount = largest + 1;
		if (newInfluenceCount != influenceCount) // work is possible
		{
			// Set the new influences
			controller->SetInfluenceCount(newInfluenceCount);

			for (FCDGeometryIndexTranslationMap::const_iterator  it = translationMap.begin(), itEnd = translationMap.end(); it != itEnd; ++it)
			{
				FCDSkinControllerVertex& newInfluence = copiedInfluences[it->first];
				size_t pairCount = newInfluence.GetPairCount();

				const UInt32List& curList = it->second;
				for (UInt32List::const_iterator it = curList.begin(); it != curList.end(); it++)
				{
					FCDSkinControllerVertex* vertex = controller->GetVertexInfluence(*it);
					vertex->SetPairCount(pairCount);
					for (size_t j = 0; j < pairCount; j++)
					{
						FCDJointWeightPair* pair = newInfluence.GetPair(j);
						FCDJointWeightPair* p = vertex->GetPair(j);
						p->jointIndex = pair->jointIndex;
						p->weight = pair->weight;
					}
				}
			}
		}
		SAFE_DELETE_ARRAY(copiedInfluences); */
	}
}

