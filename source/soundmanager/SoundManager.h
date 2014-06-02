/* Copyright (C) 2014 Wildfire Games.
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

#include "ISoundManager.h"
#include "data/SoundData.h"
#include "items/ISoundItem.h"
#include "scripting/SoundGroup.h"

#include "lib/external_libraries/openal.h"
#include "lib/file/vfs/vfs_path.h"
#include "ps/Profiler2.h"
#include "simulation2/system/Entity.h"

#include <vector>
#include <map>

#define AL_CHECK CSoundManager::al_check(__func__, __LINE__);

struct ALSourceHolder
{
	/// Title of the column
	ALuint 		ALSource;
	ISoundItem*	SourceItem;
};

typedef std::vector<VfsPath> PlayList;
typedef std::vector<ISoundItem*> ItemsList;
typedef std::map<entity_id_t, ISoundItem*> ItemsMap;
typedef	std::map<std::wstring, CSoundGroup*> SoundGroupMap;

class CSoundManagerWorker;


class CSoundManager : public ISoundManager
{
	NONCOPYABLE(CSoundManager);

protected:

	ALuint m_ALEnvironment;
	ALCcontext* m_Context;
	ALCdevice* m_Device;
	ISoundItem* m_CurrentTune;
	ISoundItem* m_CurrentEnvirons;
	CSoundManagerWorker* m_Worker;
	CMutex m_DistressMutex;
	PlayList* m_PlayListItems;
	SoundGroupMap m_SoundGroups;

	float m_Gain;
	float m_MusicGain;
	float m_AmbientGain;
	float m_ActionGain;
	float m_UIGain;
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

	void StartWorker();

	ISoundItem* LoadItem(const VfsPath& itemPath);
	ISoundItem* ItemForData(CSoundData* itemData);
	ISoundItem* ItemForEntity(entity_id_t source, CSoundData* sndData);

	Status ReloadChangedFiles(const VfsPath& path);

	void ClearPlayListItems();
	void StartPlayList(bool doLoop);
	void AddPlayListItem(const VfsPath& itemPath);

	static void CreateSoundManager();
	static void SetEnabled(bool doEnable);
	static Status ReloadChangedFileCB(void* param, const VfsPath& path);

	static void CloseGame();

	static void al_ReportError(ALenum err, const char* caller, int line);
	static void al_check(const char* caller, int line);

	void SetMusicEnabled(bool isEnabled);
	void setSoundEnabled(bool enabled);

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

	void PlayAsMusic(const VfsPath& itemPath, bool looping);
	void PlayAsAmbient(const VfsPath& itemPath, bool looping);
	void PlayAsUI(const VfsPath& itemPath, bool looping);
	void PlayAsGroup(const VfsPath& groupPath, CVector3D sourcePos, entity_id_t source, bool ownedSound);

	void PlayGroupItem(ISoundItem* anItem, ALfloat groupGain);

	bool InDistress();
	void SetDistressThroughShortage();
	void SetDistressThroughError();

	void Pause(bool pauseIt);
	void PauseMusic(bool pauseIt);
	void PauseAmbient(bool pauseIt);
	void PauseAction(bool pauseIt);
	void SetAmbientItem(ISoundItem* anItem);

	void SetMasterGain(float gain);
	void SetMusicGain(float gain);
	void SetAmbientGain(float gain);
	void SetActionGain(float gain);
	void SetUIGain(float gain);

protected:
	void InitListener();
	Status AlcInit();
	void SetMusicItem(ISoundItem* anItem);

private:
	CSoundManager(CSoundManager* UNUSED(other)){};
};

#else // !CONFIG2_AUDIO

#define AL_CHECK

#endif // !CONFIG2_AUDIO

#endif // INCLUDED_SOUNDMANAGER_H

