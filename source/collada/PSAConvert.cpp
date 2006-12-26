#include "precompiled.h"

#include "PSAConvert.h"
#include "CommonConvert.h"

#include "FCollada.h"
#include "FCDocument/FCDocument.h"
#include "FCDocument/FCDAnimated.h"
#include "FCDocument/FCDAnimationCurve.h"
#include "FCDocument/FCDController.h"
#include "FCDocument/FCDControllerInstance.h"
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

struct BoneTransform
{
	float translation[3];
	float orientation[4];
};

class PSAConvert
{
public:
	/**
	 * Converts a COLLADA XML document into the PSA animation format.
	 *
	 * @param input XML document to parse
	 * @param output callback for writing the PSA data; called lots of times
	 *               with small strings
	 * @param xmlErrors output - errors reported by the XML parser
	 * @throws ColladaException on failure
	 */
	static void ColladaToPSA(const char* input, OutputCB& output, std::string& xmlErrors)
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
		FMMatrix44 entityTransform;
		if (! FindSingleInstance(root, instance, entityTransform))
			throw ColladaException("Couldn't find object to convert");

		assert(instance);
		Log(LOG_INFO, "Converting '%s'", instance->GetEntity()->GetName().c_str());

		if (instance->GetEntity()->GetType() == FCDEntity::CONTROLLER)
		{
			FCDController* controller = (FCDController*)instance->GetEntity();

			REQUIRE(controller->HasSkinController(), "has skin controller");
			FCDSkinController* skin = controller->GetSkinController();

			// Find the first and last times which have animations
			float timeStart = FLT_MAX, timeEnd = -FLT_MAX;
			for (size_t i = 0; i < skin->GetJointCount(); ++i)
			{
				FCDJointMatrixPair* joint = skin->GetJoint(i);
				REQUIRE(joint->joint != NULL, "joint exists");

				// Skip unanimated joints
				if (joint->joint->GetTransformCount() == 0)
					continue;

				REQUIRE(joint->joint->GetTransformCount() == 1, "joint has single transform");

				FCDTransform* transform = joint->joint->GetTransform(0);

				// Skip unanimated joints again. (TODO: Which of these happens in practice?)
				if (! transform->IsAnimated())
					continue;

				// Iterate over all curves
				FCDAnimated* anim = transform->GetAnimated();
				FCDAnimationCurveListList& curvesList = anim->GetCurves();
				for (size_t j = 0; j < curvesList.size(); ++j)
				{
					FCDAnimationCurveList& curves = curvesList[j];
					for (size_t k = 0; k < curves.size(); ++k)
					{
						FCDAnimationCurve* curve = curves[k];
						timeStart = std::min(timeStart, curve->GetKeys().front());
						timeEnd = std::max(timeEnd, curve->GetKeys().back());
					}
				}
			}

			float frameLength = 1.f / 30.f;

			// Count frames; don't include the last keyframe
			size_t frameCount = (size_t)((timeEnd - timeStart) / frameLength - 0.5f);

			size_t boneCount = StdSkeletons::GetBoneCount();

			std::vector<BoneTransform> boneTransforms;

			for (size_t frame = 0; frame < frameCount; ++frame)
			{
				float time = timeStart + frameLength * frame;

				BoneTransform boneDefault  = { { 0, 0, 0 }, { 0, 0, 0, 1 } };
				std::vector<BoneTransform> frameBoneTransforms (boneCount, boneDefault);

				// Move the model into the new animated pose
				for (size_t i = 0; i < skin->GetJointCount(); ++i)
				{
					FCDTransform* transform = skin->GetJoint(i)->joint->GetTransform(0);
					FCDAnimated* anim = transform->GetAnimated();
					anim->Evaluate(time);
				}

				// Convert the pose into the form require by the game
				for (size_t i = 0; i < skin->GetJointCount(); ++i)
				{
					FCDSceneNode* jointNode = skin->GetJoint(i)->joint;
					FMMatrix44 worldTransform = jointNode->CalculateWorldTransform();

					HMatrix matrix;
					memcpy(matrix, worldTransform.Transposed().m, sizeof(matrix));

					AffineParts parts;
					decomp_affine(matrix, &parts);

					BoneTransform b = {
						{ parts.t.x, parts.t.y, parts.t.z },
						{ parts.q.x, parts.q.y, parts.q.z, parts.q.w }
					};

					int boneId = StdSkeletons::FindStandardBoneID(jointNode->GetName());
					REQUIRE(boneId >= 0, "recognised bone name");
					frameBoneTransforms[boneId] = b;
				}

				// Push frameBoneTransforms onto the back of boneTransforms
				copy(frameBoneTransforms.begin(), frameBoneTransforms.end(), inserter(boneTransforms, boneTransforms.end()));
			}

			// Convert into game's coordinate space
			TransformVertices(boneTransforms);

			// Write out the file
			WritePSA(output, frameCount, boneCount, boneTransforms);
		}
		else
		{
			throw ColladaException("Unrecognised object type");
		}
	}

	/**
	 * Writes the animation data in the PSA format.
	 */
	static void WritePSA(OutputCB& output, size_t frameCount, size_t boneCount, const std::vector<BoneTransform>& boneTransforms)
	{
		output("PSSA", 4);  // magic number
		write<uint32>(output, 1); // version number
		write<uint32>(output, (uint32)(
			4 + 0 + // name
			4 + // frameLength
			4 + 4 + // numBones, numFrames
			7*4*boneCount*frameCount // boneStates
			)); // data size

		// Name
		write<uint32>(output, 0);

		// Frame length
		write<float>(output, 1000/30.f);

		write<uint32>(output, (uint32)boneCount);
		write<uint32>(output, (uint32)frameCount);

		for (size_t i = 0; i < boneCount*frameCount; ++i)
		{
			output((char*)&boneTransforms[i], 7*4);
		}
	}

	static void TransformVertices(std::vector<BoneTransform>& bones)
	{
		// (See PMDConvert.cpp for explanatory comments)
		for (size_t i = 0; i < bones.size(); ++i)
		{
			std::swap(bones[i].translation[1], bones[i].translation[2]);
			std::swap(bones[i].orientation[1], bones[i].orientation[2]);
			bones[i].orientation[3] = -bones[i].orientation[3];
		}
	}
};


// The above stuff is just in a class since I don't like having to bother
// with forward declarations of functions - but provide the plain function
// interface here:

void ColladaToPSA(const char* input, OutputCB& output, std::string& xmlErrors)
{
	PSAConvert::ColladaToPSA(input, output, xmlErrors);
}
