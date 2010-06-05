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

#include "ps/Game.h"
#include "ps/Player.h"

CUnit::CUnit(CObjectEntry* object, CObjectManager& objectManager,
			 const std::set<CStr>& actorSelections)
: m_Object(object), m_Model(object->m_Model->Clone()),
  m_ID(invalidUnitId), m_ActorSelections(actorSelections),
  m_ObjectManager(objectManager)
{
	m_Animation = new CUnitAnimation(*this);
}

CUnit::~CUnit()
{
	delete m_Animation;
	delete m_Model;
}

CUnit* CUnit::Create(const CStrW& actorName, const std::set<CStr>& selections, CObjectManager& objectManager)
{
	CObjectBase* base = objectManager.FindObjectBase(actorName);

	if (! base)
		return NULL;

	std::set<CStr> actorSelections = base->CalculateRandomVariation(selections);

	std::vector<std::set<CStr> > selectionsVec;
	selectionsVec.push_back(actorSelections);

	CObjectEntry* obj = objectManager.FindObjectVariation(base, selectionsVec);

	if (! obj)
		return NULL;

	return new CUnit(obj, objectManager, actorSelections);
}

void CUnit::SetAnimationState(const CStr& name, bool once, float speed, float desync, bool keepSelection, const CStrW& soundgroup)
{
	m_Animation->SetAnimationState(name, once, speed, desync, keepSelection, soundgroup);
}

void CUnit::SetAnimationSync(float actionTime, float repeatTime)
{
	m_Animation->SetAnimationSync(actionTime, repeatTime);
}

void CUnit::UpdateModel(float frameTime)
{
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
		CModel* newModel = newObject->m_Model->Clone();

		// Copy the old instance-specific settings from the old model to the new instance
		newModel->SetTransform(m_Model->GetTransform());
		newModel->SetPlayerID(m_Model->GetPlayerID());
		newModel->CopyAnimationFrom(m_Model);

		delete m_Model;
		m_Model = newModel;
		m_Object = newObject;

		m_Animation->ReloadUnit(); // TODO: maybe this should try to preserve animation state?
	}
}
