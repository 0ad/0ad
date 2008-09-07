/*
	Copyright (C) 2005-2007 Feeling Software Inc.
	Portions of the code are:
	Copyright (C) 2005-2007 Sony Computer Entertainment America
	
	MIT License: http://www.opensource.org/licenses/mit-license.php
*/

#include "StdAfx.h"
#include "FArchiveXML.h"
#include "FCDocument/FCDocument.h"
#include "FCDocument/FCDEntity.h"
#include "FCDocument/FCDTargetedEntity.h"
#include "FCDocument/FCDAsset.h"
#include "FCDocument/FCDExtra.h"
#include "FCDocument/FCDAnimated.h"
#include "FCDocument/FCDAnimationChannel.h"
#include "FCDocument/FCDExternalReferenceManager.h"
#include "FCDocument/FCDPlaceHolder.h"

bool FArchiveXML::LoadObject(FCDObject* UNUSED(object), xmlNode* UNUSED(node))
{ 
	return true; 
}					

bool FArchiveXML::LoadExtra(FCDObject* object, xmlNode* extraNode)
{ 
	FCDExtra* extra = (FCDExtra*)object;

	bool status = true;

	// Do NOT assume that we have an <extra> element: we may be parsing a type switch instead.
	FCDEType* parsingType = NULL;
	if (IsEquivalent(extraNode->name, DAE_EXTRA_ELEMENT))
	{
		parsingType = extra->AddType(ReadNodeProperty(extraNode, DAE_TYPE_ATTRIBUTE));
	}
	if (parsingType == NULL) parsingType = extra->GetDefaultType();
	FArchiveXML::LoadSwitch(parsingType, &parsingType->GetObjectType(), extraNode);

	extra->SetDirtyFlag();
	return status;
}				

bool FArchiveXML::LoadExtraNode(FCDObject* object, xmlNode* customNode)
{ 	
	FCDENode* fcdenode = (FCDENode*)object;

	bool status = true;

	// Read in the node's name and children
	fcdenode->SetName((const char*) customNode->name);
	FArchiveXML::LoadExtraNodeChildren(fcdenode, customNode);
	
	// If there are no child nodes, we have a tree leaf: parse in the content and its animation
	if (fcdenode->GetChildNodeCount() == 0)
	{
		fstring content = TO_FSTRING(ReadNodeContentFull(customNode));
		if (!content.empty()) fcdenode->SetContent(content);
	}
	FArchiveXML::LinkAnimatedCustom(fcdenode->GetAnimated(), customNode);

	// Read in the node's attributes
	for (xmlAttr* a = customNode->properties; a != NULL; a = a->next)
	{
		fcdenode->AddAttribute((const char*) a->name, (a->children != NULL) ? TO_FSTRING((const char*) (a->children->content)) : FS(""));
	}

	fcdenode->SetDirtyFlag();
	return status;
}	

bool FArchiveXML::LoadExtraTechnique(FCDObject* object, xmlNode* techniqueNode)
{ 
	// Read in only the child elements: none of the attributes
	return FArchiveXML::LoadExtraNodeChildren((FCDENode*) object, techniqueNode);
}		

bool FArchiveXML::LoadExtraType(FCDObject* object, xmlNode* extraNode)
{
	FCDEType* eType = (FCDEType*)object;

	bool status = true;

	// Do NOT verify that we have an <extra> element: we may be parsing a technique switch instead.

	// Read in the techniques
	xmlNodeList techniqueNodes;
	FindChildrenByType(extraNode, DAE_TECHNIQUE_ELEMENT, techniqueNodes);
	for (xmlNodeList::iterator itN = techniqueNodes.begin(); itN != techniqueNodes.end(); ++itN)
	{
		xmlNode* techniqueNode = (*itN);
		fm::string profile = ReadNodeProperty(techniqueNode, DAE_PROFILE_ATTRIBUTE);
		FCDETechnique* technique = eType->AddTechnique(profile);
		status &= (FArchiveXML::LoadExtraTechnique(technique, techniqueNode));
	}

	eType->SetDirtyFlag();
	return status;
}

