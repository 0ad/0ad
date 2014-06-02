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

#include "precompiled.h"

#include "ISoundManager.h"
#include "SoundManager.h"
#include "data/SoundData.h"
#include "items/CBufferItem.h"
#include "items/CSoundItem.h"
#include "items/CStreamItem.h"

#include "lib/external_libraries/libsdl.h"
#include "ps/CLogger.h"
#include "ps/CStr.h"
#include "ps/Filesystem.h"
#include "ps/Profiler2.h"

ISoundManager* g_SoundManager = NULL;

#define SOURCE_NUM 64

#if CONFIG2_AUDIO

class CSoundManagerWorker
{
	NONCOPYABLE(CSoundManagerWorker);

public:
	CSoundManagerWorker()
	{
		m_Items = new ItemsList;
		m_DeadItems = new ItemsList;
		m_Shutdown = false;

		int ret = pthread_create(&m_WorkerThread, NULL, &RunThread, this);
		ENSURE(ret == 0);
	}

	~CSoundManagerWorker()
	{
		delete m_Items;
		CleanupItems();
		delete m_DeadItems;
	}

	bool Shutdown()
	{
		{
			CScopeLock lock(m_WorkerMutex);

			m_Shutdown = true;

			ItemsList::iterator lstr = m_Items->begin();
			while (lstr != m_Items->end())
			{
				delete *lstr;
				++lstr;
			}

		}

		pthread_join(m_WorkerThread, NULL);

		return true;
	}

	void addItem(ISoundItem* anItem)
	{
		CScopeLock lock(m_WorkerMutex);
		m_Items->push_back(anItem);
	}

	void CleanupItems()
	{
		CScopeLock lock(m_DeadItemsMutex);
		AL_CHECK
		ItemsList::iterator deadItems = m_DeadItems->begin();
		while (deadItems != m_DeadItems->end())
		{
			delete *deadItems;
			++deadItems;

			AL_CHECK
		}
		m_DeadItems->clear();
	}

private:
	static void* RunThread(void* data)
	{
		debug_SetThreadName("CSoundManagerWorker");
		g_Profiler2.RegisterCurrentThread("soundmanager");

		static_cast<CSoundManagerWorker*>(data)->Run();

		return NULL;
	}

	void Run()
	{
		while (true)
		{
			// Handle shutdown requests as soon as possible
			if (GetShutdown())
				return;

			int pauseTime = 500;
			if (g_SoundManager->InDistress())
				pauseTime = 50;

			{
				CScopeLock lock(m_WorkerMutex);
		
				ItemsList::iterator lstr = m_Items->begin();
				ItemsList* nextItemList = new ItemsList;

				while (lstr != m_Items->end())
				{
					AL_CHECK
					if ((*lstr)->IdleTask())
					{
						if ((pauseTime == 500) && (*lstr)->IsFading())
							pauseTime = 100;

						nextItemList->push_back(*lstr);
					}
					else
					{
						CScopeLock lock(m_DeadItemsMutex);
						m_DeadItems->push_back(*lstr);
					}
					++lstr;

					AL_CHECK
				}

				delete m_Items;
				m_Items = nextItemList;

				AL_CHECK
			}
			SDL_Delay(pauseTime);
		}
	}

	bool GetShutdown()
	{
		CScopeLock lock(m_WorkerMutex);
		return m_Shutdown;
	}

private:
	// Thread-related members:
	pthread_t m_WorkerThread;
	CMutex m_WorkerMutex;
	CMutex m_DeadItemsMutex;

	// Shared by main thread and worker thread:
	// These variables are all protected by a mutexes
	ItemsList* m_Items;
	ItemsList* m_DeadItems;

	bool m_Shutdown;

	CSoundManagerWorker(ISoundManager* UNUSED(other)){};
};

void ISoundManager::CreateSoundManager()
{
	if (!g_SoundManager)
	{
		g_SoundManager = new CSoundManager();
		g_SoundManager->StartWorker();
	}
}

void ISoundManager::SetEnabled(bool doEnable)
{
	if (g_SoundManager && !doEnable) 
		SAFE_DELETE(g_SoundManager);
	else if (!g_SoundManager && doEnable) 
		ISoundManager::CreateSoundManager();
}
void ISoundManager::CloseGame()
{
	if (CSoundManager* aSndMgr = (CSoundManager*)g_SoundManager)
		aSndMgr->SetAmbientItem(NULL);
}

