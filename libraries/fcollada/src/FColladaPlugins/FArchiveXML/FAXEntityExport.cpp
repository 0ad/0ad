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

xmlNode* FArchiveXML::WriteObject(FCDObject* UNUSED(object), xmlNode* UNUSED(parentNode))
{
	FUBreak;
	return NULL;
}

xmlNode* FArchiveXML::WriteExtra(FCDObject* object, xmlNode* parentNode)
{
	xmlNode* extraNode = NULL;

	FCDExtra* extra = (FCDExtra*)object;
	if (extra->HasContent())
	{
		size_t typeCount = extra->GetTypeCount();
		for (size_t i = 0; i < typeCount; ++i)
		{
			// Add the <extra> element and its types
			extraNode = FArchiveXML::LetWriteObject(extra->GetType(i), parentNode);
		}
	}
	return extraNode;
}

xmlNode* FArchiveXML::WriteExtraTechnique(FCDObject* object, xmlNode* parentNode)
{
	FCDETechnique* eTechnique = (FCDETechnique*)object;

	// Create the technique for this profile and write out the children
	xmlNode* techniqueNode = AddTechniqueChild(parentNode, eTechnique->GetProfile());
	FArchiveXML::WriteChildrenFCDENode(eTechnique, techniqueNode);
	return techniqueNode;
}

xmlNode* FArchiveXML::WriteExtraType(FCDObject* object, xmlNode* parentNode)
{
	FCDEType* eType = (FCDEType*)object;

	if (eType->GetName().empty() && eType->GetTechniqueCount() == 0) return NULL;

	// Add the <extra> element and its techniques
	xmlNode* extraNode = AddChild(parentNode, DAE_EXTRA_ELEMENT);
	if (!eType->GetName().empty()) AddAttribute(extraNode, DAE_TYPE_ATTRIBUTE, eType->GetName());
	FArchiveXML::WriteTechniquesFCDEType(eType, extraNode);
	return extraNode;
}

xmlNode* FArchiveXML::WriteAsset(FCDObject* object, xmlNode* parentNode)
{
	FCDAsset* asset = (FCDAsset*)object;

	xmlNode* assetNode = AddChild(parentNode, DAE_ASSET_ELEMENT);

	// Update the 'last modified time'
	FCDAsset* hackedAsset = const_cast<FCDAsset*>(asset);
	hackedAsset->GetModifiedDateTime() = FUDateTime::GetNow();

	// Write out the contributors first.
	for (size_t i = 0; i < asset->GetContributorCount(); ++i)
	{
		FArchiveXML::LetWriteObject(asset->GetContributor(i), assetNode);
	}

	// Write out the parameters, one by one and in the correct order.
	AddChild(assetNode, DAE_CREATED_ASSET_PARAMETER, FUStringConversion::ToString(asset->GetCreationDateTime()));
	if (!asset->GetKeywords().empty()) AddChild(assetNode, DAE_KEYWORDS_ASSET_PARAMETER, asset->GetKeywords());
	AddChild(assetNode, DAE_MODIFIED_ASSET_PARAMETER, FUStringConversion::ToString(asset->GetModifiedDateTime()));
	if (!asset->GetRevision().empty()) AddChild(assetNode, DAE_REVISION_ASSET_PARAMETER, asset->GetRevision());
	if (!asset->GetSubject().empty()) AddChild(assetNode, DAE_SUBJECT_ASSET_PARAMETER, asset->GetSubject());
	if (!asset->GetTitle().empty()) AddChild(assetNode, DAE_TITLE_ASSET_PARAMETER, asset->GetTitle());

	// Finally: <unit> and <up_axis>
	if (asset->GetHasUnitsFlag())
	{
		xmlNode* unitNode = AddChild(assetNode, DAE_UNITS_ASSET_PARAMETER);
		AddAttribute(unitNode, DAE_METERS_ATTRIBUTE, asset->GetUnitConversionFactor());
		AddAttribute(unitNode, DAE_NAME_ATTRIBUTE, asset->GetUnitName());
	}

	if (asset->GetHasUpAxisFlag())
	{
		const char* upAxisString = DAE_Y_UP;
		if (IsEquivalent(asset->GetUpAxis(), FMVector3::YAxis)) upAxisString = DAE_Y_UP;
		else if (IsEquivalent(asset->GetUpAxis(), FMVector3::XAxis)) upAxisString = DAE_X_UP;
		else if (IsEquivalent(asset->GetUpAxis(), FMVector3::ZAxis)) upAxisString = DAE_Z_UP;
		AddChild(assetNode, DAE_UPAXIS_ASSET_PARAMETER, upAxisString);
	}
	return assetNode;
	
}

