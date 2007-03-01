/*
    Copyright (C) 2005-2007 Feeling Software Inc.
    MIT License: http://www.opensource.org/licenses/mit-license.php
*/

/**
	@file FCDExternalReference.h
	This file contains the FCDExternalReference class.
*/

#ifndef _FCD_EXTERNAL_REFERENCE_H_
#define _FCD_EXTERNAL_REFERENCE_H_

#ifndef __FCD_OBJECT_H_
#include "FCDocument/FCDObject.h"
#endif // __FCD_OBJECT_H_
#ifndef _FU_URI_H_
#include "FUtils/FUUri.h"
#endif // _FU_URI_H_

class FCDEntityInstance;
class FCDPlaceHolder;

/**
	A COLLADA external reference for an entity instance.
	FCollada only exposes external references for entity instances.
	Other types of external references: geometry sources, morph targets, etc.
	are not supported.

	The entity instance for an external reference cannot be modified
	and tracks it, so that if is it released manually, the reference is also released.

	The placeholder and the document referenced by the entity instance can
	be modified manually or by the entity instance.

	@ingroup FCDocument
*/
class FCOLLADA_EXPORT FCDExternalReference : public FCDObject, FUObjectTracker
{
private:
	DeclareObjectType(FCDObject);

	FCDEntityInstance* instance;
	FCDPlaceHolder* placeHolder;
	fm::string entityId;

public:
	FCDExternalReference(FCDocument* document, FCDEntityInstance* instance);
	virtual ~FCDExternalReference();

	/** Retrieves the entity instance that this external reference encapsulates.
		@return The entity instance. */
	FCDEntityInstance* GetInstance() { return instance; }
	const FCDEntityInstance* GetInstance() const { return instance; } /**< See above. */

	/** Retrieves the placeholder for the externally referenced COLLADA document.
		@return The COLLADA document placeholder. */
	FCDPlaceHolder* GetPlaceHolder() { return placeHolder; }
	const FCDPlaceHolder* GetPlaceHolder() const { return placeHolder; } /**< See above. */

	/** Retrieves the COLLADA id of the entity that is externally referenced.
		@return The COLLADA id of the referenced entity. */
	const fm::string& GetEntityId() const { return entityId; }
	
	/** Sets the COLLADA id of the referenced entity.
		@param entityId The COLLADA id of the referenced entity. */
	void SetEntityId(const fm::string& id) { entityId = id; SetDirtyFlag(); }

	/** Sets the COLLADA document referenced by the entity instance.
		@param document The COLLADA document referenced by the entity instance. */
	void SetEntityDocument(FCDocument* document);

	/** Sets the COLLADA document place holder that replaces the
		external COLLADA document for this reference.
		@param placeHolder The COLLADA document place holder. */
	void SetPlaceHolder(FCDPlaceHolder* placeHolder);

	/** Retrieves the full URI of the external reference.
		This points to the COLLADA document and the id of the referenced entity.
		@return The referenced entity URI. */
	FUUri GetUri(bool relative = true) const;

	/** Sets the URI of the external reference.
		This points to the COLLADA document and the id of the referenced entity.
		@param _uri The referenced entity URL. */
	void SetUri(const FUUri& uri);

protected:
	virtual void OnObjectReleased(FUObject* object);
};

#endif // _FCD_XREF_ENTITY_H_
