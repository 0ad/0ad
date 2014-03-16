/*
	Copyright (C) 2005-2007 Feeling Software Inc.
	Portions of the code are:
	Copyright (C) 2005-2007 Sony Computer Entertainment America
	
	MIT License: http://www.opensource.org/licenses/mit-license.php
*/

#include "StdAfx.h"
#include "FCDocument/FCDocument.h"
#include "FCDocument/FCDGeometryMesh.h"
#include "FCDocument/FCDGeometryPolygons.h"
#include "FCDocument/FCDGeometryPolygonsInput.h"
#include "FCDocument/FCDGeometryPolygonsTools.h"
#include "FCDocument/FCDGeometrySource.h"
#include "FCDocument/FCDAnimated.h"

namespace FCDGeometryPolygonsTools
{
	// Triangulates a mesh.
	void Triangulate(FCDGeometryMesh* mesh)
	{
		if (mesh == NULL) return;

		size_t polygonsCount = mesh->GetPolygonsCount();
		for (size_t i = 0; i < polygonsCount; ++i)
		{
			Triangulate(mesh->GetPolygons(i), false);
		}

		// Recalculate the mesh/polygons statistics
		mesh->Recalculate();
	}

	// Triangulates a polygons set.
	void Triangulate(FCDGeometryPolygons* polygons, bool recalculate)
	{
		if (polygons == NULL) return;
		if (polygons->GetPrimitiveType() == FCDGeometryPolygons::LINE_STRIPS || polygons->GetPrimitiveType() == FCDGeometryPolygons::LINES || polygons->GetPrimitiveType() == FCDGeometryPolygons::POINTS) return;

		// Pre-allocate and ready the end index/count buffers
		size_t oldFaceCount = polygons->GetFaceVertexCountCount();
		UInt32List oldFaceVertexCounts(polygons->GetFaceVertexCounts(), oldFaceCount);
		polygons->SetFaceVertexCountCount(0);
		fm::pvector<FCDGeometryPolygonsInput> indicesOwners;
		fm::vector<UInt32List> oldDataIndices;
		size_t inputCount = polygons->GetInputCount();
		for (size_t i = 0; i < inputCount; ++i)
		{
			FCDGeometryPolygonsInput* input = polygons->GetInput(i);
			if (input->GetIndexCount() == 0) continue;
			uint32* indices = input->GetIndices();
			size_t oldIndexCount = input->GetIndexCount();
			oldDataIndices.push_back(UInt32List(indices, oldIndexCount));
			indicesOwners.push_back(input);
			input->SetIndexCount(0);
			input->ReserveIndexCount(oldIndexCount);
		}
		size_t dataIndicesCount = oldDataIndices.size();

		// Rebuild the index/count buffers.
		// Drop holes and polygons with less than three vertices. 
		size_t oldOffset = 0;
		for (size_t oldFaceIndex = 0; oldFaceIndex < oldFaceCount; ++oldFaceIndex)
		{
			size_t oldFaceVertexCount = oldFaceVertexCounts[oldFaceIndex];
			bool isHole = polygons->IsHoleFaceHole((uint32) oldFaceIndex);
			if (!isHole && oldFaceVertexCount >= 3)
			{
				if (polygons->GetPrimitiveType() == FCDGeometryPolygons::POLYGONS || polygons->GetPrimitiveType() == FCDGeometryPolygons::TRIANGLE_FANS)
				{
					// Fan-triangulation: works well on convex polygons.
					size_t triangleCount = oldFaceVertexCount - 2;
					for (size_t triangleIndex = 0; triangleIndex < triangleCount; ++triangleIndex)
					{
						for (size_t j = 0; j < dataIndicesCount; ++j)
						{
							UInt32List& oldData = oldDataIndices[j];
							FCDGeometryPolygonsInput* input = indicesOwners[j];
							input->AddIndex(oldData[oldOffset]);
							input->AddIndex(oldData[oldOffset + triangleIndex + 1]);
							input->AddIndex(oldData[oldOffset + triangleIndex + 2]);
						}
						polygons->AddFaceVertexCount(3);
					}
				}
				else if (polygons->GetPrimitiveType() == FCDGeometryPolygons::TRIANGLE_STRIPS)
				{
					// Fan-triangulation: works well on convex polygons.
					size_t triangleCount = oldFaceVertexCount - 2;
					for (size_t triangleIndex = 0; triangleIndex < triangleCount; ++triangleIndex)
					{
						for (size_t j = 0; j < dataIndicesCount; ++j)
						{
							UInt32List& oldData = oldDataIndices[j];
							FCDGeometryPolygonsInput* input = indicesOwners[j];
							if ((triangleIndex & 0x1) == 0x0)
							{
								input->AddIndex(oldData[oldOffset + triangleIndex + 0]);
								input->AddIndex(oldData[oldOffset + triangleIndex + 1]);
								input->AddIndex(oldData[oldOffset + triangleIndex + 2]);
							}
							else
							{
								// Flip the winding of every second triangle in the strip.
								input->AddIndex(oldData[oldOffset + triangleIndex + 0]);
								input->AddIndex(oldData[oldOffset + triangleIndex + 2]);
								input->AddIndex(oldData[oldOffset + triangleIndex + 1]);
							}
						}
						polygons->AddFaceVertexCount(3);
					}
				}
			}
			oldOffset += oldFaceVertexCount;
		}

		polygons->SetPrimitiveType(FCDGeometryPolygons::POLYGONS);
		polygons->SetHoleFaceCount(0);

		if (recalculate) polygons->Recalculate();
	}

	static uint32 CompressSortedVector(FMVector3& toInsert, FloatList& insertedList, UInt32List& compressIndexReferences)
	{
		// Look for this vector within the already inserted list.
		size_t start = 0, end = compressIndexReferences.size(), mid;
		for (mid = (start + end) / 2; start < end; mid = (start + end) / 2)
		{
			uint32 index = compressIndexReferences[mid];
			if (toInsert.x == insertedList[3 * index]) break;
			else if (toInsert.x < insertedList[3 * index]) end = mid;
			else start = mid + 1;
		}

		// Look for the tolerable range within the binary-sorted dimension.
		size_t rangeStart, rangeEnd;
		for (rangeStart = mid; rangeStart > 0; --rangeStart)
		{
			uint32 index = compressIndexReferences[rangeStart - 1];
			if (!IsEquivalent(insertedList[3 * index], toInsert.x)) break;
		}
		for (rangeEnd = min(mid + 1, compressIndexReferences.size()); rangeEnd < compressIndexReferences.size(); ++rangeEnd)
		{
			uint32 index = compressIndexReferences[rangeEnd];
			if (!IsEquivalent(insertedList[3 * index], toInsert.x)) break;
		}
		FUAssert(rangeStart < rangeEnd || (rangeStart == rangeEnd && rangeEnd == compressIndexReferences.size()), return 0);

		// Look for an equivalent vector within the tolerable range
		for (size_t g = rangeStart; g < rangeEnd; ++g)
		{
			uint32 index = compressIndexReferences[g];
			if (IsEquivalent(toInsert, *(const FMVector3*) &insertedList[3 * index])) return index;
		}

		// Insert this new vector in the list and add the index reference at the correct position.
		uint32 compressIndex = (uint32) (insertedList.size() / 3);
		compressIndexReferences.insert(compressIndexReferences.begin() + mid, compressIndex);
		insertedList.push_back(toInsert.x);
		insertedList.push_back(toInsert.y);
		insertedList.push_back(toInsert.z);
		return compressIndex;
	}

