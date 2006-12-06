#include "precompiled.h"

#include "Converter.h"

#include "FCollada.h"
#include "FCDocument/FCDocument.h"
#include "FCDocument/FCDSceneNode.h"
#include "FCDocument/FCDGeometry.h"
#include "FCDocument/FCDGeometryMesh.h"
#include "FCDocument/FCDGeometryPolygons.h"
#include "FCDocument/FCDGeometrySource.h"

#include <cassert>
#include <vector>

/** Throws a ColladaException unless the value is true. */
#define REQUIRE(value, message) require_(__LINE__, value, "Failed assertion", message)

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

struct VertexBlend
{
	uint8 bones[4];
	float weights[4];
};

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

			FCDGeometry* geom = (FCDGeometry*)instance->GetEntity();
			REQUIRE(geom->IsMesh(), "geometry is mesh");
			FCDGeometryMesh* mesh = geom->GetMesh();
			REQUIRE(mesh->IsTriangles(), "mesh is made of triangles");
			REQUIRE(mesh->GetPolygonsCount() == 1, "mesh has single set of polygons");
			FCDGeometryPolygons* polys = mesh->GetPolygons(0);

			size_t vertices = polys->GetFaceVertexCount();
			FloatList position, normal, texcoord;
			DeindexInput(polys, FUDaeGeometryInput::POSITION, position, 3);
			DeindexInput(polys, FUDaeGeometryInput::NORMAL, normal, 3);

			if (polys->FindInput(FUDaeGeometryInput::TEXCOORD))
			{
				DeindexInput(polys, FUDaeGeometryInput::TEXCOORD, texcoord, 2);
			}
			else
			{
				// Accept untextured models
				texcoord.resize(vertices*2, 0.f);
			}

			assert(position.size() == vertices*3);
			assert(normal.size() == vertices*3);
			assert(texcoord.size() == vertices*2);

 			TransformVertices(position, normal, transform);
			// TODO: optimise at least enough to merge identical vertices
 
 			WritePMD(output, vertices, 0, &position[0], &normal[0], &texcoord[0], NULL, NULL);
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
		static const VertexBlend noBlend = { 0xFF, 0xFF, 0xFF, 0xFF, 0, 0, 0, 0 };

		assert(position);
		assert(normal);
		assert(texcoord);
		if (boneCount) assert(boneWeights && boneTransforms);

		output("PSMD", 4);  // magic number
		write<uint32>(output, 2); // version number
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
			write(output, boneTransforms[i]);
		}

		// Prop points data
		write<uint32>(output, 0);
	}

	/**
	 * Converts from value-array plus indexes-into-array-per-vertex, into
	 *  values-per-vertex (because that's what PMD wants).
	 */
	static void DeindexInput(FCDGeometryPolygons* polys, FUDaeGeometryInput::Semantic semantic, FloatList& out, size_t outStride)
	{
		FCDGeometryPolygonsInput* input = polys->FindInput(semantic);
		UInt32List* indices = polys->FindIndices(input);
		REQUIRE(input && indices, "has expected polygon input");
		FCDGeometrySource* source = input->GetSource();
		FloatList& data = source->GetSourceData();
		size_t stride = source->GetSourceStride();

		for (size_t i = 0; i < indices->size(); ++i)
			for (size_t j = 0; j < outStride; ++j)
				out.push_back(data[(*indices)[i]*stride + j]);
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

			FoundInstance f;
			f.transform = transform * node->ToMatrix();
			f.instance = node->GetInstance(i);
			instances.push_back(f);
		}
	}
};


// The above stuff is just in a class since I don't like having to bother
// with forward declarations of functions - but provide the plain function
// interface here:

void ColladaToPMD(const char* input, OutputFn output, std::string& xmlErrors)
{
	Converter::ColladaToPMD(input, output, xmlErrors);
}
