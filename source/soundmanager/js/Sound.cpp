/* Copyright (C) 2012 Wildfire Games.
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

#include "Sound.h"

#include "lib/utf8.h"
#include "maths/Vector3D.h"
#include "ps/Filesystem.h"
#include "soundmanager/SoundManager.h"


JSound::JSound(const VfsPath& pathname)
{
	m_SndItem = g_SoundManager->LoadItem(&pathname);
}

JSound::~JSound()
{
	if (m_SndItem)
	{
		m_SndItem->FadeAndDelete(0.2);
		m_SndItem = 0;
	}
}

bool JSound::ClearSoundItem()
{
	m_SndItem = 0L;
	return true;
}

bool JSound::SetGain(JSContext* cx, uintN UNUSED(argc), jsval* argv)
{
	if (! m_SndItem)
		return false;

	float gain;
	if (! ToPrimitive<float>(cx, argv[0], gain))
		return false;
	
	m_SndItem->SetGain(gain);	
	return true;
}

bool JSound::SetPitch(JSContext* cx, uintN UNUSED(argc), jsval* argv)
{
	if (! m_SndItem)
		return false;

	float pitch;
	if (! ToPrimitive<float>(cx, argv[0], pitch))
		return false;
	
	m_SndItem->SetPitch(pitch);
	return true;
}

bool JSound::SetPosition(JSContext* cx, uintN argc, jsval* argv)
{
	if (! m_SndItem)
		return false;
	
	ENSURE(argc >= 1); // FIXME
	
	CVector3D pos;
	// absolute world coords
	if (!ToPrimitive<CVector3D>(cx, argv[0], pos))
		return false;

	m_SndItem->SetLocation(pos);
	
	return true;
}


bool JSound::Fade(JSContext* cx, uintN UNUSED(argc), jsval* argv)
{
	if (! m_SndItem)
		return false;
	
//	ENSURE(argc >= 3); // FIXME
	float initial_gain, final_gain;
	float length;
	if (! (ToPrimitive<float>(cx, argv[0], initial_gain)
		   && ToPrimitive<float>(cx, argv[1], final_gain)
		   && ToPrimitive<float>(cx, argv[2], length)))
		return false;
	
	m_SndItem->SetGain(initial_gain);
	m_SndItem->FadeToIn(final_gain, length);

	return true;
}

bool JSound::Play(JSContext* UNUSED(cx), uintN UNUSED(argc), jsval* UNUSED(argv))
{
	if (! m_SndItem)
		return false;

	m_SndItem->Play();

	return true;
}

bool JSound::Loop(JSContext* UNUSED(cx), uintN UNUSED(argc), jsval* UNUSED(argv))
{
	if (! m_SndItem)
		return false;

	m_SndItem->PlayLoop();

	return true;
}

bool JSound::Free(JSContext* UNUSED(cx), uintN UNUSED(argc), jsval* UNUSED(argv))
{
	if (m_SndItem)
	{
		m_SndItem->FadeAndDelete(0.2);
		m_SndItem = 0;
	}

	return true;
}

void JSound::ScriptingInit()
{
	AddMethod<CStr, &JSound::ToString>("toString", 0);
	AddMethod<bool, &JSound::Play>("play", 0);
	AddMethod<bool, &JSound::Loop>("loop", 0);
	AddMethod<bool, &JSound::Free>("free", 0);
	AddMethod<bool, &JSound::SetGain>("setGain", 0);
	AddMethod<bool, &JSound::SetPitch>("setPitch", 0);
	AddMethod<bool, &JSound::SetPosition>("setPosition", 0);
	AddMethod<bool, &JSound::Fade>("fade", 0);
	
	CJSObject<JSound>::ScriptingInit("Sound", &JSound::Construct, 1);
}

CStr JSound::ToString(JSContext* UNUSED(cx), uintN UNUSED(argc), jsval* UNUSED(argv))
{
	return "[object Sound: " + (m_SndItem ? m_SndItem->GetName() : "(null)") + "]";
}

JSBool JSound::Construct(JSContext* cx, uintN UNUSED(argc), jsval* vp)
{
//	JSU_REQUIRE_MIN_PARAMS(1);
	
	CStrW filename;
	if (! ToPrimitive<CStrW>(cx, JS_ARGV(cx, vp)[0], filename))
		return JS_FALSE;
	
	JSound* newObject = new JSound(filename);
	newObject->m_EngineOwned = false;
	JS_SET_RVAL(cx, vp, OBJECT_TO_JSVAL(newObject->GetScript()));
	
	return JS_TRUE;
}
