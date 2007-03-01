/*
    Copyright (C) 2005-2007 Feeling Software Inc.
    MIT License: http://www.opensource.org/licenses/mit-license.php
*/

/**
	@file FCDEmitterInstance.h
	This file contains the FCDEmitterInstance class.
*/

#ifndef _FCD_EMITTER_INSTANCE_H_
#define _FCD_EMITTER_INSTANCE_H_

#ifndef _FCD_ENTITY_INSTANCE_H_
#include "FCDocument/FCDEntityInstance.h"
#endif // _FCD_ENTITY_INSTANCE_H_

class FCDGeometryPolygons;
class FCDMaterial;
class FCDMaterialInstance;

typedef FUObjectContainer<FCDMaterialInstance> FCDMaterialInstanceContainer;

/**
	A COLLADA emitter instance.

	As with geometric objects, the material assigned to the particles of an
	emitter is instance-dependant.
*/
class FCDEmitterInstance : public FCDEntityInstance
{
private:
    DeclareObjectType(FCDEntityInstance);

	FCDMaterialInstanceContainer materialInstances;

public:
	/** Constructor: do not use directly.
		Instead, use the FCDSceneNode::AddInstance function.
		@param document The COLLADA document that owns the emitter instance.
		@param parent The parent visual scene node.
		@param entityType The type of the entity to instantiate. Unless this class
			is overwritten, FCDEntity::CONTROLLER should be given. */
	FCDEmitterInstance(FCDocument* document, FCDSceneNode* parent, FCDEntity::Type entityType = FCDEntity::CONTROLLER);

	/** Destructor. */
	virtual ~FCDEmitterInstance();

	/** Clones the emitter instance.
		@param clone The emitter instance to become the clone.
			If this pointer is NULL, a new emitter instance will be created
			and you will need to release the returned pointer.
		@return The clone. */
	virtual FCDEntityInstance* Clone(FCDEntityInstance* clone = NULL) const;

	/** Retrieves the number of material instances declared for this emitter instance.
		@return The number of material instances. */
	inline size_t GetMaterialInstanceCount() const { return materialInstances.size(); }

	/** Retrieves a specific material instance from the list for this emitter instance.
		@param index An index to a material instance.
		@return The indexed material instance. This pointer will be NULL
			if the given index is out-of-bounds. */
	inline FCDMaterialInstance* GetMaterialInstance(size_t index) { FUAssert(index < materialInstances.size(), return NULL); return materialInstances.at(index); }
	inline const FCDMaterialInstance* GetMaterialInstance(size_t index) const { FUAssert(index < materialInstances.size(), return NULL); return materialInstances.at(index); } /**< See above. */

	/** Adds a new material instance.
		Since the emitted particles may be meshes, an emitter instance may contain more
		than one material instance. In these cases, the rules for associating materials
		with geometries are applied as for FCDGeometryInstance objects.
		@return The new material instance. */
	FCDMaterialInstance* AddMaterialInstance();

	/** Adds a new material instance.
		Since the emitted particles may be meshes, an emitter instance may contain more
		than one material instance. In these cases, the rules for associating materials
		with geometries are applied as for FCDGeometryInstance objects.
		@param material The instanced material. This pointer can be NULL.
		@param polygons A polygon set of the instanced mesh to associated with
			the given material. This pointer can be NULL.
		@return The new material instance. */
	FCDMaterialInstance* AddMaterialInstance(FCDMaterial* material, FCDGeometryPolygons* polygons = NULL);

	/** [INTERNAL] Reads in the emitter instance from a given extra tree node.
		@param instanceNode The extra tree node.
		@return The status of the import. If the status is 'false',
			it may be dangerous to extract information from the instance.*/
	virtual bool LoadFromExtra(FCDENode* instanceNode);

	/** [INTERNAL] Reads in the emitter instance properties from a XML tree node.
		@param instanceNode The XML tree node.
		@return The status of the import. If the status is 'false', it may be dangerous to
			use the emitter instance. */
	virtual bool LoadFromXML(xmlNode* instanceNode);

	/** [INTERNAL] Writes out the emitter instance to the given extra tree node.
		@param parentNode The extra tree parent node in which to insert the instance.
		@return The create extra tree node. */
	virtual FCDENode* WriteToExtra(FCDENode* parentNode) const;

	/** [INTERNAL] Writes out the emitter instance to the given COLLADA XML tree node.
		@param parentNode The COLLADA XML parent node in which to insert the instance.
		@return The created XML tree node. */
	virtual xmlNode* WriteToXML(xmlNode* parentNode) const;
};

#endif // _FCD_EMITTER_INSTANCE_H_