	struct TangentialVertex
	{
		float* normalPointer;
		float* texCoordPointer;
		FMVector3 tangent;
		uint32 count;
		uint32 tangentId;
		uint32 binormalId;
	};
	typedef fm::vector<TangentialVertex> TangentialVertexList;

	// Generates the texture tangents and binormals for a given source of texture coordinates.
	void GenerateTextureTangentBasis(FCDGeometryMesh* mesh, FCDGeometrySource* texcoordSource, bool generateBinormals)
	{
		if (texcoordSource == NULL || mesh == NULL) return;

		// First count the positions.
		FCDGeometrySource* positionSource = mesh->FindSourceByType(FUDaeGeometryInput::POSITION);
		if (positionSource == NULL) return;
		size_t globalVertexCount = positionSource->GetValueCount();

		// Allocate the tangential vertices.
		// This temporary buffer is necessary to ensure we have smooth tangents/binormals.
		TangentialVertexList* globalVertices = new TangentialVertexList[globalVertexCount];
		memset(globalVertices, 0, sizeof(TangentialVertexList) * globalVertexCount);

		// This operation is done on the tessellation: fill in the list of tangential vertices.
		size_t polygonsCount = mesh->GetPolygonsCount();
		for (size_t i = 0; i < polygonsCount; ++i)
		{
			FCDGeometryPolygons* polygons = mesh->GetPolygons(i);

			// Verify that this polygons set uses the given texture coordinate source.
			FCDGeometryPolygonsInput* texcoordInput = polygons->FindInput(texcoordSource);
			if (texcoordInput == NULL) continue;

			// Retrieve the data and index buffer of positions/normals/texcoords for this polygons set.
			FCDGeometryPolygonsInput* positionInput = polygons->FindInput(FUDaeGeometryInput::POSITION);
			FCDGeometryPolygonsInput* normalsInput = polygons->FindInput(FUDaeGeometryInput::NORMAL);
			if (positionInput == NULL || normalsInput == NULL) continue;
			FCDGeometrySource* positionSource = positionInput->GetSource();
			FCDGeometrySource* normalsSource = normalsInput->GetSource();
			FCDGeometrySource* texcoordSource = texcoordInput->GetSource();
			if (positionSource == NULL || normalsSource == NULL || texcoordSource == NULL) continue;
			uint32 positionStride = positionSource->GetStride();
			uint32 normalsStride = normalsSource->GetStride();
			uint32 texcoordStride = texcoordSource->GetStride();
			if (positionStride < 3 || normalsStride < 3 || texcoordStride < 2) continue;
			uint32* positionIndices = positionInput->GetIndices();
			uint32* normalsIndices = normalsInput->GetIndices();
			uint32* texcoordIndices = texcoordInput->GetIndices();
			size_t indexCount = positionInput->GetIndexCount();
			if (positionIndices == NULL || normalsIndices == NULL || texcoordIndices == NULL) continue;
			if (indexCount == 0 || indexCount != normalsInput->GetIndexCount() || indexCount != texcoordInput->GetIndexCount()) continue;
			float* positionData = positionSource->GetData();
			float* normalsData = normalsSource->GetData();
			float* texcoordData = texcoordSource->GetData();
			size_t positionDataLength = positionSource->GetDataCount();
			size_t normalsDataLength = normalsSource->GetDataCount();
			size_t texcoordDataLength = texcoordSource->GetDataCount();
			if (positionDataLength == 0 || normalsDataLength == 0 || texcoordDataLength == 0) continue;

			// Iterate of the faces of the polygons set. This includes holes.
			size_t faceCount = polygons->GetFaceVertexCountCount();
			size_t faceVertexOffset = 0;
			for (size_t faceIndex = 0; faceIndex < faceCount; ++faceIndex)
			{
				size_t faceVertexCount = polygons->GetFaceVertexCounts()[faceIndex];
				for (size_t vertexIndex = 0; vertexIndex < faceVertexCount; ++vertexIndex)
				{
					// For each face-vertex pair, retrieve the current/previous/next vertex position/normal/texcoord.
					size_t previousVertexIndex = (vertexIndex > 0) ? vertexIndex - 1 : faceVertexCount - 1;
					size_t nextVertexIndex = (vertexIndex < faceVertexCount - 1) ? vertexIndex + 1 : 0;
					FMVector3& previousPosition = *(FMVector3*)&positionData[positionIndices[faceVertexOffset + previousVertexIndex] * positionStride];
					FMVector2& previousTexcoord = *(FMVector2*)&texcoordData[texcoordIndices[faceVertexOffset + previousVertexIndex] * texcoordStride];
					FMVector3& currentPosition = *(FMVector3*)&positionData[positionIndices[faceVertexOffset + vertexIndex] * positionStride];
					float* texcoordPtr = &texcoordData[texcoordIndices[faceVertexOffset + vertexIndex] * texcoordStride];
					FMVector2& currentTexcoord = *(FMVector2*)texcoordPtr;
					FMVector3& nextPosition = *(FMVector3*)&positionData[positionIndices[faceVertexOffset + nextVertexIndex] * positionStride];
					FMVector2& nextTexcoord = *(FMVector2*)&texcoordData[texcoordIndices[faceVertexOffset + nextVertexIndex] * texcoordStride];
					float* normalPtr = &normalsData[normalsIndices[faceVertexOffset + vertexIndex] * normalsStride];
					FMVector3& normal = *(FMVector3*)normalPtr;

					// The formulae to calculate the tangent-space basis vectors is taken from Maya 7.0 API documentation:
					// "Appendix A: Tangent and binormal vectors".

					// Prepare the edge vectors.
					FMVector3 previousEdge(0.0f, previousTexcoord.x - currentTexcoord.x, previousTexcoord.y - currentTexcoord.y);
					FMVector3 nextEdge(0.0f, nextTexcoord.x - currentTexcoord.x, nextTexcoord.y - currentTexcoord.y);
					FMVector3 previousDisplacement = (previousPosition - currentPosition);
					FMVector3 nextDisplacement = (nextPosition - currentPosition);
					FMVector3 tangent;

					// Calculate the X-coordinate of the tangent vector.
					previousEdge.x = previousDisplacement.x;
					nextEdge.x = nextDisplacement.x;
					FMVector3 crossEdge = nextEdge ^ previousEdge;
					if (IsEquivalent(crossEdge.x, 0.0f)) crossEdge.x = 1.0f; // degenerate
					tangent.x = crossEdge.y / crossEdge.x;

					// Calculate the Y-coordinate of the tangent vector.
					previousEdge.x = previousDisplacement.y;
					nextEdge.x = nextDisplacement.y;
					crossEdge = nextEdge ^ previousEdge;
					if (IsEquivalent(crossEdge.x, 0.0f)) crossEdge.x = 1.0f; // degenerate
					tangent.y = crossEdge.y / crossEdge.x;

					// Calculate the Z-coordinate of the tangent vector.
					previousEdge.x = previousDisplacement.z;
					nextEdge.x = nextDisplacement.z;
					crossEdge = nextEdge ^ previousEdge;
					if (IsEquivalent(crossEdge.x, 0.0f)) crossEdge.x = 1.0f; // degenerate
					tangent.z = crossEdge.y / crossEdge.x;

					// Take the normal vector at this face-vertex pair, out of the calculated tangent vector
					tangent = tangent - normal * (tangent * normal);
					tangent.NormalizeIt();

					// Add this tangent to our tangential vertex.
					FUAssert(positionIndices[faceVertexOffset + vertexIndex] < globalVertexCount, continue);
					TangentialVertexList& list = globalVertices[positionIndices[faceVertexOffset + vertexIndex]];
					size_t vertexCount = list.size();
					bool found = false;
					for (size_t v = 0; v < vertexCount; ++v)
					{
						if ((normalPtr == list[v].normalPointer) && (texcoordPtr == list[v].texCoordPointer))
						{
							list[v].tangent += tangent;
							list[v].count++;
							found = true;
						}
					}
					if (!found)
					{
						TangentialVertex v;
						v.normalPointer = normalPtr;
						v.texCoordPointer = texcoordPtr;
						v.count = 1; v.tangent = tangent;
						v.tangentId = v.binormalId = ~(uint32)0;
						list.push_back(v);
					}
				}
				faceVertexOffset += faceVertexCount;
			}
		}

		FCDGeometrySource* tangentSource = NULL;
		FCDGeometrySource* binormalSource = NULL;
		FloatList tangentData;
		FloatList binormalData;
		UInt32List tangentCompressionIndices;
		UInt32List binormalCompressionIndices;

		// Iterate over the polygons again: this time create the source/inputs for the tangents and binormals.
		for (size_t i = 0; i < polygonsCount; ++i)
		{
			FCDGeometryPolygons* polygons = mesh->GetPolygons(i);

			// Verify that this polygons set uses the given texture coordinate source.
			FCDGeometryPolygonsInput* texcoordInput = polygons->FindInput(texcoordSource);
			if (texcoordInput == NULL) continue;

			// Retrieve the data and index buffer of positions/normals/texcoords for this polygons set.
			FCDGeometryPolygonsInput* positionInput = polygons->FindInput(FUDaeGeometryInput::POSITION);
			FCDGeometryPolygonsInput* normalsInput = polygons->FindInput(FUDaeGeometryInput::NORMAL);
			if (positionInput == NULL || normalsInput == NULL) continue;
			FCDGeometrySource* normalsSource = normalsInput->GetSource();
			FCDGeometrySource* texcoordSource = texcoordInput->GetSource();
			if (normalsSource == NULL || texcoordSource == NULL) continue;
			uint32 normalsStride = normalsSource->GetStride();
			uint32 texcoordStride = texcoordSource->GetStride();
			if (normalsStride < 3 || texcoordStride < 2) continue;
			uint32* positionIndices = positionInput->GetIndices();
			uint32* normalsIndices = normalsInput->GetIndices();
			uint32* texcoordIndices = texcoordInput->GetIndices();
			size_t indexCount = positionInput->GetIndexCount();
			if (positionIndices == NULL || normalsIndices == NULL || texcoordIndices == NULL) continue;
			if (indexCount == 0 || indexCount != normalsInput->GetIndexCount() || indexCount != texcoordInput->GetIndexCount()) continue;
			float* normalsData = normalsSource->GetData();
			float* texcoordData = texcoordSource->GetData();
			size_t normalsDataLength = normalsSource->GetDataCount();
			size_t texcoordDataLength = texcoordSource->GetDataCount();
			if (normalsDataLength == 0 || texcoordDataLength == 0) continue;

			// Create the texture tangents/binormals sources
			if (tangentSource == NULL)
			{
				tangentSource = mesh->AddSource(FUDaeGeometryInput::TEXTANGENT);
				tangentSource->SetDaeId(texcoordSource->GetDaeId() + "-tangents");
				tangentData.reserve(texcoordSource->GetDataCount());
				if (generateBinormals)
				{
					binormalSource = mesh->AddSource(FUDaeGeometryInput::TEXBINORMAL);
					binormalSource->SetDaeId(texcoordSource->GetDaeId() + "-binormals");
					binormalData.reserve(tangentSource->GetDataCount());
				}
			}

			// Calculate the next available offset
			uint32 inputOffset = 0;
			size_t inputCount = polygons->GetInputCount();
			for (size_t j = 0; j < inputCount; ++j)
			{
				inputOffset = max(inputOffset, polygons->GetInput(j)->GetOffset());
			}

			// Create the polygons set input for both the tangents and binormals
			FCDGeometryPolygonsInput* tangentInput = polygons->AddInput(tangentSource, inputOffset + 1);
			tangentInput->SetSet(texcoordInput->GetSet());
			tangentInput->ReserveIndexCount(indexCount);
			FCDGeometryPolygonsInput* binormalInput = NULL;
			if (binormalSource != NULL)
			{
				binormalInput = polygons->AddInput(binormalSource, inputOffset + 2);
				binormalInput->SetSet(tangentInput->GetSet());
				binormalInput->ReserveIndexCount(indexCount);
			}

			// Iterate of the faces of the polygons set. This includes holes.
			size_t vertexCount = positionInput->GetIndexCount();
			size_t faceVertexOffset = 0;
			for (size_t vertexIndex = 0; vertexIndex < vertexCount; ++vertexIndex)
			{
				// For each face-vertex pair retrieve the current vertex normal&texcoord.
				float* normalPtr = &normalsData[normalsIndices[faceVertexOffset + vertexIndex] * normalsStride];
				float* texcoordPtr = 
						&texcoordData[texcoordIndices[faceVertexOffset + vertexIndex] * texcoordStride];

				FUAssert(positionIndices[faceVertexOffset + vertexIndex] < globalVertexCount, continue);
				TangentialVertexList& list = globalVertices[positionIndices[faceVertexOffset + vertexIndex]];
				size_t vertexCount = list.size();
				for (size_t v = 0; v < vertexCount; ++v)
				{
					if ((list[v].normalPointer == normalPtr) && (list[v].texCoordPointer == texcoordPtr))
					{
						if (list[v].tangentId == ~(uint32)0)
						{
							list[v].tangent /= (float) list[v].count; // Average the tangent.
							list[v].tangent.Normalize();
							list[v].tangentId = CompressSortedVector(list[v].tangent, tangentData, tangentCompressionIndices);
						}
						tangentInput->AddIndex(list[v].tangentId);

						if (binormalInput != NULL)
						{
							if (list[v].binormalId == ~(uint32)0)
							{
								// Calculate and store the binormal.
								FMVector3 binormal = (*(FMVector3*)normalPtr ^ list[v].tangent).Normalize();
								uint32 compressedIndex = CompressSortedVector(binormal, binormalData, binormalCompressionIndices);
								list[v].binormalId = compressedIndex;
							}
							binormalInput->AddIndex(list[v].binormalId);
						}
					}
				}
			}
		}

		if (tangentSource != NULL) tangentSource->SetData(tangentData, 3);
		if (binormalSource != NULL) binormalSource->SetData(binormalData, 3);
	}

