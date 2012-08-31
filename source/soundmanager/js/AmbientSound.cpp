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

#include "AmbientSound.h"

#include "lib/config2.h"
#include "lib/utf8.h"
#include "maths/Vector3D.h"
#include "ps/CLogger.h"
#include "ps/Filesystem.h"

#include "soundmanager/SoundManager.h"

JAmbientSound::JAmbientSound(const VfsPath& pathname) : m_FileName(pathname)
{
}

// start playing the sound, all ambient sounds loop
bool JAmbientSound::Play(JSContext* UNUSED(cx), uintN UNUSED(argc), jsval* UNUSED(argv))
{
#if CONFIG2_AUDIO
	if ( g_SoundManager ) {
		ISoundItem*	aSnd = g_SoundManager->LoadItem(m_FileName);

		if (aSnd)
			aSnd->PlayAsAmbient();
		else
			LOGERROR(L"sound item could not be loaded to play: %ls\n", m_FileName.string().c_str());
	}
#endif // CONFIG2_AUDIO
	return true;
}

// start playing the sound, all ambient sounds loop
bool JAmbientSound::Loop(JSContext* UNUSED(cx), uintN UNUSED(argc), jsval* UNUSED(argv))
{
#if CONFIG2_AUDIO
	if ( g_SoundManager ) {
		ISoundItem*	aSnd = g_SoundManager->LoadItem(m_FileName);

		if (aSnd)
			aSnd->PlayAsAmbient();
		else
			LOGERROR(L"sound item could not be loaded to loop: %ls\n", m_FileName.string().c_str());
	}
#endif // CONFIG2_AUDIO
	return true;
}
bool JAmbientSound::Free(JSContext* UNUSED(cx), uintN UNUSED(argc), jsval* UNUSED(argv))
{
#if CONFIG2_AUDIO
	if ( g_SoundManager )
		g_SoundManager->SetAmbientItem(0L);
#endif // CONFIG2_AUDIO

	return true;
}

// Script-bound functions


void JAmbientSound::ScriptingInit()
{
	AddMethod<CStr, &JAmbientSound::ToString>("toString", 0);
	AddMethod<bool, &JAmbientSound::Play>("play", 0);
	AddMethod<bool, &JAmbientSound::Loop>("loop", 0);
	AddMethod<bool, &JAmbientSound::Free>("free", 0);
	
	CJSObject<JAmbientSound>::ScriptingInit("AmbientSound", &JAmbientSound::Construct, 1);
}

CStr JAmbientSound::ToString(JSContext* UNUSED(cx), uintN UNUSED(argc), jsval* UNUSED(argv))
{
	std::ostringstream stringStream;
	stringStream << "[object AmbientSound: ";
	stringStream << m_FileName.string().c_str();
	
	return stringStream.str();
}

JSBool JAmbientSound::Construct(JSContext* cx, uintN UNUSED(argc), jsval* vp)
{
//	JSU_REQUIRE_MIN_PARAMS(1);
	
	CStrW filename;
	if (! ToPrimitive<CStrW>(cx, JS_ARGV(cx, vp)[0], filename))
		return JS_FALSE;

	JAmbientSound* newObject = new JAmbientSound(filename);
	newObject->m_EngineOwned = false;

	JS_SET_RVAL(cx, vp, OBJECT_TO_JSVAL(newObject->GetScript()));

	return JS_TRUE;
}