void CSoundManager::al_ReportError(ALenum err, const char* caller, int line)
{
	LOGERROR(L"OpenAL error: %hs; called from %hs (line %d)\n", alGetString(err), caller, line);
}

void CSoundManager::al_check(const char* caller, int line)
{
	ALenum err = alGetError();
	if (err != AL_NO_ERROR)
		al_ReportError(err, caller, line);
}

Status CSoundManager::ReloadChangedFiles(const VfsPath& UNUSED(path))
{
//	LOGERROR(L"GUI file '%ls' changed - reloading page", path.string().c_str());

	return INFO::OK;
}

/*static*/ Status CSoundManager::ReloadChangedFileCB(void* param, const VfsPath& path)
{
	return static_cast<CSoundManager*>(param)->ReloadChangedFiles(path);
}

CSoundManager::CSoundManager()
{
	m_CurrentEnvirons = NULL;
	m_ALSourceBuffer = NULL;
	m_Device = NULL;
	m_Context = NULL;
	m_Worker = NULL;
	m_PlayListItems	= NULL;
	m_CurrentTune = NULL;
	m_Gain = 1;
	m_MusicGain = 1;
	m_AmbientGain = 1;
	m_ActionGain = 1;
	m_UIGain = 1;
	m_BufferCount = 50;
	m_BufferSize = 98304;
	m_SoundEnabled = true;
	m_MusicEnabled = true;
	m_MusicPaused = false;
	m_AmbientPaused = false;
	m_ActionPaused = false;

	m_DistressTime = 0;
	m_DistressErrCount = 0;

	m_PlayingPlaylist = false;
	m_LoopingPlaylist = false;
	m_RunningPlaylist = false;
	m_PlaylistGap = 0;

	m_Enabled = false;
	AlcInit();

	if (m_Enabled)
	{
		InitListener();

		m_PlayListItems = new PlayList;
	}

	RegisterFileReloadFunc(ReloadChangedFileCB, this);
}

CSoundManager::~CSoundManager()
{
	UnregisterFileReloadFunc(ReloadChangedFileCB, this);

	if (m_Worker)
	{
		AL_CHECK
		m_Worker->Shutdown();
		AL_CHECK
		m_Worker->CleanupItems();
		AL_CHECK

		delete m_Worker;
	}
	AL_CHECK

	for (std::map<std::wstring, CSoundGroup*>::iterator it = m_SoundGroups.begin(); it != m_SoundGroups.end(); ++it)
		delete it->second;
	m_SoundGroups.clear();

	if (m_PlayListItems)
		delete m_PlayListItems;

	if (m_ALSourceBuffer != NULL)
		delete[] m_ALSourceBuffer;

	if (m_Context)
		alcDestroyContext(m_Context);
	
	if (m_Device)
		alcCloseDevice(m_Device);
}

void CSoundManager::StartWorker()
{
	if (m_Enabled)
		m_Worker = new CSoundManagerWorker();
}

Status CSoundManager::AlcInit()
{	
	Status ret = INFO::OK;

	m_Device = alcOpenDevice(NULL);
	if (m_Device)
	{
		ALCint attribs[] = {ALC_STEREO_SOURCES, 16, 0};
		m_Context = alcCreateContext(m_Device, &attribs[0]);

		if (m_Context)
		{
			alcMakeContextCurrent(m_Context);
			m_ALSourceBuffer = new ALSourceHolder[SOURCE_NUM];
			ALuint* sourceList = new ALuint[SOURCE_NUM];

			alGenSources(SOURCE_NUM, sourceList);
			ALCenum err = alcGetError(m_Device);

			if (err == ALC_NO_ERROR)
			{
				for (int x = 0; x < SOURCE_NUM; x++)
				{
					m_ALSourceBuffer[x].ALSource = sourceList[x];
					m_ALSourceBuffer[x].SourceItem = NULL;
				}
				m_Enabled = true;
			}
			else
			{
				LOGERROR(L"error in gensource = %d", err);
			}
			delete[] sourceList;
		}
	}

	// check if init succeeded.
	// some OpenAL implementations don't indicate failure here correctly;
	// we need to check if the device and context pointers are actually valid.
	ALCenum err = alcGetError(m_Device);
	const char* dev_name = (const char*)alcGetString(m_Device, ALC_DEVICE_SPECIFIER);

	if (err == ALC_NO_ERROR && m_Device && m_Context)
		debug_printf(L"Sound: AlcInit success, using %hs\n", dev_name);
	else
	{
		LOGERROR(L"Sound: AlcInit failed, m_Device=%p m_Context=%p dev_name=%hs err=%x\n", m_Device, m_Context, dev_name, err);

// FIXME Hack to get around exclusive access to the sound device
#if OS_UNIX
		ret = INFO::OK;
#else
		ret = ERR::FAIL;
#endif // !OS_UNIX
	}

	return ret;
}