	struct HashIndexMapItem { UInt32List allValues; UInt32List newIndex; };
	typedef fm::vector<UInt32List> UInt32ListList;
	typedef fm::pvector<FCDGeometryPolygonsInput> InputList;
	typedef fm::map<uint32, HashIndexMapItem> HashIndexMap;
	typedef fm::pvector<FCDGeometryIndexTranslationMap> FCDGeometryIndexTranslationMapList;

	void GenerateUniqueIndices(FCDGeometryMesh* mesh, FCDGeometryPolygons* polygonsToProcess, FCDNewIndicesList& outIndices, FCDGeometryIndexTranslationMapList& outTranslationMaps)
	{
		// Prepare a list of unique index buffers.
		size_t polygonsCount = mesh->GetPolygonsCount();
		if (polygonsCount == 0) return;
		size_t totalVertexCount = 0;

		size_t outIndicesMinSize = (polygonsToProcess == NULL) ? polygonsCount : 1;
		if (outIndices.size() < outIndicesMinSize) outIndices.resize(outIndicesMinSize);

		// Fill in the index buffers for each polygons set.
		for (size_t p = 0; p < polygonsCount; ++p)
		{
			FCDGeometryPolygons* polygons = mesh->GetPolygons(p);
			// DO NOT -EVER- TOUCH MY INDICES - (Says Psuedo-FCDGeometryPoints)
			// Way to much code assumes (and carefully guards) the existing sorted structure
			if (polygons->GetPrimitiveType() == FCDGeometryPolygons::POINTS) return;
			if (polygonsToProcess != NULL && polygons != polygonsToProcess) continue;
			
			// Find the list we are going to pump our new indices into
			UInt32List& outPolyIndices = (polygonsToProcess == NULL) ? outIndices[p] : outIndices.front();

			// Find all the indices list to determine the hash size.
			InputList idxOwners;
			size_t inputCount = polygons->GetInputCount();
			for (size_t i = 0; i < inputCount; ++i)
			{
				if (polygons->GetInput(i)->OwnsIndices())
				{
					// Drop index lists with the wrong number of values and avoid repeats
					FUAssert(idxOwners.empty() || idxOwners.front()->GetIndexCount() == polygons->GetInput(i)->GetIndexCount(), continue);
					if (idxOwners.find(polygons->GetInput(i)) == idxOwners.end())
					{
						idxOwners.push_back(polygons->GetInput(i));
					}
				}
			}
			size_t listCount = idxOwners.size();
			if (listCount == 0) continue; // no inputs?

			// Set-up a simple hashing function.
			UInt32List hashingFunction;
			uint32 hashSize = (uint32) listCount;
			hashingFunction.reserve(hashSize);
			for (uint32 h = 0; h < hashSize; ++h) hashingFunction.push_back(32 * h / hashSize);

			// Iterate over the index lists, hashing/merging the indices.
			HashIndexMap hashMap;
			size_t originalIndexCount = idxOwners.front()->GetIndexCount();
			outPolyIndices.reserve(originalIndexCount);

			// optimization: cache the indices
			uint32** indices = new uint32*[listCount];
			for (size_t l = 0; l < listCount; ++l)
			{
				indices[l] = idxOwners[l]->GetIndices();
			}
			for (size_t i = 0; i < originalIndexCount; ++i)
			{
				// Generate the hash value for this vertex-face pair.
				uint32 hashValue = 0;
				for (size_t l = 0; l < listCount; ++l)
				{
					hashValue ^= (indices[l][i]) << hashingFunction[l];
				}

				// Look for this value in the already-collected ones.
				HashIndexMap::iterator it = hashMap.find(hashValue);
				HashIndexMapItem* hashItem;
				uint32 newIndex = (uint32) totalVertexCount;
				if (it != hashMap.end())
				{
					hashItem = &((*it).second);
					size_t repeatCount = hashItem->allValues.size() / listCount;
					for (size_t r = 0; r < repeatCount && newIndex == totalVertexCount; ++r)
					{
						size_t l;
						for (l = 0; l < listCount; ++l)
						{
							if (indices[l][i] != hashItem->allValues[r * listCount + l]) break;
						}
						if (l == listCount)
						{
							// We have a match: re-use this index.
							newIndex = hashItem->newIndex[r];
						}
					}
				}
				else
				{
					HashIndexMap::iterator k = hashMap.insert(hashValue, HashIndexMapItem());
					hashItem = &k->second;

					// optimization: since we will be adding the indices to allValues, know there will be listCount
					hashItem->allValues.reserve(listCount);
				}

				if (newIndex == totalVertexCount)
				{
					// Append this new value/index to the hash map item and to the index buffer.
					for (size_t l = 0; l < listCount; ++l)
					{
						hashItem->allValues.push_back(indices[l][i]);
					}
					hashItem->newIndex.push_back(newIndex);
					totalVertexCount++;
				}
				outPolyIndices.push_back(newIndex);
			}
			SAFE_DELETE_ARRAY(indices);
		}

		// We now have lists of new indices.  Create maps so we can quickly
		// map the old data to match the new stuff.
		size_t meshSourceCount = mesh->GetSourceCount();
		if (outTranslationMaps.size() < meshSourceCount) outTranslationMaps.resize(meshSourceCount);
		for (size_t d = 0; d < meshSourceCount; ++d)
		{
			FCDGeometrySource* oldSource = mesh->GetSource(d);
			FCDGeometryIndexTranslationMap* thisMap = new FCDGeometryIndexTranslationMap();
			outTranslationMaps[d] = thisMap;

			// When processing just one polygons set, duplicate the source
			// so that the other polygons set can correctly point to the original source.
			for (size_t p = 0; p < polygonsCount; ++p)
			{
				FCDGeometryPolygons* polygons = mesh->GetPolygons(p);
				if (polygonsToProcess != NULL && polygonsToProcess  != polygons) continue;
				const UInt32List& outPolyIndices = (polygonsToProcess == NULL) ? outIndices[p] : outIndices.front();
				FCDGeometryPolygonsInput* oldInput = polygons->FindInput(oldSource);
				if (oldInput == NULL) continue;

				// Retrieve the old list of indices and de-reference the data values.
				uint32* oldIndexList = oldInput->GetIndices();
				size_t oldIndexCount = oldInput->GetIndexCount();
				if (oldIndexList == NULL || oldIndexCount == 0) continue;

				size_t indexCount = min(oldIndexCount, outPolyIndices.size());
				for (size_t i = 0; i < indexCount; ++i)
				{
					uint32 newIndex = outPolyIndices[i];
					uint32 oldIndex = oldIndexList[i];
					if (oldIndex >= oldSource->GetValueCount()) continue;

					// Add this value to the translation map.
					FCDGeometryIndexTranslationMap::iterator itU = thisMap->find(oldIndex);
					if (itU == thisMap->end()) { itU = thisMap->insert(oldIndex, UInt32List()); }
					UInt32List::iterator itF = itU->second.find(newIndex);
					if (itF == itU->second.end()) itU->second.push_back(newIndex);
				}
			}
		}
	}

