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

#include "JMusicSound.h"
#include "maths/Vector3D.h"

#include "lib/utf8.h"
#include "lib/res/sound/snd_mgr.h"
#include "lib/res/h_mgr.h"	// h_filename
#include "ps/Filesystem.h"

#include "soundmanager/CSoundManager.h"


JMusicSound::JMusicSound(const VfsPath& pathname)
{
    debug_printf(L"playing music sound %ls\n\n", pathname.string().c_str());
    OsPath itsPth = OsPath( pathname.string().c_str() );
    mSndItem = g_SoundManager->loadItem( itsPth );
    if ( mSndItem )
        mSndItem->playAsMusic();
}

JMusicSound::~JMusicSound()
{
}

bool JMusicSound::SetGain(JSContext* cx, uintN argc, jsval* argv)
{
	float gain;
	if (! ToPrimitive<float>(cx, argv[0], gain))
		return false;
    debug_printf(L"set music gain to %f", gain );
    
	return true;
}

bool JMusicSound::SetPitch(JSContext* cx, uintN argc, jsval* argv)
{
	float pitch;
	if (! ToPrimitive<float>(cx, argv[0], pitch))
		return false;
    
    debug_printf(L"set music pitch to %f", pitch );
	return true;
}

bool JMusicSound::SetPosition(JSContext* cx, uintN argc, jsval* argv)
{
//	if (! m_Handle)
//		return false;
    
	ENSURE(argc >= 1); // FIXME
    
	CVector3D pos;
	if (ToPrimitive<CVector3D>(cx, argv[0], pos))
        debug_printf(L"set music position to %f,%f,%f", pos[0], pos[1], pos[2] );

        
        
        // absolute world coords
//	if (ToPrimitive<CVector3D>(cx, argv[0], pos))
//		(void)snd_set_pos(m_Handle, pos[0], pos[1], pos[2]);
	// relative, 0 offset - right on top of the listener
	// (we don't need displacement from the listener, e.g. always behind)
//	else
//		(void)snd_set_pos(m_Handle, 0,0,0, true);
    
	return true;
}


bool JMusicSound::Fade(JSContext* cx, uintN argc, jsval* argv)
{
//	if (! m_Handle)
//		return false;
    
//	ENSURE(argc >= 3); // FIXME
    float initial_gain, final_gain;
    float length;
    if (! (ToPrimitive<float>(cx, argv[0], initial_gain)
           && ToPrimitive<float>(cx, argv[1], final_gain)
           && ToPrimitive<float>(cx, argv[2], length)))
        return false;
    
    debug_printf(L"playing music pulling a fade from %f to %f over %f", initial_gain, final_gain,length );

	return true;
}

// start playing the sound (one-shot).
// it will automatically be freed when done.
bool JMusicSound::Play(JSContext* UNUSED(cx), uintN UNUSED(argc), jsval* UNUSED(argv))
{
    debug_printf(L"playing music hitting play ");

	return true;
}

// request the sound be played until free() is called. returns immediately.
bool JMusicSound::Loop(JSContext* UNUSED(cx), uintN UNUSED(argc), jsval* UNUSED(argv))
{
    debug_printf(L"loop music sound\n\n");
//	if (! m_Handle)
		return false;
    
//	(void)snd_set_loop(m_Handle, true);
//	(void)snd_play(m_Handle);
	return true;
}

// stop sound if currently playing and free resources.
// doesn't need to be called unless played via loop() -
// sounds are freed automatically when done playing.
bool JMusicSound::Free(JSContext* UNUSED(cx), uintN UNUSED(argc), jsval* UNUSED(argv))
{
    debug_printf(L"free music sound \n\n" );
//	if (! m_Handle)
//		return false;
    
//	(void)snd_free(m_Handle);	// resets it to 0
	return true;
}


// Script-bound functions


void JMusicSound::ScriptingInit()
{
	AddMethod<CStr, &JMusicSound::ToString>("toString", 0);
	AddMethod<bool, &JMusicSound::Play>("play", 0);
	AddMethod<bool, &JMusicSound::Loop>("loop", 0);
	AddMethod<bool, &JMusicSound::Free>("free", 0);
	AddMethod<bool, &JMusicSound::SetGain>("setGain", 0);
	AddMethod<bool, &JMusicSound::SetPitch>("setPitch", 0);
	AddMethod<bool, &JMusicSound::SetPosition>("setPosition", 0);
	AddMethod<bool, &JMusicSound::Fade>("fade", 0);
    
	CJSObject<JMusicSound>::ScriptingInit("MusicSound", &JMusicSound::Construct, 1);
}

CStr JMusicSound::ToString(JSContext* UNUSED(cx), uintN UNUSED(argc), jsval* UNUSED(argv))
{
	return "[object MusicSound: " ;//+ (m_Handle ? utf8_from_wstring(h_filename(m_Handle).string()) : "(null)") + "]";
}

JSBool JMusicSound::Construct(JSContext* cx, uintN argc, jsval* vp)
{
//	JSU_REQUIRE_MIN_PARAMS(1);
    
	CStrW filename;
	if (! ToPrimitive<CStrW>(cx, JS_ARGV(cx, vp)[0], filename))
        return JS_FALSE;
    
	JMusicSound* newObject = new JMusicSound(filename);
	newObject->m_EngineOwned = false;
	JS_SET_RVAL(cx, vp, OBJECT_TO_JSVAL(newObject->GetScript()));
    
	return JS_TRUE;
}
