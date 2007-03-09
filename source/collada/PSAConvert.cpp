#include "precompiled.h"

#include "PSAConvert.h"
#include "CommonConvert.h"

#include "FCollada.h"
#include "FCDocument/FCDocument.h"
#include "FCDocument/FCDocumentTools.h"
#include "FCDocument/FCDAnimated.h"
#include "FCDocument/FCDAnimationCurve.h"
#include "FCDocument/FCDController.h"
#include "FCDocument/FCDControllerInstance.h"
#include "FCDocument/FCDExtra.h"
#include "FCDocument/FCDGeometry.h"
#include "FCDocument/FCDGeometryMesh.h"
#include "FCDocument/FCDGeometryPolygons.h"
#include "FCDocument/FCDGeometrySource.h"
#include "FCDocument/FCDSceneNode.h"

#include "StdSkeletons.h"
#include "Decompose.h"
#include "Maths.h"
#include "GeomReindex.h"

#include <cassert>
#include <vector>
#include <limits>

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
		FColladaErrorHandler err (xmlErrors);

		FColladaDocument doc;
		doc.LoadFromText(input);

		FCDSceneNode* root = doc.GetDocument()->GetVisualSceneRoot();
		REQUIRE(root != NULL, "has root object");

		// Find the instance to convert
		FCDEntityInstance* instance;
		FMMatrix44 entityTransform;
		if (! FindSingleInstance(root, instance, entityTransform))
			throw ColladaException("Couldn't find object to convert");

		assert(instance);
		Log(LOG_INFO, "Converting '%s'", instance->GetEntity()->GetName().c_str());

		FMVector3 upAxis = doc.GetDocument()->GetAsset()->GetUpAxis();
		bool yUp = (upAxis.y != 0); // assume either Y_UP or Z_UP (TODO: does anyone ever do X_UP?)

		if (instance->GetType() == FCDEntityInstance::CONTROLLER)
		{
			FCDControllerInstance* controllerInstance = (FCDControllerInstance*)instance;

			FixSkeletonRoots(controllerInstance);

			float frameLength = 1.f / 30.f; // currently we always want to create PMDs at fixed 30fps

			// Find the extents of the animation:

			float timeStart, timeEnd;
			GetAnimationRange(doc, controllerInstance, timeStart, timeEnd);

			// Count frames; don't include the last keyframe
			size_t frameCount = (size_t)((timeEnd - timeStart) / frameLength - 0.5f);
			// (TODO: sort out the timing/looping problems)

			size_t boneCount = StdSkeletons::GetBoneCount();

			std::vector<BoneTransform> boneTransforms;

			for (size_t frame = 0; frame < frameCount; ++frame)
			{
				float time = timeStart + frameLength * frame;

				BoneTransform boneDefault  = { { 0, 0, 0 }, { 0, 0, 0, 1 } };
				std::vector<BoneTransform> frameBoneTransforms (boneCount, boneDefault);

				// Move the model into the new animated pose
				// (We can't tell exactly which nodes should be animated, so
				// just update the entire world recursively)
				EvaluateAnimations(root, time);

				// Convert the pose into the form require by the game
				for (size_t i = 0; i < controllerInstance->GetJointCount(); ++i)
				{
					FCDSceneNode* joint = controllerInstance->GetJoint(i);

					int boneId = StdSkeletons::FindStandardBoneID(joint->GetName());
					if (boneId < 0)
						continue; // not a recognised bone - ignore it, same as before

					FMMatrix44 worldTransform = joint->CalculateWorldTransform();

					HMatrix matrix;
					memcpy(matrix, worldTransform.Transposed().m, sizeof(matrix));

					AffineParts parts;
					decomp_affine(matrix, &parts);

					BoneTransform b = {
						{ parts.t.x, parts.t.y, parts.t.z },
						{ parts.q.x, parts.q.y, parts.q.z, parts.q.w }
					};

					frameBoneTransforms[boneId] = b;
				}

				// Push frameBoneTransforms onto the back of boneTransforms
				copy(frameBoneTransforms.begin(), frameBoneTransforms.end(),
					inserter(boneTransforms, boneTransforms.end()));
			}

			// Convert into game's coordinate space
			TransformVertices(boneTransforms, yUp);

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
		write(output, (uint32)1); // version number
		write(output, (uint32)(
			4 + 0 + // name
			4 + // frameLength
			4 + 4 + // numBones, numFrames
			7*4*boneCount*frameCount // boneStates
			)); // data size

		// Name
		write(output, (uint32)0);

		// Frame length
		write(output, 1000.f/30.f);

		write(output, (uint32)boneCount);
		write(output, (uint32)frameCount);

		for (size_t i = 0; i < boneCount*frameCount; ++i)
		{
			output((char*)&boneTransforms[i], 7*4);
		}
	}

	static void TransformVertices(std::vector<BoneTransform>& bones, bool yUp)
	{
		// (See PMDConvert.cpp for explanatory comments)
		for (size_t i = 0; i < bones.size(); ++i)
		{
			if (yUp)
			{
				bones[i].translation[0] = -bones[i].translation[0];
				bones[i].orientation[0] = -bones[i].orientation[0];
				bones[i].orientation[3] = -bones[i].orientation[3];
			}
			else
			{
				std::swap(bones[i].translation[1], bones[i].translation[2]);
				std::swap(bones[i].orientation[1], bones[i].orientation[2]);
				bones[i].orientation[3] = -bones[i].orientation[3];
			}
		}
	}

	static void GetAnimationRange(FColladaDocument& doc, FCDControllerInstance* controllerInstance,
		float& timeStart, float& timeEnd)
	{
		// FCollada tools export <extra> info in the scene to specify the start
		// and end times.
		// If that isn't available, we have to search for the earliest and latest
		// keyframes on any of the bones.
		if (doc.GetDocument()->HasStartTime() && doc.GetDocument()->HasEndTime())
		{
			timeStart = doc.GetDocument()->GetStartTime();
			timeEnd = doc.GetDocument()->GetEndTime();
			return;
		}

		// XSI exports relevant information in
		// <extra><technique profile="XSI"><SI_Scene><xsi_param sid="start">
		// (and 'end' and 'frameRate') so use those
		if (GetAnimationRange_XSI(doc, timeStart, timeEnd))
			return;

		timeStart = std::numeric_limits<float>::max();
		timeEnd = -std::numeric_limits<float>::max();
		for (size_t i = 0; i < controllerInstance->GetJointCount(); ++i)
		{
			FCDSceneNode* joint = controllerInstance->GetJoint(i);
			REQUIRE(joint != NULL, "joint exists");

			int boneId = StdSkeletons::FindStandardBoneID(joint->GetName());
			if (boneId < 0)
			{
				// unrecognised joint - it's probably just a prop point
				// or something, so ignore it
				continue;
			}

			// Skip unanimated joints
			if (joint->GetTransformCount() == 0)
				continue;

			for (size_t j = 0; j < joint->GetTransformCount(); ++j)
			{
				FCDTransform* transform = joint->GetTransform(j);

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
		}
	}

	static bool GetAnimationRange_XSI(FColladaDocument& doc, float& timeStart, float& timeEnd)
	{
		FCDExtra* extra = doc.GetExtra();
		if (! extra) return false;

		FCDEType* type = extra->GetDefaultType();
		if (! type) return false;

		FCDETechnique* technique = type->FindTechnique("XSI");
		if (! technique) return false;

		FCDENode* scene = technique->FindChildNode("SI_Scene");
		if (! scene) return false;

		float start = FLT_MAX, end = -FLT_MAX, framerate = 0.f;

		FCDENodeList paramNodes;
		scene->FindChildrenNodes("xsi_param", paramNodes);
		for (FCDENodeList::iterator it = paramNodes.begin(); it != paramNodes.end(); ++it)
		{
			if ((*it)->ReadAttribute("sid") == "start")
				start = FUStringConversion::ToFloat((*it)->GetContent());
			else if ((*it)->ReadAttribute("sid") == "end")
				end = FUStringConversion::ToFloat((*it)->GetContent());
			else if ((*it)->ReadAttribute("sid") == "frameRate")
				framerate = FUStringConversion::ToFloat((*it)->GetContent());
		}

		if (framerate != 0.f && start != FLT_MAX && end != -FLT_MAX)
		{
			timeStart = start / framerate;
			timeEnd = end / framerate;
			return true;
		}

		return false;
	}

	static void EvaluateAnimations(FCDSceneNode* node, float time)
	{
		for (size_t i = 0; i < node->GetTransformCount(); ++i)
		{
			FCDTransform* transform = node->GetTransform(i);
			FCDAnimated* anim = transform->GetAnimated();
			if (anim)
				anim->Evaluate(time);
		}

		for (size_t i = 0; i < node->GetChildrenCount(); ++i)
			EvaluateAnimations(node->GetChild(i), time);
	}

};


// The above stuff is just in a class since I don't like having to bother
// with forward declarations of functions - but provide the plain function
// interface here:

void ColladaToPSA(const char* input, OutputCB& output, std::string& xmlErrors)
{
	PSAConvert::ColladaToPSA(input, output, xmlErrors);
}
