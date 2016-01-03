/* Copyright (C) 2016 Wildfire Games.
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

#include "CommonConvert.h"

#include "StdSkeletons.h"
#include "XMLFix.h"

#include "FCollada.h"
#include "FCDocument/FCDSceneNode.h"
#include "FCDocument/FCDSkinController.h"
#include "FUtils/FUDaeSyntax.h"
#include "FUtils/FUFileManager.h"

#include <cassert>
#include <algorithm>

void require_(int line, bool value, const char* type, const char* message)
{
	if (value) return;
	char linestr[16];
	sprintf(linestr, "%d", line);
	throw ColladaException(std::string(type) + " (line " + linestr + "): " + message);
}

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

FColladaErrorHandler::FColladaErrorHandler(std::string& xmlErrors_)
: xmlErrors(xmlErrors_)
{
	// Grab all the error output from libxml2, for useful error reporting
	xmlSetGenericErrorFunc(&xmlErrors, &errorHandler);

	FUError::AddErrorCallback(FUError::DEBUG_LEVEL, this, &FColladaErrorHandler::OnError);
	FUError::AddErrorCallback(FUError::WARNING_LEVEL, this, &FColladaErrorHandler::OnError);
	FUError::AddErrorCallback(FUError::ERROR_LEVEL, this, &FColladaErrorHandler::OnError);
}

FColladaErrorHandler::~FColladaErrorHandler()
{
	xmlSetGenericErrorFunc(NULL, NULL);

	FUError::RemoveErrorCallback(FUError::DEBUG_LEVEL, this, &FColladaErrorHandler::OnError);
	FUError::RemoveErrorCallback(FUError::WARNING_LEVEL, this, &FColladaErrorHandler::OnError);
	FUError::RemoveErrorCallback(FUError::ERROR_LEVEL, this, &FColladaErrorHandler::OnError);
}

void FColladaErrorHandler::OnError(FUError::Level errorLevel, uint32 errorCode, uint32 UNUSED(lineNumber))
{
	// Ignore warnings about missing materials, since we ignore materials entirely anyway
	if (errorCode == FUError::WARNING_INVALID_POLYGON_MAT_SYMBOL)
		return;

	const char* errorString = FUError::GetErrorString((FUError::Code) errorCode);
	if (! errorString)
		errorString = "Unknown error code";

	if (errorLevel == FUError::DEBUG_LEVEL)
		Log(LOG_INFO, "FCollada %d: %s", errorCode, errorString);
	else if (errorLevel == FUError::WARNING_LEVEL)
		Log(LOG_WARNING, "FCollada %d: %s", errorCode, errorString);
	else
		throw ColladaException(errorString);
}


//////////////////////////////////////////////////////////////////////////

void FColladaDocument::LoadFromText(const char *text)
{
	document.reset(FCollada::NewTopDocument());

	const char* newText = NULL;
	size_t newTextSize = 0;
	FixBrokenXML(text, &newText, &newTextSize);

	// Log(LOG_INFO, "%s", newText);
	bool status = FCollada::LoadDocumentFromMemory("unknown.dae", document.get(), (void*)newText, newTextSize);

	if (newText != text)
		xmlFree((void*)newText);

	REQUIRE_SUCCESS(status);
}

void FColladaDocument::ReadExtras(xmlNode* UNUSED(colladaNode))
{
	// TODO: This was needed to recognise and load XSI models.
	// XSI support should be reintroduced some time, but this function
	// may not be necessary since FCollada might now provide access to the
	// 'extra' data via a proper API.

	/*
	if (! IsEquivalent(colladaNode->name, DAE_COLLADA_ELEMENT))
		return;

	extra.reset(new FCDExtra(document.get()));

	xmlNodeList extraNodes;
	FUXmlParser::FindChildrenByType(colladaNode, DAE_EXTRA_ELEMENT, extraNodes);
	for (xmlNodeList::iterator it = extraNodes.begin(); it != extraNodes.end(); ++it)
	{
		xmlNode* extraNode = (*it);
		extra->LoadFromXML(extraNode);
	}
	*/
}

//////////////////////////////////////////////////////////////////////////

