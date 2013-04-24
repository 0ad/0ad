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

#include "MusicList.h"

#include "lib/config2.h"
#include "lib/utf8.h"
#include "maths/Vector3D.h"
#include "ps/CStr.h"
#include "ps/Filesystem.h"
 #include "ps/CLogger.h"

#include "soundmanager/SoundManager.h"
#include "gui/GUI.h"
#include <sstream>


JMusicList::JMusicList()
{
    g_SoundManager->ClearPlayListItems();
}

bool JMusicList::AddItem(JSContext* cx, uintN UNUSED(argc), jsval* vp)
{ 
  CStrW filename;
  if (! ToPrimitive<CStrW>(cx, vp[0], filename))
    return false;

  g_SoundManager->AddPlayListItem( new VfsPath( filename ) );
  return true;
}


bool JMusicList::Play(JSContext* UNUSED(cx), uintN UNUSED(argc), jsval* UNUSED(argv))
{
  #if CONFIG2_AUDIO
  if ( g_SoundManager )
    g_SoundManager->StartPlayList( false );
  #endif // CONFIG2_AUDIO

  return true;
}

// request the sound be played until free() is called. returns immediately.
bool JMusicList::Loop(JSContext* UNUSED(cx), uintN UNUSED(argc), jsval* UNUSED(argv))
{
  #if CONFIG2_AUDIO
  if ( g_SoundManager )
    g_SoundManager->StartPlayList( true );

  #endif // CONFIG2_AUDIO

  return true;
}

void JMusicList::ScriptingInit()
{
  AddMethod<CStr, &JMusicList::ToString>("toString", 0);
  AddMethod<bool, &JMusicList::Play>("play", 0);
  AddMethod<bool, &JMusicList::Loop>("loop", 0);
  AddMethod<bool, &JMusicList::AddItem>("addItem", 1);

  CJSObject<JMusicList>::ScriptingInit("MusicList", &JMusicList::Construct, 0);
}

CStr JMusicList::ToString(JSContext* UNUSED(cx), uintN UNUSED(argc), jsval* UNUSED(argv))
{
  std::ostringstream stringStream;
  stringStream << "[object MusicList]";

  return stringStream.str();
}

JSBool JMusicList::Construct(JSContext* UNUSED(cx), uintN UNUSED(argc), jsval* vp)
{ 
  CStrW filename;

  JMusicList* newObject = new JMusicList();
  newObject->m_EngineOwned = false;
  JS_SET_RVAL(cx, vp, OBJECT_TO_JSVAL(newObject->GetScript()));

  return JS_TRUE;
}
