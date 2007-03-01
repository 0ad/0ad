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
	@file FCDGeometryInstance.h
	This file contains the FCDGeometryInstance class.
*/
#ifndef _FCD_GEOMETRY_ENTITY_H_
#define _FCD_GEOMETRY_ENTITY_H_

#ifndef _FCD_ENTITY_INSTANCE_H_
#include "FCDocument/FCDEntityInstance.h"
#endif // _FCD_ENTITY_INSTANCE_H_

class FCDocument;
class FCDMaterial;
class FCDMaterialInstance;
class FCDGeometryPolygons;

typedef fm::pvector<FCDMaterialInstance> FCDMaterialInstanceList; /**< A dynamically-sized array for material instances. */
typedef FUObjectContainer<FCDMaterialInstance> FCDMaterialInstanceContainer; /**< A dynamically-sized containment array for material instances. */

/**
	A COLLADA geometry instance.
	It is during the instantiation of geometries that the mesh polygons
	are attached to actual materials.
*/
class FCOLLADA_EXPORT FCDGeometryInstance : public FCDEntityInstance
{
private:
	DeclareObjectType(FCDEntityInstance);
	FCDMaterialInstanceContainer materials;

public:
	FCDGeometryInstance(FCDocument* document, FCDSceneNode* parent, FCDEntity::Type entityType = FCDEntity::GEOMETRY);
	virtual ~FCDGeometryInstance();

	/** Retrieves the entity instance class type.
		This is used to determine the up-class for the entity instance object.
		@deprecated Instead use: FCDEntityInstance::HasType(FCDGeometryInstance::GetClassType())
		@return The class type: GEOMETRY. */
	virtual Type GetType() const { return GEOMETRY; }

	// Access Bound Materials
	FCDMaterialInstance* FindMaterialInstance(const fchar* semantic);
	inline FCDMaterialInstance* FindMaterialInstance(const fstring& semantic) { return FindMaterialInstance(semantic.c_str()); }
	const FCDMaterialInstance* FindMaterialInstance(const fchar* semantic) const;
	inline const FCDMaterialInstance* FindMaterialInstance(const fstring& semantic) const { return FindMaterialInstance(semantic.c_str()); }
	inline size_t GetMaterialInstanceCount() const { return materials.size(); }
	inline FCDMaterialInstance* GetMaterialInstance(size_t index) { FUAssert(index < materials.size(), return NULL); return materials.at(index); }
	inline const FCDMaterialInstance* GetMaterialInstance(size_t index) const { FUAssert(index < materials.size(), return NULL); return materials.at(index); }
	inline FCDMaterialInstanceContainer& GetMaterialInstances() { return materials; }
	inline const FCDMaterialInstanceContainer& GetMaterialInstances() const { return materials; }
	FCDMaterialInstance* AddMaterialInstance();
	FCDMaterialInstance* AddMaterialInstance(FCDMaterial* material, FCDGeometryPolygons* polygons);
	FCDMaterialInstance* AddMaterialInstance(FCDMaterial* material, const fchar* semantic);
	inline FCDMaterialInstance* AddMaterialInstance(FCDMaterial* material, const fstring& semantic) { return AddMaterialInstance(material, semantic.c_str()); }

	/** Clones the geometry instance.
		@param clone The geometry instance to become the clone.
		@return The cloned geometry instance. */
	virtual FCDEntityInstance* Clone(FCDEntityInstance* clone = NULL) const;

	// Load the geometry instance from the COLLADA document
	virtual bool LoadFromXML(xmlNode* instanceNode);

	// Write out the instantiation information to the XML node tree
	virtual xmlNode* WriteToXML(xmlNode* parentNode) const;
};

#endif // _FCD_GEOMETRY_ENTITY_H_
