#include "precompiled.h"

#include "GeomReindex.h"

#include "FCollada.h"
#include "FCDocument/FCDEntity.h"
#include "FCDocument/FCDGeometryPolygons.h"
#include "FCDocument/FCDGeometryPolygonsInput.h"
#include "FCDocument/FCDGeometrySource.h"
#include "FCDocument/FCDSkinController.h"

#include <cassert>
#include <vector>
#include <map>
#include <algorithm>

struct VertexData
{
	VertexData(const float* pos, const float* norm, const float* tex, const std::vector<FCDJointWeightPair>& weights)
		: x(pos[0]), y(pos[1]), z(pos[2]),
		nx(norm[0]), ny(norm[1]), nz(norm[2]),
		u(tex[0]), v(tex[1]),
		weights(weights)
	{
	}

	float x, y, z;
	float nx, ny, nz;
	float u, v;
	std::vector<FCDJointWeightPair> weights;
};

bool similar(float a, float b)
{
	return (fabsf(a - b) < 0.000001f);
}

bool operator==(const FCDJointWeightPair& a, const FCDJointWeightPair& b)
{
	return (a.jointIndex == b.jointIndex && similar(a.weight, b.weight));
}

bool operator<(const FCDJointWeightPair& a, const FCDJointWeightPair& b)
{
	// Sort by decreasing weight, then by increasing joint ID
	if (a.weight > b.weight)
		return true;
	else if (a.weight < b.weight)
		return false;
	else if (a.jointIndex < b.jointIndex)
		return true;
	else
		return false;
}


bool operator==(const VertexData& a, const VertexData& b)
{
	return (similar(a.x,  b.x)  && similar(a.y,  b.y)  && similar(a.z,  b.z)
		 && similar(a.nx, b.nx) && similar(a.ny, b.ny) && similar(a.nz, b.nz)
		 && similar(a.u,  b.u)  && similar(a.v,  b.v)
		 && (a.weights == b.weights));
}

bool operator<(const VertexData& a, const VertexData& b)
{
#define CMP(f) if (a.f < b.f) return true; if (a.f > b.f) return false
	CMP(x);  CMP(y);  CMP(z);
	CMP(nx); CMP(ny); CMP(nz);
	CMP(u);  CMP(v);
	CMP(weights);
#undef CMP
	return false;
}

template <typename T>
struct InserterWithoutDuplicates
{
	InserterWithoutDuplicates(std::vector<T>& vec) : vec(vec)
	{
	}

	size_t add(const T& val)
	{
		typename std::map<T, size_t>::iterator it = btree.find(val);
		if (it != btree.end())
			return it->second;

		size_t idx = vec.size();
		vec.push_back(val);
		btree.insert(std::make_pair(val, idx));
		return idx;
	}

	std::vector<T>& vec;
	std::map<T, size_t> btree; // for faster lookups (so we can build a duplicate-free list in O(n log n) instead of O(n^2))

private:
	InserterWithoutDuplicates& operator=(const InserterWithoutDuplicates&);
};

void CanonicaliseWeights(std::vector<FCDJointWeightPair>& weights)
{
	// Convert weight-lists into a standard format, so simple vector equality
	// can be used to determine equivalence
	std::sort(weights.begin(), weights.end());
}

