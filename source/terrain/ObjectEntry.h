#ifndef _OBJECTENTRY_H
#define _OBJECTENTRY_H

class CModel;

#include <vector>
#include "CStr.h"
#include "terrain/Bound.h"


class CObjectEntry
{
public:
	struct Anim {
		CStr m_AnimName;
		CStr m_FileName;
	};

public:
	CObjectEntry(int type);
	~CObjectEntry();
	
	bool BuildModel();

	bool Load(const char* filename);
	bool Save(const char* filename);

	// object name
	CStr m_Name;
	// texture name
	CStr m_TextureName;
	// model name
	CStr m_ModelName;
	// animation name
	std::vector<Anim> m_Animations;
	// object space bounds of model
//	CBound m_Bound;
	// corresponding model 
	CModel* m_Model;
	// type of object; index into object managers types array
	int m_Type;
};


#endif
