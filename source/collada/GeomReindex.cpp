/* Copyright (C) 2009 Wildfire Games.
 * This file is part of 0 A.D.
 *
 * 0 A.D. is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * 0 A.D. is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with 0 A.D.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "precompiled.h"

#include "GeomReindex.h"

#include "FCollada.h"
#include "FCDocument/FCDEntity.h"
#include "FCDocument/FCDGeometryMesh.h"
#include "FCDocument/FCDGeometryPolygons.h"
#include "FCDocument/FCDGeometryPolygonsInput.h"
#include "FCDocument/FCDGeometrySource.h"
#include "FCDocument/FCDSkinController.h"

#include <cassert>
#include <vector>
#include <map>
#include <algorithm>

typedef std::pair<float, float> uv_pair_type;

struct VertexData
{
	VertexData(const float* pos, const float* norm, const std::vector<uv_pair_type> &uvs,
		   const std::vector<FCDJointWeightPair>& weights)
		: x(pos[0]), y(pos[1]), z(pos[2]),
		nx(norm[0]), ny(norm[1]), nz(norm[2]),
		uvs(uvs),
		weights(weights)
	{
	}

	float x, y, z;
	float nx, ny, nz;
	std::vector<uv_pair_type> uvs;
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

bool operator==(const uv_pair_type& a, const uv_pair_type& b)
{
	return similar(a.first, b.first) && similar(a.second, b.second);
}

bool operator==(const VertexData& a, const VertexData& b)
{
	return (similar(a.x,  b.x)  && similar(a.y,  b.y)  && similar(a.z,  b.z)
		 && similar(a.nx, b.nx) && similar(a.ny, b.ny) && similar(a.nz, b.nz)
		 && (a.uvs == b.uvs)
		 && (a.weights == b.weights));
}

bool operator<(const VertexData& a, const VertexData& b)
{
#define CMP(f) if (a.f < b.f) return true; if (a.f > b.f) return false
	CMP(x);  CMP(y);  CMP(z);
	CMP(nx); CMP(ny); CMP(nz);
	CMP(uvs);
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

	FCDGeometrySourceList texcoordSources;
	polys->GetParent()->FindSourcesByType(FUDaeGeometryInput::TEXCOORD, texcoordSources);

	FCDGeometrySource* sourcePosition = inputPosition->GetSource();
	FCDGeometrySource* sourceNormal   = inputNormal  ->GetSource();

	const float* dataPosition = sourcePosition->GetData();
	const float* dataNormal   = sourceNormal  ->GetData();

	if (skin)
	{
#ifndef NDEBUG
		size_t numVertexPositions = sourcePosition->GetDataCount() / sourcePosition->GetStride();
		assert(skin->GetInfluenceCount() == numVertexPositions);
#endif
	}

	uint32 stridePosition = sourcePosition->GetStride();
	uint32 strideNormal   = sourceNormal  ->GetStride();

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

		std::vector<uv_pair_type> uvs;
		for (size_t set = 0; set < texcoordSources.size(); ++set)
		{
			const float* dataTexcoord = texcoordSources[set]->GetData();
			uint32 strideTexcoord = texcoordSources[set]->GetStride();

			uv_pair_type p;
			p.first = dataTexcoord[indicesTexcoord[i]*strideTexcoord];
			p.second = dataTexcoord[indicesTexcoord[i]*strideTexcoord + 1];
			uvs.push_back(p);
		}

		VertexData vtx (
			&dataPosition[indicesPosition[i]*stridePosition],
			&dataNormal  [indicesNormal  [i]*strideNormal],
			uvs,
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
		newWeightedMatches.push_back(vertexes[i].weights);
	}

	// (Slightly wasteful to duplicate this array so many times, but FCollada
	// doesn't seem to support multiple inputs with the same source data)
	inputPosition->SetIndices(&indicesCombined.front(), indicesCombined.size());
	inputNormal  ->SetIndices(&indicesCombined.front(), indicesCombined.size());
	inputTexcoord->SetIndices(&indicesCombined.front(), indicesCombined.size());

	for (size_t set = 0; set < texcoordSources.size(); ++set)
	{
		newDataTexcoord.clear();
		for (size_t i = 0; i < vertexes.size(); ++i)
		{
			newDataTexcoord.push_back(vertexes[i].uvs[set].first);
			newDataTexcoord.push_back(vertexes[i].uvs[set].second);
		}
		texcoordSources[set]->SetData(newDataTexcoord, 2);
	}

	sourcePosition->SetData(newDataPosition, 3);
	sourceNormal  ->SetData(newDataNormal,   3);

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
