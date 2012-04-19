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
#include "JSound.h"
#include "maths/Vector3D.h"

#include "lib/utf8.h"
#include "lib/res/sound/snd_mgr.h"
#include "lib/res/h_mgr.h"	// h_filename
#include "ps/Filesystem.h"


JSound::JSound(const VfsPath& pathname)
{
//	m_Handle = snd_open(g_VFS, pathname);
    debug_printf(L"playing sound %ls\n\n", pathname.string().c_str());
}

JSound::~JSound()
{
}

bool JSound::SetGain(JSContext* cx, uintN argc, jsval* argv)
{

	return true;
}

bool JSound::SetPitch(JSContext* cx, uintN argc, jsval* argv)
{
}

bool JSound::SetPosition(JSContext* cx, uintN argc, jsval* argv)
{
//	if (! m_Handle)
//		return false;
    
	ENSURE(argc >= 1); // FIXME
    
	CVector3D pos;
	// absolute world coords
//	if (ToPrimitive<CVector3D>(cx, argv[0], pos))
//		(void)snd_set_pos(m_Handle, pos[0], pos[1], pos[2]);
	// relative, 0 offset - right on top of the listener
	// (we don't need displacement from the listener, e.g. always behind)
//	else
//		(void)snd_set_pos(m_Handle, 0,0,0, true);
    
	return true;
}


bool JSound::Fade(JSContext* cx, uintN argc, jsval* argv)
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
    
    debug_printf(L"pulling a fade from %f to %f over %f", initial_gain, final_gain,length );

	return true;
}

// start playing the sound (one-shot).
// it will automatically be freed when done.
bool JSound::Play(JSContext* UNUSED(cx), uintN UNUSED(argc), jsval* UNUSED(argv))
{
    debug_printf(L"hitting play ");

	return true;
}

// request the sound be played until free() is called. returns immediately.
bool JSound::Loop(JSContext* UNUSED(cx), uintN UNUSED(argc), jsval* UNUSED(argv))
{
    debug_printf(L"loop sound\n\n");
//	if (! m_Handle)
		return false;
    
//	(void)snd_set_loop(m_Handle, true);
//	(void)snd_play(m_Handle);
	return true;
}

// stop sound if currently playing and free resources.
// doesn't need to be called unless played via loop() -
// sounds are freed automatically when done playing.
bool JSound::Free(JSContext* UNUSED(cx), uintN UNUSED(argc), jsval* UNUSED(argv))
{
    debug_printf(L"free sound \n\n" );
//	if (! m_Handle)
//		return false;
    
//	(void)snd_free(m_Handle);	// resets it to 0
	return true;
}


// Script-bound functions


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
	return "[object Sound: " ;//+ (m_Handle ? utf8_from_wstring(h_filename(m_Handle).string()) : "(null)") + "]";
}

JSBool JSound::Construct(JSContext* cx, uintN argc, jsval* vp)
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