CommonConvert::CommonConvert(const char* text, std::string& xmlErrors)
: m_Err(xmlErrors)
{
	m_Doc.LoadFromText(text);
	FCDSceneNode* root = m_Doc.GetDocument()->GetVisualSceneRoot();
	REQUIRE(root != NULL, "has root object");

	// Find the instance to convert
	if (! FindSingleInstance(root, m_Instance, m_EntityTransform))
		throw ColladaException("Couldn't find object to convert");

	assert(m_Instance);
	Log(LOG_INFO, "Converting '%s'", m_Instance->GetEntity()->GetName().c_str());

	m_IsXSI = false;
	FCDAsset* asset = m_Doc.GetDocument()->GetAsset();
	if (asset && asset->GetContributorCount() >= 1)
	{
		std::string tool (asset->GetContributor(0)->GetAuthoringTool());
		if (tool.find("XSI") != tool.npos)
			m_IsXSI = true;
	}

	FMVector3 upAxis = m_Doc.GetDocument()->GetAsset()->GetUpAxis();
	m_YUp = (upAxis.y != 0); // assume either Y_UP or Z_UP (TODO: does anyone ever do X_UP?)
}

CommonConvert::~CommonConvert()
{
}

//////////////////////////////////////////////////////////////////////////

// HACK: The originals don't get exported properly from FCollada (3.02, DLL), so define
// them here instead of fixing it correctly.
const FMVector3 FMVector3_XAxis(1.0f, 0.0f, 0.0f);
static float identity[] = { 1,0,0,0, 0,1,0,0, 0,0,1,0, 0,0,0,1 };
FMMatrix44 FMMatrix44_Identity(identity);

struct FoundInstance
{
	FCDEntityInstance* instance;
	FMMatrix44 transform;
};

static bool IsVisible_XSI(FCDSceneNode* node, bool& visible)
{
	// Look for <extra><technique profile="XSI"><SI_Visibility><xsi_param sid="visibility">

	FCDExtra* extra = node->GetExtra();
	if (! extra) return false;

	FCDEType* type = extra->GetDefaultType();
	if (! type) return false;

	FCDETechnique* technique = type->FindTechnique("XSI");
	if (! technique) return false;

	FCDENode* visibility1 = technique->FindChildNode("SI_Visibility");
	if (! visibility1) return false;

	FCDENode* visibility2 = visibility1->FindChildNode("xsi_param");
	if (! visibility2) return false;

	if (IsEquivalent(visibility2->GetContent(), "TRUE"))
		visible = true;
	else if (IsEquivalent(visibility2->GetContent(), "FALSE"))
		visible = false;
	return true;
}

static bool IsVisible(FCDSceneNode* node)
{
	bool visible = false;

	// Try the XSI visibility property
	if (IsVisible_XSI(node, visible))
		return visible;

	// Else fall back to the FCollada-specific setting
	visible = (node->GetVisibility() != 0.0);
	return visible;
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

		// Ignore invisible objects, because presumably nobody wanted to export them
		if (! IsVisible(node))
			continue;

		FoundInstance f;
		f.transform = transform * node->ToMatrix();
		f.instance = node->GetInstance(i);
		instances.push_back(f);
		Log(LOG_INFO, "Found convertible object '%s'", node->GetName().c_str());
	}
}

