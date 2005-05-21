#include "precompiled.h"

#include "Unit.h"
#include "Model.h"
#include "ObjectEntry.h"

CUnit::~CUnit() {
	delete m_Model;
}

void CUnit::ShowAmmunition()
{
	if( !m_Object->m_AmmunitionModel || !m_Object->m_AmmunitionPoint )
		return;
	m_Model->AddProp( m_Object->m_AmmunitionPoint, m_Object->m_AmmunitionModel->Clone() );
}

void CUnit::HideAmmunition()
{
	if( !m_Object->m_AmmunitionModel || !m_Object->m_AmmunitionPoint )
		return;
		// Find out what the usual prop is:
	std::vector<CModel::Prop>& props = m_Object->m_Model->GetProps();
	std::vector<CModel::Prop>::iterator it;
	for( it = props.begin(); it != props.end(); ++it )
		if( it->m_Point == m_Object->m_AmmunitionPoint )
		{
			m_Model->AddProp( m_Object->m_AmmunitionPoint, it->m_Model->Clone() );
			return;
		}
	// No usual prop.
	m_Model->RemoveProp( m_Object->m_AmmunitionPoint );
}

bool CUnit::SetRandomAnimation(const CStr& name, bool once)
{
	CSkeletonAnim* anim = GetRandomAnimation(name);
	if (anim)
	{
		m_Model->SetAnimation(anim, once);
		return true;
	}
	else
	{
		// TODO - report an error?
		return false;
	}
}

CSkeletonAnim* CUnit::GetRandomAnimation(const CStr& name)
{
	CSkeletonAnim* anim = m_Object->GetRandomAnimation(name);
	// Fall back to 'idle', if no matching animation is found
	if (anim == NULL && name != "idle")
		anim = m_Object->GetRandomAnimation("idle");

	return anim;
}

bool CUnit::IsPlayingAnimation(const CStr& name)
{
	return (m_Model->GetAnimation()->m_Name == name);
}