	void GenerateUniqueIndices(FCDGeometryMesh* mesh, FCDGeometryPolygons* polygonsToProcess, FCDGeometryIndexTranslationMap* translationMap)
	{
		// Prepare a list of unique index buffers.
		size_t polygonsCount = mesh->GetPolygonsCount();
		if (polygonsCount == 0) return;
		UInt32ListList indexBuffers; indexBuffers.resize(polygonsCount);
		size_t totalVertexCount = 0;

		// Fill in the index buffers for each polygons set.
		for (size_t p = 0; p < polygonsCount; ++p)
		{
			UInt32List& indexBuffer = indexBuffers[p];
			FCDGeometryPolygons* polygons = mesh->GetPolygons(p);
			// DO NOT -EVER- TOUCH MY INDICES - (Says Psuedo-FCDGeometryPoints)
			// Way to much code assumes (and carefully guards) the existing sorted structure
			if (polygons->GetPrimitiveType() == FCDGeometryPolygons::POINTS) return;
			if (polygonsToProcess != NULL && polygons != polygonsToProcess) continue;

			// Find all the indices list to determine the hash size.
			InputList idxOwners;
			size_t inputCount = polygons->GetInputCount();
			for (size_t i = 0; i < inputCount; ++i)
			{
				FCDGeometryPolygonsInput* input = polygons->GetInput(i);
				if (input->OwnsIndices())
				{
					// Drop index lists with the wrong number of values and avoid repeats
					FUAssert(idxOwners.empty() || idxOwners.front()->GetIndexCount() == input->GetIndexCount(), continue);
					if (idxOwners.find(input) == idxOwners.end()) idxOwners.push_back(input);
				}
			}
			size_t listCount = idxOwners.size();
			if (listCount == 0) continue; // no inputs?

			// Set-up a simple hashing function.
			UInt32List hashingFunction;
			uint32 hashSize = (uint32) listCount;
			hashingFunction.reserve(hashSize);
			for (uint32 h = 0; h < hashSize; ++h) hashingFunction.push_back(32 * h / hashSize);

			// Iterate over the index lists, hashing/merging the indices.
			HashIndexMap hashMap;
			size_t originalIndexCount = idxOwners.front()->GetIndexCount();
			indexBuffer.reserve(originalIndexCount);

			// optimization: cache the indices
			uint32** indices = new uint32*[listCount];
			for (size_t l = 0; l < listCount; ++l)
			{
				indices[l] = idxOwners[l]->GetIndices();
			}
			for (size_t i = 0; i < originalIndexCount; ++i)
			{
				// Generate the hash value for this vertex-face pair.
				uint32 hashValue = 0;
				for (size_t l = 0; l < listCount; ++l)
				{
					hashValue ^= (indices[l][i]) << hashingFunction[l];
				}

				// Look for this value in the already-collected ones.
				HashIndexMap::iterator it = hashMap.find(hashValue);
				HashIndexMapItem* hashItem;
				uint32 newIndex = (uint32) totalVertexCount;
				if (it != hashMap.end())
				{
					hashItem = &((*it).second);
					size_t repeatCount = hashItem->allValues.size() / listCount;
					for (size_t r = 0; r < repeatCount && newIndex == totalVertexCount; ++r)
					{
						size_t l;
						for (l = 0; l < listCount; ++l)
						{
							if (indices[l][i] != hashItem->allValues[r * listCount + l]) break;
						}
						if (l == listCount)
						{
							// We have a match: re-use this index.
							newIndex = hashItem->newIndex[r];
						}
					}
				}
				else
				{
					HashIndexMap::iterator k = hashMap.insert(hashValue, HashIndexMapItem());
					hashItem = &k->second;

					// optimization: since we will be adding the indices to allValues, know there will be listCount
					hashItem->allValues.reserve(listCount);
				}

				if (newIndex == totalVertexCount)
				{
					// Append this new value/index to the hash map item and to the index buffer.
					for (size_t l = 0; l < listCount; ++l)
					{
						hashItem->allValues.push_back(indices[l][i]);
					}
					hashItem->newIndex.push_back(newIndex);
					totalVertexCount++;
				}
				indexBuffer.push_back(newIndex);
			}
			SAFE_DELETE_ARRAY(indices);
		}

		// De-reference the source data so that all the vertex data match the new indices.
		size_t meshSourceCount = mesh->GetSourceCount();
		for (size_t d = 0; d < meshSourceCount; ++d)
		{
			FCDGeometrySource* oldSource = mesh->GetSource(d);
			uint32 stride = oldSource->GetStride();
			const float* oldVertexData = oldSource->GetData();
			bool isPositionSource = oldSource->GetType() == FUDaeGeometryInput::POSITION && translationMap != NULL;
			FloatList vertexBuffer;
			vertexBuffer.resize(stride * totalVertexCount, 0.0f);

			// When processing just one polygons set, duplicate the source
			// so that the other polygons set can correctly point to the original source.
			FCDGeometrySource* newSource = (polygonsToProcess != NULL) ? mesh->AddSource(oldSource->GetType()) : oldSource;

			FCDAnimatedList newAnimatedList;
			newAnimatedList.clear();
			for (size_t p = 0; p < polygonsCount; ++p)
			{
				const UInt32List& indexBuffer = indexBuffers[p];
				FCDGeometryPolygons* polygons = mesh->GetPolygons(p);
				if (polygonsToProcess != NULL && polygonsToProcess != polygons) continue;
				FCDGeometryPolygonsInput* oldInput = polygons->FindInput(oldSource);
				if (oldInput == NULL) continue;

				// Retrieve the old list of indices and de-reference the data values.
				uint32* oldIndexList = oldInput->GetIndices();
				size_t oldIndexCount = oldInput->GetIndexCount();
				if (oldIndexList == NULL || oldIndexCount == 0) continue;

				size_t indexCount = min(oldIndexCount, indexBuffer.size());
				for (size_t i = 0; i < indexCount; ++i)
				{
					uint32 newIndex = indexBuffer[i];
					uint32 oldIndex = oldIndexList[i];
					if (oldIndex >= oldSource->GetValueCount()) continue;

					FUObjectContainer<FCDAnimated>& animatedValues = oldSource->GetAnimatedValues();
					FCDAnimated* oldAnimated = NULL;
					for (size_t j = 0; j < animatedValues.size(); j++)
					{
						FCDAnimated* animated = animatedValues[j];
						if (animated->GetValue(0) == &(oldVertexData[stride * oldIndex]))
						{
							oldAnimated = animated;
							break;
						}
					}
					if (oldAnimated != NULL)
					{
						FCDAnimated* newAnimated = oldAnimated->Clone(oldAnimated->GetDocument());
						newAnimated->SetArrayElement(newIndex);
						newAnimatedList.push_back(newAnimated);
					}

					// [GLaforte - 12-10-2006] Potential performance optimization: this may copy the same data over itself many times.
					for (uint32 s = 0; s < stride; ++s)
					{
						vertexBuffer[stride * newIndex + s] = oldVertexData[stride * oldIndex + s];
					}
					
					// Add this value to the vertex position translation map.
					if (isPositionSource)
					{
						FCDGeometryIndexTranslationMap::iterator itU = translationMap->find(oldIndex);
						if (itU == translationMap->end()) { itU = translationMap->insert(oldIndex, UInt32List()); }
						UInt32List::iterator itF = itU->second.find(newIndex);
						if (itF == itU->second.end()) itU->second.push_back(newIndex);
					}
				}

				if (polygonsToProcess != NULL)
				{
					// Change the relevant input, if it exists, to point towards the new source.
					uint32 set = oldInput->GetSet();
					SAFE_RELEASE(oldInput);
					FCDGeometryPolygonsInput* newInput = polygons->AddInput(newSource, 0);
					newInput->SetSet(set);
				}
			}

			// Set the compiled data in the source.
			// [ewhittom] In some occasions we have no indices and yet
			// vertices (for example in incomplete meshes). In these cases we do
			// not want to clear the vertices.
			if (!(vertexBuffer.empty() && newSource->GetDataCount() != 0))
			{
				newSource->SetData(vertexBuffer, stride);
			}
			FUObjectContainer<FCDAnimated>& animatedList = newSource->GetAnimatedValues();
			animatedList.clear();
			for (FCDAnimatedList::iterator it = newAnimatedList.begin();
					it != newAnimatedList.end(); it++)
			{
				animatedList.push_back(*it);
			}
		}

		if (polygonsToProcess == NULL)
		{
			// Next, make all the sources per-vertex.
			size_t _sourceCount = mesh->GetSourceCount();
			for (size_t s = 0; s < _sourceCount; ++s)
			{
				FCDGeometrySource* it = mesh->GetSource(s);
				if (!mesh->IsVertexSource(it))
				{
					mesh->AddVertexSource(it);
				}
			}
		}

		// Enforce the index buffers.
		for (size_t p = 0; p < polygonsCount; ++p)
		{
			const UInt32List& indexBuffer = indexBuffers[p];
			FCDGeometryPolygons* polygons = mesh->GetPolygons(p);
			if (polygonsToProcess != NULL && polygons != polygonsToProcess) continue;

			size_t inputCount = polygons->GetInputCount();
			for (size_t i = 0; i < inputCount; i++)
			{
				FCDGeometryPolygonsInput* anyInput = polygons->GetInput(i);
				if (anyInput->GetSource()->GetDataCount() == 0) continue;
				// [ewhittom] Allow empty index buffers with non-empty vertex buffers
				if (indexBuffer.empty()) continue;

				anyInput->SetIndices(&indexBuffer.front(), indexBuffer.size());
			}
		}
	}



