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

#include "MusicSound.h"

#include "lib/config2.h"
#include "lib/utf8.h"
#include "maths/Vector3D.h"
#include "ps/CStr.h"
#include "ps/Filesystem.h"
#include "soundmanager/SoundManager.h"


JMusicSound::JMusicSound(const VfsPath& pathname) : m_FileName(pathname)
{
}

bool JMusicSound::Play(JSContext* UNUSED(cx), uintN UNUSED(argc), jsval* UNUSED(argv))
{
#if CONFIG2_AUDIO
	if ( g_SoundManager ) {
		ISoundItem* aSnd = g_SoundManager->LoadItem(m_FileName);
		if (aSnd != NULL)
			aSnd->PlayAsMusic();
	}
#endif // CONFIG2_AUDIO
	return true;
}

// request the sound be played until free() is called. returns immediately.
bool JMusicSound::Loop(JSContext* UNUSED(cx), uintN UNUSED(argc), jsval* UNUSED(argv))
{
#if CONFIG2_AUDIO
	if ( g_SoundManager ) {
		ISoundItem* aSnd = g_SoundManager->LoadItem(m_FileName);
		if (aSnd != NULL)
			aSnd->PlayAsMusic();
	}
#endif // CONFIG2_AUDIO
	return true;
}

void JMusicSound::ScriptingInit()
{
	AddMethod<CStr, &JMusicSound::ToString>("toString", 0);
	AddMethod<bool, &JMusicSound::Play>("play", 0);
	AddMethod<bool, &JMusicSound::Loop>("loop", 0);
	
	CJSObject<JMusicSound>::ScriptingInit("MusicSound", &JMusicSound::Construct, 1);
}

CStr JMusicSound::ToString(JSContext* UNUSED(cx), uintN UNUSED(argc), jsval* UNUSED(argv))
{
	std::ostringstream stringStream;
	stringStream << "[object MusicSound: ";
	stringStream << m_FileName.string().c_str();
	
	return stringStream.str();
}

JSBool JMusicSound::Construct(JSContext* cx, uintN UNUSED(argc), jsval* vp)
{	
	CStrW filename;
	if (! ToPrimitive<CStrW>(cx, JS_ARGV(cx, vp)[0], filename))
		return JS_FALSE;
	
	JMusicSound* newObject = new JMusicSound(filename);
	newObject->m_EngineOwned = false;
	JS_SET_RVAL(cx, vp, OBJECT_TO_JSVAL(newObject->GetScript()));

	return JS_TRUE;
}