void ReindexGeometry(FCDGeometryPolygons* polys, FCDSkinController* skin)
{
	// Given geometry with:
	//   positions, normals, texcoords, bone blends
	// each with their own data array and index array, change it to
	// have a single optimised index array shared by all vertexes.

	FCDGeometryPolygonsInput* inputPosition = polys->FindInput(FUDaeGeometryInput::POSITION);
	FCDGeometryPolygonsInput* inputNormal   = polys->FindInput(FUDaeGeometryInput::NORMAL);
	FCDGeometryPolygonsInput* inputTexcoord = polys->FindInput(FUDaeGeometryInput::TEXCOORD);

	size_t numVertices = polys->GetFaceVertexCount();

	assert(inputPosition->GetIndexCount() == numVertices);
	assert(inputNormal  ->GetIndexCount() == numVertices);
	assert(inputTexcoord->GetIndexCount() == numVertices);

	const uint32* indicesPosition = inputPosition->GetIndices();
	const uint32* indicesNormal   = inputNormal->GetIndices();
	const uint32* indicesTexcoord = inputTexcoord->GetIndices();

	assert(indicesPosition);
	assert(indicesNormal);
	assert(indicesTexcoord); // TODO - should be optional, because textureless meshes aren't unreasonable

	FCDGeometrySource* sourcePosition = inputPosition->GetSource();
	FCDGeometrySource* sourceNormal   = inputNormal  ->GetSource();
	FCDGeometrySource* sourceTexcoord = inputTexcoord->GetSource();

	const float* dataPosition = sourcePosition->GetData();
	const float* dataNormal   = sourceNormal  ->GetData();
	const float* dataTexcoord = sourceTexcoord->GetData();

	if (skin)
	{
#ifndef NDEBUG
		size_t numVertexPositions = sourcePosition->GetDataCount() / sourcePosition->GetStride();
		assert(skin->GetInfluenceCount() == numVertexPositions);
#endif
	}

	uint32 stridePosition = sourcePosition->GetStride();
	uint32 strideNormal   = sourceNormal  ->GetStride();
	uint32 strideTexcoord = sourceTexcoord->GetStride();

	std::vector<uint32> indicesCombined;
	std::vector<VertexData> vertexes;
	InserterWithoutDuplicates<VertexData> inserter(vertexes);

	for (size_t i = 0; i < numVertices; ++i)
	{
		std::vector<FCDJointWeightPair> weights;
		if (skin)
		{
			FCDSkinControllerVertex* influences = skin->GetVertexInfluence(indicesPosition[i]);
			assert(influences != NULL);
			for (size_t j = 0; j < influences->GetPairCount(); ++j)
			{
				FCDJointWeightPair* pair = influences->GetPair(j);
				assert(pair != NULL);
				weights.push_back(*pair);
			}
			CanonicaliseWeights(weights);
		}

		VertexData vtx (
			&dataPosition[indicesPosition[i]*stridePosition],
			&dataNormal  [indicesNormal  [i]*strideNormal],
			&dataTexcoord[indicesTexcoord[i]*strideTexcoord],
			weights
		);
		size_t idx = inserter.add(vtx);
		indicesCombined.push_back((uint32)idx);
	}

	// TODO: rearrange indicesCombined (and rearrange vertexes to match) to use
	// the vertex cache efficiently
	// (<http://home.comcast.net/~tom_forsyth/papers/fast_vert_cache_opt.html> etc)

	FloatList newDataPosition;
	FloatList newDataNormal;
	FloatList newDataTexcoord;
	std::vector<std::vector<FCDJointWeightPair> > newWeightedMatches;

	for (size_t i = 0; i < vertexes.size(); ++i)
	{
		newDataPosition.push_back(vertexes[i].x);
		newDataPosition.push_back(vertexes[i].y);
		newDataPosition.push_back(vertexes[i].z);
		newDataNormal  .push_back(vertexes[i].nx);
		newDataNormal  .push_back(vertexes[i].ny);
		newDataNormal  .push_back(vertexes[i].nz);
		newDataTexcoord.push_back(vertexes[i].u);
		newDataTexcoord.push_back(vertexes[i].v);
		newWeightedMatches.push_back(vertexes[i].weights);
	}

	// (Slightly wasteful to duplicate this array so many times, but FCollada
	// doesn't seem to support multiple inputs with the same source data)
	inputPosition->SetIndices(&indicesCombined.front(), indicesCombined.size());
	inputNormal  ->SetIndices(&indicesCombined.front(), indicesCombined.size());
	inputTexcoord->SetIndices(&indicesCombined.front(), indicesCombined.size());

	sourcePosition->SetData(newDataPosition, 3);
	sourceNormal  ->SetData(newDataNormal,   3);
	sourceTexcoord->SetData(newDataTexcoord, 2);

	if (skin)
	{
		skin->SetInfluenceCount(newWeightedMatches.size());
		for (size_t i = 0; i < newWeightedMatches.size(); ++i)
		{
			skin->GetVertexInfluence(i)->SetPairCount(0);
			for (size_t j = 0; j < newWeightedMatches[i].size(); ++j)
				skin->GetVertexInfluence(i)->AddPair(newWeightedMatches[i][j].jointIndex, newWeightedMatches[i][j].weight);
		}
	}
}
