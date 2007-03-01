/*
    Copyright (C) 2005-2007 Feeling Software Inc.
    MIT License: http://www.opensource.org/licenses/mit-license.php
*/

/**
	@file FCDExternalReferenceManager.h
	This file contains the FCDExternalReferenceManager class.
*/

#ifndef _FCD_EXTERNAL_REFERENCE_MANAGER_H_
#define _FCD_EXTERNAL_REFERENCE_MANAGER_H_

#ifndef __FCD_OBJECT_H_
#include "FCDocument/FCDObject.h"
#endif // __FCD_OBJECT_H_

class FCDEntityInstance;
class FCDExternalReference;
class FCDPlaceHolder;

typedef FUObjectContainer<FCDExternalReference> FCDExternalReferenceContainer;
typedef FUObjectContainer<FCDPlaceHolder> FCDPlaceHolderContainer;

class FCOLLADA_EXPORT FCDExternalReferenceManager : public FCDObject
{
private:
	DeclareObjectType(FCDObject);

	FCDExternalReferenceContainer references;
	FCDPlaceHolderContainer placeHolders;

public:
	FCDExternalReferenceManager(FCDocument* document);
	virtual ~FCDExternalReferenceManager();

	FCDExternalReference* AddExternalReference(FCDEntityInstance* instance);

	FCDPlaceHolder* AddPlaceHolder(FCDocument* document);
	FCDPlaceHolder* AddPlaceHolder(const fstring& fileUrl);

	size_t GetPlaceHolderCount() const { return placeHolders.size(); }
	FCDPlaceHolder* GetPlaceHolder(size_t index) { FUAssert(index < placeHolders.size(), return NULL); return placeHolders.at(index); }
	const FCDPlaceHolder* GetPlaceHolder(size_t index) const { FUAssert(index < placeHolders.size(), return NULL); return placeHolders.at(index); }

	const FCDPlaceHolder* FindPlaceHolder(const fstring& fileUrl) const;
	FCDPlaceHolder* FindPlaceHolder(const fstring& fileUrl) { return const_cast<FCDPlaceHolder*>(const_cast<const FCDExternalReferenceManager*>(this)->FindPlaceHolder(fileUrl)); }
	const FCDPlaceHolder* FindPlaceHolder(const FCDocument* document) const;
	FCDPlaceHolder* FindPlaceHolder(FCDocument* document)  { return const_cast<FCDPlaceHolder*>(const_cast<const FCDExternalReferenceManager*>(this)->FindPlaceHolder(document)); }

	static void RegisterLoadedDocument(FCDocument* document);
};

#endif // _FCD_EXTERNAL_REFERENCE_MANAGER_H_
