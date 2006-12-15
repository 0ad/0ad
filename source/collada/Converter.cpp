#include "precompiled.h"

#include "Converter.h"

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

#include <cassert>
#include <vector>

/** Throws a ColladaException unless the value is true. */
#define REQUIRE(value, message) require_(__LINE__, value, "Assertion not satisfied", message)

/** Throws a ColladaException unless the status is successful. */
#define REQUIRE_SUCCESS(status) require_(__LINE__, status)

void require_(int line, bool value, const char* type, const char* message)
{
	if (value) return;
	char linestr[16];
	sprintf(linestr, "%d", line);
	throw ColladaException(std::string(type) + " (line " + linestr + "): " + message);
}
void require_(int line, const FUStatus& status)
{
	require_(line, status, "FCollada error", status.GetErrorString());
}

/** Outputs a structure, using sizeof to get the size. */
template<typename T> void write(OutputFn output, const T& data)
{
	output((char*)&data, sizeof(T));
}

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


/** Error handler for libxml2 */
void errorHandler(void* ctx, const char* msg, ...)
{
	char buffer[1024];
	va_list ap;
	va_start(ap, msg);
	vsnprintf(buffer, sizeof(buffer), msg, ap);
	buffer[sizeof(buffer)-1] = '\0';
	va_end(ap);

	*((std::string*)ctx) += buffer;
}

class Converter
{
public:
	/**
	 * Converts a COLLADA XML document into the PMD format.
	 *
	 * @param input XML document to parse
	 * @param output callback for writing the PMD data; called lots of times
	 *               with small strings
	 * @param xmlErrors output - errors reported by the XML parser
	 * @throws ColladaException on failure
	 */
	static void ColladaToPMD(const char* input, OutputFn output, std::string& xmlErrors)
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

			size_t vertices = polys->GetFaceVertexCount();
			FloatList position, normal, texcoord;

			DeindexInput(polys, FUDaeGeometryInput::POSITION, position, 3);
			DeindexInput(polys, FUDaeGeometryInput::NORMAL, normal, 3);

			if (polys->FindInput(FUDaeGeometryInput::TEXCOORD))
				DeindexInput(polys, FUDaeGeometryInput::TEXCOORD, texcoord, 2);
			else // Accept untextured models
				texcoord.resize(vertices*2, 0.f);

			assert(position.size() == vertices*3);
			assert(normal.size() == vertices*3);
			assert(texcoord.size() == vertices*2);

			TransformVertices(position, normal, transform);
			// TODO: optimise at least enough to merge identical vertices

			WritePMD(output, vertices, 0, &position[0], &normal[0], &texcoord[0], NULL, NULL);
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

			// Convert the bone influences into VertexBlend structures for the PMD

			std::vector<VertexBlend> vertexBlends; // one per vertex

			const FCDWeightedMatches& boneWeights = skin->GetVertexInfluences();
			for (size_t i = 0; i < boneWeights.size(); ++i)
			{
				VertexBlend influences = defaultInfluences;

				assert(boneWeights[i].size() <= maxInfluences); // guaranteed by ReduceInfluences
				for (size_t j = 0; j < boneWeights[i].size(); ++j)
				{
					uint32 jointIdx = boneWeights[i][j].jointIndex;
					REQUIRE(jointIdx <= 0xFF, "sensible number of joints");
					FCDSceneNode* joint = skin->GetJoint(jointIdx)->joint;
					if (! joint)
					{
						Log(LOG_WARNING, "Vertexes influenced by nonexistent joint");
						continue;
					}
					int boneId = StdSkeletons::FindStandardBoneID(joint->GetName());
					REQUIRE(boneId >= 0, "recognised bone name");
					influences.bones[j] = (uint8)boneId;
					influences.weights[j] = boneWeights[i][j].weight;
				}

				vertexBlends.push_back(influences);
			}

