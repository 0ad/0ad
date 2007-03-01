/*
    Copyright (C) 2005-2007 Feeling Software Inc.
    MIT License: http://www.opensource.org/licenses/mit-license.php
*/

#ifndef _FCD_PHYSICS_MODEL_ENTITY_H_
#define _FCD_PHYSICS_MODEL_ENTITY_H_

#ifndef _FCD_ENTITY_INSTANCE_H_
#include "FCDocument/FCDEntityInstance.h"
#endif // _FCD_ENTITY_INSTANCE_H_

class FCDocument;
class FCDForceField;
class FCDPhysicsRigidBody;
class FCDPhysicsRigidBodyInstance;
class FCDPhysicsRigidConstraint;
class FCDPhysicsRigidConstraintInstance;
typedef FUObjectContainer<FCDEntityInstance> FCDEntityInstanceContainer;

class FCOLLADA_EXPORT FCDPhysicsModelInstance : public FCDEntityInstance
{
private:
	DeclareObjectType(FCDEntityInstance);
	FCDEntityInstanceContainer instances;

public:
	FCDPhysicsModelInstance(FCDocument* document);
	virtual ~FCDPhysicsModelInstance();

	FCDEntityInstanceContainer& GetInstances() { return instances; }
	const FCDEntityInstanceContainer& GetInstances() const { return instances; }
	size_t GetInstanceCount() const { return instances.size(); }
	FCDEntityInstance* GetInstance(size_t index) { FUAssert(index < GetInstanceCount(), return NULL); return instances.at(index); }
	const FCDEntityInstance* GetInstance(size_t index) const { FUAssert(index < GetInstanceCount(), return NULL); return instances.at(index); }
	FCDPhysicsRigidBodyInstance* AddRigidBodyInstance(FCDPhysicsRigidBody* rigidBody = NULL);
	FCDPhysicsRigidConstraintInstance* AddRigidConstraintInstance(FCDPhysicsRigidConstraint* rigidConstraint = NULL);
	FCDEntityInstance* AddForceFieldInstance(FCDForceField* forceField = NULL);

	// FCDEntity override for RTTI-like
	virtual Type GetType() const { return FCDEntityInstance::PHYSICS_MODEL; }

	virtual FCDEntityInstance* Clone(FCDEntityInstance* clone = NULL) const;

	// Load the geometry instance from the COLLADA document
	virtual bool LoadFromXML(xmlNode* instanceNode);

	// Write out the instantiation information to the XML node tree
	virtual xmlNode* WriteToXML(xmlNode* parentNode) const;
};

#endif // _FCD_PHYSICS_MODEL_ENTITY_H_
