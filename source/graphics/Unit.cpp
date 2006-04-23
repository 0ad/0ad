#include "precompiled.h"

#include "Unit.h"
#include "Model.h"
#include "ObjectEntry.h"
#include "ObjectManager.h"
#include "SkeletonAnim.h"
#include "SkeletonAnimDef.h"

CUnit::CUnit(CObjectEntry* object, CEntity* entity, const std::set<CStrW>& actorSelections)
: m_Object(object), m_Model(object->m_Model->Clone()), m_Entity(entity),
  m_ID(-1), m_ActorSelections(actorSelections)
{
}


CUnit::~CUnit()
{
	delete m_Model;
}

void CUnit::ShowAmmunition()
{
	if (!m_Object->m_AmmunitionModel || !m_Object->m_AmmunitionPoint)
		return;
	m_Model->AddProp(m_Object->m_AmmunitionPoint, m_Object->m_AmmunitionModel->Clone(), m_Object);
}

void CUnit::HideAmmunition()
{
	if (!m_Object->m_AmmunitionModel || !m_Object->m_AmmunitionPoint)
		return;

	// Find out what the usual prop is:
	std::vector<CModel::Prop>& props = m_Object->m_Model->GetProps();
	std::vector<CModel::Prop>::iterator it;
	for (it = props.begin(); it != props.end(); ++it)
	{
		if (it->m_Point == m_Object->m_AmmunitionPoint)
		{
			m_Model->AddProp(m_Object->m_AmmunitionPoint, it->m_Model->Clone(), m_Object);
			return;
		}
	}
	// No usual prop.
	m_Model->RemoveProp(m_Object->m_AmmunitionPoint);
}


static CSkeletonAnim* GetRandomAnimation(const CStr& name, CObjectEntry* object)
{
	CSkeletonAnim* anim = object->GetRandomAnimation(name);

	// Fall back to 'idle', if no matching animation is found
	if (anim == NULL && name != "idle")
		anim = object->GetRandomAnimation("idle");

	// Every object should have an idle animation (even if it's a dummy static one)
	debug_assert(anim != NULL);

	return anim;
}

static bool SetRandomAnimation(const CStr& name, bool once, float speed,
							   CModel* model, CObjectEntry* object)
{
	CSkeletonAnim* anim = GetRandomAnimation(name, object);
	if (anim)
	{
		float actualSpeed = 1000.f;
		if (speed && anim->m_AnimDef)
			actualSpeed = speed * anim->m_AnimDef->GetDuration();
		model->SetAnimation(anim, once, actualSpeed);

		// Recursively apply the animation name to props
		const std::vector<CModel::Prop>& props = model->GetProps();
		for (std::vector<CModel::Prop>::const_iterator it = props.begin(); it != props.end(); ++it)
		{
			bool ok = SetRandomAnimation(name, once, speed, it->m_Model, it->m_ObjectEntry);
			if (! ok)
				return false;
		}

		return true;
	}
	else
	{
		// This shouldn't happen, since GetRandomAnimation tries to always
		// return something valid
		return false;
	}
}



bool CUnit::SetRandomAnimation(const CStr& name, bool once, float speed)
{
	return ::SetRandomAnimation(name, once, speed, m_Model, m_Object);
}

CSkeletonAnim* CUnit::GetRandomAnimation(const CStr& name)
{
	return ::GetRandomAnimation(name, m_Object);
}

bool CUnit::IsPlayingAnimation(const CStr& name)
{
	return (m_Model->GetAnimation() && m_Model->GetAnimation()->m_Name == name);
}


void CUnit::SetPlayerID(int id)
{
	m_PlayerID = id;
	m_Model->SetPlayerID(m_PlayerID);
}

void CUnit::SetEntitySelection(const CStrW& selection)
{
	CStrW selection_lc = selection.LowerCase();

	// If we've already selected this, don't do anything
	if (m_EntitySelections.find(selection_lc) != m_EntitySelections.end())
		return;

	// Just allow one selection at a time
	m_EntitySelections.clear();
	m_EntitySelections.insert(selection_lc);

	ReloadObject();
}

void CUnit::ReloadObject()
{
	std::vector<std::set<CStrW> > selections;
	// TODO: push world selections (seasons, etc) (and reload whenever they're changed)
	selections.push_back(m_EntitySelections);
	selections.push_back(m_ActorSelections);

	// If these selections give a different object, change this unit to use it
	CObjectEntry* newObject = g_ObjMan.FindObjectVariation(m_Object->m_Base, selections);
	if (newObject != m_Object)
	{
		CModel* newModel = newObject->m_Model->Clone();
		// Copy old settings to the new model
		newModel->SetPlayerID(m_PlayerID);
		newModel->SetTransform(m_Model->GetTransform());
		// TODO: preserve selection of animation, anim offset, etc?

		delete m_Model;
		m_Model = newModel;
		m_Object = newObject;
	}
}
