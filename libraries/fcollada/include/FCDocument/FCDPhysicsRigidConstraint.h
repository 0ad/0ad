/*
    Copyright (C) 2005-2007 Feeling Software Inc.
    MIT License: http://www.opensource.org/licenses/mit-license.php
*/

#ifndef _FCD_PHYSICS_RIGID_CONSTRAINT_H_
#define _FCD_PHYSICS_RIGID_CONSTRAINT_H_

#ifndef _FCD_ENTITY_H_
#include "FCDocument/FCDEntity.h"
#endif // _FCD_ENTITY_H_
#ifndef _FCD_TRANSFORM_H_
#include "FCDocument/FCDTransform.h" /** @todo Remove this include by moving the FCDTransform::Type enum to FUDaeEnum.h. */
#endif // _FCD_TRANSFORM_H_
#ifndef _FU_DAE_ENUM_H_
#include "FUtils/FUDaeEnum.h"
#endif // _FU_DAE_ENUM_H_

class FCDocument;
class FCDTransform;
class FCDPhysicsModel;
class FCDPhysicsRigidBody;
class FCDSceneNode;
typedef FUObjectContainer<FCDTransform> FCDTransformContainer;

class FCOLLADA_EXPORT FCDPhysicsRigidConstraint : public FCDEntity
{
private:
	DeclareObjectType(FCDEntity);
	FCDPhysicsModel* parent;

	fm::string sid;

	bool enabled;
	bool interpenetrate;
	FUObjectPtr<FCDPhysicsRigidBody> referenceRigidBody;
	FUObjectPtr<FCDSceneNode> referenceNode;
	FUObjectPtr<FCDPhysicsRigidBody> targetRigidBody;
	FUObjectPtr<FCDSceneNode> targetNode;
	FMVector3 limitsLinearMin;
	FMVector3 limitsLinearMax;
	FMVector3 limitsSCTMin;
	FMVector3 limitsSCTMax;

	float springLinearStiffness;
	float springLinearDamping;
	float springLinearTargetValue;

	float springAngularStiffness;
	float springAngularDamping;
	float springAngularTargetValue;

	FCDTransformContainer transformsRef;
	FCDTransformContainer transformsTar;

public:
	FCDPhysicsRigidConstraint(FCDocument* document, FCDPhysicsModel* _parent);
	virtual ~FCDPhysicsRigidConstraint();

	// Returns the entity type
	virtual Type GetType() const { return PHYSICS_RIGID_CONSTRAINT; }
	FCDPhysicsModel* GetParent() { return parent; }
	const FCDPhysicsModel* GetParent() const { return parent; }

	/** Retrieves the wanted sub-id for this transform.
		A wanted sub-id will always be exported, even if the transform is not animated.
		But the wanted sub-id may be modified if it isn't unique within the scope.
		@return The sub-id. */
	inline const fm::string& GetSubId() const { return sid; }

	/** Sets the wanted sub-id for this transform.
		A wanted sub-id will always be exported, even if the transform is not animated.
		But the wanted sub-id may be modified if it isn't unique within the scope.
		@param subId The wanted sub-id. */
	inline void SetSubId(const fm::string& subId) { sid = subId; SetDirtyFlag(); }
	inline void SetSubId(const char* subId) { sid = subId; SetDirtyFlag(); } /**< See above. */

	const bool& GetEnabled() const { return enabled;}
	void SetEnabled(bool _enabled) { enabled = _enabled; SetDirtyFlag(); }
	const bool& GetInterpenetrate() const { return interpenetrate;}
	void SetInterpenetrate(bool _interpenetrate) { interpenetrate = _interpenetrate; SetDirtyFlag(); }

	FCDPhysicsRigidBody* GetReferenceRigidBody() { return referenceRigidBody; }
	FCDPhysicsRigidBody* GetTargetRigidBody() { return targetRigidBody; }
	const FCDPhysicsRigidBody* GetReferenceRigidBody() const { return referenceRigidBody; }
	const FCDPhysicsRigidBody* GetTargetRigidBody() const { return targetRigidBody; }
	void SetReferenceRigidBody(FCDPhysicsRigidBody* _referenceRigidBody) { referenceRigidBody = _referenceRigidBody; referenceNode = NULL; SetDirtyFlag(); }
	void SetTargetRigidBody(FCDPhysicsRigidBody* _targetRigidBody) { targetRigidBody = _targetRigidBody; targetNode = NULL; SetDirtyFlag(); }

