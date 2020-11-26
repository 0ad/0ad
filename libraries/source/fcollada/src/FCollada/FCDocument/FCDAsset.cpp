/*
	Copyright (C) 2005-2007 Feeling Software Inc.
	Portions of the code are:
	Copyright (C) 2005-2007 Sony Computer Entertainment America
	
	MIT License: http://www.opensource.org/licenses/mit-license.php
*/

#include "StdAfx.h"
#include "FCDocument/FCDocument.h"
#include "FCDocument/FCDAsset.h"
#include "FUtils/FUDateTime.h"
#include "FUtils/FUFileManager.h"

//
// FCDAsset
//

ImplementObjectType(FCDAsset);
ImplementParameterObject(FCDAsset, FCDAssetContributor, contributors, new FCDAssetContributor(parent->GetDocument()));

FCDAsset::FCDAsset(FCDocument* document)
:	FCDObject(document)
,	InitializeParameterNoArg(contributors)
,	InitializeParameterNoArg(keywords)
,	InitializeParameterNoArg(revision)
,	InitializeParameterNoArg(subject)
,	InitializeParameterNoArg(title)
,	upAxis(FMVector3::YAxis)
,	unitName(FC("meter")), unitConversionFactor(1.0f)
{
	creationDateTime = modifiedDateTime = FUDateTime::GetNow();
	ResetHasUnitsFlag();
	ResetHasUpAxisFlag();
}

FCDAsset::~FCDAsset()
{
}

FCDAssetContributor* FCDAsset::AddContributor()
{
	FCDAssetContributor* contributor = new FCDAssetContributor(GetDocument());
	contributors.push_back(contributor);
	return contributor;
}

// Clone another asset element.
FCDAsset* FCDAsset::Clone(FCDAsset* clone, bool cloneAllContributors) const
{
	if (clone == NULL) clone = new FCDAsset(const_cast<FCDocument*>(GetDocument()));

	// Clone all the asset-level parameters.
	clone->creationDateTime = creationDateTime;
	clone->modifiedDateTime = FUDateTime::GetNow();
	clone->keywords = keywords;
	clone->revision = revision;
	clone->subject = subject;
	clone->title = title;
	clone->upAxis = upAxis;
	clone->unitName = unitName;
	clone->unitConversionFactor = unitConversionFactor;
	clone->SetFlag(TestFlag(FLAG_HasUnits | FLAG_HasUpAxis));

	if (cloneAllContributors)
	{
		// Clone all the contributors
		for (const FCDAssetContributor** it = contributors.begin(); it != contributors.end(); ++it)
		{
			FCDAssetContributor* clonedContributor = clone->AddContributor();
			(*it)->Clone(clonedContributor);
		}
	}

	return clone;
}

//
// FCDAssetContributor
//

ImplementObjectType(FCDAssetContributor);

FCDAssetContributor::FCDAssetContributor(FCDocument* document)
:	FCDObject(document)
,	InitializeParameterNoArg(author)
,	InitializeParameterNoArg(authoringTool)
,	InitializeParameterNoArg(comments)
,	InitializeParameterNoArg(copyright)
,	InitializeParameterNoArg(sourceData)
{
}

FCDAssetContributor::~FCDAssetContributor()
{
}

FCDAssetContributor* FCDAssetContributor::Clone(FCDAssetContributor* clone) const
{
	if (clone == NULL) clone = new FCDAssetContributor(const_cast<FCDocument*>(GetDocument()));

	clone->author = author;
	clone->authoringTool = authoringTool;
	clone->comments = comments;
	clone->copyright = copyright;
	clone->sourceData = sourceData;

	return clone;
}

// Returns whether this contributor element contain any valid data
bool FCDAssetContributor::IsEmpty() const
{
	return author->empty() && authoringTool->empty() && comments->empty() && copyright->empty() && sourceData->empty();
}
