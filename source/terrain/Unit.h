#ifndef _UNIT_H
#define _UNIT_H

class CModel;
class CObjectEntry;

class CUnit
{
public:
	// object from which unit was created
	CObjectEntry* m_Object;
	// object model representation
	CModel* m_Model;
};

#endif
