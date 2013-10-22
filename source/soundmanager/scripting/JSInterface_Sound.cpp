/* Copyright (C) 2013 Wildfire Games.
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
#include "scriptinterface/ScriptInterface.h"

#include "JSInterface_Sound.h"

#include "lib/config2.h"
#include "lib/utf8.h"
#include "maths/Vector3D.h"
#include "ps/Filesystem.h"
#include "soundmanager/SoundManager.h"

#include <sstream>

namespace JSI_Sound
{
  #if CONFIG2_AUDIO

  void StartMusic(void* UNUSED(cbdata))
  {
    if ( CSoundManager* sndManager = (CSoundManager*)g_SoundManager )
      sndManager->SetMusicEnabled(true);
  }

  void StopMusic(void* UNUSED(cbdata))
  {
    if ( CSoundManager* sndManager = (CSoundManager*)g_SoundManager )
      sndManager->SetMusicEnabled(false);
  }

  void ClearPlaylist(void* UNUSED(cbdata))
  {
    if ( CSoundManager* sndManager = (CSoundManager*)g_SoundManager )
      sndManager->ClearPlayListItems();
  }

  void AddPlaylistItem(void* UNUSED(cbdata), std::wstring filename)
  {  
    if ( CSoundManager* sndManager = (CSoundManager*)g_SoundManager )
      sndManager->AddPlayListItem(VfsPath(filename));
  }

  void StartPlaylist(void* UNUSED(cbdata), bool looping)
  {
    if ( CSoundManager* sndManager = (CSoundManager*)g_SoundManager )
      sndManager->StartPlayList( looping );
  }

  void PlayMusic(void* UNUSED(cbdata), std::wstring filename, bool looping)
  {
    if ( CSoundManager* sndManager = (CSoundManager*)g_SoundManager )
      sndManager->PlayAsMusic( filename, looping);
  }

  void PlayUISound(void* UNUSED(cbdata), std::wstring filename, bool looping)
  {
    if ( CSoundManager* sndManager = (CSoundManager*)g_SoundManager )
      sndManager->PlayAsUI( filename, looping);
  }

  void PlayAmbientSound(void* UNUSED(cbdata), std::wstring filename, bool looping)
  {
    if ( CSoundManager* sndManager = (CSoundManager*)g_SoundManager )
      sndManager->PlayAsAmbient( filename, looping);
  }

  bool MusicPlaying(void* UNUSED(cbdata))
  {
    return true;
  }



  #else
    bool MusicPlaying(void* UNUSED(cbdata) ){ return false; }
    void PlayAmbientSound(void* UNUSED(cbdata), std::wstring UNUSED(filename), bool UNUSED(looping) ){}
    void PlayUISound(void* UNUSED(cbdata), std::wstring UNUSED(filename), bool UNUSED(looping) ) {}
    void PlayMusic(void* UNUSED(cbdata), std::wstring UNUSED(filename), bool UNUSED(looping) ) {}
    void StartPlaylist(void* UNUSED(cbdata), bool UNUSED(looping) ){}
    void AddPlaylistItem(void* UNUSED(cbdata), std::wstring UNUSED(filename) ){}
    void ClearPlaylist(void* UNUSED(cbdata) ){}
    void StopMusic(void* UNUSED(cbdata) ){}
    void StartMusic(void* UNUSED(cbdata) ){}

  #endif


  void RegisterScriptFunctions(ScriptInterface& scriptInterface)
  {
    scriptInterface.RegisterFunction<void, &StartMusic>("StartMusic");
    scriptInterface.RegisterFunction<void, &StopMusic>("StopMusic");
    scriptInterface.RegisterFunction<void, &ClearPlaylist>("ClearPlaylist");
    scriptInterface.RegisterFunction<void, std::wstring, &AddPlaylistItem>("AddPlaylistItem");
    scriptInterface.RegisterFunction<void, bool, &StartPlaylist>("StartPlaylist");
    scriptInterface.RegisterFunction<void, std::wstring, bool, &PlayMusic>("PlayMusic");
    scriptInterface.RegisterFunction<void, std::wstring, bool, &PlayUISound>("PlayUISound");
    scriptInterface.RegisterFunction<void, std::wstring, bool, &PlayAmbientSound>("PlayAmbientSound");
    scriptInterface.RegisterFunction<bool, &MusicPlaying>("MusicPlaying");
  }
}


