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

#ifndef INCLUDED_SOUNDMANAGER_H
#define INCLUDED_SOUNDMANAGER_H

#include "lib/config2.h"

#if CONFIG2_AUDIO

#include "lib/file/vfs/vfs_path.h"
#include "soundmanager/items/ISoundItem.h"
#include "simulation2/system/Entity.h"
#include "soundmanager/data/SoundData.h"
#include "soundmanager/items/ISoundItem.h"
#include "ps/Profiler2.h"

#include <vector>
#include <map>

#define AL_CHECK CSoundManager::al_check(__func__, __LINE__);

typedef std::vector<VfsPath> PlayList;
typedef std::vector<ISoundItem*> ItemsList;
typedef std::map<entity_id_t, ISoundItem*> ItemsMap;

class CSoundManagerWorker;


struct ALSourceHolder
{
	/// Title of the column
	ALuint 		ALSource;
	ISoundItem*	SourceItem;
};

class CSoundManager
{
protected:

	ALuint m_ALEnvironment;
	ALCcontext* m_Context;
	ALCdevice* m_Device;
	ISoundItem* m_CurrentTune;
	ISoundItem* m_CurrentEnvirons;
	CSoundManagerWorker* m_Worker;
	CMutex m_DistressMutex;
	PlayList* m_PlayListItems;

	float m_Gain;
	float m_MusicGain;
	float m_AmbientGain;
	float m_ActionGain;
	bool m_Enabled;
	long m_BufferSize;
	int m_BufferCount;
	bool m_MusicEnabled;
	bool m_SoundEnabled;

	bool m_MusicPaused;
	bool m_AmbientPaused;
	bool m_ActionPaused;
	bool m_RunningPlaylist;
 	bool m_PlayingPlaylist;
  bool m_LoopingPlaylist;

	long m_PlaylistGap;
	long m_DistressErrCount;
	long m_DistressTime;

	ALSourceHolder* m_ALSourceBuffer;

public:
	CSoundManager();
	virtual ~CSoundManager();

	ISoundItem* LoadItem(const VfsPath& itemPath);
	ISoundItem* ItemForData(CSoundData* itemData);
	ISoundItem* ItemForEntity( entity_id_t source, CSoundData* sndData);

	void ClearPlayListItems();
	void StartPlayList( bool doLoop );
	void AddPlayListItem( const VfsPath& itemPath);

	static void ScriptingInit();
	static void CreateSoundManager();
	static void SetEnabled(bool doEnable);
	
	static void al_ReportError(ALenum err, const char* caller, int line);
	static void al_check(const char* caller, int line);

	void SetMusicEnabled (bool isEnabled);
	void setSoundEnabled( bool enabled );

	ALuint GetALSource(ISoundItem* anItem);
	void ReleaseALSource(ALuint theSource);
	ISoundItem* ItemFromData(CSoundData* itemData);

	ISoundItem* ItemFromWAV(VfsPath& fname);
	ISoundItem* ItemFromOgg(VfsPath& fname);

	ISoundItem* GetSoundItem(unsigned long itemRow);
	unsigned long Count();
	void IdleTask();
	
	void SetMemoryUsage(long bufferSize, int bufferCount);
	long GetBufferCount();
	long GetBufferSize();

	void SetMusicItem(ISoundItem* anItem);
	void SetAmbientItem(ISoundItem* anItem);
	void PlayActionItem(ISoundItem* anItem);
	void PlayGroupItem(ISoundItem* anItem, ALfloat groupGain);

	void SetMasterGain(float gain);
	void SetMusicGain(float gain);
	void SetAmbientGain(float gain);
	void SetActionGain(float gain);
	bool InDistress();
	void SetDistressThroughShortage();
	void SetDistressThroughError();

	void Pause(bool pauseIt);
	void PauseMusic (bool pauseIt);
	void PauseAmbient (bool pauseIt);
	void PauseAction (bool pauseIt);

protected:
	void InitListener();
	virtual Status AlcInit();

private:
	CSoundManager(CSoundManager* UNUSED(other)){};
};

#else // !CONFIG2_AUDIO

#define AL_CHECK

class CSoundManager
{
public:
	static void ScriptingInit();
};
#endif // !CONFIG2_AUDIO


extern CSoundManager*  g_SoundManager;

#endif // INCLUDED_SOUNDMANAGER_H