bool CSoundManager::InDistress()
{
	CScopeLock lock(m_DistressMutex);

	if (m_DistressTime == 0)
		return false;
	else if ((timer_Time() - m_DistressTime) > 10)
	{
		m_DistressTime = 0;
// Coming out of distress mode
		m_DistressErrCount = 0;
		return false;
	}

	return true;
}

void CSoundManager::SetDistressThroughShortage()
{
	CScopeLock lock(m_DistressMutex);

// Going into distress for normal reasons
	
	m_DistressTime = timer_Time();
}

void CSoundManager::SetDistressThroughError()
{
	CScopeLock lock(m_DistressMutex);

// Going into distress due to unknown error
	
	m_DistressTime = timer_Time();
	m_DistressErrCount++;
}



ALuint CSoundManager::GetALSource(ISoundItem* anItem)
{
	for (int x = 0; x < SOURCE_NUM; x++)
	{
		if (!m_ALSourceBuffer[x].SourceItem)
		{
			m_ALSourceBuffer[x].SourceItem = anItem;
			return m_ALSourceBuffer[x].ALSource;
		}
	}
	SetDistressThroughShortage();
	return 0;
}

void CSoundManager::ReleaseALSource(ALuint theSource)
{
	for (int x = 0; x < SOURCE_NUM; x++)
	{
		if (m_ALSourceBuffer[x].ALSource == theSource)
		{
			m_ALSourceBuffer[x].SourceItem = NULL;
			return;
		}
	}
}

long CSoundManager::GetBufferCount()
{
	return m_BufferCount;
}
long CSoundManager::GetBufferSize()
{
	return m_BufferSize;
}

void CSoundManager::AddPlayListItem(const VfsPath& itemPath)
{
	if (m_Enabled)
		m_PlayListItems->push_back(itemPath);
}

void CSoundManager::ClearPlayListItems()
{
	if (m_Enabled)
	{
		if (m_PlayingPlaylist)
			SetMusicItem(NULL);

		m_PlayingPlaylist = false;
		m_LoopingPlaylist = false;
		m_RunningPlaylist = false;

		m_PlayListItems->clear();
	}
}

void CSoundManager::StartPlayList(bool doLoop)
{
	if (m_Enabled && m_MusicEnabled)
	{
		if (m_PlayListItems->size() > 0)
		{
			m_PlayingPlaylist = true;
			m_LoopingPlaylist = doLoop;
			m_RunningPlaylist = false;

			ISoundItem* aSnd = LoadItem((m_PlayListItems->at(0)));
			if (aSnd)
				SetMusicItem(aSnd);
			else
				SetMusicItem(NULL);
		}
	}
}

void CSoundManager::SetMasterGain(float gain)
{
	if (m_Enabled)
	{
		m_Gain = gain;
		alListenerf(AL_GAIN, m_Gain);
		AL_CHECK
	}
}

void CSoundManager::SetMusicGain(float gain)
{
	m_MusicGain = gain;
}
void CSoundManager::SetAmbientGain(float gain)
{
	m_AmbientGain = gain;
}
void CSoundManager::SetActionGain(float gain)
{
	m_ActionGain = gain;
}
void CSoundManager::SetUIGain(float gain)
{
	m_UIGain = gain;
}


