#include "PSProp.h"
#include "ExpProp.h"
#include "ExpUtil.h"
#include "decomp.h"

PMDExpProp::PMDExpProp(INode* node) : m_Node(node)
{
}

bool PMDExpProp::IsProp(Object* obj)
{
	return obj->ClassID()==PSPROP_CLASS_ID ? true : false;
}

ExpProp* PMDExpProp::Build()
{
	ExpProp* prop=new ExpProp;
	prop->m_Name=m_Node->GetName();
	prop->m_Parent=m_Node->GetParentNode();

	// build local transformation matrix
	INode *parent;
	Matrix3 parentTM, nodeTM, localTM;
	nodeTM = m_Node->GetNodeTM(0);
	parent = m_Node->GetParentNode();
	parentTM = parent->GetNodeTM(0);
	localTM = nodeTM*Inverse(parentTM);

	// decompose it to get translation and rotation
	AffineParts parts;
	decomp_affine(localTM,&parts);

	// get translation from affine parts
	MAXtoGL(parts.t);
	prop->m_Position=CVector3D(parts.t.x,parts.t.y,parts.t.z);

	// get rotation from affine parts
	prop->m_Rotation.m_V.X=parts.q.x;
	prop->m_Rotation.m_V.Y=parts.q.z;
	prop->m_Rotation.m_V.Z=parts.q.y;
	prop->m_Rotation.m_W=parts.q.w;

	return prop;
}