xmlNode* FArchiveXML::WriteAssetContributor(FCDObject* object, xmlNode* parentNode)
{
	FCDAssetContributor* assetContributor = (FCDAssetContributor*)object;

	xmlNode* contributorNode = NULL;
	if (!assetContributor->IsEmpty())
	{
		contributorNode = AddChild(parentNode, DAE_CONTRIBUTOR_ASSET_ELEMENT);
		if (!assetContributor->GetAuthor().empty()) AddChild(contributorNode, DAE_AUTHOR_ASSET_PARAMETER, assetContributor->GetAuthor());
		if (!assetContributor->GetAuthoringTool().empty()) AddChild(contributorNode, DAE_AUTHORINGTOOL_ASSET_PARAMETER, assetContributor->GetAuthoringTool());
		if (!assetContributor->GetComments().empty()) AddChild(contributorNode, DAE_COMMENTS_ASSET_PARAMETER, assetContributor->GetComments());
		if (!assetContributor->GetCopyright().empty()) AddChild(contributorNode, DAE_COPYRIGHT_ASSET_PARAMETER, assetContributor->GetCopyright());
		if (!assetContributor->GetSourceData().empty())
		{
			FUUri uri(assetContributor->GetSourceData());
			fstring sourceDataUrl = uri.GetAbsoluteUri();
			ConvertFilename(sourceDataUrl);
			AddChild(contributorNode, DAE_SOURCEDATA_ASSET_PARAMETER, sourceDataUrl);
		}
	}
	return contributorNode;
}

xmlNode* FArchiveXML::WriteEntityReference(FCDObject* UNUSED(object), xmlNode* UNUSED(parentNode))
{
	//
	// Not reachable
	//
	FUBreak;
	return NULL;

}

xmlNode* FArchiveXML::WriteExternalReferenceManager(FCDObject* UNUSED(object), xmlNode* UNUSED(parentNode))
{
	//
	// Not reachable
	//
	FUBreak;
	return NULL;

}

xmlNode* FArchiveXML::WritePlaceHolder(FCDObject* UNUSED(object), xmlNode* UNUSED(parentNode))
{
	//
	// Not reachable
	//
	FUBreak;
	return NULL;
}


xmlNode* FArchiveXML::WriteExtraNode(FCDObject* object, xmlNode* parentNode)
{
	FCDENode* eNode = (FCDENode*)object;

	xmlNode* customNode = AddChild(parentNode, eNode->GetName(), TO_FSTRING(eNode->GetContent()));
	
	// Write out the attributes
	size_t attributeCount = eNode->GetAttributeCount();
	for (size_t i = 0; i < attributeCount; ++i)
	{
		const FCDEAttribute* attribute = eNode->GetAttribute(i);
		FUXmlWriter::AddAttribute(customNode, attribute->GetName().c_str(), attribute->GetValue());
	}

	// Write out the animated element
	if (eNode->GetAnimated() != NULL && eNode->GetAnimated()->HasCurve())
	{
		FArchiveXML::WriteAnimatedValue(eNode->GetAnimated(), customNode, eNode->GetName());
	}

	// Write out the children
	FArchiveXML::WriteChildrenFCDENode(eNode, customNode);
	return customNode;
}

// Write out the child nodes to the XML tree node
void FArchiveXML::WriteChildrenFCDENode(FCDENode* eNode, xmlNode* customNode)
{
	for (size_t i = 0; i < eNode->GetChildNodeCount(); ++i)
	{
		FArchiveXML::WriteExtraNode(eNode->GetChildNode(i), customNode);
	}
}

void FArchiveXML::WriteTechniquesFCDEType(FCDEType* eType, xmlNode* parentNode)
{
	// Write the techniques within the given parent XML tree node.
	size_t techniqueCount = eType->GetTechniqueCount();
	for (size_t i = 0; i < techniqueCount; ++i)
	{
		FArchiveXML::LetWriteObject(eType->GetTechnique(i), parentNode);
	}
}

void FArchiveXML::WriteTechniquesFCDExtra(FCDExtra* extra, xmlNode* parentNode)
{
	// Write the types within the given parent XML tree node.
	size_t typeCount = extra->GetTypeCount();
	for (size_t i = 0; i < typeCount; ++i)
	{
		FArchiveXML::WriteTechniquesFCDEType(extra->GetType(i), parentNode);
	}
}
