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

#include "UnitAnimation.h"

#include "graphics/Model.h"
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

CUnitAnimation::CUnitAnimation(CUnit& unit)
: m_Unit(unit), m_State("idle"), m_Looping(true), m_Speed(1.f), m_OriginalSpeed(1.f), m_Desync(0.f)
{
}

void CUnitAnimation::SetAnimationState(const CStr& name, bool once, float speed, float desync, bool keepSelection, const CStrW& actionSound)
{
	if (name == m_State)
		return;

	m_State = name;
	m_Looping = !once;
	m_OriginalSpeed = speed;
	m_Desync = desync;
	m_ActionSound = actionSound;

	m_Speed = DesyncSpeed(m_OriginalSpeed, m_Desync);

	if (! keepSelection)
		m_Unit.SetEntitySelection(name);

	m_Unit.SetRandomAnimation(m_State, !m_Looping);
}

void CUnitAnimation::SetAnimationSync(float actionTime, float repeatTime)
{
	CModel* model = m_Unit.GetModel();
	float duration = model->m_Anim->m_AnimDef->GetDuration();

	// Set the speed so it loops once in repeatTime
	float speed = duration / repeatTime;

	// Need to offset so that start+actionTime*speed = ActionPos
	float start = model->m_Anim->m_ActionPos - actionTime*speed;
	// Wrap it so that it's within the animation
	start = fmodf(start, duration);
	if (start < 0)
		start += duration;

	// Make the animation run at the computed timings
	model->m_AnimTime = start;
	m_Speed = m_OriginalSpeed = speed;
	m_Desync = 0.f;
}

void CUnitAnimation::Update(float time)
{
	CModel* model = m_Unit.GetModel();

	// Convert from real time to scaled animation time
	float advance = time*m_Speed;

	// Check if the current animation will trigger the action events
	bool action = false;
	bool action2 = false;
	m_Unit.GetModel()->CheckActionTriggers(advance, action, action2);

	// Choose a new random animation if we're going to loop
	if (m_Looping && model->NeedsNewAnim(time))
	{
		// Re-desynchronise the animation, to keep the irregular drifting between separate units
		m_Speed = DesyncSpeed(m_OriginalSpeed, m_Desync);
		advance = time*m_Speed;
		// TODO: should subtract time remaining in previous animation from advance, to be a bit more precise

		m_Unit.SetRandomAnimation(m_State, false);
		// TODO: this really ought to interpolate smoothly into the new animation,
		// instead of just cutting off the end of the previous one and jumping
		// straight into the new.

		// Check if the start of the new animation will trigger the action events
		// (This is additive with the previous CheckActionTriggers)
		m_Unit.GetModel()->CheckActionTriggers(advance, action, action2);
	}

	// TODO: props should get a new random animation once they loop, independent
	// of the object they're propped onto, if we ever bother with animated props

	// If we're going to advance past the action point in this update, then perform the action
	if (action)
	{
		m_Unit.HideAmmunition();

		if (!m_ActionSound.empty())
		{
			CmpPtr<ICmpSoundManager> cmpSoundManager(*g_Game->GetSimulation2(), SYSTEM_ENTITY);
			if (!cmpSoundManager.null())
				cmpSoundManager->PlaySoundGroup(m_ActionSound, m_Unit.GetID());
		}
	}

	// Ditto for the second action point
	if (action2)
	{
		m_Unit.ShowAmmunition();
	}

	// TODO: some animations (e.g. wood chopping) have two action points that should trigger sounds,
	// so we ought to support that somehow

	model->Update(advance);
}