			BoneTransform boneDefault  = { { 0, 0, 0 }, { 0, 0, 0, 1 } };
			std::vector<BoneTransform> bones (StdSkeletons::GetBoneCount(), boneDefault);

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
				bones[boneId] = b;
			}

			size_t vertices = polys->GetFaceVertexCount();
			FloatList position, normal, texcoord;
			std::vector<VertexBlend> blends;
			DeindexInput(polys, FUDaeGeometryInput::POSITION, position, 3);
			DeindexInput(polys, vertexBlends, blends);
			DeindexInput(polys, FUDaeGeometryInput::NORMAL, normal, 3);
			if (polys->FindInput(FUDaeGeometryInput::TEXCOORD))
				DeindexInput(polys, FUDaeGeometryInput::TEXCOORD, texcoord, 2);
			else // Accept untextured models
				texcoord.resize(vertices*2, 0.f);

			assert(position.size() == vertices*3);
			assert(normal.size() == vertices*3);
			assert(texcoord.size() == vertices*2);

			TransformVertices(position, normal, bones, transform);

			WritePMD(output, vertices, bones.size(), &position[0], &normal[0], &texcoord[0], &blends[0], &bones[0]);
		}
		else
		{
			throw ColladaException("Unrecognised object type");
		}

	}

	/**
	 * Writes the model data in the PMD format.
	 */
	static void WritePMD(OutputFn output, size_t vertexCount, size_t boneCount,
		float* position, float* normal, float* texcoord,
		VertexBlend* boneWeights, BoneTransform* boneTransforms)
	{
		static const VertexBlend noBlend = { { 0xFF, 0xFF, 0xFF, 0xFF }, { 0, 0, 0, 0 } };

		assert(position);
		assert(normal);
		assert(texcoord);
		if (boneCount) assert(boneWeights && boneTransforms);

		output("PSMD", 4);  // magic number
		write<uint32>(output, 3); // version number
		write<uint32>(output, (uint32)(
			4 + 13*4*vertexCount + // vertices
			4 + 6*vertexCount/3 + // faces
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
			if (boneWeights)
				write(output, boneWeights[i]);
			else
				write(output, noBlend);
		}

		// Face data
		// (TODO: this is really very rubbish and inefficient)
		write<uint32>(output, (uint32)vertexCount/3);
		for (uint16 i = 0; i < vertexCount/3; ++i)
		{
			uint16 vertexCount[3] = { i*3, i*3+1, i*3+2 };
			write(output, vertexCount);
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
	 * Converts from value-array plus indexes-into-array-per-vertex, into
	 *  values-per-vertex (because that's what PMD wants).
	 */
	static void DeindexInput(const FCDGeometryPolygons* polys, FUDaeGeometryInput::Semantic semantic, FloatList& out, size_t outStride)
	{
		const FCDGeometryPolygonsInput* input = polys->FindInput(semantic);
		const UInt32List* indices = polys->FindIndices(input);
		REQUIRE(input && indices, "has expected polygon input");
		const FCDGeometrySource* source = input->GetSource();
		const FloatList& data = source->GetSourceData();
		size_t stride = source->GetSourceStride();

		for (size_t i = 0; i < indices->size(); ++i)
			for (size_t j = 0; j < outStride; ++j)
				out.push_back(data[(*indices)[i]*stride + j]);
	}

	static void DeindexInput(const FCDGeometryPolygons* polys, const std::vector<VertexBlend>& inBlends, std::vector<VertexBlend>& outBlends)
	{
		assert(outBlends.empty());

		const FCDGeometryPolygonsInput* input = polys->FindInput(FUDaeGeometryInput::POSITION);
		const UInt32List* indices = polys->FindIndices(input);

		for (size_t i = 0; i < indices->size(); ++i)
			outBlends.push_back(inBlends[(*indices)[i]]);
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

	//////////////////////////////////////////////////////////////////////////

	struct FoundInstance
	{
		FCDEntityInstance* instance;
		FMMatrix44 transform;
	};

	/**
	 * Tries to find a single suitable entity instance in the scene. Fails if there
	 * are none, or if there are too many and it's not clear which one should
	 * be converted.
	 *
	 * @param node root scene node to search under
	 * @param instance output - the found entity instance (if any)
	 * @param transform - the world-space transform of the found entity
	 *
	 * @return true if one was found
	 */
	static bool FindSingleInstance(FCDSceneNode* node, FCDEntityInstance*& instance, FMMatrix44& transform)
	{
		std::vector<FoundInstance> instances;

		FindInstances(node, instances, FMMatrix44::Identity, true);
		if (instances.size() > 1)
		{
			Log(LOG_ERROR, "Found too many export-marked objects");
			return false;
		}
		if (instances.empty())
		{
			FindInstances(node, instances, FMMatrix44::Identity, false);
			if (instances.size() > 1)
			{
				Log(LOG_ERROR, "Found too many possible objects to convert - try adding the 'export' property to disambiguate one");
				return false;
			}
			if (instances.empty())
			{
				Log(LOG_ERROR, "Didn't find any objects in the scene");
				return false;
			}
		}

		assert(instances.size() == 1); // if we got this far
		instance = instances[0].instance;
		transform = instances[0].transform;
		return true;
	}

	/**
	 * Recursively finds all entities under the current node. If onlyMarked is
	 * set, only matches entities where the user-defined property was set to
	 * "export" in the modelling program.
	 *
	 * @param node root of subtree to search
	 * @param instances output - appends matching entities
	 * @param transform transform matrix of current subtree
	 * @param onlyMarked only match entities with "export" property
	 */
	static void FindInstances(FCDSceneNode* node, std::vector<FoundInstance>& instances, const FMMatrix44& transform, bool onlyMarked)
	{
		for (size_t i = 0; i < node->GetChildrenCount(); ++i)
		{
			FCDSceneNode* child = node->GetChild(i);
			FindInstances(child, instances, transform * node->ToMatrix(), onlyMarked);
		}

		for (size_t i = 0; i < node->GetInstanceCount(); ++i)
		{
			if (onlyMarked)
			{
				if (node->GetNote() != "export")
					continue;
			}

			// Only accept instances of appropriate types, and not e.g. lights
			FCDEntity::Type type = node->GetInstance(i)->GetEntityType();
			if (! (type == FCDEntity::GEOMETRY || type == FCDEntity::CONTROLLER))
				continue;

			FoundInstance f;
			f.transform = transform * node->ToMatrix();
			f.instance = node->GetInstance(i);
			instances.push_back(f);
		}
	}
	
	//////////////////////////////////////////////////////////////////////////

	static bool ReverseSortWeight(const FCDJointWeightPair& a, const FCDJointWeightPair& b)
	{
		return (a.weight > b.weight);
	}

	/**
	 * Like FCDSkinController::ReduceInfluences but works correctly.
	 * Additionally, multiple influences for the same joint-vertex pair are
	 * collapsed into a single influence.
	 */
	static void SkinReduceInfluences(FCDSkinController* skin, uint32 maxInfluenceCount, float minimumWeight)
	{
		FCDWeightedMatches& weightedMatches = skin->GetWeightedMatches();
		for (FCDWeightedMatches::iterator itM = weightedMatches.begin(); itM != weightedMatches.end(); ++itM)
		{
			FCDJointWeightPairList& weights = (*itM);

			FCDJointWeightPairList newWeights;
			for (FCDJointWeightPairList::iterator itW = weights.begin(); itW != weights.end(); ++itW)
			{
				// If this joint already has an influence, just add the weight
				// instead of adding a new influence
				bool done = false;
				for (FCDJointWeightPairList::iterator itNW = newWeights.begin(); itNW != newWeights.end(); ++itNW)
				{
					if (itW->jointIndex == itNW->jointIndex)
					{
						itNW->weight += itW->weight;
						done = true;
						break;
					}
				}

				if (done)
					continue;

				// Not had this joint before, so add it
				newWeights.push_back(*itW);
			}

			// Put highest-weighted influences at the front of the list
			sort(newWeights.begin(), newWeights.end(), ReverseSortWeight);

			// Limit the maximum number of influences
			if (newWeights.size() > maxInfluenceCount)
				newWeights.resize(maxInfluenceCount);

			// Enforce the minimum weight per influence
			while (!newWeights.empty() && newWeights.back().weight < minimumWeight)
				newWeights.pop_back();

			// Renormalise, so sum(weights)=1
			float totalWeight = 0;
			for (FCDJointWeightPairList::iterator itNW = newWeights.begin(); itNW != newWeights.end(); ++itNW)
				totalWeight += itNW->weight;
			for (FCDJointWeightPairList::iterator itNW = newWeights.begin(); itNW != newWeights.end(); ++itNW)
				itNW->weight /= totalWeight;

			// Copy new weights into the skin
			weights = newWeights;
		}

		skin->SetDirtyFlag();
	}

};


// The above stuff is just in a class since I don't like having to bother
// with forward declarations of functions - but provide the plain function
// interface here:

void ColladaToPMD(const char* input, OutputFn output, std::string& xmlErrors)
{
	Converter::ColladaToPMD(input, output, xmlErrors);
}
