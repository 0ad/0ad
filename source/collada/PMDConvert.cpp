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

			FCDGeometryPolygonsInput* inputPosition = polys->FindInput(FUDaeGeometryInput::POSITION);
			FCDGeometryPolygonsInput* inputNormal   = polys->FindInput(FUDaeGeometryInput::NORMAL);
			FCDGeometryPolygonsInput* inputTexcoord = polys->FindInput(FUDaeGeometryInput::TEXCOORD);

			const uint32* indicesCombined = inputPosition->GetIndices();
			size_t indicesCombinedCount = inputPosition->GetIndexCount();
			// (ReindexGeometry guarantees position/normal/texcoord have the same indexes)

			FCDGeometrySource* sourcePosition = inputPosition->GetSource();
			FCDGeometrySource* sourceNormal   = inputNormal  ->GetSource();
			FCDGeometrySource* sourceTexcoord = inputTexcoord->GetSource();

			float* dataPosition = sourcePosition->GetData();
			float* dataNormal   = sourceNormal  ->GetData();
			float* dataTexcoord = sourceTexcoord->GetData();
			size_t vertexCount = sourcePosition->GetDataCount() / 3;
			assert(sourcePosition->GetDataCount() == vertexCount*3);
			assert(sourceNormal  ->GetDataCount() == vertexCount*3);
			assert(sourceTexcoord->GetDataCount() == vertexCount*2);

			TransformVertices(dataPosition, dataNormal, vertexCount, converter.GetEntityTransform(), converter.IsYUp());

			std::vector<VertexBlend> boneWeights;
			std::vector<BoneTransform> boneTransforms;
			std::vector<PropPoint> propPoints;
			AddDefaultPropPoints(propPoints);

			WritePMD(output, indicesCombined, indicesCombinedCount, dataPosition, dataNormal, dataTexcoord, vertexCount, boneWeights, boneTransforms, propPoints);
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

				for (size_t j = 0; j < vertexInfluences[i].GetPairCount(); ++j)
				{
					uint32 jointIdx = vertexInfluences[i].GetPair(j)->jointIndex;
					REQUIRE(jointIdx <= 0xFF, "sensible number of joints (<256)"); // because we only have a u8 to store them in

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
			// standard bone, and whose name begins with "prop-".

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
					if (child->GetName().find("prop-") != 0)
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
			FCDGeometryPolygonsInput* inputTexcoord = polys->FindInput(FUDaeGeometryInput::TEXCOORD);


			const uint32* indicesCombined = inputPosition->GetIndices();
			size_t indicesCombinedCount = inputPosition->GetIndexCount();
			// (ReindexGeometry guarantees position/normal/texcoord have the same indexes)

			FCDGeometrySource* sourcePosition = inputPosition->GetSource();
			FCDGeometrySource* sourceNormal   = inputNormal  ->GetSource();
			FCDGeometrySource* sourceTexcoord = inputTexcoord->GetSource();

			float* dataPosition = sourcePosition->GetData();
			float* dataNormal   = sourceNormal  ->GetData();
			float* dataTexcoord = sourceTexcoord->GetData();
			size_t vertexCount = sourcePosition->GetDataCount() / 3;
			assert(sourcePosition->GetDataCount() == vertexCount*3);
			assert(sourceNormal  ->GetDataCount() == vertexCount*3);
			assert(sourceTexcoord->GetDataCount() == vertexCount*2);

			TransformVertices(dataPosition, dataNormal, vertexCount, boneTransforms, propPoints,
				converter.GetEntityTransform(), skin->GetBindShapeTransform(),
				converter.IsYUp(), converter.IsXSI());

			WritePMD(output, indicesCombined, indicesCombinedCount, dataPosition, dataNormal, dataTexcoord, vertexCount, boneWeights, boneTransforms, propPoints);
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
		const float* position, const float* normal, const float* texcoord, size_t vertexCount,
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
		write(output, (uint32)3); // version number
		write(output, (uint32)(
			4 + 13*4*vertexCount + // vertices
			4 + 6*faceCount + // faces
			4 + 7*4*boneCount + // bones
			4 + propPointsSize // props
			)); // data size

		// Vertex data
		write<uint32>(output, (uint32)vertexCount);
		for (size_t i = 0; i < vertexCount; ++i)
		{
			output((char*)&position[i*3], 12);
			output((char*)&normal  [i*3], 12);
			output((char*)&texcoord[i*2],  8);
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
			output((char*)propPoints[i].name.c_str(), nameLen);
			write(output, propPoints[i].translation);
			write(output, propPoints[i].orientation);
			write(output, propPoints[i].bone);
		}
	}

	static FCDGeometryPolygons* GetPolysFromGeometry(FCDGeometry* geom)
	{
		REQUIRE(geom->IsMesh(), "geometry is mesh");
		FCDGeometryMesh* mesh = geom->GetMesh();

// 		if (! mesh->IsTriangles())
// 			FCDGeometryPolygonsTools::Triangulate(mesh);
		// disabled for now - just let the exporter triangulate the mesh
		
		REQUIRE(mesh->IsTriangles(), "mesh is made of triangles");
		REQUIRE(mesh->GetPolygonsCount() == 1, "mesh has single set of polygons");
		FCDGeometryPolygons* polys = mesh->GetPolygons(0);
		REQUIRE(polys->FindInput(FUDaeGeometryInput::POSITION) != NULL, "mesh has vertex positions");
		REQUIRE(polys->FindInput(FUDaeGeometryInput::NORMAL) != NULL, "mesh has vertex normals");
		REQUIRE(polys->FindInput(FUDaeGeometryInput::TEXCOORD) != NULL, "mesh has vertex tex coords");
		return polys;
	}

	/**
	 * Applies world-space transform to vertex data, and flips into other-handed
	 * coordinate space.
	 */
	static void TransformVertices(float* position, float* normal, size_t vertexCount,
		const FMMatrix44& transform, bool yUp)
	{
		for (size_t i = 0; i < vertexCount; ++i)
		{
			FMVector3 pos (&position[i*3], 0);
			FMVector3 norm (&normal[i*3], 0);

			// Apply the scene-node transforms
			pos = transform.TransformCoordinate(pos);
			norm = FMVector3_Normalize(transform.TransformVector(norm));

			// Convert from Y_UP or Z_UP to the game's coordinate system

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

	static void TransformVertices(float* position, float* normal, size_t vertexCount,
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

			// Convert from Y_UP or Z_UP to the game's coordinate system

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
				std::swap(propPoints[i].translation[1], propPoints[i].translation[2]);
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