	void ApplyUniqueIndices(float* targData, float* srcData, uint32 stride, const FCDGeometryIndexTranslationMap* translationMap)
	{
		for (FCDGeometryIndexTranslationMap::const_iterator it = translationMap->begin(), itEnd = translationMap->end(); it != itEnd; ++it)
		{
			const UInt32List& curList = it->second;
			for (UInt32List::const_iterator uit = curList.begin(); uit != curList.end(); ++uit)
			{
				for (uint32 s = 0; s < stride; ++s)
				{
					targData[stride * (*uit) + s] = srcData[stride * it->first + s];
				}
			}
		}
	}

	void ApplyUniqueIndices(FCDGeometrySource* targSource, uint32 nValues, const FCDGeometryIndexTranslationMap* translationMap)
	{
		size_t tmSize = translationMap->size();
		FUAssert(targSource->GetValueCount() == tmSize, return);

		uint32 stride = targSource->GetStride();
		FloatList oldData(targSource->GetData(), targSource->GetDataCount());

		targSource->SetDataCount(nValues * stride);
		float* targData = targSource->GetData();

		ApplyUniqueIndices(targData, oldData.begin(), stride, translationMap);
	}
		
	void ApplyUniqueIndices(FCDGeometryMesh* targMesh, FCDGeometryMesh* baseMesh, const UInt32List& newIndices, const FCDGeometryIndexTranslationMapList& translationMaps)
	{
		uint32 largest = 0;
		const FCDGeometryIndexTranslationMap* aMap = translationMaps[0];
		for (FCDGeometryIndexTranslationMap::const_iterator it = aMap->begin(), itEnd = aMap->end(); it != itEnd; ++it)
		{
			const UInt32List& curList = it->second;
			for (UInt32List::const_iterator uit = curList.begin(); uit != curList.end(); ++uit)
			{
				largest = max(largest, *uit);
			}
		}

		uint32 newBufferLen = largest + 1;

		for (size_t i = 0; i < targMesh->GetSourceCount(); i++)
		{
			FCDGeometrySource* targSource = targMesh->GetSource(i);
			for (size_t j = 0; j < baseMesh->GetSourceCount(); j++)
			{
				// it is possible the sources are out of order. make sure we
				// are using the correct ones.
				FCDGeometrySource* baseSource = baseMesh->GetSource(j);
				if (baseSource->GetType() == targSource->GetType())
				{
					const FCDGeometryIndexTranslationMap* translationMap = translationMaps[j];
					ApplyUniqueIndices(targSource, newBufferLen, translationMap);
				}
			}
			// Set the source to be per-vertex... this means it has data for every vertex?
			targMesh->AddVertexSource(targSource);
		}

		// Reset the indices.  The indices in newIndices are all polygon indices combined
		const uint32* newIdxPtr = newIndices.begin();
		size_t nNewIndices = newIndices.size();
		for (size_t p = 0; p < targMesh->GetPolygonsCount(); p++)
		{
			FCDGeometryPolygons* polygons = targMesh->GetPolygons(p);
			FCDGeometryPolygonsInput* anInput = polygons->GetInput(0);
			size_t nIndices = anInput->GetIndexCount();
			 // Check if we are valid, if not, the best we can do is not crash
			FUAssert(nIndices >= nNewIndices, nIndices = nNewIndices);
			anInput->SetIndices(newIdxPtr, nIndices);

			newIdxPtr += nIndices;
			nNewIndices -= nIndices;
		}
	}

#if _MSC_VER
# pragma warning(push)
# pragma warning(disable : 4127) // conditional expression constant
# pragma warning(disable : 4244) // conversion from 'const float' to 'uint8'
#endif
	
#define INVALID_VTX_IDX		(uint16(~0)) // This value is the default (invalid) value in the vtx data map

