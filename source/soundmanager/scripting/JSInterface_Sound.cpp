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
  void StartMusic(void* UNUSED(cbdata))
  {
  #if CONFIG2_AUDIO
    CSoundManager* sndManager = (CSoundManager*)g_SoundManager;
    if ( sndManager )
      sndManager->SetMusicEnabled(true);
  #endif
  }
  void StopMusic(void* UNUSED(cbdata))
  {
  #if CONFIG2_AUDIO
    CSoundManager* sndManager = (CSoundManager*)g_SoundManager;
    if ( sndManager )
      sndManager->SetMusicEnabled(false);
  #endif
  }

  void ClearPlaylist(void* UNUSED(cbdata))
  {
  #if CONFIG2_AUDIO
    CSoundManager* sndManager = (CSoundManager*)g_SoundManager;
    if ( sndManager )
      sndManager->ClearPlayListItems();
  #endif
  }

  void AddPlaylistItem(void* UNUSED(cbdata), std::wstring filename)
  {
  #if CONFIG2_AUDIO
    CSoundManager* sndManager = (CSoundManager*)g_SoundManager;
    if ( sndManager )
      sndManager->AddPlayListItem( new VfsPath( filename ) );
  #else
    UNUSED2(filename);
  #endif
  }

  void LoopPlaylist(void* UNUSED(cbdata))
  {
  #if CONFIG2_AUDIO
    CSoundManager* sndManager = (CSoundManager*)g_SoundManager;
    if ( sndManager )
      sndManager->StartPlayList( true );
  #endif
  }
  void PlayPlaylist(void* UNUSED(cbdata))
  {
  #if CONFIG2_AUDIO
    CSoundManager* sndManager = (CSoundManager*)g_SoundManager;
    if ( sndManager )
      sndManager->StartPlayList( false );
  #endif
  }

  void LoopMusic(void* UNUSED(cbdata), std::wstring filename)
  {
  #if CONFIG2_AUDIO
    if ( g_SoundManager ) {
      CSoundManager* sndManager = (CSoundManager*)g_SoundManager;
      ISoundItem* aSnd = sndManager->LoadItem(filename);
      if (aSnd != NULL)
        aSnd->PlayAsMusic();
    }
  #else
    UNUSED2(filename);
  #endif // CONFIG2_AUDIO
  }
  void PlayMusic(void* UNUSED(cbdata), std::wstring filename)
  {
  #if CONFIG2_AUDIO
    if ( g_SoundManager ) {
      CSoundManager* sndManager = (CSoundManager*)g_SoundManager;
      ISoundItem* aSnd = sndManager->LoadItem(filename);
      if (aSnd != NULL)
        aSnd->PlayAsMusic();
    }
  #else
    UNUSED2(filename);
  #endif // CONFIG2_AUDIO
  }

  void LoopAmbientSound(void* UNUSED(cbdata), std::wstring filename)
  {
  #if CONFIG2_AUDIO
    if ( g_SoundManager ) {
      CSoundManager* sndManager = (CSoundManager*)g_SoundManager;
      ISoundItem* aSnd = sndManager->LoadItem(filename);
      if (aSnd != NULL)
        aSnd->PlayAsAmbient();
    }
  #else
    UNUSED2(filename);
  #endif // CONFIG2_AUDIO
  }

  bool MusicPlaying(void* UNUSED(cbdata))
  {
  #if CONFIG2_AUDIO
    return true;
  #else
    return false;
  #endif // CONFIG2_AUDIO
  }

  void RegisterScriptFunctions(ScriptInterface& scriptInterface)
  {
    scriptInterface.RegisterFunction<void, &StartMusic>("StartMusic");
    scriptInterface.RegisterFunction<void, &StopMusic>("StopMusic");
    scriptInterface.RegisterFunction<void, &ClearPlaylist>("ClearPlaylist");
    scriptInterface.RegisterFunction<void, std::wstring, &AddPlaylistItem>("AddPlaylistItem");
    scriptInterface.RegisterFunction<void, &LoopPlaylist>("LoopPlaylist");
    scriptInterface.RegisterFunction<void, &PlayPlaylist>("PlayPlaylist");
    scriptInterface.RegisterFunction<void, std::wstring, &LoopMusic>("LoopMusic");
    scriptInterface.RegisterFunction<void, std::wstring, &PlayMusic>("PlayMusic");
    scriptInterface.RegisterFunction<void, std::wstring, &LoopAmbientSound>("LoopAmbientSound");
    scriptInterface.RegisterFunction<bool, &MusicPlaying>("MusicPlaying");
  }
}