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

// JS sound binding

// interface rationale:
// - can't just expose fire and forget playSound to script code:
//   we sometimes need to loop until a certain condition is met
//   (e.g. building is complete) => need means of access (Handle) to sound.
//
// - the current 64-bit Handle can't be stored as-is by JS code;
//   we could make it 32 bit, but that limits its usefulness
//   (barely enough tag bits).
//
// - instead, we provide a thin class wrapper (using scriptableobject.h)
//   on top of the snd API that encapsulates the Handle.

#ifndef INCLUDED_JSI_SOUND
#define INCLUDED_JSI_SOUND

#include "scripting/ScriptableObject.h"
#include "lib/res/handle.h"

class JSI_Sound : public CJSObject<JSI_Sound>
{
public:

	Handle m_Handle;

	// note: filename is stored by handle manager; no need to keep a copy here.

	JSI_Sound(const VfsPath& pathname);
	~JSI_Sound();

	// Script-bound functions

	CStr ToString(JSContext* cx, uintN argc, jsval* argv);

	// start playing the sound (one-shot).
	// it will automatically be freed when done.
	bool Play(JSContext* cx, uintN argc, jsval* argv);

	// request the sound be played until free() is called. returns immediately.
	bool Loop(JSContext* cx, uintN argc, jsval* argv);

	// stop sound if currently playing and free resources.
	// doesn't need to be called unless played via loop() -
	// sounds are freed automatically when done playing.
	bool Free(JSContext* cx, uintN argc, jsval* argv);

	bool SetGain(JSContext* cx, uintN argc, jsval* argv);

	bool SetPitch(JSContext* cx, uintN argc, jsval* argv);

	bool SetPosition(JSContext* cx, uintN argc, jsval* argv);

	bool Fade(JSContext* cx, uintN argc, jsval* argv);

	static JSBool Construct(JSContext* cx, uintN argc, jsval* vp);

	static void ScriptingInit();
};

#endif	// #ifndef INCLUDED_JSI_SOUND