	template<class VAL, bool translateValue, bool isColor>
	void PackVertexBuffers(uint8* destBuffer, uint32 destBuffStride, 
		const FCDGeometrySource* source, uint32 vCount, uint16* vtxPackingMap,
		const FCDGeometryIndexTranslationMap& translationMap)
	{
		const float* srcBuffer = source->GetData();
		const uint32 srcBufferStride = source->GetStride();

		// Special case for missing Alpha channel
		if (isColor) if (srcBufferStride == 3) vCount = 3;
		FUAssert(srcBufferStride >= vCount, return);

		for (FCDGeometryIndexTranslationMap::const_iterator tmitr = translationMap.begin();
				tmitr != translationMap.end(); ++tmitr)
		{
			const UInt32List& newIdxList = tmitr->second;
			for (UInt32List::const_iterator ditr = newIdxList.begin(); ditr != newIdxList.end(); ++ditr) 
			{
				uint32 newIdx = *ditr;
				if (vtxPackingMap[newIdx] != INVALID_VTX_IDX)
				{
					const float* srcData = srcBuffer + tmitr->first * srcBufferStride;
					VAL& setValue = *(VAL*)(destBuffer + vtxPackingMap[newIdx] * destBuffStride);
					if (isColor) 
					{
						for (uint32 i = 0; i < vCount; ++i)
						{
							setValue[i] = (uint8)(srcData[i] * 255);
						}
						if (vCount == 3) setValue[3] = 255;
					}
					else
					{
						for (uint32 i = 0; i < vCount; ++i)
						{
							setValue[i] = srcData[i];
						}
					}
				}
			}
		}
	}

#if _MSC_VER
# pragma warning(pop)
#endif

