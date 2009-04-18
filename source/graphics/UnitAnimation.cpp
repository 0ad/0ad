/* Copyright (C) 2009 Wildfire Games.
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
#include "ps/CStr.h"

namespace
{
	// Randomly modify the speed, so that units won't stay perfectly
	// synchronised if they're playing animations of the same length
	float DesyncSpeed(float speed)
	{
// 		const float var = 0.05f; // max fractional variation from default
// 		return speed * (1.f - var + 2.f*var*(rand(0, 256)/255.f));
		// TODO: enable this desyncing for cases where we don't care about
		// accurate looping, and just don't do it for e.g. projectile-launchers
		// where we do care
		return speed;
	}
}

CUnitAnimation::CUnitAnimation(CUnit& unit)
: m_Unit(unit), m_State("idle"), m_Looping(true), m_Speed(0.f), m_OriginalSpeed(0.f), m_TimeToNextSync(0.f)
{
}

void CUnitAnimation::SetAnimationState(const CStr& name, bool once, float speed, bool keepSelection)
{
	if (name == m_State)
		return;

	m_State = name;
	m_Looping = !once;
	m_Speed = m_OriginalSpeed = speed;
	m_TimeToNextSync = 0.f;

	if (! keepSelection)
		m_Unit.SetEntitySelection(name);

	m_Unit.SetRandomAnimation(m_State, !m_Looping, DesyncSpeed(m_Speed));
}

void CUnitAnimation::SetAnimationSync(float timeUntilActionPos)
{
	// We need to finish looping our animation at the specified time from now.
	// Assume it's playing at nearly the right speed, and we just need to perhaps
	// shift it a little bit to stay in sync.

	m_TimeToNextSync = timeUntilActionPos;

	CModel* model = m_Unit.GetModel();

	// Calculate the required playback speed so ActionPos coincides with timeUntilActionPos
	float currentPos = model->m_AnimTime / model->m_Anim->m_AnimDef->GetDuration();
	float length = (model->m_Anim->m_ActionPos - currentPos);
	if (length < 0.f)
		length += 1.f;
	float requiredSpeed = length / m_TimeToNextSync;

	// Shift in the right direction
	if (requiredSpeed > m_OriginalSpeed)
		m_Speed = std::min(requiredSpeed, m_OriginalSpeed*1.1f);
	else if (requiredSpeed < m_OriginalSpeed)
		m_Speed = std::max(requiredSpeed, m_OriginalSpeed*0.9f);

	model->m_AnimSpeed = m_Speed * model->m_Anim->m_AnimDef->GetDuration() * model->m_Anim->m_Speed;

	// TODO: this should use the ActionPos2, instead of totally ignoring it
	m_Unit.ShowAmmunition();
}

void CUnitAnimation::Update(float time)
{
	CModel* model = m_Unit.GetModel();

	// Choose a new random animation if we're going to loop
	if (m_Looping && model->NeedsNewAnim(time))
	{
		m_Unit.SetRandomAnimation(m_State, !m_Looping, DesyncSpeed(m_Speed));
		// TODO: this really ought to transition smoothly into the new animation,
		// instead of just cutting off the end of the previous one and jumping
		// straight into the new.
	}

	if (m_TimeToNextSync >= 0.0 && m_TimeToNextSync-time < 0.0)
		m_Unit.HideAmmunition();

	m_TimeToNextSync -= time;

	// TODO: props should get a new random animation once they loop, independent
	// of the object they're propped onto

	model->Update(time);
}
