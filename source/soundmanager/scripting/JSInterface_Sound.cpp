/* Copyright (C) 2020 Wildfire Games.
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
#include "scriptinterface/ScriptInterface.h"
#include "soundmanager/SoundManager.h"

#include <sstream>

namespace JSI_Sound
{
#if CONFIG2_AUDIO

	void StartMusic(ScriptInterface::CmptPrivate* UNUSED(pCmptPrivate))
	{
		if (CSoundManager* sndManager = (CSoundManager*)g_SoundManager)
			sndManager->SetMusicEnabled(true);
	}

	void StopMusic(ScriptInterface::CmptPrivate* UNUSED(pCmptPrivate))
	{
		if (CSoundManager* sndManager = (CSoundManager*)g_SoundManager)
			sndManager->SetMusicEnabled(false);
	}

	void ClearPlaylist(ScriptInterface::CmptPrivate* UNUSED(pCmptPrivate))
	{
		if (CSoundManager* sndManager = (CSoundManager*)g_SoundManager)
			sndManager->ClearPlayListItems();
	}

	void AddPlaylistItem(ScriptInterface::CmptPrivate* UNUSED(pCmptPrivate), const std::wstring& filename)
	{
		if (CSoundManager* sndManager = (CSoundManager*)g_SoundManager)
			sndManager->AddPlayListItem(VfsPath(filename));
	}

	void StartPlaylist(ScriptInterface::CmptPrivate* UNUSED(pCmptPrivate), bool looping)
	{
		if (CSoundManager* sndManager = (CSoundManager*)g_SoundManager)
			sndManager->StartPlayList(looping );
	}

	void PlayMusic(ScriptInterface::CmptPrivate* UNUSED(pCmptPrivate), const std::wstring& filename, bool looping)
	{
		if (CSoundManager* sndManager = (CSoundManager*)g_SoundManager)
			sndManager->PlayAsMusic(filename, looping);
	}

	void PlayUISound(ScriptInterface::CmptPrivate* UNUSED(pCmptPrivate), const std::wstring& filename, bool looping)
	{
		if (CSoundManager* sndManager = (CSoundManager*)g_SoundManager)
			sndManager->PlayAsUI(filename, looping);
	}

	void PlayAmbientSound(ScriptInterface::CmptPrivate* UNUSED(pCmptPrivate), const std::wstring& filename, bool looping)
	{
		if (CSoundManager* sndManager = (CSoundManager*)g_SoundManager)
			sndManager->PlayAsAmbient(filename, looping);
	}

	bool MusicPlaying(ScriptInterface::CmptPrivate* UNUSED(pCmptPrivate))
	{
		return true;
	}

	void SetMasterGain(ScriptInterface::CmptPrivate* UNUSED(pCmptPrivate), float gain)
	{
		if (CSoundManager* sndManager = (CSoundManager*)g_SoundManager)
			sndManager->SetMasterGain(gain);
	}

	void SetMusicGain(ScriptInterface::CmptPrivate* UNUSED(pCmptPrivate), float gain)
	{
		if (CSoundManager* sndManager = (CSoundManager*)g_SoundManager)
			sndManager->SetMusicGain(gain);
	}

	void SetAmbientGain(ScriptInterface::CmptPrivate* UNUSED(pCmptPrivate), float gain)
	{
		if (CSoundManager* sndManager = (CSoundManager*)g_SoundManager)
			sndManager->SetAmbientGain(gain);
	}

	void SetActionGain(ScriptInterface::CmptPrivate* UNUSED(pCmptPrivate), float gain)
	{
		if (CSoundManager* sndManager = (CSoundManager*)g_SoundManager)
			sndManager->SetActionGain(gain);
	}

	void SetUIGain(ScriptInterface::CmptPrivate* UNUSED(pCmptPrivate), float gain)
	{
		if (CSoundManager* sndManager = (CSoundManager*)g_SoundManager)
			sndManager->SetUIGain(gain);
	}

#else

	bool MusicPlaying(ScriptInterface::CmptPrivate* UNUSED(pCmptPrivate) ){ return false; }
	void PlayAmbientSound(ScriptInterface::CmptPrivate* UNUSED(pCmptPrivate), const std::wstring& UNUSED(filename), bool UNUSED(looping) ){}
	void PlayUISound(ScriptInterface::CmptPrivate* UNUSED(pCmptPrivate), const std::wstring& UNUSED(filename), bool UNUSED(looping) ) {}
	void PlayMusic(ScriptInterface::CmptPrivate* UNUSED(pCmptPrivate), const std::wstring& UNUSED(filename), bool UNUSED(looping) ) {}
	void StartPlaylist(ScriptInterface::CmptPrivate* UNUSED(pCmptPrivate), bool UNUSED(looping) ){}
	void AddPlaylistItem(ScriptInterface::CmptPrivate* UNUSED(pCmptPrivate), const std::wstring& UNUSED(filename) ){}
	void ClearPlaylist(ScriptInterface::CmptPrivate* UNUSED(pCmptPrivate) ){}
	void StopMusic(ScriptInterface::CmptPrivate* UNUSED(pCmptPrivate) ){}
	void StartMusic(ScriptInterface::CmptPrivate* UNUSED(pCmptPrivate) ){}
	void SetMasterGain(ScriptInterface::CmptPrivate* UNUSED(pCmptPrivate), float UNUSED(gain)){}
	void SetMusicGain(ScriptInterface::CmptPrivate* UNUSED(pCmptPrivate), float UNUSED(gain)){}
	void SetAmbientGain(ScriptInterface::CmptPrivate* UNUSED(pCmptPrivate), float UNUSED(gain)){}
	void SetActionGain(ScriptInterface::CmptPrivate* UNUSED(pCmptPrivate), float UNUSED(gain)){}
	void SetUIGain(ScriptInterface::CmptPrivate* UNUSED(pCmptPrivate), float UNUSED(gain)){}

#endif

	void RegisterScriptFunctions(const ScriptInterface& scriptInterface)
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
		scriptInterface.RegisterFunction<void, float, &SetMasterGain>("SetMasterGain");
		scriptInterface.RegisterFunction<void, float, &SetMusicGain>("SetMusicGain");
		scriptInterface.RegisterFunction<void, float, &SetAmbientGain>("SetAmbientGain");
		scriptInterface.RegisterFunction<void, float, &SetActionGain>("SetActionGain");
		scriptInterface.RegisterFunction<void, float, &SetUIGain>("SetUIGain");
	}
}
