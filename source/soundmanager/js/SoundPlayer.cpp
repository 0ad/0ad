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

#include "SoundPlayer.h"

#include "lib/utf8.h"
#include "maths/Vector3D.h"
#include "ps/Filesystem.h"
#include "soundmanager/SoundManager.h"


JSoundPlayer::JSoundPlayer()
{
}

JSoundPlayer::~JSoundPlayer()
{
}

bool JSoundPlayer::StartMusic(JSContext* UNUSED(cx), uintN UNUSED(argc), jsval* UNUSED(argv))
{
	g_SoundManager->SetMusicEnabled(true);

	return true;
}

// request the sound be played until free() is called. returns immediately.
bool JSoundPlayer::StopMusic(JSContext* UNUSED(cx), uintN UNUSED(argc), jsval* UNUSED(argv))
{
	g_SoundManager->SetMusicEnabled(false);

	return true;
}

void JSoundPlayer::ScriptingInit()
{
	AddMethod<CStr, &JSoundPlayer::ToString>("toString", 0);
	AddMethod<bool, &JSoundPlayer::StartMusic>("startMusic", 0);
	AddMethod<bool, &JSoundPlayer::StopMusic>("stopMusic", 0);
	
	CJSObject<JSoundPlayer>::ScriptingInit("SoundPlayer", &JSoundPlayer::Construct, 1);
}

JSBool JSoundPlayer::Construct(JSContext* UNUSED(cx), uintN UNUSED(argc), jsval* vp)
{	
	JSoundPlayer* newObject = new JSoundPlayer();
	newObject->m_EngineOwned = false;
	JS_SET_RVAL(cx, vp, OBJECT_TO_JSVAL(newObject->GetScript()));
	
	return JS_TRUE;
}

CStr JSoundPlayer::ToString(JSContext* UNUSED(cx), uintN UNUSED(argc), jsval* UNUSED(argv))
{
	std::ostringstream stringStream;
	stringStream << "[object MusicPlayer]";
	
	return stringStream.str();
}

