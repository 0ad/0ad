#ifndef _EXPPROP_H
#define _EXPPROP_H

#include "CStr.h"
#include "Vector3D.h"
#include "Quaternion.h"

////////////////////////////////////////////////////////////////////////
// ExpProp: prop object used on export
class ExpProp
{
public:
	// name of prop
	CStr m_Name;
	// position relative to parent
	CVector3D m_Position;
	// rotation relative to parent
	CQuaternion m_Rotation;
	// parent node
	INode* m_Parent;
};

////////////////////////////////////////////////////////////////////////
// PMDExpProp: class used for building output props
class PMDExpProp
{
public:
	PMDExpProp(INode* node);

	static bool IsProp(Object* obj);
	ExpProp* Build();

private:
	// the node we're constructing the prop from
	INode* m_Node;
};

#endif