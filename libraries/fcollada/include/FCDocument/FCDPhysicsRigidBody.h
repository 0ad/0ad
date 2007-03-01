/*
    Copyright (C) 2005-2007 Feeling Software Inc.
    MIT License: http://www.opensource.org/licenses/mit-license.php
*/

#ifndef _FCD_PHYSICSRIGIDBODY_H_
#define _FCD_PHYSICSRIGIDBODY_H_

#ifndef _FCD_ENTITY_H_
#include "FCDocument/FCDEntity.h"
#endif // _FCD_ENTITY_H_

class FCDocument;
class FCDPhysicsMaterial;
class FCDPhysicsParameterGeneric;
class FCDPhysicsShape;
typedef FUObjectContainer<FCDPhysicsParameterGeneric> FCDPhysicsParameterContainer;
typedef FUObjectContainer<FCDPhysicsShape> FCDPhysicsShapeContainer;
template <class Type> class FCDPhysicsParameter;

class FCOLLADA_EXPORT FCDPhysicsRigidBody : public FCDEntity
{
private:
	DeclareObjectType(FCDEntity);

	bool ownsPhysicsMaterial;
	FUObjectPtr<FCDPhysicsMaterial> physicsMaterial;
	
	FCDPhysicsParameterContainer parameters;
	FCDPhysicsShapeContainer physicsShape;

public:
	FCDPhysicsRigidBody(FCDocument* document);
	virtual ~FCDPhysicsRigidBody();

	// Returns the entity type
	virtual Type GetType() const { return FCDEntity::PHYSICS_RIGID_BODY; }

	// Sub-id
	inline const fm::string& GetSubId() const { return Parent::GetDaeId(); }
	inline void SetSubId(const char* sid) { Parent::SetDaeId(sid); }
	inline void SetSubId(const fm::string& sid) { Parent::SetDaeId(sid); }

	// Physics Parameters
	FCDPhysicsParameterContainer& GetParameters() { return parameters; }
	const FCDPhysicsParameterContainer& GetParameters() const { return parameters; }
	size_t GetParameterCount() const { return parameters.size(); }
	FCDPhysicsParameterGeneric* GetParameter(size_t index) { FUAssert(index < parameters.size(), return NULL); return parameters.at(index); }
	const FCDPhysicsParameterGeneric* GetParameter(size_t index) const { FUAssert(index < parameters.size(), return NULL); return parameters.at(index); }
	FCDPhysicsParameterGeneric* FindParameterByReference(const fm::string& reference);
	void CopyParameter(FCDPhysicsParameterGeneric* parameter);
	void AddParameter(FCDPhysicsParameterGeneric* parameter);
	template <class ValueType> FCDPhysicsParameterGeneric* AddParameter(const char* name, const ValueType& value) { FCDPhysicsParameter<ValueType>* p = new FCDPhysicsParameter<ValueType>(GetDocument(), name); p->SetValue(value); AddParameter(p); return p; }

	// Physics Material
	FCDPhysicsMaterial* GetPhysicsMaterial() { return physicsMaterial; }
	const FCDPhysicsMaterial* GetPhysicsMaterial() const { return physicsMaterial; }
	void SetPhysicsMaterial(FCDPhysicsMaterial* physicsMaterial);
	FCDPhysicsMaterial* AddOwnPhysicsMaterial();

	// Physics Shapes
	FCDPhysicsShapeContainer& GetPhysicsShapeList() { return physicsShape; }
	const FCDPhysicsShapeContainer& GetPhysicsShapeList() const { return physicsShape; }
	size_t GetPhysicsShapeCount() const { return physicsShape.size(); }
	FCDPhysicsShape* GetPhysicsShape(size_t index) { FUAssert(index < physicsShape.size(), return NULL) return physicsShape.at(index); }
	const FCDPhysicsShape* GetPhysicsShape(size_t index) const { FUAssert(index < physicsShape.size(), return NULL) return physicsShape.at(index); }
	FCDPhysicsShape* AddPhysicsShape();

	void Flatten();

	// Create a copy of this physics model, with the vertices overwritten
	virtual FCDEntity* Clone(FCDEntity* clone = NULL, bool cloneChildren = false) const;

	// Read in the <physics_model> node of the COLLADA document
	virtual bool LoadFromXML(xmlNode* node);

	// Write out the <physics_model> node to the COLLADA XML tree
	virtual xmlNode* WriteToXML(xmlNode* parentNode) const;
};

#endif // _FCD_PHYSICSRIGIDBODY_H_
