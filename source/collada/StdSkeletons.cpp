/* Copyright (C) 2015 Wildfire Games.
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

#include "libxml/parser.h"
#include "libxml/xmlerror.h"

#include "StdSkeletons.h"

#include "CommonConvert.h"

#include "FUtils/FUXmlParser.h"

#include <map>

namespace
{
	struct SkeletonMap : public std::map<std::string, const Skeleton*>
	{
		~SkeletonMap()
		{
			for (iterator it = begin(); it != end(); ++it)
				delete it->second;
		}
	};

	SkeletonMap g_StandardSkeletons;
	SkeletonMap g_MappedSkeletons;

	struct Bone
	{
		std::string parent;
		std::string name;
		int targetId;
		int realTargetId;
	};
}

struct Skeleton_impl
{
	std::string title;
	std::vector<Bone> bones;
	const Skeleton* target;
};

Skeleton::Skeleton() : m(new Skeleton_impl) { }
Skeleton::~Skeleton() { }

const Skeleton* Skeleton::FindSkeleton(const std::string& name)
{
	return g_MappedSkeletons[name];
}

int Skeleton::GetBoneID(const std::string& name) const
{
	for (size_t i = 0; i < m->bones.size(); ++i)
		if (m->bones[i].name == name)
			return m->bones[i].targetId;
	return -1;
}

int Skeleton::GetRealBoneID(const std::string& name) const
{
	for (size_t i = 0; i < m->bones.size(); ++i)
		if (m->bones[i].name == name)
			return m->bones[i].realTargetId;
	return -1;
}

int Skeleton::GetBoneCount() const
{
	return (int)m->target->m->bones.size();
}

namespace
{
	bool AlreadyUsedTargetBone(const std::vector<Bone>& bones, int targetId)
	{
		for (size_t i = 0; i < bones.size(); ++i)
			if (bones[i].targetId == targetId)
				return true;
		return false;
	}

	// Recursive helper function used by LoadSkeletonData
	void LoadSkeletonBones(xmlNode* parent, std::vector<Bone>& bones, const Skeleton* targetSkeleton, const std::string& targetName)
	{
		xmlNodeList boneNodes;
		FUXmlParser::FindChildrenByType(parent, "bone", boneNodes);
		for (xmlNodeList::iterator boneNode = boneNodes.begin(); boneNode != boneNodes.end(); ++boneNode)
		{
			std::string name (FUXmlParser::ReadNodeProperty(*boneNode, "name"));

			Bone b;
			b.name = name;

			std::string newTargetName = targetName;

			if (targetSkeleton)
			{
				xmlNode* targetNode = FUXmlParser::FindChildByType(*boneNode, "target");
				if (targetNode)
					newTargetName = FUXmlParser::ReadNodeContentFull(targetNode);
				// else fall back to the parent node's target

				b.targetId = targetSkeleton->GetBoneID(newTargetName);
				REQUIRE(b.targetId != -1, "skeleton bone target matches some standard_skeleton bone name");

				if (AlreadyUsedTargetBone(bones, b.targetId))
					b.realTargetId = -1;
				else
					b.realTargetId = b.targetId;
			}
			else
			{
				// No target - this is a standard skeleton

				b.targetId = (int)bones.size();
				b.realTargetId = b.targetId;
			}

			bones.push_back(b);

			LoadSkeletonBones(*boneNode, bones, targetSkeleton, newTargetName);
		}
	}

	void LoadSkeletonData(xmlNode* root)
	{
		xmlNodeList skeletonNodes;
		FUXmlParser::FindChildrenByType(root, "standard_skeleton", skeletonNodes);
		FUXmlParser::FindChildrenByType(root, "skeleton", skeletonNodes);
		for (xmlNodeList::iterator skeletonNode = skeletonNodes.begin();
			skeletonNode != skeletonNodes.end(); ++skeletonNode)
		{
			std::unique_ptr<Skeleton> skeleton (new Skeleton());

			std::string title (FUXmlParser::ReadNodeProperty(*skeletonNode, "title"));

			skeleton->m->title = title;

			if (IsEquivalent((*skeletonNode)->name, "standard_skeleton"))
			{
				skeleton->m->target = NULL;

				LoadSkeletonBones(*skeletonNode, skeleton->m->bones, NULL, "");

				std::string id (FUXmlParser::ReadNodeProperty(*skeletonNode, "id"));
				REQUIRE(! id.empty(), "standard_skeleton has id");

				g_StandardSkeletons[id] = skeleton.release();
			}
			else
			{
				// Non-standard skeletons need to choose a standard skeleton
				// as their target to be mapped onto

				std::string target (FUXmlParser::ReadNodeProperty(*skeletonNode, "target"));
				const Skeleton* targetSkeleton = g_StandardSkeletons[target];
				REQUIRE(targetSkeleton != NULL, "skeleton target matches some standard_skeleton id");

				skeleton->m->target = targetSkeleton;

				LoadSkeletonBones(*skeletonNode, skeleton->m->bones, targetSkeleton, "");

				// Currently the only supported identifier is a precise name match,
				// so just look for that
				xmlNode* identifier = FUXmlParser::FindChildByType(*skeletonNode, "identifier");
				REQUIRE(identifier != NULL, "skeleton has <identifier>");
				xmlNode* identRoot = FUXmlParser::FindChildByType(identifier, "root");
				REQUIRE(identRoot != NULL, "skeleton identifier has <root>");
				std::string identRootName (FUXmlParser::ReadNodeContentFull(identRoot));

				g_MappedSkeletons[identRootName] = skeleton.release();
			}
		}
	}
}

void errorHandler(void* ctx, const char* msg, ...);

void Skeleton::LoadSkeletonDataFromXml(const char* xmlData, size_t xmlLength, std::string& xmlErrors)
{
	xmlDoc* doc = NULL;
	try
	{
		xmlSetGenericErrorFunc(&xmlErrors, &errorHandler);
		doc = xmlParseMemory(xmlData, (int)xmlLength);
		if (doc)
		{
			xmlNode* root = xmlDocGetRootElement(doc);
			LoadSkeletonData(root);
			xmlFreeDoc(doc);
			doc = NULL;
		}
		xmlSetGenericErrorFunc(NULL, NULL);
	}
	catch (const ColladaException&)
	{
		if (doc)
			xmlFreeDoc(doc);
		xmlSetGenericErrorFunc(NULL, NULL);
		throw;
	}

	if (! xmlErrors.empty())
		throw ColladaException("XML parsing failed");
}
