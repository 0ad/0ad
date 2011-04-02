/* Copyright (C) 2010 Wildfire Games.
 * This file is part of 0 A.D.
 *
 * 0 A.D. is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * 0 A.D. is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with 0 A.D.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "precompiled.h"

#include "Unit.h"
#include "Model.h"
#include "ObjectBase.h"
#include "ObjectEntry.h"
#include "ObjectManager.h"
#include "SkeletonAnim.h"
#include "SkeletonAnimDef.h"
#include "UnitAnimation.h"

CUnit::CUnit(CObjectEntry* object, CObjectManager& objectManager,
			 const std::set<CStr>& actorSelections)
: m_Object(object), m_Model(object->m_Model->Clone()),
  m_ID(INVALID_ENTITY), m_ActorSelections(actorSelections),
  m_ObjectManager(objectManager)
{
	if (m_Model->ToCModel())
		m_Animation = new CUnitAnimation(m_ID, m_Model->ToCModel(), m_Object);
	else
		m_Animation = NULL;
}

CUnit::~CUnit()
{
	delete m_Animation;
	delete m_Model;
}

CUnit* CUnit::Create(const CStrW& actorName, uint32_t seed, const std::set<CStr>& selections, CObjectManager& objectManager)
{
	CObjectBase* base = objectManager.FindObjectBase(actorName);

	if (! base)
		return NULL;

	std::set<CStr> actorSelections = base->CalculateRandomVariation(seed, selections);

	std::vector<std::set<CStr> > selectionsVec;
	selectionsVec.push_back(actorSelections);

	CObjectEntry* obj = objectManager.FindObjectVariation(base, selectionsVec);

	if (! obj)
		return NULL;

	return new CUnit(obj, objectManager, actorSelections);
}

void CUnit::UpdateModel(float frameTime)
{
	if (m_Animation)
		m_Animation->Update(frameTime*1000.0f);
}

void CUnit::SetEntitySelection(const CStr& selection)
{
	CStr selection_lc = selection.LowerCase();

	// If we've already selected this, don't do anything
	if (m_EntitySelections.find(selection_lc) != m_EntitySelections.end())
		return;

	// Just allow one selection at a time
	m_EntitySelections.clear();
	m_EntitySelections.insert(selection_lc);

	ReloadObject();
}

void CUnit::SetActorSelections(const std::set<CStr>& selections)
{
	m_ActorSelections = selections;
	ReloadObject();
}

void CUnit::ReloadObject()
{
	std::vector<std::set<CStr> > selections;
	// TODO: push world selections (seasons, etc) (and reload whenever they're changed)
	selections.push_back(m_EntitySelections);
	selections.push_back(m_ActorSelections);

	// If these selections give a different object, change this unit to use it
	CObjectEntry* newObject = m_ObjectManager.FindObjectVariation(m_Object->m_Base, selections);
	if (newObject && newObject != m_Object)
	{
		// Clone the new object's base (non-instance) model
		CModelAbstract* newModel = newObject->m_Model->Clone();

		// Copy the old instance-specific settings from the old model to the new instance
		newModel->SetTransform(m_Model->GetTransform());
		newModel->SetPlayerID(m_Model->GetPlayerID());
		if (newModel->ToCModel() && m_Model->ToCModel())
		{
			newModel->ToCModel()->CopyAnimationFrom(m_Model->ToCModel());

			// Copy flags that belong to this model instance (not those defined by the actor XML)
			int instanceFlags = (MODELFLAG_SILHOUETTE_DISPLAY|MODELFLAG_SILHOUETTE_OCCLUDER) & m_Model->ToCModel()->GetFlags();
			newModel->ToCModel()->AddFlagsRec(instanceFlags);
		}

		delete m_Model;
		m_Model = newModel;
		m_Object = newObject;

		if (m_Model->ToCModel())
		{
			if (m_Animation)
				m_Animation->ReloadUnit(m_Model->ToCModel(), m_Object); // TODO: maybe this should try to preserve animation state?
			else
				m_Animation = new CUnitAnimation(m_ID, m_Model->ToCModel(), m_Object);
		}
		else
		{
			SAFE_DELETE(m_Animation);
		}
	}
}
