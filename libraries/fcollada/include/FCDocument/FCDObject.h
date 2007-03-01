/*
    Copyright (C) 2005-2007 Feeling Software Inc.
    MIT License: http://www.opensource.org/licenses/mit-license.php
*/

/**
	@file FCDObject.h
	This file contains the FCDObject and the FCDObjectWithId classes.
*/

#ifndef __FCD_OBJECT_H_
#define __FCD_OBJECT_H_

#ifndef _FU_OBJECT_H_
#include "FUtils/FUObject.h"
#endif // _FU_OBJECT_H_

/**
	A basic COLLADA document object.
	
	All the objects owned by the COLLADA document derive from this class.
	The FCDocument object is accessible through this interface to all the object which it owns.

	Space for an handle which has no meaning to FCollada is available in this base class, for our users.
	You can therefore attach your own objects to most FCollada objects. If you assign memory buffers
	to the user-specified handle, be aware that FCollada will make no attempt to release it.

	A dirty flag is also available within this object. All FCollada objects should set
	the dirty flag when modifications are made to the objects, but FCollada will never reset it.
	This flag should be used by multi-tier applications. This flag defaults to 'true'.

	@ingroup FCDocument
*/
class FCOLLADA_EXPORT FCDObject : public FUObject
{
private:
	DeclareObjectType(FUObject);

	// The COLLADA document that owns this object
	FCDocument* document;

	// An handle which has no meaning to FCollada but is available for users to
	// attach objects to most FCollada objects.
	void* userHandle;

	// A flag container for dirtiness.
	// This data value will probably be extended to a full bit-flag sometime.
	bool dirty;

public:
	/** Constructor: sets the COLLADA document object.
		@param document The COLLADA document which owns this object. */
	FCDObject(FCDocument* document);

	/** Destructor. */
	virtual ~FCDObject() {}

	/** Retrieves the COLLADA document which owns this object.
		@return The COLLADA document. */
	inline FCDocument* GetDocument() { return document; }
	inline const FCDocument* GetDocument() const { return document; } /**< See above. */

	/** Retrieves whether a given object is a local reference from this object.
		@param object A data object.
		@return Whether a reference from this object to the given object is local. */
	inline bool IsLocal(const FCDObject* object) const { return document == object->document; }

	/** Retrieves whether a given object is an external reference from this object.
		@param object A data object.
		@return Whether a reference from this object to the given object is external. */
	inline bool IsExternal(const FCDObject* object) const { return document != object->document; }

	/** Retrieves the object's user-specified handle.
		This handle is available for users and has no
		meaning to FCollada.
		@return The object user-specified handle. */
	inline void* GetUserHandle() const { return userHandle; }
	
	/** Sets the object's user-specified handle.
		This handle is available for users and has no
		meaning to FCollada.
		@param handle The user-specified handle. */
	inline void SetUserHandle(void* handle) { userHandle = handle; SetDirtyFlag(); }

	/** Sets the dirty flag. */
	inline void SetDirtyFlag() { dirty = true; }

	/** Resets the dirty flag. */
	inline void ResetDirtyFlag() { dirty = false; }

	/** Retrieves the status of the dirty flag. */
	inline bool GetDirtyFlag() const { return dirty; }
};

/**
	A basic COLLADA object which has a unique COLLADA id.
	
	Many COLLADA structures such as entities and sources need a unique COLLADA id.
	The COLLADA document contains a map of all the COLLADA ids known in its scope.
	The interface of the FCDObjectWithId class allows for the retrieval and the modification
	of the unique COLLADA id attached to these objects.

	A unique COLLADA id is built, if none are provided, using the 'baseId' field of the constructor.
	A unique COLLADA id is generated only on demand.

	@ingroup FCDocument
*/
class FCOLLADA_EXPORT FCDObjectWithId : public FCDObject
{
private:
	DeclareObjectType(FCDObject);

	fm::string daeId;
	bool hasUniqueId;

	// An optional sub id.  This can be used to identify bones.
	fm::string daeSubId;

public:
	/** Constructor: sets the prefix COLLADA id to be used if no COLLADA id is provided.
		@param document The COLLADA document which owns this object.
		@param baseId The prefix COLLADA id to be used if no COLLADA id is provided. */
	FCDObjectWithId(FCDocument* document, const char* baseId = "ObjectWithID");

	/** Destructor. */
	virtual ~FCDObjectWithId();

	/** Retrieves the unique COLLADA id for this object.
		If no unique COLLADA id has been previously generated or provided, this function
		has the side-effect of generating a unique COLLADA id.
		@return The unique COLLADA id. */
	const fm::string& GetDaeId() const;

	/** Sets the COLLADA id for this object.
		There is no guarantee that the given COLLADA id will be used, as it may not be unique.
		You can call the GetDaeId function after this call to retrieve the final, unique COLLADA id.
		@param id The wanted COLLADA id for this object. This COLLADA id does not need to be unique.
			If the COLLADA id is not unique, a new unique COLLADA id will be generated. */
	void SetDaeId(const fm::string& id);

	/** Sets the COLLADA id for this object.
		There is no guarantee that the given COLLADA id will be used, as it may not be unique.
		@param id The wanted COLLADA id for this object. This COLLADA id does not need to be unique.
			If the COLLADA id is not unique, a new unique COLLADA id will be generated and
			this formal variable will be modified to contain the new COLLADA id. */
	void SetDaeId(fm::string& id);

	/** Retrieves the optional subid.  This id is neither unique nor guaranteed to exist.
		@return The set SubId of the node. */
	const fm::string& GetSubId() const;

	/** Sets the Sub Id for this object.
		The SubId of an object is not required to be unique.
		@param id The new sub id of the object. */
	void SetSubId(fm::string& newId);
	

	/** [INTERNAL] Release the unique COLLADA id of an object.
		Use this function wisely, as it leaves the object id-less and without a way to automatically
		generate a COLLADA id. */
	void RemoveDaeId();

	/** [INTERNAL] Clones the object. The unique COLLADA id will be copied over to the clone object.
		Use carefully: when a cloned object with an id is released, it
		does remove the unique COLLADA id from the unique name map.
		@param clone The object clone. */
	void Clone(FCDObjectWithId* clone) const;
};

#endif // __FCD_OBJECT_H_
