#ifndef _OBJECTENTRY_H
#define _OBJECTENTRY_H

class CModel;
class CSkeletonAnim;

#include <vector>
#include "CStr.h"
#include "ObjectBase.h"

class CObjectEntry
{
public:
	CObjectEntry(int type, CObjectBase* base);
	~CObjectEntry();

	bool BuildRandomVariant(CObjectBase::variation_key& vars, CObjectBase::variation_key::iterator& vars_it);

	// Base actor. Contains all the things that don't change between
	// different variations of the actor.
	CObjectBase* m_Base;

	CObjectBase::Prop* FindProp(const char* proppointname);

	// texture name
	CStr m_TextureName;
	// model name
	CStr m_ModelName;
	// list of valid animations for this object
	std::vector<CObjectBase::Anim> m_Animations;
	CSkeletonAnim* m_IdleAnim;
	CSkeletonAnim* m_WalkAnim;
	CSkeletonAnim* m_DeathAnim;
	CSkeletonAnim* m_MeleeAnim;
	CSkeletonAnim* m_RangedAnim;
	CSkeletonAnim* m_CorpseAnim;

	CSkeletonAnim* GetNamedAnimation( CStr animationName );
	// list of props attached to object
	std::vector<CObjectBase::Prop> m_Props;
	// corresponding model
	CModel* m_Model;
	// type of object; index into object managers types array
	int m_Type;
};


#endif