	FCDSceneNode* GetReferenceNode() { return referenceNode; }
	FCDSceneNode* GetTargetNode() { return targetNode; }
	const FCDSceneNode* GetReferenceNode() const { return referenceNode; }
	const FCDSceneNode* GetTargetNode() const { return targetNode; }
	void SetReferenceNode(FCDSceneNode* _referenceNode) { referenceNode = _referenceNode; referenceRigidBody = NULL; SetDirtyFlag(); }
	void SetTargetNode(FCDSceneNode* _targetNode) { targetNode = _targetNode; targetRigidBody = NULL; SetDirtyFlag(); }

	FCDTransformContainer& GetTransformsRef() { return transformsRef; }
	FCDTransformContainer& GetTransformsTar() { return transformsTar; }
	const FCDTransformContainer& GetTransformsRef() const { return transformsRef; }
	const FCDTransformContainer& GetTransformsTar() const { return transformsTar; }
	FCDTransform* AddTransformRef(FCDTransform::Type type, size_t index = (size_t)-1);
	FCDTransform* AddTransformTar(FCDTransform::Type type, size_t index = (size_t)-1);

	const FMVector3& GetLimitsLinearMin() const { return limitsLinearMin;}
	const FMVector3& GetLimitsLinearMax() const { return limitsLinearMax;}
	const FMVector3& GetLimitsSCTMin() const { return limitsSCTMin;}
	const FMVector3& GetLimitsSCTMax() const { return limitsSCTMax;}
	void SetLimitsLinearMin(const FMVector3& _limitsLinearMin) { limitsLinearMin = _limitsLinearMin; SetDirtyFlag(); }
	void SetLimitsLinearMax(const FMVector3& _limitsLinearMax) { limitsLinearMax = _limitsLinearMax; SetDirtyFlag(); }
	void SetLimitsSCTMin(const FMVector3& _limitsSCTMin) { limitsSCTMin = _limitsSCTMin; SetDirtyFlag(); }
	void SetLimitsSCTMax(const FMVector3& _limitsSCTMax) { limitsSCTMax = _limitsSCTMax; SetDirtyFlag(); }

	float GetSpringLinearStiffness() const { return springLinearStiffness;}
	float GetSpringLinearDamping() const { return springLinearDamping;}
	float GetSpringLinearTargetValue() const { return springLinearTargetValue;}
	float GetSpringAngularStiffness() const { return springAngularStiffness;}
	float GetSpringAngularDamping() const { return springAngularDamping;}
	float GetSpringAngularTargetValue() const { return springAngularTargetValue;}
	void SetSpringLinearStiffness(float _springLinearStiffness) { springLinearStiffness = _springLinearStiffness; SetDirtyFlag(); }
	void SetSpringLinearDamping(float _springLinearDamping) { springLinearDamping = _springLinearDamping; SetDirtyFlag(); }
	void SetSpringLinearTargetValue(float _springLinearTargetValue) { springLinearTargetValue = _springLinearTargetValue; SetDirtyFlag(); }
	void SetSpringAngularStiffness(float _springAngularStiffness) { springAngularStiffness = _springAngularStiffness; SetDirtyFlag(); }
	void SetSpringAngularDamping(float _springAngularDamping) { springAngularDamping = _springAngularDamping; SetDirtyFlag(); }
	void SetSpringAngularTargetValue(float _springAngularTargetValue) { springAngularTargetValue = _springAngularTargetValue; SetDirtyFlag(); }

	FCDAnimated* GetAnimatedEnabled();
	FCDAnimated* GetAnimatedInterpenetrate();
	const FCDAnimated* GetAnimatedEnabled() const;
	const FCDAnimated* GetAnimatedInterpenetrate() const;

	virtual FCDEntity* Clone(FCDEntity* clone = NULL, bool cloneChildren = false) const;
	virtual bool LoadFromXML(xmlNode* node);
	virtual xmlNode* WriteToXML(xmlNode* parentNode) const;
};

#endif // _FCD_PHYSICS_RIGID_CONSTRAINT_H_