ISoundItem* CSoundManager::LoadItem(const VfsPath& itemPath)
{	
	AL_CHECK

	if (m_Enabled)
	{
		CSoundData* itemData = CSoundData::SoundDataFromFile(itemPath);

		AL_CHECK
		if (itemData)
			return CSoundManager::ItemForData(itemData);
	}

	return NULL;
}

ISoundItem* CSoundManager::ItemForData(CSoundData* itemData)
{	
	AL_CHECK
	ISoundItem* answer = NULL;

	AL_CHECK
	
	if (m_Enabled && (itemData != NULL))
	{
		if (itemData->IsOneShot())
		{
			if (itemData->GetBufferCount() == 1)
				answer = new CSoundItem(itemData);
			else
				answer = new CBufferItem(itemData);
		}
		else
		{
			answer = new CStreamItem(itemData);
		}

		if (answer && m_Worker)
			m_Worker->addItem(answer);
	}

	return answer;
}

void CSoundManager::IdleTask()
{
	if (m_Enabled)
	{
		if (m_CurrentTune)
		{
			m_CurrentTune->EnsurePlay();
			if (m_PlayingPlaylist && m_RunningPlaylist)
			{
				if (m_CurrentTune->Finished())
				{
					if (m_PlaylistGap == 0)
					{
						m_PlaylistGap = timer_Time() + 15;
					}
					else if (m_PlaylistGap < timer_Time())
					{
						m_PlaylistGap = 0;
						PlayList::iterator it = find(m_PlayListItems->begin(), m_PlayListItems->end(), *(m_CurrentTune->GetName()));
						if (it != m_PlayListItems->end())
						{
							++it;

							Path nextPath;
							if (it == m_PlayListItems->end())
								nextPath = m_PlayListItems->at(0);
							else
								nextPath = *it;

							ISoundItem* aSnd = LoadItem(nextPath);
							if (aSnd)
								SetMusicItem(aSnd);
						}
					}
				}
			}
		}

		if (m_CurrentEnvirons)
			m_CurrentEnvirons->EnsurePlay();

		if (m_Worker)
			m_Worker->CleanupItems();
	}
}

ISoundItem*	CSoundManager::ItemForEntity(entity_id_t UNUSED(source), CSoundData* sndData)
{
	ISoundItem* currentItem = NULL;

	if (m_Enabled)
		currentItem = ItemForData(sndData);

	return currentItem;
}


void CSoundManager::InitListener()
{
	ALfloat listenerPos[] = {0.0, 0.0, 0.0};
	ALfloat listenerVel[] = {0.0, 0.0, 0.0};
	ALfloat	listenerOri[] = {0.0, 0.0, -1.0, 0.0, 1.0, 0.0};

	alListenerfv(AL_POSITION, listenerPos);
	alListenerfv(AL_VELOCITY, listenerVel);
	alListenerfv(AL_ORIENTATION, listenerOri);

	alDistanceModel(AL_LINEAR_DISTANCE);
}

void CSoundManager::PlayGroupItem(ISoundItem* anItem, ALfloat groupGain)
{
	if (anItem)
	{
		if (m_Enabled && (m_ActionGain > 0))
		{
			anItem->SetGain(m_ActionGain * groupGain);
			anItem->PlayAndDelete();
			AL_CHECK
		}
	}
}

void CSoundManager::SetMusicEnabled(bool isEnabled)
{
	if (m_CurrentTune && !isEnabled)
	{
		m_CurrentTune->FadeAndDelete(1.00);
		m_CurrentTune = NULL;
	}
	m_MusicEnabled = isEnabled;
}

void CSoundManager::PlayAsGroup(const VfsPath& groupPath, CVector3D sourcePos, entity_id_t source, bool ownedSound)
{
	// Make sure the sound group is loaded
	CSoundGroup* group;
	if (m_SoundGroups.find(groupPath.string()) == m_SoundGroups.end())
	{
		group = new CSoundGroup();
		if (!group->LoadSoundGroup(L"audio/" + groupPath.string()))
		{
			LOGERROR(L"Failed to load sound group '%ls'", groupPath.string().c_str());
			delete group;
			group = NULL;
		}
		// Cache the sound group (or the null, if it failed)
		m_SoundGroups[groupPath.string()] = group;
	}
	else
	{
		group = m_SoundGroups[groupPath.string()];
	}

	// Failed to load group -> do nothing
	if (group && (ownedSound || !group->TestFlag(eOwnerOnly)))
		group->PlayNext(sourcePos, source);
}

