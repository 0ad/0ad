/* Copyright (C) 2012 Wildfire Games.
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

#include "PMDConvert.h"
#include "CommonConvert.h"

#include "FCollada.h"
#include "FCDocument/FCDAsset.h"
#include "FCDocument/FCDocument.h"
#include "FCDocument/FCDocumentTools.h"
#include "FCDocument/FCDController.h"
#include "FCDocument/FCDControllerInstance.h"
#include "FCDocument/FCDGeometry.h"
#include "FCDocument/FCDGeometryMesh.h"
#include "FCDocument/FCDGeometryPolygons.h"
#include "FCDocument/FCDGeometryPolygonsInput.h"
#include "FCDocument/FCDGeometryPolygonsTools.h"
#include "FCDocument/FCDGeometrySource.h"
#include "FCDocument/FCDSceneNode.h"
#include "FCDocument/FCDSkinController.h"

#include "StdSkeletons.h"
#include "Decompose.h"
#include "Maths.h"
#include "GeomReindex.h"

#include <cassert>
#include <vector>
#include <algorithm>

const size_t maxInfluences = 4;
struct VertexBlend
{
	uint8 bones[maxInfluences];
	float weights[maxInfluences];
};
VertexBlend defaultInfluences = { { 0xFF, 0xFF, 0xFF, 0xFF }, { 0, 0, 0, 0 } };

struct PropPoint
{
	std::string name;
	float translation[3];
	float orientation[4];
	uint8 bone;
};

// Based on FMVector3::Normalize, but that function uses a static member
// FMVector3::XAxis which causes irritating linker errors. Rather than trying
// to make that XAxis work in a cross-platform way, just reimplement Normalize:
static FMVector3 FMVector3_Normalize(const FMVector3& vec)
{
	float l = vec.Length();
	if (l > 0.0f)
		return FMVector3(vec.x/l, vec.y/l, vec.z/l);
	else
		return FMVector3(1.0f, 0.0f, 0.0f);
}

static void AddStaticPropPoints(std::vector<PropPoint> &propPoints, const FMMatrix44& upAxisTransform, FCDSceneNode* node)
{
	if (node->GetName().find("prop-") == 0 || node->GetName().find("prop_") == 0)
	{
		// Strip off the "prop-" from the name
		std::string propPointName (node->GetName().substr(5));

		Log(LOG_INFO, "Adding prop point %s", propPointName.c_str());

		// CalculateWorldTransform applies transformations recursively for all parents of this node
		// upAxisTransform transforms this node to right-handed Z_UP coordinates

		FMMatrix44 transform = upAxisTransform * node->CalculateWorldTransform();

		HMatrix matrix;
		memcpy(matrix, transform.Transposed().m, sizeof(matrix));

		AffineParts parts;
		decomp_affine(matrix, &parts);

		// Add prop point in game coordinates

		PropPoint p = {
			propPointName,

			// Flip translation across the x-axis by swapping y and z
			{ parts.t.x, parts.t.z, parts.t.y },

			// To convert the quaternions: imagine you're using the axis/angle
			// representation, then swap the y,z basis vectors and change the
			// direction of rotation by negating the angle ( => negating sin(angle)
			// => negating x,y,z => changing (x,y,z,w) to (-x,-z,-y,w)
			// but then (-x,-z,-y,w) == (x,z,y,-w) so do that instead)
			{ parts.q.x, parts.q.z, parts.q.y, -parts.q.w },

			0xff
		};
		propPoints.push_back(p);
	}

	// Search children for prop points
	for (size_t i = 0; i < node->GetChildrenCount(); ++i)
		AddStaticPropPoints(propPoints, upAxisTransform, node->GetChild(i));
}

class PMDConvert
{
public:
	/**
	 * Converts a COLLADA XML document into the PMD mesh format.
	 *
	 * @param input XML document to parse
	 * @param output callback for writing the PMD data; called lots of times
	 *               with small strings
	 * @param xmlErrors output - errors reported by the XML parser
	 * @throws ColladaException on failure
	 */
	static void ColladaToPMD(const char* input, OutputCB& output, std::string& xmlErrors)
	{
		CommonConvert converter(input, xmlErrors);

		if (converter.GetInstance().GetEntity()->GetType() == FCDEntity::GEOMETRY)
		{
			Log(LOG_INFO, "Found static geometry");

			FCDGeometryPolygons* polys = GetPolysFromGeometry((FCDGeometry*)converter.GetInstance().GetEntity());

			// Convert the geometry into a suitable form for the game
			ReindexGeometry(polys);

			std::vector<VertexBlend> boneWeights;	// unused
			std::vector<BoneTransform> boneTransforms;	// unused
			std::vector<PropPoint> propPoints;

			// Get the raw vertex data

			FCDGeometryPolygonsInput* inputPosition = polys->FindInput(FUDaeGeometryInput::POSITION);
			FCDGeometryPolygonsInput* inputNormal   = polys->FindInput(FUDaeGeometryInput::NORMAL);

			const uint32* indicesCombined = inputPosition->GetIndices();
			size_t indicesCombinedCount = inputPosition->GetIndexCount();
			// (ReindexGeometry guarantees position/normal/texcoord have the same indexes)

			FCDGeometrySource* sourcePosition = inputPosition->GetSource();
			FCDGeometrySource* sourceNormal   = inputNormal  ->GetSource();

			FCDGeometrySourceList texcoordSources;
			polys->GetParent()->FindSourcesByType(FUDaeGeometryInput::TEXCOORD, texcoordSources);

			float* dataPosition = sourcePosition->GetData();
			float* dataNormal   = sourceNormal  ->GetData();
			size_t vertexCount = sourcePosition->GetDataCount() / 3;
			assert(sourcePosition->GetDataCount() == vertexCount*3);
			assert(sourceNormal  ->GetDataCount() == vertexCount*3);

			std::vector<float*> dataTexcoords;
			for (size_t i = 0; i < texcoordSources.size(); ++i)
			{
				dataTexcoords.push_back(texcoordSources[i]->GetData());
			}

			// Transform mesh coordinate system to game coordinates
			// (doesn't modify prop points)

			TransformStaticModel(dataPosition, dataNormal, vertexCount, converter.GetEntityTransform(), converter.IsYUp());

			// Add static prop points
			//	which are empty child nodes of the main parent

			// Default prop points are already given in game coordinates
			AddDefaultPropPoints(propPoints);

			// Calculate transform to convert from COLLADA-defined up_axis to Z-up because
			//	it's relatively straightforward to convert that to game coordinates
			FMMatrix44 upAxisTransform = FMMatrix44_Identity;
			if (converter.IsYUp())
			{
				// Prop points are rotated -90 degrees about the X-axis, reverse that rotation
				// (do this once now because it's easier than messing with quaternions later)
				upAxisTransform = FMMatrix44::XAxisRotationMatrix(1.57f);
			}

			AddStaticPropPoints(propPoints, upAxisTransform, converter.GetInstance().GetParent());

			WritePMD(output, indicesCombined, indicesCombinedCount, dataPosition, dataNormal, dataTexcoords, vertexCount, boneWeights, boneTransforms, propPoints);
		}
		else if (converter.GetInstance().GetType() == FCDEntityInstance::CONTROLLER)
		{
			Log(LOG_INFO, "Found skinned geometry");

			FCDControllerInstance& controllerInstance = static_cast<FCDControllerInstance&>(converter.GetInstance());

			// (NB: GetType is deprecated and should be replaced with HasType,
			// except that has irritating linker errors when using a DLL, so don't
			// bother)

			assert(converter.GetInstance().GetEntity()->GetType() == FCDEntity::CONTROLLER); // assume this is always true?
			FCDController* controller = static_cast<FCDController*>(converter.GetInstance().GetEntity());

			FCDSkinController* skin = controller->GetSkinController();
			REQUIRE(skin != NULL, "is skin controller");

			FixSkeletonRoots(controllerInstance);

			// Data for joints is stored in two places - avoid overflows by limiting
			// to the minimum of the two sizes, and warn if they're different (which
			// happens in practice for slightly-broken meshes)
			size_t jointCount = std::min(skin->GetJointCount(), controllerInstance.GetJointCount());
			if (skin->GetJointCount() != controllerInstance.GetJointCount())
			{
				Log(LOG_WARNING, "Mismatched bone counts (skin has %d, skeleton has %d)",
					skin->GetJointCount(), controllerInstance.GetJointCount());
				for (size_t i = 0; i < skin->GetJointCount(); ++i)
					Log(LOG_INFO, "Skin joint %d: %s", i, skin->GetJoint(i)->GetId().c_str());
				for (size_t i = 0; i < controllerInstance.GetJointCount(); ++i)
					Log(LOG_INFO, "Skeleton joint %d: %s", i, controllerInstance.GetJoint(i)->GetName().c_str());
			}

			// Get the skinned mesh for this entity
			FCDGeometry* baseGeometry = controller->GetBaseGeometry();
			REQUIRE(baseGeometry != NULL, "controller has base geometry");
			FCDGeometryPolygons* polys = GetPolysFromGeometry(baseGeometry);

			// Make sure it doesn't use more bones per vertex than the game can handle
			SkinReduceInfluences(skin, maxInfluences, 0.001f);

			// Convert the geometry into a suitable form for the game
			ReindexGeometry(polys, skin);

			const Skeleton& skeleton = FindSkeleton(controllerInstance);

			// Convert the bone influences into VertexBlend structures for the PMD:

			bool hasComplainedAboutNonexistentJoints = false; // because we want to emit a warning only once

			std::vector<VertexBlend> boneWeights; // one per vertex

			const FCDSkinControllerVertex* vertexInfluences = skin->GetVertexInfluences();
			for (size_t i = 0; i < skin->GetInfluenceCount(); ++i)
			{
				VertexBlend influences = defaultInfluences;

				assert(vertexInfluences[i].GetPairCount() <= maxInfluences);
					// guaranteed by ReduceInfluences; necessary for avoiding
					// out-of-bounds writes to the VertexBlend

				if (vertexInfluences[i].GetPairCount() == 0)
				{
					// Blender exports some models with vertices that have no influences,
					//	which I've not found details about in the COLLADA spec, however,
					//	it seems to work OK to treat these vertices the same as if they
					//	were only influenced by the bind-shape matrix (see comment below),
					//	so we use the same special case here.
					influences.bones[0] = (uint8)jointCount;
					influences.weights[0] = 1.0f;
				}

				for (size_t j = 0; j < vertexInfluences[i].GetPairCount(); ++j)
				{
					if (vertexInfluences[i].GetPair(j)->jointIndex == -1)
					{
						// This is a special case we must handle, according to the COLLADA spec:
						//	"An index of -1 into the array of joints refers to the bind shape"
						//
						//	which basically means when skinning the vertex it's relative to the
						//	bind-shape transform instead of an animated bone. Since our skinning
						//	is in world space, we will have already applied the bind-shape transform,
						//	so we don't have to worry about that, though we DO have to apply the
						//	world space transform of the model for the indicated vertex.
						//
						//	To indicate this special case, we use a bone ID set to the total number
						//	of bones in the model, which will have a special "bone matrix" reserved
						//	that contains the world space transform of the model during skinning.
						//	(see http://trac.wildfiregames.com/ticket/1012)
						influences.bones[j] = (uint8)jointCount;
						influences.weights[j] = vertexInfluences[i].GetPair(j)->weight;
					}
					else
					{
						// Check for less than 254 joints because we store them in a u8,
						//	0xFF is a reserved value (no influence), and we reserve one slot
						//	for the above special case.
						uint32 jointIdx = vertexInfluences[i].GetPair(j)->jointIndex;
						REQUIRE(jointIdx < 0xFE, "sensible number of joints (<254)");

						// Find the joint on the skeleton, after checking it really exists
						FCDSceneNode* joint = NULL;
						if (jointIdx < controllerInstance.GetJointCount())
							joint = controllerInstance.GetJoint(jointIdx);

						// Complain on error
						if (! joint)
						{
							if (! hasComplainedAboutNonexistentJoints)
							{
								Log(LOG_WARNING, "Vertexes influenced by nonexistent joint");
								hasComplainedAboutNonexistentJoints = true;
							}
							continue;
						}

						// Store into the VertexBlend
						int boneId = skeleton.GetBoneID(joint->GetName().c_str());
						if (boneId < 0)
						{
							// The relevant joint does exist, but it's not a recognised
							// bone in our chosen skeleton structure
							Log(LOG_ERROR, "Vertex influenced by unrecognised bone '%s'", joint->GetName().c_str());
							continue;
						}

						influences.bones[j] = (uint8)boneId;
						influences.weights[j] = vertexInfluences[i].GetPair(j)->weight;
					}
				}

				boneWeights.push_back(influences);
			}

			// Convert the bind pose into BoneTransform structures for the PMD:

			BoneTransform boneDefault  = { { 0, 0, 0 }, { 0, 0, 0, 1 } }; // identity transform
			std::vector<BoneTransform> boneTransforms (skeleton.GetBoneCount(), boneDefault);

			for (size_t i = 0; i < jointCount; ++i)
			{
				FCDSceneNode* joint = controllerInstance.GetJoint(i);

				int boneId = skeleton.GetRealBoneID(joint->GetName().c_str());
				if (boneId < 0)
				{
					// unrecognised joint - it's probably just a prop point
					// or something, so ignore it
					continue;
				}

				FMMatrix44 bindPose = skin->GetJoint(i)->GetBindPoseInverse().Inverted();

				HMatrix matrix;
				memcpy(matrix, bindPose.Transposed().m, sizeof(matrix));
					// set matrix = bindPose^T, to match what decomp_affine wants

				AffineParts parts;
				decomp_affine(matrix, &parts);

				BoneTransform b = {
					{ parts.t.x, parts.t.y, parts.t.z },
					{ parts.q.x, parts.q.y, parts.q.z, parts.q.w }
				};

				boneTransforms[boneId] = b;
			}

			// Construct the list of prop points.
			// Currently takes all objects that are directly attached to a
			// standard bone, and whose name begins with "prop-" or "prop_".

			std::vector<PropPoint> propPoints;
			AddDefaultPropPoints(propPoints);

			for (size_t i = 0; i < jointCount; ++i)
			{
				FCDSceneNode* joint = controllerInstance.GetJoint(i);

				int boneId = skeleton.GetBoneID(joint->GetName().c_str());
				if (boneId < 0)
				{
					// unrecognised joint name - ignore, same as before
					continue;
				}

				// Check all the objects attached to this bone
				for (size_t j = 0; j < joint->GetChildrenCount(); ++j)
				{
					FCDSceneNode* child = joint->GetChild(j);
					if (child->GetName().find("prop-") != 0 && child->GetName().find("prop_") != 0)
					{
						// doesn't begin with "prop-", so skip it
						continue;
					}
					// Strip off the "prop-" from the name
					std::string propPointName (child->GetName().substr(5));

					Log(LOG_INFO, "Adding prop point %s", propPointName.c_str());

					// Get translation and orientation of local transform

					FMMatrix44 localTransform = child->ToMatrix();

					HMatrix matrix;
					memcpy(matrix, localTransform.Transposed().m, sizeof(matrix));

					AffineParts parts;
					decomp_affine(matrix, &parts);

					// Add prop point to list

					PropPoint p = {
						propPointName,
						{ parts.t.x, parts.t.y, parts.t.z },
						{ parts.q.x, parts.q.y, parts.q.z, parts.q.w },
						(uint8)boneId
					};
					propPoints.push_back(p);
				}
			}

			// Get the raw vertex data

			FCDGeometryPolygonsInput* inputPosition = polys->FindInput(FUDaeGeometryInput::POSITION);
			FCDGeometryPolygonsInput* inputNormal   = polys->FindInput(FUDaeGeometryInput::NORMAL);

			const uint32* indicesCombined = inputPosition->GetIndices();
			size_t indicesCombinedCount = inputPosition->GetIndexCount();
			// (ReindexGeometry guarantees position/normal/texcoord have the same indexes)

			FCDGeometrySource* sourcePosition = inputPosition->GetSource();
			FCDGeometrySource* sourceNormal   = inputNormal  ->GetSource();

			FCDGeometrySourceList texcoordSources;
			polys->GetParent()->FindSourcesByType(FUDaeGeometryInput::TEXCOORD, texcoordSources);

			float* dataPosition = sourcePosition->GetData();
			float* dataNormal   = sourceNormal  ->GetData();
			size_t vertexCount = sourcePosition->GetDataCount() / 3;
			assert(sourcePosition->GetDataCount() == vertexCount*3);
			assert(sourceNormal  ->GetDataCount() == vertexCount*3);

			std::vector<float*> dataTexcoords;
			for (size_t i = 0; i < texcoordSources.size(); ++i)
			{
				dataTexcoords.push_back(texcoordSources[i]->GetData());
			}

			// Transform model coordinate system to game coordinates

			TransformSkinnedModel(dataPosition, dataNormal, vertexCount, boneTransforms, propPoints,
				converter.GetEntityTransform(), skin->GetBindShapeTransform(),
				converter.IsYUp(), converter.IsXSI());

			WritePMD(output, indicesCombined, indicesCombinedCount, dataPosition, dataNormal, dataTexcoords, vertexCount, boneWeights, boneTransforms, propPoints);
		}
		else
		{
			throw ColladaException("Unrecognised object type");
		}

	}

	/**
	 * Adds the default "root" prop-point.
	 */
	static void AddDefaultPropPoints(std::vector<PropPoint>& propPoints)
	{
		PropPoint root;
		root.name = "root";
		root.translation[0] = root.translation[1] = root.translation[2] = 0.0f;
		root.orientation[0] = root.orientation[1] = root.orientation[2] = 0.0f;
		root.orientation[3] = 1.0f;
		root.bone = 0xFF;
		propPoints.push_back(root);
	}

	/**
	 * Writes the model data in the PMD format.
	 */
	static void WritePMD(OutputCB& output,
		const uint32* indices, size_t indexCount,
		const float* position, const float* normal,
		const std::vector<float*>& texcoords,
		size_t vertexCount,
		const std::vector<VertexBlend>& boneWeights, const std::vector<BoneTransform>& boneTransforms,
		const std::vector<PropPoint>& propPoints)
	{
		static const VertexBlend noBlend = { { 0xFF, 0xFF, 0xFF, 0xFF }, { 0, 0, 0, 0 } };

		size_t faceCount = indexCount/3;
		size_t boneCount = boneTransforms.size();
		if (boneCount)
			assert(boneWeights.size() == vertexCount);

		size_t propPointsSize = 0; // can't calculate this statically, so loop over all the prop points
		for (size_t i = 0; i < propPoints.size(); ++i)
		{
			propPointsSize += 4 + propPoints[i].name.length();
			propPointsSize += 3*4 + 4*4 + 1;
		}

		output("PSMD", 4);  // magic number
		write(output, (uint32)4); // version number
		write(output, (uint32)(
			// for UVs, we add one uint32 (i.e. 4 bytes) per model that gives the number of
			// texcoord sets in the model, plus 2 floats per new UV
			// pair per vertex (i.e. 8 bytes * number of pairs * vertex count)
			4 + 11*4*vertexCount + 4 + 8*texcoords.size()*vertexCount + // vertices
			4 + 6*faceCount + // faces
			4 + 7*4*boneCount + // bones
			4 + propPointsSize // props
			)); // data size

		// Vertex data
		write<uint32>(output, (uint32)vertexCount);
		write<uint32>(output, (uint32)texcoords.size()); // UV pairs per vertex
		for (size_t i = 0; i < vertexCount; ++i)
		{
			output((char*)&position[i*3], 12);
			output((char*)&normal  [i*3], 12);

			for (size_t s = 0; s < texcoords.size(); ++s)
			{
				output((char*)&texcoords[s][i*2], 8);
			}

			if (boneCount)
				write(output, boneWeights[i]);
			else
				write(output, noBlend);
		}

		// Face data
		write(output, (uint32)faceCount);
		for (size_t i = 0; i < indexCount; ++i)
		{
			write(output, (uint16)indices[i]);
		}

		// Bones data
		write(output, (uint32)boneCount);
		for (size_t i = 0; i < boneCount; ++i)
		{
			output((char*)&boneTransforms[i], 7*4);
		}

		// Prop points data
		write(output, (uint32)propPoints.size());
		for (size_t i = 0; i < propPoints.size(); ++i)
		{
			uint32 nameLen = (uint32)propPoints[i].name.length();
			write(output, nameLen);
			output(propPoints[i].name.c_str(), nameLen);
			write(output, propPoints[i].translation);
			write(output, propPoints[i].orientation);
			write(output, propPoints[i].bone);
		}
	}

	static FCDGeometryPolygons* GetPolysFromGeometry(FCDGeometry* geom)
	{
		REQUIRE(geom->IsMesh(), "geometry is mesh");
		FCDGeometryMesh* mesh = geom->GetMesh();

 		if (! mesh->IsTriangles())
 			FCDGeometryPolygonsTools::Triangulate(mesh);

		REQUIRE(mesh->IsTriangles(), "mesh is made of triangles");
		REQUIRE(mesh->GetPolygonsCount() == 1, "mesh has single set of polygons");
		FCDGeometryPolygons* polys = mesh->GetPolygons(0);
		REQUIRE(polys->FindInput(FUDaeGeometryInput::POSITION) != NULL, "mesh has vertex positions");
		REQUIRE(polys->FindInput(FUDaeGeometryInput::NORMAL) != NULL, "mesh has vertex normals");
		REQUIRE(polys->FindInput(FUDaeGeometryInput::TEXCOORD) != NULL, "mesh has vertex tex coords");
		return polys;
	}

	/**
	 * Applies world-space transform to vertex data and transforms Collada's right-handed
	 *	Y-up / Z-up coordinates to the game's left-handed Y-up coordinate system
	 *
	 * TODO: Maybe we should use FCDocumentTools::StandardizeUpAxisAndLength in addition
	 *		to this, so we'd only have one up-axis case to worry about, but it doesn't seem to
	 *		correctly adjust the prop points in Y_UP models.
	 */
	static void TransformStaticModel(float* position, float* normal, size_t vertexCount,
		const FMMatrix44& transform, bool yUp)
	{
		for (size_t i = 0; i < vertexCount; ++i)
		{
			FMVector3 pos (&position[i*3], 0);
			FMVector3 norm (&normal[i*3], 0);

			// Apply the scene-node transforms
			pos = transform.TransformCoordinate(pos);
			norm = FMVector3_Normalize(transform.TransformVector(norm));

			// Convert from right-handed Y_UP or Z_UP to the game's coordinate system (left-handed Y-up)

			if (yUp)
			{
				pos.z = -pos.z;
				norm.z = -norm.z;
			}
			else
			{
				std::swap(pos.y, pos.z);
				std::swap(norm.y, norm.z);
			}

			// Copy back to array

			position[i*3] = pos.x;
			position[i*3+1] = pos.y;
			position[i*3+2] = pos.z;

			normal[i*3] = norm.x;
			normal[i*3+1] = norm.y;
			normal[i*3+2] = norm.z;
		}
	}

	/**
	 * Applies world-space transform to vertex data and transforms Collada's right-handed
	 *	Y-up / Z-up coordinates to the game's left-handed Y-up coordinate system
	 *
	 * TODO: Maybe we should use FCDocumentTools::StandardizeUpAxisAndLength in addition
	 *		to this, so we'd only have one up-axis case to worry about, but it doesn't seem to
	 *		correctly adjust the prop points in Y_UP models.
	 */
	static void TransformSkinnedModel(float* position, float* normal, size_t vertexCount,
		std::vector<BoneTransform>& bones, std::vector<PropPoint>& propPoints,
		const FMMatrix44& transform, const FMMatrix44& bindTransform, bool yUp, bool isXSI)
	{
		FMMatrix44 scaledTransform; // for vertexes
		FMMatrix44 scaleMatrix; // for bones

		// HACK: see comment in PSAConvert::TransformVertices
		if (isXSI)
		{
			scaleMatrix = DecomposeToScaleMatrix(transform);
			scaledTransform = DecomposeToScaleMatrix(bindTransform) * transform;
		}
		else
		{
			scaleMatrix = FMMatrix44_Identity;
			scaledTransform = bindTransform;
		}

		// Update the vertex positions and normals
		for (size_t i = 0; i < vertexCount; ++i)
		{
			FMVector3 pos (&position[i*3], 0);
			FMVector3 norm (&normal[i*3], 0);

			// Apply the scene-node transforms
			pos = scaledTransform.TransformCoordinate(pos);
			norm = FMVector3_Normalize(scaledTransform.TransformVector(norm));

			// Convert from right-handed Y_UP or Z_UP to the game's coordinate system (left-handed Y-up)

			if (yUp)
			{
				pos.z = -pos.z;
				norm.z = -norm.z;
			}
			else
			{
				std::swap(pos.y, pos.z);
				std::swap(norm.y, norm.z);
			}

			// and copy back into the original array

			position[i*3] = pos.x;
			position[i*3+1] = pos.y;
			position[i*3+2] = pos.z;

			normal[i*3] = norm.x;
			normal[i*3+1] = norm.y;
			normal[i*3+2] = norm.z;
		}

		TransformBones(bones, scaleMatrix, yUp);

		// And do the same for prop points
		for (size_t i = 0; i < propPoints.size(); ++i)
		{
			if (yUp)
			{
				propPoints[i].translation[0] = -propPoints[i].translation[0];
				propPoints[i].orientation[0] = -propPoints[i].orientation[0];
				propPoints[i].orientation[3] = -propPoints[i].orientation[3];
			}
			else
			{
				// Flip translation across the x-axis by swapping y and z
				std::swap(propPoints[i].translation[1], propPoints[i].translation[2]);

				// To convert the quaternions: imagine you're using the axis/angle
				// representation, then swap the y,z basis vectors and change the
				// direction of rotation by negating the angle ( => negating sin(angle)
				// => negating x,y,z => changing (x,y,z,w) to (-x,-z,-y,w)
				// but then (-x,-z,-y,w) == (x,z,y,-w) so do that instead)
				std::swap(propPoints[i].orientation[1], propPoints[i].orientation[2]);
				propPoints[i].orientation[3] = -propPoints[i].orientation[3];
			}
		}

	}
};


// The above stuff is just in a class since I don't like having to bother
// with forward declarations of functions - but provide the plain function
// interface here:

void ColladaToPMD(const char* input, OutputCB& output, std::string& xmlErrors)
{
	PMDConvert::ColladaToPMD(input, output, xmlErrors);
}
