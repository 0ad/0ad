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

#ifndef _FCD_PHYSICS_SCENE_NODE_
#define _FCD_PHYSICS_SCENE_NODE_

#ifndef _FCD_ENTITY_H_
#include "FCDocument/FCDEntity.h"
#endif // _FCD_ENTITY_H_

class FCDocument;
class FCDEntityInstance;
class FCDExtra;
class FCDPhysicsModel;
class FCDPhysicsModelInstance;

typedef fm::pvector<FCDPhysicsModelInstance> FCDPhysicsModelInstanceList;
typedef FUObjectContainer<FCDPhysicsModelInstance> FCDPhysicsModelInstanceContainer;

class FCOLLADA_EXPORT FCDPhysicsScene : public FCDEntity
{
private:
	DeclareObjectType(FCDEntity);
	FMVector3 gravity;
	float timestep;
	FCDPhysicsModelInstanceContainer instances;

public:
	FCDPhysicsScene(FCDocument* document);
	virtual ~FCDPhysicsScene();

	// Returns the entity type
	virtual Type GetType() const { return PHYSICS_SCENE_NODE; }
	
	FCDPhysicsModelInstanceContainer& GetInstances() { return instances; }
	const FCDPhysicsModelInstanceContainer& GetInstances() const { return instances; }
	size_t GetNumInstances() const { return instances.size(); };
	FCDPhysicsModelInstance* GetInstance(size_t index) { FUAssert(index < GetNumInstances(), return NULL); return instances.at(index); }
	const FCDPhysicsModelInstance* GetInstance(size_t index) const { FUAssert(index < GetNumInstances(), return NULL); return instances.at(index); }
	FCDPhysicsModelInstance* AddInstance(FCDPhysicsModel* model = NULL);

	// Visibility parameter
	const FMVector3& GetGravity() const { return gravity; }
	void SetGravity(const FMVector3& _gravity) { gravity = _gravity; SetDirtyFlag(); }
	const float& GetTimestep() const { return timestep; }
	void SetTimestep(float _timestep) { timestep = _timestep; SetDirtyFlag(); }

	// Clones the physics scene
	virtual FCDEntity* Clone(FCDEntity* clone = NULL, bool cloneChildren = false) const;

	// Parse a <physics_scene> node from a COLLADA document
	virtual bool LoadFromXML(xmlNode* sceneNode);

	// Write out a <physics_scene> element to a COLLADA XML document
	void WriteToNodeXML(xmlNode* node) const;
	virtual xmlNode* WriteToXML(xmlNode* parentNode) const;
};

#endif // _FCD_SCENE_NODE_