void CSoundManager::PlayAsMusic(const VfsPath& itemPath, bool looping)
{
	if (m_Enabled)
	{
		UNUSED2(looping);

		ISoundItem* aSnd = LoadItem(itemPath);
		if (aSnd != NULL)
			SetMusicItem(aSnd);
	}
}

void CSoundManager::PlayAsAmbient(const VfsPath& itemPath, bool looping)
{
	if (m_Enabled)
	{
		UNUSED2(looping);
		ISoundItem* aSnd = LoadItem(itemPath);
		if (aSnd != NULL)
			SetAmbientItem(aSnd);
	}
}


void CSoundManager::PlayAsUI(const VfsPath& itemPath, bool looping)
{
	if (m_Enabled)
	{
		IdleTask();
	
		if (ISoundItem* anItem = LoadItem(itemPath))
		{
			if (m_UIGain > 0)
			{
				anItem->SetGain(m_UIGain);
				anItem->SetLooping(looping);
				anItem->PlayAndDelete();
			}
		}
		AL_CHECK
	}
}

void CSoundManager::Pause(bool pauseIt)
{
	PauseMusic(pauseIt);
	PauseAmbient(pauseIt);
	PauseAction(pauseIt);
}

void CSoundManager::PauseMusic(bool pauseIt)
{
	if (m_CurrentTune && pauseIt && !m_MusicPaused)
	{
		m_CurrentTune->FadeAndPause(1.0);
	}
	else if (m_CurrentTune && m_MusicPaused && !pauseIt && m_MusicEnabled)
	{
		m_CurrentTune->SetGain(0);
		m_CurrentTune->Resume();
		m_CurrentTune->FadeToIn(m_MusicGain, 1.0);
	}
	m_MusicPaused = pauseIt;
}

void CSoundManager::PauseAmbient(bool pauseIt)
{
	if (m_CurrentEnvirons && pauseIt)
		m_CurrentEnvirons->Pause();
	else if (m_CurrentEnvirons)
		m_CurrentEnvirons->Resume();

	m_AmbientPaused = pauseIt;
}

void CSoundManager::PauseAction(bool pauseIt)
{
	m_ActionPaused = pauseIt;
}

void CSoundManager::SetMusicItem(ISoundItem* anItem)
{
	if (m_Enabled)
	{
		AL_CHECK
		if (m_CurrentTune)
		{
			m_CurrentTune->FadeAndDelete(2.00);
			m_CurrentTune = NULL;
		}

		IdleTask();

		if (anItem)
		{
			if (m_MusicEnabled)
			{
				m_CurrentTune = anItem;
				m_CurrentTune->SetGain(0);

				if (m_PlayingPlaylist)
				{
					m_RunningPlaylist = true;
					m_CurrentTune->Play();
				}
				else
					m_CurrentTune->PlayLoop();

				m_MusicPaused = false;
				m_CurrentTune->FadeToIn(m_MusicGain, 1.00);
			}
			else
			{
				anItem->StopAndDelete();
			}
		}
		AL_CHECK
	}
}

void CSoundManager::SetAmbientItem(ISoundItem* anItem)
{
	if (m_Enabled)
	{
		if (m_CurrentEnvirons)
		{
			m_CurrentEnvirons->FadeAndDelete(3.00);
			m_CurrentEnvirons = NULL;
		}
		IdleTask();
	
		if (anItem)
		{
			if (m_AmbientGain > 0)
			{
				m_CurrentEnvirons = anItem;
				m_CurrentEnvirons->SetGain(0);
				m_CurrentEnvirons->PlayLoop();
				m_CurrentEnvirons->FadeToIn(m_AmbientGain, 2.00);
			}
		}
		AL_CHECK
	}
}
#else // CONFIG2_AUDIO

void ISoundManager::CreateSoundManager(){}
void ISoundManager::SetEnabled(bool UNUSED(doEnable)){}
void ISoundManager::CloseGame(){}

#endif // CONFIG2_AUDIO

