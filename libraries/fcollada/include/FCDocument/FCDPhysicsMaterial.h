/*
    Copyright (C) 2005-2007 Feeling Software Inc.
    MIT License: http://www.opensource.org/licenses/mit-license.php
*/

#ifndef _FCD_PHYSICSMATERIAL_H_
#define _FCD_PHYSICSMATERIAL_H_

#ifndef _FCD_ENTITY_H_
#include "FCDocument/FCDEntity.h"
#endif // _FCD_ENTITY_H_

class FCDocument;

class FCOLLADA_EXPORT FCDPhysicsMaterial : public FCDEntity
{
private:
	DeclareObjectType(FCDEntity);
	float staticFriction;
	float dynamicFriction;
	float restitution;

public:
	FCDPhysicsMaterial(FCDocument* document);
	virtual ~FCDPhysicsMaterial();

	// Accessors
	virtual Type GetType() const { return FCDEntity::PHYSICS_MATERIAL; }
	float GetStaticFriction() const { return staticFriction; }
	void  SetStaticFriction(float _staticFriction) { staticFriction = _staticFriction; SetDirtyFlag(); }
	float GetDynamicFriction() const { return dynamicFriction; }
	void  SetDynamicFriction(float _dynamicFriction) { dynamicFriction = _dynamicFriction; SetDirtyFlag(); }
	float GetRestitution() const { return restitution; }
	void  SetRestitution(float _restitution) { restitution = _restitution; SetDirtyFlag(); }

	// Cloning
	virtual FCDEntity* Clone(FCDEntity* clone = NULL, bool cloneChildren = false) const;

	// Parse COLLADA document's <material> element
	virtual bool LoadFromXML(xmlNode* physicsMaterialNode);

	// Write out the <material> element to the COLLADA XML tree
	virtual xmlNode* WriteToXML(xmlNode* parentNode) const;
};

#endif // _FCD_MATERIAL_H_
