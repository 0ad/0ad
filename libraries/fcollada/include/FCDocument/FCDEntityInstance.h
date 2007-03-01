/*
    Copyright (C) 2005-2007 Feeling Software Inc.
    MIT License: http://www.opensource.org/licenses/mit-license.php
*/
/*
	Based on the FS Import classes:
	Copyright (C) 2005-2006 Feeling Software Inc
	Copyright (C) 2005-2006 Autodesk Media Entertainment
	MIT License: http://www.opensource.org/licenses/mit-license.php
*/

/**
	@file FCDEntityInstance.h
	This file contains the FCDEntityInstance class.
*/

#ifndef _FCD_ENTITY_INSTANCE_H_
#define _FCD_ENTITY_INSTANCE_H_

class FCDocument;
class FCDEntity;
class FCDExternalReference;
class FCDENode;
class FCDSceneNode;

#ifndef _FCD_ENTITY_H_
#include "FCDocument/FCDEntity.h"
#endif // _FCD_ENTITY_H_

/**
	A COLLADA entity instance.
	COLLADA allows for quite a bit of per-instance settings
	for entities. This information is held by the up-classes of this class.
	This base class is simply meant to hold the entity that is instantiated.

	The entity instance tracks the entity, so that when an entity is released,
	all its instances are released.

	@ingroup FCDocument
*/
class FCOLLADA_EXPORT FCDEntityInstance : public FCDObject, FUObjectTracker
{
public:
	/** The class type of the entity instance class.
		Used this information to up-cast an entity instance. */
	enum Type
	{
		SIMPLE, /**< A simple entity instance that has no per-instance information.
					This is used for lights, cameras, physics materials and force fields: there is no up-class. */
		GEOMETRY, /**< A geometry entity(FCDGeometryInstance). */
		CONTROLLER, /**< A controller entity(FCDControllerInstance). */
		MATERIAL, /**< A material entity(FCDMaterialInstance). */
		PHYSICS_MODEL, /**< A physics model(FCDPhysicsModelInstance). */
		PHYSICS_RIGID_BODY, /**< A physics rigid body(FCDPhysicsRigidBodyInstance). */
		PHYSICS_RIGID_CONSTRAINT /**< A physics rigid constraint(FCDPhysicsRigidConstraintInstance). */
	};

private:
	DeclareObjectType(FCDObject);
	FCDSceneNode* parent;

	FCDEntity* entity;
	FCDEntity::Type entityType;

	FCDExternalReference* externalReference;
	bool useExternalReferenceID;

public:
	/** Constructor: do not use directly.
		Instead, use the appropriate allocation function.
		For scene node instance: FCDSceneNode::AddInstance.
		@param document The COLLADA document that owns the entity instance.
		@param parent The parent visual scene node.
		@param type The type of entity to instantiate. */
	FCDEntityInstance(FCDocument* document, FCDSceneNode* parent, FCDEntity::Type type);

	/** Destructor. */
	virtual ~FCDEntityInstance();

	/** Retrieves the entity instance class type.
		This is used to determine the up-class for the entity instance object.
		@deprecated Instead use: FCDEntityInstance::HasType().
		@return The class type: SIMPLE for entity instances with no up-class. */
	virtual Type GetType() const { return SIMPLE; }

	/** Retrieves the instantiated entity type.
		The instantiated entity type will never change.
		@return The instantiated entity type. */
	FCDEntity::Type GetEntityType() const { return entityType; }

	/** Retrieves the instantiated entity.
		@return The instantiated entity. */
	inline FCDEntity* GetEntity() { return const_cast<FCDEntity*>(const_cast<const FCDEntityInstance*>(this)->GetEntity()); }
	const FCDEntity* GetEntity() const; /**< See above. */

	/** Sets the instantiated entity.
		The type of the entity will be verified.
		@param entity The instantiated entity. */
	void SetEntity(FCDEntity* entity);

	/** [INTERNAL] Loads the instantiated entity from an external document.
		This function is called by the document placeholder when the external
		document is loaded.
		@param externalDocument The externally referenced document.
		@param daeId The external referenced entity COLLADA id. */
	void LoadExternalEntity(FCDocument* externalDocument, const fm::string& daeId);

	/** Retrieves whether this entity instance points to an external entity.
		@return Whether this is an external entity instantiation. */
	bool IsExternalReference() const { return externalReference != NULL; }

	/** Retrieves the external reference for this entity instance.
		@return The external reference for this entity instance. This
			pointer will be NULL if the entity instance is local. */
	FCDExternalReference* GetExternalReference();
	const FCDExternalReference* GetExternalReference() const { return externalReference; }

	/** Retrieves if we should use the external reference ID instead of the entity ID in
		XML output.
		@return The boolean value.*/
	bool IsUsingExternalReferenceID() const { return useExternalReferenceID; }

	/** Sets if we should use the external reference ID instead of the entity ID in
		XML output.
		@param The boolean value.*/
	void UseExternalReferenceID( bool u ) { useExternalReferenceID = u; }

	/** Retrieves the parent of the entity instance.
		@return the parent visual scene node. This pointer will be NULL
			when the instance is not created in the visual scene graph. */
	inline FCDSceneNode* GetParent() { return parent; }
	inline const FCDSceneNode* GetParent() const { return parent; }

	/** [INTERNAL] Links controllers with their created nodes */
	virtual bool LinkImport(){ return true; };

	/** [INTERNAL] Links controllers with their created nodes */
	virtual bool LinkExport(){ return true; };

	/** [INTERNAL] Reads in the entity instance from a given extra tree node.
		@param instanceNode The extra tree node.
		@return The status of the import. If the status is 'false',
			it may be dangerous to extract information from the instance.*/
	virtual bool LoadFromExtra(FCDENode* instanceNode);

	/** [INTERNAL] Reads in the entity instance from a given COLLADA XML tree node.
		@param instanceNode The COLLADA XML tree node.
		@return The status of the import. If the status is 'false',
			it may be dangerous to extract information from the instance.*/
	virtual bool LoadFromXML(xmlNode* instanceNode);

	/** [INTERNAL] Writes out the entity instance to the given extra tree node.
		@param parentNode The extra tree parent node in which to insert the instance.
		@return The create extra tree node. */
	virtual FCDENode* WriteToExtra(FCDENode* parentNode) const;

	/** [INTERNAL] Writes out the entity instance to the given COLLADA XML tree node.
		@param parentNode The COLLADA XML parent node in which to insert the instance.
		@return The created XML tree node. */
	virtual xmlNode* WriteToXML(xmlNode* parentNode) const;

	/** Clones the entity instance.
		@param clone The entity instance to become the clone.
		@return The cloned entity instance. */
	virtual FCDEntityInstance* Clone(FCDEntityInstance* clone = NULL) const;

protected:
	/** [INTERNAL] Retrieves the COLLADA name for the instantiation of a given entity type.
		@param type The entity class type.
		@return The COLLADA name to instantiate an entity of the given class type. */
	static const char* GetInstanceClassType(FCDEntity::Type type);

	/** Callback when the instantiated entity is being released.
		@param object A tracked object. */
	virtual void OnObjectReleased(FUObject* object);
};

#endif // _FCD_ENTITY_INSTANCE_H_