	void PackVertexBufferV3(uint8* destBuffer, uint32 destBuffStride, 
		const FCDGeometrySource* source, uint32 vCount, uint16* vtxPackingMap,
		const FCDGeometryIndexTranslationMap& translationMap)
	{
		PackVertexBuffers<FMVector3, false, false>(destBuffer, destBuffStride, source, vCount, vtxPackingMap, translationMap);
	}

	void PackVertexBufferColor(uint8* destBuffer, uint32 destBuffStride, 
		const FCDGeometrySource* source, uint32 vCount, uint16* vtxPackingMap,
		const FCDGeometryIndexTranslationMap& translationMap)
	{
		PackVertexBuffers<FMColor, false, false>(destBuffer, destBuffStride, source, vCount, vtxPackingMap, translationMap);
	}

	void PackVertexBufferV2(uint8* destBuffer, uint32 destBuffStride, 
		const FCDGeometrySource* source, uint32 vCount, uint16* vtxPackingMap,
		const FCDGeometryIndexTranslationMap& translationMap)
	{
		PackVertexBuffers<FMVector2, false, false>(destBuffer, destBuffStride, source, vCount, vtxPackingMap, translationMap);
	}

	// Iterate over the indices, remapping them using the provided map until we have consumed 
	// either max indices or max vertices, whichever is first
	// return the number of indices actually consumed.
	uint16 GenerateVertexPackingMap(size_t maxIndex, size_t maxIndices, size_t maxVertices, const uint32* inIndices, uint16* outIndices, UInt16List* outPackingMap, uint16* outNVertices/*=NULL*/)
	{
		FUAssert(inIndices != NULL && outPackingMap != NULL, return 0);
		FUAssert(maxIndices < INVALID_VTX_IDX, maxIndices = INVALID_VTX_IDX - 1);
		outPackingMap->resize(maxIndex + 1, INVALID_VTX_IDX);
		uint16 nIndices, nVertices = 0;
		for (nIndices = 0; nIndices < maxIndices; ++nIndices)
		{
			if (outPackingMap->at(*inIndices) == INVALID_VTX_IDX)  // New vtx
			{
				// map this index to the next available vtx space
				outPackingMap->at(*inIndices) = nVertices;
				// Optimization - dont compare this more often
				// than absolutely necessary.  So if we need to 
				// force a break using the maxIndices instead
				 if (++nVertices >= maxVertices) maxIndices = nIndices;
			}
			if (outIndices != NULL)
			{
				*outIndices = outPackingMap->at(*inIndices);
				++outIndices;
			}
			++inIndices;
		}
		// Sanity check
		FUAssert(nVertices <= nIndices, nVertices = nIndices);
		// Extra requested return
		if (outNVertices != NULL) *outNVertices = nVertices;
		return nIndices;
	}

	uint32 FindLargestUniqueIndex(const FCDGeometryIndexTranslationMap& aMap)
	{
		uint32 largestIdx = 0;
		FCDGeometryIndexTranslationMap::const_iterator it, itEnd = aMap.end();
		for (it = aMap.begin(); it != itEnd; ++it)
		{
			const UInt32List& curList = it->second;
			for (UInt32List::const_iterator uit = curList.begin(); uit != curList.end(); ++uit)
			{
				largestIdx = max(largestIdx, *uit);
			}
		}
		return largestIdx;
	}