bool FindSingleInstance(FCDSceneNode* node, FCDEntityInstance*& instance, FMMatrix44& transform)
{
	std::vector<FoundInstance> instances;

	FindInstances(node, instances, FMMatrix44_Identity, true);
	if (instances.size() > 1)
	{
		Log(LOG_ERROR, "Found too many export-marked objects");
		return false;
	}
	if (instances.empty())
	{
		FindInstances(node, instances, FMMatrix44_Identity, false);
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

//////////////////////////////////////////////////////////////////////////

static bool ReverseSortWeight(const FCDJointWeightPair& a, const FCDJointWeightPair& b)
{
	return (a.weight > b.weight);
}

void SkinReduceInfluences(FCDSkinController* skin, size_t maxInfluenceCount, float minimumWeight)
{
	// Approximately equivalent to:
	//    skin->ReduceInfluences(maxInfluenceCount, minimumWeight);
	// except this version merges multiple weights for the same joint

	for (size_t i = 0; i < skin->GetInfluenceCount(); ++i)
	{
		FCDSkinControllerVertex& influence = *skin->GetVertexInfluence(i);

		std::vector<FCDJointWeightPair> newWeights;
		for (size_t i = 0; i < influence.GetPairCount(); ++i)
		{
			FCDJointWeightPair* weight = influence.GetPair(i);

			for (size_t j = 0; j < newWeights.size(); ++j)
			{
				FCDJointWeightPair& newWeight = newWeights[j];
				if (weight->jointIndex == newWeight.jointIndex)
				{
					newWeight.weight += weight->weight;
					goto MERGED_WEIGHTS;
				}
			}

			newWeights.push_back(*weight);
MERGED_WEIGHTS: ;
		}

		// Put highest-weighted influences at the front of the list
		std::sort(newWeights.begin(), newWeights.end(), ReverseSortWeight);

		// Limit the maximum number of influences
		if (newWeights.size() > maxInfluenceCount)
			newWeights.resize(maxInfluenceCount);

		// Enforce the minimum weight per influence
		// (This is done here rather than in the earlier loop, because several
		// small weights for the same bone might add up to a value above the
		// threshold)
		while (!newWeights.empty() && newWeights.back().weight < minimumWeight)
			newWeights.pop_back();

		// Renormalise, so sum(weights)=1
		float totalWeight = 0;
		for (std::vector<FCDJointWeightPair>::iterator itNW = newWeights.begin(); itNW != newWeights.end(); ++itNW)
			totalWeight += itNW->weight;
		for (std::vector<FCDJointWeightPair>::iterator itNW = newWeights.begin(); itNW != newWeights.end(); ++itNW)
			itNW->weight /= totalWeight;

		// Copy new weights into the skin
		influence.SetPairCount(0);
		for (std::vector<FCDJointWeightPair>::iterator itNW = newWeights.begin(); itNW != newWeights.end(); ++itNW)
			influence.AddPair(itNW->jointIndex, itNW->weight);
	}

	skin->SetDirtyFlag();
}


void FixSkeletonRoots(FCDControllerInstance& UNUSED(controllerInstance))
{
	// TODO: Need to reintroduce XSI support at some point
#if 0
	// HACK: The XSI exporter doesn't do a <skeleton> and FCollada doesn't
	// seem to know where else to look, so just guess that it's somewhere
	// under Scene_Root
	if (controllerInstance.GetSkeletonRoots().empty())
	{
		// HACK (evil): SetSkeletonRoot is declared but not defined, and there's
		// no other proper way to modify the skeleton-roots list, so cheat horribly
		FUUriList& uriList = const_cast<FUUriList&>(controllerInstance.GetSkeletonRoots());
		uriList.push_back(FUUri("Scene_Root"));
		controllerInstance.LinkImport();
	}
#endif
}

const Skeleton& FindSkeleton(const FCDControllerInstance& controllerInstance)
{
	// I can't see any proper way to determine the real root of the skeleton,
	// so just choose an arbitrary bone and search upwards until we find a
	// recognised ancestor (or until we fall off the top of the tree)

	const Skeleton* skeleton = NULL;
	const FCDSceneNode* joint = controllerInstance.GetJoint(0);
	while (joint && (skeleton = Skeleton::FindSkeleton(joint->GetName().c_str())) == NULL)
	{
		joint = joint->GetParent();
	}
	REQUIRE(skeleton != NULL, "recognised skeleton structure");
	return *skeleton;
}

void TransformBones(std::vector<BoneTransform>& bones, const FMMatrix44& scaleTransform, bool yUp)
{
	for (size_t i = 0; i < bones.size(); ++i)
	{
		// Apply the desired transformation to the bone coordinates
		FMVector3 trans(bones[i].translation, 0);
		trans = scaleTransform.TransformCoordinate(trans);
		bones[i].translation[0] = trans.x;
		bones[i].translation[1] = trans.y;
		bones[i].translation[2] = trans.z;

		// DON'T apply the transformation to orientation, because I can't get
		// that kind of thing to work in practice (interacting nicely between
		// the models and animations), so this function assumes the transform
		// just does scaling, so there's no need to rotate anything. (But I think
		// this code would work for rotation, though not very efficiently.)
		/*
		FMMatrix44 m = FMQuaternion(bones[i].orientation[0], bones[i].orientation[1], bones[i].orientation[2], bones[i].orientation[3]).ToMatrix();
		m *= scaleTransform;
		HMatrix matrix;
		memcpy(matrix, m.Transposed().m, sizeof(matrix));
		AffineParts parts;
		decomp_affine(matrix, &parts);

		bones[i].orientation[0] = parts.q.x;
		bones[i].orientation[1] = parts.q.y;
		bones[i].orientation[2] = parts.q.z;
		bones[i].orientation[3] = parts.q.w;
		*/

		if (yUp)
		{
			// TODO: this is all just guesses which seem to work for data
			// exported from XSI, rather than having been properly thought
			// through
			bones[i].translation[2] = -bones[i].translation[2];
			bones[i].orientation[2] = -bones[i].orientation[2];
			bones[i].orientation[3] = -bones[i].orientation[3];
		}
		else
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
}