bool FArchiveXML::LoadAsset(FCDObject* object, xmlNode* assetNode)
{ 
	FCDAsset* asset = (FCDAsset*)object;

	bool status = true;
	for (xmlNode* child = assetNode->children; child != NULL; child = child->next)
	{
		if (child->type != XML_ELEMENT_NODE) continue;

		fm::string content = ReadNodeContentFull(child);
		if (IsEquivalent(child->name, DAE_CONTRIBUTOR_ASSET_ELEMENT))
		{
			FCDAssetContributor* contributor = asset->AddContributor();
			status &= FArchiveXML::LoadAssetContributor(contributor, child);
		}
		else if (IsEquivalent(child->name, DAE_CREATED_ASSET_PARAMETER))
		{
			FUStringConversion::ToDateTime(content, asset->GetCreationDateTime());
		}
		else if (IsEquivalent(child->name, DAE_KEYWORDS_ASSET_PARAMETER))
		{
			asset->SetKeywords(TO_FSTRING(content));
		}
		else if (IsEquivalent(child->name, DAE_MODIFIED_ASSET_PARAMETER))
		{
			FUStringConversion::ToDateTime(content, asset->GetModifiedDateTime()); 
		}
		else if (IsEquivalent(child->name, DAE_REVISION_ASSET_PARAMETER))
		{
			asset->SetRevision(TO_FSTRING(content));
		}
		else if (IsEquivalent(child->name, DAE_SUBJECT_ASSET_PARAMETER))
		{
			asset->SetSubject(TO_FSTRING(content));
		}
		else if (IsEquivalent(child->name, DAE_TITLE_ASSET_PARAMETER))
		{
			asset->SetTitle(TO_FSTRING(content));
		}
		else if (IsEquivalent(child->name, DAE_UNITS_ASSET_PARAMETER))
		{
			asset->SetUnitName(TO_FSTRING(ReadNodeName(child)));
			asset->SetUnitConversionFactor(FUStringConversion::ToFloat(ReadNodeProperty(child, DAE_METERS_ATTRIBUTE)));
			if (asset->GetUnitName().empty()) asset->SetUnitName(FC("UNKNOWN"));
			if (IsEquivalent(asset->GetUnitConversionFactor(), 0.0f) || asset->GetUnitConversionFactor() < 0.0f) asset->SetUnitConversionFactor(1.0f);
		}
		else if (IsEquivalent(child->name, DAE_UPAXIS_ASSET_PARAMETER))
		{
			if (IsEquivalent(content, DAE_X_UP)) asset->SetUpAxis(FMVector3::XAxis);
			else if (IsEquivalent(content, DAE_Y_UP)) asset->SetUpAxis(FMVector3::YAxis);
			else if (IsEquivalent(content, DAE_Z_UP)) asset->SetUpAxis(FMVector3::ZAxis);
		}
		else
		{
			FUError::Error(FUError::WARNING_LEVEL, FUError::WARNING_UNKNOWN_CHILD_ELEMENT, child->line);
		}
	}

	asset->SetDirtyFlag();
	return status;
}

bool FArchiveXML::LoadAssetContributor(FCDObject* object, xmlNode* contributorNode)
{ 
	FCDAssetContributor* assetContributor = (FCDAssetContributor*)object;

	bool status = true;
	for (xmlNode* child = contributorNode->children; child != NULL; child = child->next)
	{
		if (child->type != XML_ELEMENT_NODE) continue;

		fm::string content = ReadNodeContentFull(child);
		if (IsEquivalent(child->name, DAE_AUTHOR_ASSET_PARAMETER))
		{
			assetContributor->SetAuthor(TO_FSTRING(content));
		}
		else if (IsEquivalent(child->name, DAE_AUTHORINGTOOL_ASSET_PARAMETER))
		{
			assetContributor->SetAuthoringTool(TO_FSTRING(content));
		}
		else if (IsEquivalent(child->name, DAE_COMMENTS_ASSET_PARAMETER))
		{
			assetContributor->SetComments(TO_FSTRING(content));
		}
		else if (IsEquivalent(child->name, DAE_COPYRIGHT_ASSET_PARAMETER))
		{
			assetContributor->SetCopyright(TO_FSTRING(content));
		}
		else if (IsEquivalent(child->name, DAE_SOURCEDATA_ASSET_PARAMETER))
		{
			assetContributor->SetSourceData(TO_FSTRING(content));
		}
		else
		{
			FUError::Error(FUError::WARNING_LEVEL, FUError::WARNING_UNKNOWN_AC_CHILD_ELEMENT, child->line);
		}
	}
	assetContributor->SetDirtyFlag();
	return status;
}	

