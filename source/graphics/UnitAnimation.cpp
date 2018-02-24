/* Copyright (C) 2018 Wildfire Games.
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

#include "UnitAnimation.h"

#include "graphics/Model.h"
#include "graphics/ObjectEntry.h"
#include "graphics/SkeletonAnim.h"
#include "graphics/SkeletonAnimDef.h"
#include "graphics/Unit.h"
#include "lib/rand.h"
#include "ps/CStr.h"
#include "ps/Game.h"
#include "simulation2/Simulation2.h"
#include "simulation2/components/ICmpSoundManager.h"

// Randomly modify the speed, so that units won't stay perfectly
// synchronised if they're playing animations of the same length
static float DesyncSpeed(float speed, float desync)
{
	if (desync == 0.0f)
		return speed;

	return speed * (1.f - desync + 2.f*desync*(rand(0, 256)/255.f));
}

CUnitAnimation::CUnitAnimation(entity_id_t ent, CModel* model, CObjectEntry* object)
	: m_Entity(ent), m_State("idle"), m_Looping(true),
	  m_Speed(1.f), m_SyncRepeatTime(0.f), m_OriginalSpeed(1.f), m_Desync(0.f)
{
	ReloadUnit(model, object);
}

void CUnitAnimation::SetEntityID(entity_id_t ent)
{
	m_Entity = ent;
}

void CUnitAnimation::AddModel(CModel* model, const CObjectEntry* object)
{
	SModelAnimState state;

	state.model = model;
	state.object = object;
	state.anim = object->GetRandomAnimation(m_State, m_AnimationID);
	state.time = 0.f;
	state.pastLoadPos = false;
	state.pastActionPos = false;
	state.pastSoundPos = false;

	ENSURE(state.anim != NULL); // there must always be an idle animation

	m_AnimStates.push_back(state);

	model->SetAnimation(state.anim, !m_Looping);

	// Detect if this unit has any non-static animations
	for (CSkeletonAnim* anim : object->GetAnimations(m_State))
		if (anim->m_AnimDef != NULL)
			m_AnimStatesAreStatic = false;

	// Recursively add all props
	const std::vector<CModel::Prop>& props = model->GetProps();
	for (const CModel::Prop& prop : props)
	{
		CModel* propModel = prop.m_Model->ToCModel();
		if (propModel)
			AddModel(propModel, prop.m_ObjectEntry);
	}
}

void CUnitAnimation::ReloadAnimation()
{
	UpdateAnimationID();
	ReloadUnit(m_Model, m_Object);
}

void CUnitAnimation::ReloadUnit(CModel* model, const CObjectEntry* object)
{
	m_Model = model;
	m_Object = object;

	m_AnimStates.clear();
	m_AnimStatesAreStatic = true;
	AddModel(m_Model, m_Object);
}

void CUnitAnimation::SetAnimationState(const CStr& name, bool once, float speed, float desync, const CStrW& actionSound)
{
	m_Looping = !once;
	m_OriginalSpeed = speed;
	m_Desync = desync;
	m_ActionSound = actionSound;

	m_Speed = DesyncSpeed(m_OriginalSpeed, m_Desync);
	m_SyncRepeatTime = 0.f;

	if (name != m_State)
	{
		m_State = name;
		ReloadAnimation();
	}
}

void CUnitAnimation::SetAnimationSyncRepeat(float repeatTime)
{
	m_SyncRepeatTime = repeatTime;
}

void CUnitAnimation::SetAnimationSyncOffset(float actionTime)
{
	if (m_AnimStatesAreStatic)
		return;

	// Update all the synced prop models to each coincide with actionTime
	for (std::vector<SModelAnimState>::iterator it = m_AnimStates.begin(); it != m_AnimStates.end(); ++it)
	{
		CSkeletonAnimDef* animDef = it->anim->m_AnimDef;
		if (animDef == NULL)
			continue; // ignore static animations

		float duration = animDef->GetDuration();

		float actionPos = it->anim->m_ActionPos;
		bool hasActionPos = (actionPos != -1.f);

		if (!hasActionPos)
			continue;

		float speed = duration / m_SyncRepeatTime;

		// Need to offset so that start+actionTime*speed = actionPos
		float start = actionPos - actionTime*speed;
		// Wrap it so that it's within the animation
		start = fmodf(start, duration);
		if (start < 0)
			start += duration;

		it->time = start;
	}
}

void CUnitAnimation::Update(float time)
{
	if (m_AnimStatesAreStatic)
		return;

	// Advance all of the prop models independently
	for (std::vector<SModelAnimState>::iterator it = m_AnimStates.begin(); it != m_AnimStates.end(); ++it)
	{
		CSkeletonAnimDef* animDef = it->anim->m_AnimDef;
		if (animDef == NULL)
			continue; // ignore static animations

		float duration = animDef->GetDuration();

		float actionPos = it->anim->m_ActionPos;
		float loadPos = it->anim->m_ActionPos2;
		float soundPos = it->anim->m_SoundPos;
		bool hasActionPos = (actionPos != -1.f);
		bool hasLoadPos = (loadPos != -1.f);
		bool hasSoundPos = (soundPos != -1.f);

		// Find the current animation speed
		float speed;
		if (m_SyncRepeatTime && hasActionPos)
			speed = duration / m_SyncRepeatTime;
		else
			speed = m_Speed * it->anim->m_Speed;

		// Convert from real time to scaled animation time
		float advance = time * speed;

		// If we're going to advance past the load point in this update, then load the ammo
		if (hasLoadPos && !it->pastLoadPos && it->time + advance >= loadPos)
		{
			it->model->ShowAmmoProp();
			it->pastLoadPos = true;
		}

		// If we're going to advance past the action point in this update, then perform the action
		if (hasActionPos && !it->pastActionPos && it->time + advance >= actionPos)
		{
			if (hasLoadPos)
				it->model->HideAmmoProp();

			if ( !hasSoundPos && !m_ActionSound.empty() )
			{
				CmpPtr<ICmpSoundManager> cmpSoundManager(*g_Game->GetSimulation2(), SYSTEM_ENTITY);
				if (cmpSoundManager)
					cmpSoundManager->PlaySoundGroup(m_ActionSound, m_Entity);
			}

			it->pastActionPos = true;
		}
		if (hasSoundPos && !it->pastSoundPos && it->time + advance >= soundPos)
		{
			if (!m_ActionSound.empty() )
			{
				CmpPtr<ICmpSoundManager> cmpSoundManager(*g_Game->GetSimulation2(), SYSTEM_ENTITY);
				if (cmpSoundManager)
					cmpSoundManager->PlaySoundGroup(m_ActionSound, m_Entity);
			}

			it->pastSoundPos = true;
		}

		if (it->time + advance < duration)
		{
			// If we're still within the current animation, then simply update it
			it->time += advance;
			it->model->UpdateTo(it->time);
		}
		else if (m_Looping)
		{
			// If we've finished the current animation and want to loop...

			// Wrap the timer around
			it->time = fmod(it->time + advance, duration);

			// Pick a new random animation
			CSkeletonAnim* anim;
			if (it->model == m_Model)
			{
				// we're handling the root model
				// choose animations from the complete state
				CStr oldID = m_AnimationID;
				UpdateAnimationID();
				anim = it->object->GetRandomAnimation(m_State, m_AnimationID);
				if (oldID != m_AnimationID)
					for (SModelAnimState animState : m_AnimStates)
						if (animState.model != m_Model)
							animState.model->SetAnimation(animState.object->GetRandomAnimation(m_State, m_AnimationID));
			}
			else
				// choose animations that match the root
				anim = it->object->GetRandomAnimation(m_State, m_AnimationID);

			if (anim != it->anim)
			{
				it->anim = anim;
				it->model->SetAnimation(anim, !m_Looping);
			}

			it->pastActionPos = false;
			it->pastLoadPos = false;
			it->pastSoundPos = false;

			it->model->UpdateTo(it->time);
		}
		else
		{
			// If we've finished the current animation and don't want to loop...

			// Update to very nearly the end of the last frame (but not quite the end else we'll wrap around when skinning)
			float nearlyEnd = duration - 1.f;
			if (fabs(it->time - nearlyEnd) > 1.f)
			{
				it->time = nearlyEnd;
				it->model->UpdateTo(it->time);
			}
		}
	}
}

void CUnitAnimation::UpdateAnimationID()
{
	CStr& ID = m_Object->GetRandomAnimation(m_State)->m_ID;
	m_AnimationID = ID;
}
