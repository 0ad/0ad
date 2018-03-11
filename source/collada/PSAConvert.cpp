/* Copyright (C) 2018 Wildfire Games.
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

#include "PSAConvert.h"
#include "CommonConvert.h"

#include "FCollada.h"
#include "FCDocument/FCDocument.h"
#include "FCDocument/FCDocumentTools.h"
#include "FCDocument/FCDAnimated.h"
#include "FCDocument/FCDAnimationCurve.h"
#include "FCDocument/FCDAnimationKey.h"
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
#include <iterator>
#include <algorithm>

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
		CommonConvert converter(input, xmlErrors);

		if (converter.GetInstance().GetType() == FCDEntityInstance::CONTROLLER)
		{
			FCDControllerInstance& controllerInstance = static_cast<FCDControllerInstance&>(converter.GetInstance());

			FixSkeletonRoots(controllerInstance);

			assert(converter.GetInstance().GetEntity()->GetType() == FCDEntity::CONTROLLER); // assume this is always true?
			FCDController* controller = static_cast<FCDController*>(converter.GetInstance().GetEntity());

			FCDSkinController* skin = controller->GetSkinController();
			REQUIRE(skin != NULL, "is skin controller");

			const Skeleton& skeleton = FindSkeleton(controllerInstance);

			float frameLength = 1.f / 30.f; // currently we always want to create PMDs at fixed 30fps

			// Find the extents of the animation:

			float timeStart = 0, timeEnd = 0;
			GetAnimationRange(converter.GetDocument(), skeleton, controllerInstance, timeStart, timeEnd);
			// To catch broken animations / skeletons.xml:
			REQUIRE(timeEnd > timeStart, "animation end frame must come after start frame");

			// Count frames; don't include the last keyframe
			size_t frameCount = (size_t)((timeEnd - timeStart) / frameLength - 0.5f);
			REQUIRE(frameCount > 0, "animation must have frames");
			// (TODO: sort out the timing/looping problems)

			size_t boneCount = skeleton.GetBoneCount();

			std::vector<BoneTransform> boneTransforms;

			for (size_t frame = 0; frame < frameCount; ++frame)
			{
				float time = timeStart + frameLength * frame;

				BoneTransform boneDefault  = { { 0, 0, 0 }, { 0, 0, 0, 1 } };
				std::vector<BoneTransform> frameBoneTransforms (boneCount, boneDefault);

				// Move the model into the new animated pose
				// (We can't tell exactly which nodes should be animated, so
				// just update the entire world recursively)
				EvaluateAnimations(converter.GetRoot(), time);

				// Convert the pose into the form require by the game
				for (size_t i = 0; i < controllerInstance.GetJointCount(); ++i)
				{
					FCDSceneNode* joint = controllerInstance.GetJoint(i);

					int boneId = skeleton.GetRealBoneID(joint->GetName().c_str());
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
					std::inserter(boneTransforms, boneTransforms.end()));
			}

			// Convert into game's coordinate space
			TransformVertices(boneTransforms, skin->GetBindShapeTransform(), converter.IsYUp(), converter.IsXSI());

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

	static void TransformVertices(std::vector<BoneTransform>& bones,
		const FMMatrix44& transform, bool yUp, bool isXSI)
	{
		// HACK: we want to handle scaling in XSI because that makes it easy
		// for artists to adjust the models to the right size. But this way
		// doesn't work in Max, and I can't see how to make it do so, so this
		// is only applied to models from XSI.
		if (isXSI)
		{
			TransformBones(bones, DecomposeToScaleMatrix(transform), yUp);
		}
		else
		{
			TransformBones(bones, FMMatrix44_Identity, yUp);
		}
	}

	static void GetAnimationRange(const FColladaDocument& doc, const Skeleton& skeleton,
		const FCDControllerInstance& controllerInstance,
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
		for (size_t i = 0; i < controllerInstance.GetJointCount(); ++i)
		{
			const FCDSceneNode* joint = controllerInstance.GetJoint(i);
			REQUIRE(joint != NULL, "joint exists");

			int boneId = skeleton.GetBoneID(joint->GetName().c_str());
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
				const FCDTransform* transform = joint->GetTransform(j);

				if (! transform->IsAnimated())
					continue;

				// Iterate over all curves to find the earliest and latest keys
				const FCDAnimated* anim = transform->GetAnimated();
				const FCDAnimationCurveListList& curvesList = anim->GetCurves();
				for (size_t k = 0; k < curvesList.size(); ++k)
				{
					const FCDAnimationCurveTrackList& curves = curvesList[k];
					for (size_t l = 0; l < curves.size(); ++l)
					{
						const FCDAnimationCurve* curve = curves[l];
						timeStart = std::min(timeStart, curve->GetKeys()[0]->input);
						timeEnd = std::max(timeEnd, curve->GetKeys()[curve->GetKeyCount()-1]->input);
					}
				}
			}
		}
	}

	static bool GetAnimationRange_XSI(const FColladaDocument& doc, float& timeStart, float& timeEnd)
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

	static void EvaluateAnimations(FCDSceneNode& node, float time)
	{
		for (size_t i = 0; i < node.GetTransformCount(); ++i)
		{
			FCDTransform* transform = node.GetTransform(i);
			FCDAnimated* anim = transform->GetAnimated();
			if (anim)
				anim->Evaluate(time);
		}

		for (size_t i = 0; i < node.GetChildrenCount(); ++i)
			EvaluateAnimations(*node.GetChild(i), time);
	}

};


// The above stuff is just in a class since I don't like having to bother
// with forward declarations of functions - but provide the plain function
// interface here:

void ColladaToPSA(const char* input, OutputCB& output, std::string& xmlErrors)
{
	PSAConvert::ColladaToPSA(input, output, xmlErrors);
}
