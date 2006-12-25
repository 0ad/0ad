#include "precompiled.h"

#include "GeomReindex.h"

#include <cassert>

struct VertexData
{
	VertexData(const float* pos, const float* norm, const float* tex, const FCDJointWeightPairList& weights)
		: x(pos[0]), y(pos[1]), z(pos[2]),
		nx(norm[0]), ny(norm[1]), nz(norm[2]),
		u(tex[0]), v(tex[1]),
		weights(weights)
	{
	}

	float x, y, z;
	float nx, ny, nz;
	float u, v;
	FCDJointWeightPairList weights;
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
		std::map<T, size_t>::iterator it = btree.find(val);
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

void CanonicaliseWeights(FCDJointWeightPairList& weights)
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

	UInt32List* indicesPosition = polys->FindIndices(inputPosition);
	UInt32List* indicesNormal   = polys->FindIndices(inputNormal);
	UInt32List* indicesTexcoord = polys->FindIndices(inputTexcoord);

	assert(indicesPosition);
	assert(indicesNormal);
	assert(indicesTexcoord); // TODO - should be optional, because textureless meshes aren't unreasonable

	size_t numVertices = polys->GetFaceVertexCount();

	assert(indicesPosition->size() == numVertices);
	assert(indicesNormal  ->size() == numVertices);
	assert(indicesTexcoord->size() == numVertices);

	FCDGeometrySource* sourcePosition = inputPosition->GetSource();
	FCDGeometrySource* sourceNormal   = inputNormal  ->GetSource();
	FCDGeometrySource* sourceTexcoord = inputTexcoord->GetSource();

	const FloatList& dataPosition = sourcePosition->GetSourceData();
	const FloatList& dataNormal   = sourceNormal  ->GetSourceData();
	const FloatList& dataTexcoord = sourceTexcoord->GetSourceData();

	if (skin)
	{
		size_t numVertexPositions = dataPosition.size() / sourcePosition->GetSourceStride();
		assert(skin->GetVertexInfluenceCount() == numVertexPositions);
	}

	uint32 stridePosition = sourcePosition->GetSourceStride();
	uint32 strideNormal   = sourceNormal  ->GetSourceStride();
	uint32 strideTexcoord = sourceTexcoord->GetSourceStride();

	UInt32List indicesCombined;
	std::vector<VertexData> vertexes;
	InserterWithoutDuplicates<VertexData> inserter(vertexes);

	for (size_t i = 0; i < numVertices; ++i)
	{
		FCDJointWeightPairList weights;
		if (skin)
		{
			weights = *skin->GetInfluences((*indicesPosition)[i]);
			CanonicaliseWeights(weights);
		}

		VertexData vtx (
			&dataPosition[(*indicesPosition)[i]*stridePosition],
			&dataNormal  [(*indicesNormal  )[i]*strideNormal],
			&dataTexcoord[(*indicesTexcoord)[i]*strideTexcoord],
			weights
		);
		size_t idx = inserter.add(vtx);//InsertWithoutDuplicates(vertexes, vtx);
		indicesCombined.push_back((uint32)idx);
	}

	FloatList newDataPosition;
	FloatList newDataNormal;
	FloatList newDataTexcoord;
	FCDWeightedMatches newWeightedMatches;

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
	*indicesPosition = indicesCombined;
	*indicesNormal   = indicesCombined;
	*indicesTexcoord = indicesCombined;

	sourcePosition->SetSourceData(newDataPosition, 3);
	sourceNormal  ->SetSourceData(newDataNormal,   3);
	sourceTexcoord->SetSourceData(newDataTexcoord, 3);
	if (skin)
		skin->GetWeightedMatches() = newWeightedMatches;
}
