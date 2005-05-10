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