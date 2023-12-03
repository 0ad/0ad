/* Copyright (C) 2021 Wildfire Games.
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

#include "JSInterface_Sound.h"

#include "lib/config2.h"
#include "lib/utf8.h"
#include "maths/Vector3D.h"
#include "ps/Filesystem.h"
#include "scriptinterface/FunctionWrapper.h"
#include "scriptinterface/ScriptRequest.h"
#include "soundmanager/SoundManager.h"

#include <sstream>

namespace JSI_Sound
{
#if CONFIG2_AUDIO

	void StartMusic()
	{
		if (CSoundManager* sndManager = (CSoundManager*)g_SoundManager)
			sndManager->SetMusicEnabled(true);
	}

	void StopMusic()
	{
		if (CSoundManager* sndManager = (CSoundManager*)g_SoundManager)
			sndManager->SetMusicEnabled(false);
	}

	void ClearPlaylist()
	{
		if (CSoundManager* sndManager = (CSoundManager*)g_SoundManager)
			sndManager->ClearPlayListItems();
	}

	void AddPlaylistItem(const std::wstring& filename)
	{
		if (CSoundManager* sndManager = (CSoundManager*)g_SoundManager)
			sndManager->AddPlayListItem(VfsPath(filename));
	}

	void StartPlaylist(bool looping)
	{
		if (CSoundManager* sndManager = (CSoundManager*)g_SoundManager)
			sndManager->StartPlayList(looping );
	}

	void PlayMusic(const std::wstring& filename, bool looping)
	{
		if (CSoundManager* sndManager = (CSoundManager*)g_SoundManager)
			sndManager->PlayAsMusic(filename, looping);
	}

	void PlayUISound(const std::wstring& filename, bool looping)
	{
		if (CSoundManager* sndManager = (CSoundManager*)g_SoundManager)
			sndManager->PlayAsUI(filename, looping);
	}

	void PlayAmbientSound(const std::wstring& filename, bool looping)
	{
		if (CSoundManager* sndManager = (CSoundManager*)g_SoundManager)
			sndManager->PlayAsAmbient(filename, looping);
	}

	bool MusicPlaying()
	{
		return true;
	}

	void SetMasterGain(float gain)
	{
		if (CSoundManager* sndManager = (CSoundManager*)g_SoundManager)
			sndManager->SetMasterGain(gain);
	}

	void SetMusicGain(float gain)
	{
		if (CSoundManager* sndManager = (CSoundManager*)g_SoundManager)
			sndManager->SetMusicGain(gain);
	}

	void SetAmbientGain(float gain)
	{
		if (CSoundManager* sndManager = (CSoundManager*)g_SoundManager)
			sndManager->SetAmbientGain(gain);
	}

	void SetActionGain(float gain)
	{
		if (CSoundManager* sndManager = (CSoundManager*)g_SoundManager)
			sndManager->SetActionGain(gain);
	}

	void SetUIGain(float gain)
	{
		if (CSoundManager* sndManager = (CSoundManager*)g_SoundManager)
			sndManager->SetUIGain(gain);
	}

#else

	bool MusicPlaying( ){ return false; }
	void PlayAmbientSound(const std::wstring& UNUSED(filename), bool UNUSED(looping) ){}
	void PlayUISound(const std::wstring& UNUSED(filename), bool UNUSED(looping) ) {}
	void PlayMusic(const std::wstring& UNUSED(filename), bool UNUSED(looping) ) {}
	void StartPlaylist(bool UNUSED(looping) ){}
	void AddPlaylistItem(const std::wstring& UNUSED(filename) ){}
	void ClearPlaylist( ){}
	void StopMusic( ){}
	void StartMusic( ){}
	void SetMasterGain(float UNUSED(gain)){}
	void SetMusicGain(float UNUSED(gain)){}
	void SetAmbientGain(float UNUSED(gain)){}
	void SetActionGain(float UNUSED(gain)){}
	void SetUIGain(float UNUSED(gain)){}

#endif

	void RegisterScriptFunctions(const ScriptRequest& rq)
	{
		ScriptFunction::Register<&StartMusic>(rq, "StartMusic");
		ScriptFunction::Register<&StopMusic>(rq, "StopMusic");
		ScriptFunction::Register<&ClearPlaylist>(rq, "ClearPlaylist");
		ScriptFunction::Register<&AddPlaylistItem>(rq, "AddPlaylistItem");
		ScriptFunction::Register<&StartPlaylist>(rq, "StartPlaylist");
		ScriptFunction::Register<&PlayMusic>(rq, "PlayMusic");
		ScriptFunction::Register<&PlayUISound>(rq, "PlayUISound");
		ScriptFunction::Register<&PlayAmbientSound>(rq, "PlayAmbientSound");
		ScriptFunction::Register<&MusicPlaying>(rq, "MusicPlaying");
		ScriptFunction::Register<&SetMasterGain>(rq, "SetMasterGain");
		ScriptFunction::Register<&SetMusicGain>(rq, "SetMusicGain");
		ScriptFunction::Register<&SetAmbientGain>(rq, "SetAmbientGain");
		ScriptFunction::Register<&SetActionGain>(rq, "SetActionGain");
		ScriptFunction::Register<&SetUIGain>(rq, "SetUIGain");
	}
}
