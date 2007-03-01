/*
    Copyright (C) 2005-2007 Feeling Software Inc.
    MIT License: http://www.opensource.org/licenses/mit-license.php
*/

#ifndef _FCD_ASSET_H_
#define _FCD_ASSET_H_

#ifndef __FCD_OBJECT_H_
#include "FCDocument/FCDObject.h"
#endif // __FCD_OBJECT_H_
#ifndef _FU_DATETIME_H_
#include "FUtils/FUDateTime.h"
#endif // _FU_DATETIME_H_

class FCDAssetContributor;
typedef FUObjectContainer<FCDAssetContributor> FCDAssetContributorContainer;

class FCOLLADA_EXPORT FCDAsset : public FCDObject
{
private:
	DeclareObjectType(FCDObject);

	FCDAssetContributorContainer contributors;
	FUDateTime creationDateTime;
	FUDateTime modifiedDateTime;
	fstring keywords;
	fstring revision;
	fstring subject;
	fstring title;
	FMVector3 upAxis;

	// <unit>
	fstring unitName;
	float unitConversionFactor;

	// existence flags
	bool hasUnits, hasUpAxis;

public:
	FCDAsset(FCDocument* document);
	virtual ~FCDAsset();

	// Direct contributor list access
	inline FCDAssetContributorContainer& GetContributors() { return contributors; }
	inline const FCDAssetContributorContainer& GetContributors() const { return contributors; }
	inline size_t GetContributorCount() const { return contributors.size(); }
	inline FCDAssetContributor* GetContributor(size_t index) { FUAssert(index < GetContributorCount(), return NULL); return contributors.at(index); }
	inline const FCDAssetContributor* GetContributor(size_t index) const { FUAssert(index < GetContributorCount(), return NULL); return contributors.at(index); }
	inline FCDAssetContributor* AddContributor() { return contributors.Add(GetDocument()); SetDirtyFlag(); }

	// Direct accessors
	inline const FUDateTime& GetCreationDateTime() const { return creationDateTime; }
	inline const FUDateTime& GetModifiedDateTime() const { return modifiedDateTime; }
	inline const fstring& GetKeywords() const { return keywords; }
	inline const fstring& GetRevision() const { return revision; }
	inline const fstring& GetSubject() const { return subject; }
	inline const fstring& GetTitle() const { return title; }
	inline const FMVector3& GetUpAxis() const { return upAxis; }
	inline const fstring& GetUnitName() const { return unitName; }
	inline float GetUnitConversionFactor() const { return unitConversionFactor; }

	// Direct mutators
	inline void SetKeywords(const fstring& _keywords) { keywords = _keywords; SetDirtyFlag(); }
	inline void SetRevision(const fstring& _revision) { revision = _revision; SetDirtyFlag(); }
	inline void SetSubject(const fstring& _subject) { subject = _subject; SetDirtyFlag(); }
	inline void SetTitle(const fstring& _title) { title = _title; SetDirtyFlag(); }
	inline void SetUpAxis(const FMVector3& _upAxis) { upAxis = _upAxis; hasUpAxis = true; SetDirtyFlag(); }
	inline void SetUnitName(const fstring& _unitName) { unitName = _unitName; SetDirtyFlag(); }
	inline void SetUnitConversionFactor(float factor) { unitConversionFactor = factor; hasUnits = true; SetDirtyFlag(); }

	// Access/Modify the existence flags
	inline bool HasUpAxis() { return hasUpAxis; }
	inline bool HasUnits() { return hasUnits; }
	inline void ResetUpAxis() { hasUpAxis = false; }
	inline void ResetUnits() { hasUnits = false; }

	// Clone another asset element.
	FCDAsset* Clone(FCDAsset* clone = NULL, bool cloneAllContributors = true) const;

	// Read in the <asset> element from a COLLADA XML document
	bool LoadFromXML(xmlNode* assetNode);

	// Write out the <asset> element to a COLLADA XML node tree
	// Calling this function will update the 'last modified' timestamp.
	xmlNode* WriteToXML(xmlNode* parentNode) const;
};

// Encapsulates the <asset><contributor> element
class FCOLLADA_EXPORT FCDAssetContributor : public FCDObject
{
private:
	DeclareObjectType(FCDObject);

	fstring author;
	fstring authoringTool;
	fstring comments;
	fstring copyright;
	fstring sourceData;

public:
	FCDAssetContributor(FCDocument* document);
	virtual ~FCDAssetContributor();

	// Direct accessors
	inline const fstring& GetAuthor() const { return author; }
	inline const fstring& GetAuthoringTool() const { return authoringTool; }
	inline const fstring& GetComments() const { return comments; }
	inline const fstring& GetCopyright() const { return copyright; }
	inline const fstring& GetSourceData() const { return sourceData; }

	// Direct mutators
	inline void SetAuthor(const fstring& _author) { author = _author; SetDirtyFlag(); }
	inline void SetAuthoringTool(const fstring& _authoringTool) { authoringTool = _authoringTool; SetDirtyFlag(); }
	inline void SetComments(const fstring& _comments) { comments = _comments; SetDirtyFlag(); }
	inline void SetCopyright(const fstring& _copyright) { copyright = _copyright; SetDirtyFlag(); }
	inline void SetSourceData(const fstring& _sourceData) { sourceData = _sourceData; SetDirtyFlag(); }

	// Returns whether this contributor element contain any valid data
	bool IsEmpty() const;

	// Clone another asset contributor.
	FCDAssetContributor* Clone(FCDAssetContributor* clone = NULL) const;

	// Read in the <asset><contributor> element from a COLLADA XML document
	bool LoadFromXML(xmlNode* contributorNode);

	// Write out the <asset><contributor> element to a COLLADA XML node tree
	xmlNode* WriteToXML(xmlNode* parentNode) const;
};

#endif // _FCD_ASSET_H_
