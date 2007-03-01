/*
    Copyright (C) 2005-2007 Feeling Software Inc.
    MIT License: http://www.opensource.org/licenses/mit-license.php
*/

#ifndef _FCD_PHYSICSMODEL_H_
#define _FCD_PHYSICSMODEL_H_

#ifndef _FCD_ENTITY_H_
#include "FCDocument/FCDEntity.h"
#endif // _FCD_ENTITY_H_

class FCDocument;
class FCDPhysicsRigidBody;
class FCDPhysicsRigidConstraint;
class FCDPhysicsModelInstance;

typedef FUObjectContainer<FCDPhysicsModelInstance> FCDPhysicsModelInstanceContainer;
typedef FUObjectContainer<FCDPhysicsRigidBody> FCDPhysicsRigidBodyContainer;
typedef FUObjectContainer<FCDPhysicsRigidConstraint> FCDPhysicsRigidConstraintContainer;

class FCOLLADA_EXPORT FCDPhysicsModel : public FCDEntity
{
private:
	DeclareObjectType(FCDEntity);
	FCDPhysicsModelInstanceContainer instances;
	FCDPhysicsRigidBodyContainer rigidBodies;
	FCDPhysicsRigidConstraintContainer rigidConstraints;

public:
	FCDPhysicsModel(FCDocument* document);
	virtual ~FCDPhysicsModel();

	// Returns the entity type
	virtual Type GetType() const { return FCDEntity::PHYSICS_MODEL; }

	// Direct Accessors
	FCDPhysicsModelInstanceContainer& GetInstances() { return instances; }
	const FCDPhysicsModelInstanceContainer& GetInstances() const { return instances; }
	size_t GetInstanceCount() const { return instances.size(); }
	FCDPhysicsModelInstance* GetInstance(size_t index) { FUAssert(index < instances.size(), return NULL); return instances.at(index); }
	const FCDPhysicsModelInstance* GetInstance(size_t index) const { FUAssert(index < instances.size(), return NULL); return instances.at(index); }
	FCDPhysicsModelInstance* AddPhysicsModelInstance(FCDPhysicsModel* model = NULL);

	FCDPhysicsRigidBodyContainer& GetRigidBodies() { return rigidBodies; }
	const FCDPhysicsRigidBodyContainer& GetRigidBodies() const { return rigidBodies; }
	size_t GetRigidBodyCount() const { return rigidBodies.size(); }
	FCDPhysicsRigidBody* GetRigidBody(size_t index) { FUAssert(index < rigidBodies.size(), return NULL); return rigidBodies.at(index); }
	const FCDPhysicsRigidBody* GetRigidBody(size_t index) const { FUAssert(index < rigidBodies.size(), return NULL); return rigidBodies.at(index); }
	inline FCDPhysicsRigidBody* FindRigidBodyFromSid(const fm::string& sid) { return const_cast<FCDPhysicsRigidBody*>(const_cast<const FCDPhysicsModel*>(this)->FindRigidBodyFromSid(sid)); }
	const FCDPhysicsRigidBody* FindRigidBodyFromSid(const fm::string& sid) const;
	FCDPhysicsRigidBody* AddRigidBody();

	FCDPhysicsRigidConstraintContainer& GetRigidConstraints() { return rigidConstraints; }
	const FCDPhysicsRigidConstraintContainer& GetRigidConstraints() const { return rigidConstraints; }
	size_t GetRigidConstraintCount() const { return rigidConstraints.size(); }
	FCDPhysicsRigidConstraint* GetRigidConstraint(size_t index) { FUAssert(index < GetRigidConstraintCount(), return NULL); return rigidConstraints.at(index); }
	const FCDPhysicsRigidConstraint* GetRigidConstraint(size_t index) const { FUAssert(index < GetRigidConstraintCount(), return NULL); return rigidConstraints.at(index); }
	inline FCDPhysicsRigidConstraint* FindRigidConstraintFromSid(const fm::string& sid) { return const_cast<FCDPhysicsRigidConstraint*>(const_cast<const FCDPhysicsModel*>(this)->FindRigidConstraintFromSid(sid)); }
	const FCDPhysicsRigidConstraint* FindRigidConstraintFromSid(const fm::string& sid) const;
	FCDPhysicsRigidConstraint* AddRigidConstraint();

	// Create a copy of this physics model, with the vertices overwritten
	virtual FCDEntity* Clone(FCDEntity* clone = NULL, bool cloneChildren = false) const;

	// Read in the <physics_model> node of the COLLADA document
	virtual bool LoadFromXML(xmlNode* node);

	// Write out the <physics_model> node to the COLLADA XML tree
	virtual xmlNode* WriteToXML(xmlNode* parentNode) const;
};

#endif // _FCD_PHYSICSMODEL_H_
