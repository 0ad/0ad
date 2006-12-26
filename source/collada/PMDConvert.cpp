#include "precompiled.h"

#include "PMDConvert.h"
#include "CommonConvert.h"

#include "FCollada.h"
#include "FCDocument/FCDocument.h"
#include "FCDocument/FCDController.h"
#include "FCDocument/FCDGeometry.h"
#include "FCDocument/FCDGeometryMesh.h"
#include "FCDocument/FCDGeometryPolygons.h"
#include "FCDocument/FCDGeometrySource.h"
#include "FCDocument/FCDSceneNode.h"
#include "FCDocument/FCDSkinController.h"

#include "StdSkeletons.h"
#include "Decompose.h"
#include "Maths.h"
#include "GeomReindex.h"

#include <cassert>
#include <vector>

const int maxInfluences = 4;
struct VertexBlend
{
	uint8 bones[maxInfluences];
	float weights[maxInfluences];
};
VertexBlend defaultInfluences = { { 0xFF, 0xFF, 0xFF, 0xFF }, { 0, 0, 0, 0 } };

struct BoneTransform
{
	float translation[3];
	float orientation[4];
};


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
		FUStatus ret;

		// Grab all the error output from libxml2. Be careful to never use
		// libxml2 outside this function without having first set/reset the
		// errorfunc (since xmlErrors won't be valid any more).
		xmlSetGenericErrorFunc(&xmlErrors, &errorHandler);

		std::auto_ptr<FCDocument> doc (FCollada::NewTopDocument());
		REQUIRE_SUCCESS(doc->LoadFromText("", input));

		FCDSceneNode* root = doc->GetVisualSceneRoot();

		// Find the instance to convert
		FCDEntityInstance* instance;
		FMMatrix44 transform;
		if (! FindSingleInstance(root, instance, transform))
			throw ColladaException("Couldn't find object to convert");

		assert(instance);
		Log(LOG_INFO, "Converting '%s'", instance->GetEntity()->GetName().c_str());

		if (instance->GetEntity()->GetType() == FCDEntity::GEOMETRY)
		{
			Log(LOG_INFO, "Found static geometry");

			FCDGeometryPolygons* polys = GetPolysFromGeometry((FCDGeometry*)instance->GetEntity());

			// Convert the geometry into a suitable form for the game
			ReindexGeometry(polys);

			FCDGeometryPolygonsInput* inputPosition = polys->FindInput(FUDaeGeometryInput::POSITION);
			FCDGeometryPolygonsInput* inputNormal   = polys->FindInput(FUDaeGeometryInput::NORMAL);
			FCDGeometryPolygonsInput* inputTexcoord = polys->FindInput(FUDaeGeometryInput::TEXCOORD);

			UInt32List* indicesCombined = polys->FindIndices(inputPosition); // guaranteed by ReindexGeometry

			FCDGeometrySource* sourcePosition = inputPosition->GetSource();
			FCDGeometrySource* sourceNormal   = inputNormal  ->GetSource();
			FCDGeometrySource* sourceTexcoord = inputTexcoord->GetSource();

			FloatList& dataPosition = sourcePosition->GetSourceData();
			FloatList& dataNormal   = sourceNormal  ->GetSourceData();
			FloatList& dataTexcoord = sourceTexcoord->GetSourceData();

			TransformVertices(dataPosition, dataNormal, transform);

			std::vector<VertexBlend> boneWeights;
			std::vector<BoneTransform> boneTransforms;

			WritePMD(output, *indicesCombined, dataPosition, dataNormal, dataTexcoord, boneWeights, boneTransforms);
		}
		else if (instance->GetEntity()->GetType() == FCDEntity::CONTROLLER)
		{
			FCDController* controller = (FCDController*)instance->GetEntity();

			REQUIRE(controller->HasSkinController(), "has skin controller");
			FCDSkinController* skin = controller->GetSkinController();

			// Get the skinned mesh for this entity
			FCDEntity* baseTarget = controller->GetBaseTarget();
			REQUIRE(baseTarget->GetType() == FCDEntity::GEOMETRY, "base target is geometry");
			FCDGeometryPolygons* polys = GetPolysFromGeometry((FCDGeometry*)baseTarget);

			// Make sure it doesn't use more bones per vertex than the game can handle
			SkinReduceInfluences(skin, maxInfluences, 0.001f);

			// Convert the geometry into a suitable form for the game
			ReindexGeometry(polys, skin);

			// Convert the bone influences into VertexBlend structures for the PMD:

			bool hasComplainedAboutNonexistentJoints = false;

			std::vector<VertexBlend> boneWeights; // one per vertex

			const FCDWeightedMatches& vertexInfluences = skin->GetVertexInfluences();
			for (size_t i = 0; i < vertexInfluences.size(); ++i)
			{
				VertexBlend influences = defaultInfluences;

				assert(vertexInfluences[i].size() <= maxInfluences); // guaranteed by ReduceInfluences
				for (size_t j = 0; j < vertexInfluences[i].size(); ++j)
				{
					uint32 jointIdx = vertexInfluences[i][j].jointIndex;
					REQUIRE(jointIdx <= 0xFF, "sensible number of joints");
					FCDSceneNode* joint = skin->GetJoint(jointIdx)->joint;
					if (! joint)
					{
						if (! hasComplainedAboutNonexistentJoints)
						{
							Log(LOG_WARNING, "Vertexes influenced by nonexistent joint");
							hasComplainedAboutNonexistentJoints = true;
						}
						continue;
					}
					int boneId = StdSkeletons::FindStandardBoneID(joint->GetName());
					REQUIRE(boneId >= 0, "recognised bone name");
					influences.bones[j] = (uint8)boneId;
					influences.weights[j] = vertexInfluences[i][j].weight;
				}

				boneWeights.push_back(influences);
			}

			BoneTransform boneDefault  = { { 0, 0, 0 }, { 0, 0, 0, 1 } };
			std::vector<BoneTransform> boneTransforms (StdSkeletons::GetBoneCount(), boneDefault);

			transform = skin->GetBindShapeTransform();

			for (size_t i = 0; i < skin->GetJointCount(); ++i)
			{
				FCDJointMatrixPair* joint = skin->GetJoint(i);
				
				if (! joint->joint)
				{
					Log(LOG_WARNING, "Skin has nonexistent joint");
					continue;
				}

				FMMatrix44 bindPose = joint->invertedBindPose.Inverted();

				HMatrix matrix;
				memcpy(matrix, bindPose.Transposed().m, sizeof(matrix));
					// matrix = bindPose^T, to match what decomp_affine wants

				AffineParts parts;
				decomp_affine(matrix, &parts);

				BoneTransform b = {
					{ parts.t.x, parts.t.y, parts.t.z },
					{ parts.q.x, parts.q.y, parts.q.z, parts.q.w }
				};

				int boneId = StdSkeletons::FindStandardBoneID(joint->joint->GetName());
				REQUIRE(boneId >= 0, "recognised bone name");
				boneTransforms[boneId] = b;
			}

			FCDGeometryPolygonsInput* inputPosition = polys->FindInput(FUDaeGeometryInput::POSITION);
			FCDGeometryPolygonsInput* inputNormal   = polys->FindInput(FUDaeGeometryInput::NORMAL);
			FCDGeometryPolygonsInput* inputTexcoord = polys->FindInput(FUDaeGeometryInput::TEXCOORD);

			UInt32List* indicesCombined = polys->FindIndices(inputPosition); // guaranteed by ReindexGeometry

			FCDGeometrySource* sourcePosition = inputPosition->GetSource();
			FCDGeometrySource* sourceNormal   = inputNormal  ->GetSource();
			FCDGeometrySource* sourceTexcoord = inputTexcoord->GetSource();

			FloatList& dataPosition = sourcePosition->GetSourceData();
			FloatList& dataNormal   = sourceNormal  ->GetSourceData();
			FloatList& dataTexcoord = sourceTexcoord->GetSourceData();

			TransformVertices(dataPosition, dataNormal, boneTransforms, transform);

			WritePMD(output, *indicesCombined, dataPosition, dataNormal, dataTexcoord, boneWeights, boneTransforms);
		}
		else
		{
			throw ColladaException("Unrecognised object type");
		}

	}

	/**
	 * Writes the model data in the PMD format.
	 */
	static void WritePMD(OutputCB& output,
		const UInt32List& indices,
		const FloatList& position, const FloatList& normal, const FloatList& texcoord,
		const std::vector<VertexBlend>& boneWeights, const std::vector<BoneTransform>& boneTransforms)
	{
		static const VertexBlend noBlend = { { 0xFF, 0xFF, 0xFF, 0xFF }, { 0, 0, 0, 0 } };

		size_t vertexCount = position.size()/3;
		size_t faceCount = indices.size()/3;
		size_t boneCount = boneTransforms.size();
		if (boneCount)
			assert(boneWeights.size() == vertexCount);

		output("PSMD", 4);  // magic number
		write<uint32>(output, 3); // version number
		write<uint32>(output, (uint32)(
			4 + 13*4*vertexCount + // vertices
			4 + 6*faceCount + // faces
			4 + 7*4*boneCount + // bones
			4 + 0 // props
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
		write<uint32>(output, (uint32)faceCount);
		for (size_t i = 0; i < indices.size(); ++i)
		{
			write(output, (uint16)indices[i]);
		}

		// Bones data
		write<uint32>(output, (uint32)boneCount);
		for (size_t i = 0; i < boneCount; ++i)
		{
			output((char*)&boneTransforms[i], 7*4);
		}

		// Prop points data
		write<uint32>(output, 0);
	}

	static FCDGeometryPolygons* GetPolysFromGeometry(FCDGeometry* geom)
	{
		REQUIRE(geom->IsMesh(), "geometry is mesh");
		FCDGeometryMesh* mesh = geom->GetMesh();
		REQUIRE(mesh->IsTriangles(), "mesh is made of triangles");
		REQUIRE(mesh->GetPolygonsCount() == 1, "mesh has single set of polygons");
		return mesh->GetPolygons(0);
	}

	/**
	 * Applies world-space transform to vertex data, and flips into other-handed
	 * coordinate space.
	 */
	static void TransformVertices(FloatList& position, FloatList& normal, const FMMatrix44& transform)
	{
		for (size_t i = 0; i < position.size(); i += 3)
		{
			FMVector3 pos (position[i], position[i+1], position[i+2]);
			FMVector3 norm (normal[i], normal[i+1], normal[i+2]);

			// Apply the scene-node transforms
			pos = transform.TransformCoordinate(pos);
			norm = transform.TransformVector(norm).Normalize();

			// Copy back to array, while switching the coordinate system around

			position[i+0] = pos.x;
			position[i+1] = pos.z;
			position[i+2] = pos.y;

			normal[i+0] = norm.x;
			normal[i+1] = norm.z;
			normal[i+2] = norm.y;
		}
	}

	static void TransformVertices(FloatList& position, FloatList& normal, std::vector<BoneTransform>& bones, const FMMatrix44& transform)
	{
		for (size_t vtxId = 0; vtxId < position.size()/3; ++vtxId)
		{
			FMVector3 pos (&position[vtxId*3], 0);
			FMVector3 norm (&normal[vtxId*3], 0);

			// Apply the scene-node transforms
			pos = transform.TransformCoordinate(pos);
			norm = transform.TransformVector(norm).Normalize();

			// Switch from Max's coordinate system into the game's:

			std::swap(pos.y, pos.z);
			std::swap(norm.y, norm.z);

			// and copy back into the original array

			position[vtxId*3+0] = pos.x;
			position[vtxId*3+1] = pos.y;
			position[vtxId*3+2] = pos.z;

			normal[vtxId*3+0] = norm.x;
			normal[vtxId*3+1] = norm.y;
			normal[vtxId*3+2] = norm.z;
		}

		// We also need to change the bone rest states into the new coordinate
		// system, so it'll look correct when displayed without any animation
		// applied to the skeleton.
		for (size_t i = 0; i < bones.size(); ++i)
		{
			// Convert bone translations from xyz into xzy axes:
			std::swap(bones[i].translation[1], bones[i].translation[2]);

			// To convert the quaternions: imagine you're using the axis/angle
			// representation, then swap the y,z basis vectors and change the
			// direction of rotation by negating the angle ( => negating sin(angle)
			// => negating x,y,z => changing (x,y,z,w) to (-x,-z,-y,w)
			// but then (-x,-z,-y,w) == (x,z,y,-w) so do that instead)
			std::swap(bones[i].orientation[1], bones[i].orientation[2]);
			bones[i].orientation[3] = -bones[i].orientation[3];
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
