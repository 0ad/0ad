#include "ExpSkeleton.h"
#include "ExpUtil.h"
#include "bipexp.h"
#include "decomp.h"
#include "ModelDef.h"
#include "SkeletonAnimDef.h"

/////////////////////////////////////////////////////////////////////////////////
// ExpSkeleton constructor
ExpSkeleton::ExpSkeleton() 
{
}

/////////////////////////////////////////////////////////////////////////////////
// ExpSkeleton destructor
ExpSkeleton::~ExpSkeleton() 
{
	for (int i=0;i<m_Bones.size();i++) {
		delete m_Bones[i];
	}
}
 

/////////////////////////////////////////////////////////////////////////////////
// IsFootprints: return true if given node is biped footprints, else false 
bool ExpSkeleton::IsFootprints(INode* node)
{
   // get nodes transform control
	Control* c=node->GetTMController();

	// check classID for footprints
    return (c && (c->ClassID()==FOOTPRINT_CLASS_ID)) ? true : false;
}

/////////////////////////////////////////////////////////////////////////////////
// IsBone: return true if given node is a skeletal biped bone, else false
bool ExpSkeleton::IsBone(INode* node)
{
   // get nodes transform control
	Control* c=node->GetTMController();

	// check classID for either a root or a child bone
    return (c && (c->ClassID()==BIPSLAVE_CONTROL_CLASS_ID || c->ClassID()==BIPBODY_CONTROL_CLASS_ID)) ? true : false;
}

/////////////////////////////////////////////////////////////////////////////////
// IsSkeleton: return true if given node is the root of a skeleton, else false
bool ExpSkeleton::IsSkeletonRoot(INode* node)
{
    // get nodes transform control
	Control* c=node->GetTMController();

	// check for biped root
    return (c && c->ClassID()==BIPBODY_CONTROL_CLASS_ID) ? true : false;
}

/////////////////////////////////////////////////////////////////////////////////
// BuildSkeletons: build all skeletons from given root node
void ExpSkeleton::BuildSkeletons(INode* node,std::vector<ExpSkeleton*>& skeletons)
{	
	if (IsSkeletonRoot(node)) {
		// build skeleton from this node - Build traverses all children
		// of given node
		ExpSkeleton* skeleton=new ExpSkeleton;
		skeleton->Build(node,0);
		skeletons.push_back(skeleton);
	} else {
		// traverse into children as this object wasn't a skeleton root
		for (int i=0;i<node->NumberOfChildren();i++) {
			BuildSkeletons(node->GetChildNode(i),skeletons);
		}
	}
}

////////////////////////////////////////////////////////////////////////////////
// FindBoneByNode: search skeleton looking for matching bone using given node;
// return index of given node in the skeleton, 0xff if not found
u8 ExpSkeleton::FindBoneByNode(INode* boneNode) const
{
	if (!boneNode) return 0xff;
	
	for (int j=0;j<m_Bones.size();j++) {
		ExpBone* bone=m_Bones[j];
		if (bone->m_Node==boneNode) return j;
	}

	return 0xff;
}


/////////////////////////////////////////////////////////////////////////////////
// Build: traverse node heirachy adding bones to skeleton
void ExpSkeleton::Build(INode* node,ExpBone* parent)
{
	if (IsBone(node)) {
		
		// build bone
		ExpBone* bone=new ExpBone;
		bone->m_Node=node;
		bone->m_Parent=parent;
	
		// get bone's translation and rotation relative to root at time 0
		GetBoneTransformComponents(node,0,bone->m_Translation,bone->m_Rotation);

		// build transform from bone to object space
		CMatrix3D m;
		bone->m_Transform.SetIdentity();
		bone->m_Transform.Rotate(bone->m_Rotation);
		bone->m_Transform.Translate(bone->m_Translation);

		// add bone to parent's list of children and complete bone list 
		if (parent) parent->m_Children.push_back(bone);
		m_Bones.push_back(bone);

		char buf[256];
		sprintf(buf,"bonenode 0x%p (%s) added\n",node,node->GetName());
		OutputDebugString(buf);

		for (int i=0;i<node->NumberOfChildren();i++) {
			Build(node->GetChildNode(i),bone);
		}	
	}
}

/////////////////////////////////////////////////////////////////////////////////
// BuildAnimation: build and return a complete animation for this skeleton over 
// given time range
CSkeletonAnimDef* ExpSkeleton::BuildAnimation(TimeValue start,TimeValue end,float rate)
{
	CSkeletonAnimDef* anim=new CSkeletonAnimDef;
	anim->m_Name="God Knows";
	anim->m_NumFrames=1+(end-start)/rate;
	anim->m_NumKeys=m_Bones.size();
	anim->m_FrameTime=rate;
	anim->m_Keys=new CSkeletonAnimDef::Key[anim->m_NumFrames*anim->m_NumKeys];

	u32 counter=0;
	TimeValue t=start;
	while (t<end) {

		for (int i=0;i<m_Bones.size();i++) {
			CSkeletonAnimDef::Key& key=anim->m_Keys[counter++];
			GetBoneTransformComponents(m_Bones[i]->m_Node,t,key.m_Translation,key.m_Rotation);
		}
		t+=rate;
	}

	return anim;
}

///////////////////////////////////////////////////////////////////////////////////////
// GetBoneTransformComponents: calculate the translation and rotation 
// values of the given bone, relative to the root bone, at the given time 
void ExpSkeleton::GetBoneTransformComponents(INode* node,TimeValue t,
										  CVector3D& trans,CQuaternion& rot)
{
	Matrix3 nodeTM=node->GetNodeTM(t);
	nodeTM.NoScale();		

	// decompose it to get translation and rotation
	AffineParts parts;
	decomp_affine(nodeTM,&parts);

	// get translation from affine parts
	MAXtoGL(parts.t);
	trans=CVector3D(parts.t.x,parts.t.y,parts.t.z);

	// get rotation from affine parts
	rot.m_V.X=parts.q.x;
	rot.m_V.Y=parts.q.z;
	rot.m_V.Z=parts.q.y;
	rot.m_W=parts.q.w;
}