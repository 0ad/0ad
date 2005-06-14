#ifndef __EXPSKELETON_H
#define __EXPSKELETON_H

#include <vector>
#include "MaxInc.h"
#include "Matrix3D.h"
#include "Vector3D.h"
#include "Quaternion.h"
#include "lib/types.h"

class CSkeleton;
class CSkeletonAnimDef;

////////////////////////////////////////////////////////////////////////
// ExpBone: bone type used during export
class ExpBone
{
public:
	// MAX bone node
	INode* m_Node;
	// transform from object to this bones space at time 0
	CMatrix3D m_Transform;	
	// translation relative to root bone at time 0
	CVector3D m_Translation;	
	// rotation relative to root bone at time 0
	CQuaternion m_Rotation;	
	// parent of this bone; 0 for root bone
	ExpBone* m_Parent;
	// children of this bone
	std::vector<ExpBone*> m_Children;
};

////////////////////////////////////////////////////////////////////////
// ExpSkeleton:
class ExpSkeleton
{
public:
	////////////////////////////////////////////////////////////////////////
	// static methods for skeleton creation, destruction and type queries:
	
	// build all skeletons from given root node
	static void BuildSkeletons(INode* node,std::vector<ExpSkeleton*>& skeletons);
	// return true if given node is a skeletal biped bone, else false
	static bool IsBone(INode* node);
	// return true if given node is biped footprints, else false 
	static bool IsFootprints(INode* node);
	// return true if given node is the root of a skeleton, else false
	static bool IsSkeletonRoot(INode* node);

public:
	// constructor, destructor
	ExpSkeleton();
	~ExpSkeleton();

	// search skeleton looking for matching bone using given node;
	// return index of given node in the skeleton, 0xff if not found
	u8 FindBoneByNode(INode* boneNode) const;

	// build and return a complete animation for this skeleton over given time range
	CSkeletonAnimDef* BuildAnimation(TimeValue start,TimeValue end,float rate);

private:
	// traverse node heirachy adding bones to skeleton
	void Build(INode* node,ExpBone* parent=0);
	// calculate the translation and rotation values of the given bone, 
	// relative to the root bone, at the given time 
	void GetBoneTransformComponents(INode* node,TimeValue t,CVector3D& trans,CQuaternion& rot);

public:
	// list of bones in skeleton; root bone is m_Bones[0]
	std::vector<ExpBone*> m_Bones;
};

#endif