bool FArchiveXML::LoadEntityReference(FCDObject* UNUSED(object), xmlNode* UNUSED(node))
{
	//
	// Should never reach here
	//
	FUBreak;
	return true;
}

bool FArchiveXML::LoadExternalReferenceManager(FCDObject* UNUSED(object), xmlNode* UNUSED(node))
{
	//
	// Should never reach here
	//
	FUBreak;
	return true;
}

bool FArchiveXML::LoadPlaceHolder(FCDObject* UNUSED(object), xmlNode* UNUSED(node))			
{
	//
	// Should never reach here.
	//
	FUBreak;
	return true;
}

bool FArchiveXML::LoadExtraNodeChildren(FCDENode* fcdenode, xmlNode* customNode)
{
	bool status = true;

	// Read in the node's children
	for (xmlNode* k = customNode->children; k != NULL; k = k->next)
	{
		if (k->type != XML_ELEMENT_NODE) continue;

		FCDENode* node = fcdenode->AddChildNode();
		status &= (FArchiveXML::LoadSwitch(node, &node->GetObjectType(), k));
	}

	fcdenode->SetDirtyFlag();
	return status;
}

void FArchiveXML::FindAnimationChannelsArrayIndices(FCDocument* fcdocument, xmlNode* targetArray, Int32List& animatedIndices)
{
	// Calculte the node's pointer
	fm::string pointer;
	CalculateNodeTargetPointer(targetArray, pointer);
	if (pointer.empty()) return;

	// Retrieve the channels for this pointer and extract their matrix indices.
	FCDAnimationChannelList channels;
	FArchiveXML::FindAnimationChannels(fcdocument, pointer, channels);
	for (FCDAnimationChannelList::iterator it = channels.begin(); it != channels.end(); ++it)
	{
		FCDAnimationChannelDataMap::iterator itData = FArchiveXML::documentLinkDataMap[(*it)->GetDocument()].animationChannelData.find(*it);
		FUAssert(itData != FArchiveXML::documentLinkDataMap[(*it)->GetDocument()].animationChannelData.end(),);
		FCDAnimationChannelData& data = itData->second;

		int32 animatedIndex = FUStringConversion::ParseQualifier(data.targetQualifier);
		if (animatedIndex != -1) animatedIndices.push_back(animatedIndex);
	}
}

void FArchiveXML::RegisterLoadedDocument(FCDocument* document)
{
	fm::pvector<FCDocument> allDocuments;
	FCollada::GetAllDocuments(allDocuments);
	for (FCDocument** it = allDocuments.begin(); it != allDocuments.end(); ++it)
	{
		if ((*it) != document)
		{
			FCDExternalReferenceManager* xrefManager = (*it)->GetExternalReferenceManager();

			for (size_t i = 0; i < xrefManager->GetPlaceHolderCount(); ++i)
			{
				// Set the document to the placeholders that targets it.
				FCDPlaceHolder* pHolder = xrefManager->GetPlaceHolder(i);
				if (pHolder->GetFileUrl() == document->GetFileUrl()) pHolder->LoadTarget(document);
			}
		}
	}

	// On the newly-loaded document, there may be placeholders to process.
	FCDExternalReferenceManager* xrefManager = document->GetExternalReferenceManager();
	for (size_t i = 0; i < xrefManager->GetPlaceHolderCount(); ++i)
	{
		FCDPlaceHolder* pHolder = xrefManager->GetPlaceHolder(i);
				
		// Set the document to the placeholders that targets it.
		for (FCDocument** itD = allDocuments.begin(); itD != allDocuments.end(); ++itD)
		{
			if (pHolder->GetFileUrl() == (*itD)->GetFileUrl()) pHolder->LoadTarget(*itD);
		}
	}
}