	void RevertUniqueIndices(const FCDGeometryPolygonsInput& inPInput, FCDGeometryPolygonsInput& outPInput, const FCDGeometryIndexTranslationMap& translationMap)
	{
		FUFail(;) // NOT_TESTED
		size_t tmSize = translationMap.size();
		uint32 largest = 0;
		for (FCDGeometryIndexTranslationMap::const_iterator it = translationMap.begin(), itEnd = translationMap.end(); it != itEnd; ++it)
		{
			const UInt32List& curList = it->second;
			for (UInt32List::const_iterator uit = curList.begin(); uit != curList.end(); ++uit)
			{
				largest = max(largest, *uit);
			}
		}

		uint32 oldBufferLen = largest + 1;

		const FCDGeometrySource* inSrc = inPInput.GetSource();
		FCDGeometrySource* outSrc = outPInput.GetSource();
		
		FUAssert(inSrc->GetValueCount() == oldBufferLen, return);

		uint32 stride = inSrc->GetStride();
		
		outSrc->SetStride(stride);
		outSrc->SetValueCount(tmSize);
		
		const float* inData = inSrc->GetData();
		float* outData = outSrc->GetData();

		const uint32* inIndices = inPInput.GetIndices();
		FUAssert(inIndices != NULL, return);
		UInt32List indices(inIndices, inPInput.GetIndexCount());

		for (FCDGeometryIndexTranslationMap::const_iterator it = translationMap.begin(), itEnd = translationMap.end(); it != itEnd; ++it)
		{
			const UInt32List& curList = it->second;
			FUAssert(!curList.empty(), continue);

			for (uint32 s = 0; s < stride; ++s)
			{
				//data[stride * (*uit) + s] = oldData[stride *  + s];
				outData[stride * it->first + s]  = inData[stride * curList.front() + s];
			}

			for (UInt32List::const_iterator uit = curList.begin(); uit != curList.end(); ++uit)
			{
				indices.replace(*uit, it->first);
			}
		}

		outPInput.SetIndices(indices.begin(), indices.size());
	}

	// Splits the mesh's polygons sets to ensure that none of them have
	// more than a given number of indices within their index buffers.
	void FitIndexBuffers(FCDGeometryMesh* mesh, size_t maximumIndexCount)
	{
		// Iterate over the original polygons sets, looking for ones that are too big.
		// When a polygon set is too big, a new polygon set will be added, but at the end of the list.
		size_t originalPolygonCount = mesh->GetPolygonsCount();
		for (size_t i = 0; i < originalPolygonCount; ++i)
		{
			// Iterate a over the face-vertex counts of the polygons set in order
			// to find at which face to break this polygons set.
			FCDGeometryPolygons* polygons = mesh->GetPolygons(i);
			if (polygons->GetPrimitiveType() == FCDGeometryPolygons::POINTS) continue;

			size_t faceCount = polygons->GetFaceVertexCountCount();
			if (faceCount == 0) continue;
			UInt32List faceVertexCounts(polygons->GetFaceVertexCounts(), faceCount);
			size_t inputCount = polygons->GetInputCount();

			UInt32List::iterator splitIt = faceVertexCounts.end();
			uint32 faceVertexCount = 0;
			for (splitIt = faceVertexCounts.begin(); splitIt != faceVertexCounts.end(); ++splitIt)
			{
				if (faceVertexCount + (*splitIt) > maximumIndexCount) break;
				faceVertexCount += (*splitIt);
			}
			if (splitIt == faceVertexCounts.end()) continue; // Polygons sets fit correctly.
			size_t splitIndexCount = (size_t) faceVertexCount;
			size_t splitFaceCount = splitIt - faceVertexCounts.begin();

			size_t faceCopyStart = splitFaceCount;
			size_t faceCopyEnd = faceCopyStart;
			size_t faceVertexCopyStart = splitIndexCount;
			size_t faceVertexCopyEnd = faceVertexCopyStart;
			while (faceCopyEnd < faceCount)
			{
				// Create a new polygons set and copy the basic information from the first polygons set.
				FCDGeometryPolygons* polygonsCopy = mesh->AddPolygons();
				polygonsCopy->SetMaterialSemantic(polygons->GetMaterialSemantic());

				// Figure out which faces will be moved to this polygons set.
				faceVertexCount = 0;
				for (; faceCopyEnd < faceCount; ++faceCopyEnd)
				{
					uint32 localCount = faceVertexCounts[faceCopyEnd];
					if (faceVertexCount + localCount > maximumIndexCount) break;
					faceVertexCount += localCount;
				}
				faceVertexCopyEnd += faceVertexCount;

				FUAssert(faceVertexCopyEnd > faceVertexCopyStart, continue);
				FUAssert(faceCopyEnd > faceCopyStart, continue);

				// Create the inputs and their indices over in the new polygons set.
				for (size_t j = 0; j < inputCount; ++j)
				{
					FCDGeometryPolygonsInput* input = polygons->GetInput(j);
					FCDGeometrySource* source = input->GetSource();
					FCDGeometryPolygonsInput* inputCopy;
					if (!mesh->IsVertexSource(source)) inputCopy = polygonsCopy->AddInput(source, input->GetOffset());
					else inputCopy = polygonsCopy->FindInput(source);
					FUAssert(inputCopy != NULL, continue);

					// For owners, copy the indices over.
					size_t indexCopyCount = inputCopy->GetIndexCount();
					if (indexCopyCount == 0)
					{
						uint32* indices = input->GetIndices();
						inputCopy->SetIndices(indices + faceVertexCopyStart, faceVertexCopyEnd - faceVertexCopyStart);
					}
				}

				// Copy the face-vertex counts over to the new polygons set
				// And increment the copy counters.
				size_t faceCopyCount = faceCopyEnd - faceCopyStart;
				polygonsCopy->SetFaceVertexCountCount(faceCopyCount);
				memcpy((void*) polygonsCopy->GetFaceVertexCounts(), &(*(faceVertexCounts.begin() + faceCopyStart)), faceCopyCount * sizeof(uint32));
				faceCopyStart = faceCopyEnd;
				faceVertexCopyStart = faceVertexCopyEnd;
			}

			// Remove the faces that were split away and their indices.
			for (size_t j = 0; j < inputCount; ++j)
			{
				FCDGeometryPolygonsInput* input = polygons->GetInput(j);
				if (input->OwnsIndices())
				{
					input->SetIndexCount(splitIndexCount);
				}
			}
			polygons->SetFaceVertexCountCount(splitFaceCount);
		}

		mesh->Recalculate();
	}

	// Reverses all the normals of a mesh.
	void ReverseNormals(FCDGeometryMesh* mesh)
	{
		size_t sourceCount = mesh->GetSourceCount();
		for (size_t i = 0; i < sourceCount; ++i)
		{
			FCDGeometrySource* source = mesh->GetSource(i);
			if (source->GetType() == FUDaeGeometryInput::NORMAL || source->GetType() == FUDaeGeometryInput::GEOTANGENT
				|| source->GetType() == FUDaeGeometryInput::GEOBINORMAL || source->GetType() == FUDaeGeometryInput::TEXTANGENT
				|| source->GetType() == FUDaeGeometryInput::TEXBINORMAL)
			{
				float* v = source->GetData();
				size_t dataCount = source->GetDataCount();
				for (size_t it = 0; it < dataCount; ++it)
				{
					*(v++) *= -1.0f;
				}
			}
		}
	}

}
