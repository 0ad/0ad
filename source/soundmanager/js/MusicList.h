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

#ifndef INCLUDED_MUSICLIST_H
#define INCLUDED_MUSICLIST_H

#include "scripting/ScriptableObject.h"
#include "soundmanager/items/ISoundItem.h"

class JMusicList : public CJSObject<JMusicList>
{
public: 
  JMusicList();

  // Script-bound functions

  CStr ToString(JSContext* cx, uintN argc, jsval* argv);

  bool Play(JSContext* cx, uintN argc, jsval* argv);
  bool Loop(JSContext* cx, uintN argc, jsval* argv);
  bool AddItem(JSContext* cx, uintN argc, jsval* vp);

  static JSBool Construct(JSContext* cx, uintN argc, jsval* vp);

  static void ScriptingInit();

protected:

};

#endif  // #ifndef INCLUDED_MUSICLIST_H