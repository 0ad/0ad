#ifndef _UNIT_H
#define _UNIT_H

#include <assert.h>
#include "Model.h"

class CObjectEntry;

/////////////////////////////////////////////////////////////////////////////////////////////
// CUnit: simple "actor" definition - defines a sole object within the world
class CUnit
{
public:
	// sole constructor - unit invalid without a model and object
	CUnit(CObjectEntry* object,CModel* model) : m_Object(object), m_Model(model) {
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

private:
	// object from which unit was created
	CObjectEntry* m_Object;
	// object model representation
	CModel* m_Model;
};

#endif
