#ifndef _UNIT_H
#define _UNIT_H

#include <assert.h>
#include "Model.h"

class CObjectEntry;
class CEntity;

/////////////////////////////////////////////////////////////////////////////////////////////
// CUnit: simple "actor" definition - defines a sole object within the world
class CUnit
{
public:
	// constructor - unit invalid without a model and object
	CUnit(CObjectEntry* object,CModel* model) : m_Object(object), m_Model(model), m_Entity( NULL ) {
		assert(object && model);
	}
	CUnit(CObjectEntry* object,CModel* model, CEntity* entity) : m_Object(object), m_Model(model), m_Entity( entity ) {
		assert(object && model);
	}

	// destructor
	~CUnit() {
		delete m_Model;
	}


	// get unit's template object
	CObjectEntry* GetObject() { return m_Object; }
	// get unit's model data
	CModel* GetModel() { return m_Model; }
	// get actor's entity
	CEntity* GetEntity() { return m_Entity; }

private:
	// object from which unit was created
	CObjectEntry* m_Object;
	// object model representation
	CModel* m_Model;
	// the entity that this actor represents, if any
	CEntity* m_Entity;
};

#endif
