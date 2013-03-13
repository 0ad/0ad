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

#include "SoundManager.h"

#include "soundmanager/data/SoundData.h"
#include "soundmanager/items/CSoundItem.h"
#include "soundmanager/items/CBufferItem.h"
#include "soundmanager/items/CStreamItem.h"
#include "soundmanager/js/SoundPlayer.h"
#include "soundmanager/js/AmbientSound.h"
#include "soundmanager/js/MusicSound.h"
#include "soundmanager/js/Sound.h"
#include "lib/external_libraries/libsdl.h"
#include "ps/CLogger.h"
#include "ps/CStr.h"
#include "ps/Profiler2.h"

CSoundManager* g_SoundManager = NULL;

#define		SOURCE_NUM		64

#if CONFIG2_AUDIO

class CSoundManagerWorker
{
public:
	CSoundManagerWorker()
	{
		m_Items = new ItemsList;
		m_DeadItems = new ItemsList;
		m_Shutdown = false;
		m_Enabled = false;

		int ret = pthread_create(&m_WorkerThread, NULL, &RunThread, this);
		ENSURE(ret == 0);
	}

	~CSoundManagerWorker()
	{

	}

	/**
	 * Called by main thread, when the online reporting is enabled/disabled.
	 */
	void SetEnabled(bool enabled)
	{
		CScopeLock lock(m_WorkerMutex);
		if (enabled != m_Enabled)
		{
			m_Enabled = enabled;
		}
	}

	/**
	 * Called by main thread to request shutdown.
	 * Returns true if we've shut down successfully.
	 * Returns false if shutdown is taking too long (we might be blocked on a
	 * sync network operation) - you mustn't destroy this object, just leak it
	 * and terminate.
	 */
	bool Shutdown()
	{
		{
			CScopeLock lock(m_WorkerMutex);
			m_Shutdown = true;
			m_Enabled = false;

			ItemsList::iterator lstr = m_Items->begin();
			while (lstr != m_Items->end())
			{
				delete *lstr;
				lstr++;
			}

		}

		pthread_join(m_WorkerThread, NULL);

		return true;
	}
	void addItem( ISoundItem* anItem )
	{
		CScopeLock lock(m_WorkerMutex);
		m_Items->push_back( anItem );
	}

	void CleanupItems()
	{
		CScopeLock lock(m_DeadItemsMutex);
		AL_CHECK
		ItemsList::iterator deadItems = m_DeadItems->begin();
		while (deadItems != m_DeadItems->end())
		{   
			delete *deadItems;
			deadItems++;

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

		// Wait until the main thread wakes us up
		while ( true )
		{
			g_Profiler2.RecordRegionLeave("semaphore wait");

			// Handle shutdown requests as soon as possible
			if (GetShutdown())
				return;

			// If we're not enabled, ignore this wakeup
			if (!GetEnabled())
				continue;

			int pauseTime = 1000;
			if ( g_SoundManager->InDistress() )
				pauseTime = 50;

			{
				CScopeLock lock(m_WorkerMutex);
		
				ItemsList::iterator lstr = m_Items->begin();
				ItemsList* nextItemList = new ItemsList;


				while (lstr != m_Items->end()) {

					AL_CHECK
					if ((*lstr)->IdleTask())
					{
						if ( (pauseTime == 1000) && (*lstr)->IsFading() )
							pauseTime = 100;

						nextItemList->push_back(*lstr);
					}
					else
					{
						CScopeLock lock(m_DeadItemsMutex);
						m_DeadItems->push_back(*lstr);
					}
					lstr++;

					AL_CHECK
				}

				delete m_Items;
				m_Items = nextItemList;

				AL_CHECK
			}
			SDL_Delay( pauseTime );
		}
	}

	bool GetEnabled()
	{
		CScopeLock lock(m_WorkerMutex);
		return m_Enabled;
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
	// These variables are all protected by m_WorkerMutex
	ItemsList* m_Items;
	ItemsList* m_DeadItems;

	bool m_Enabled;
	bool m_Shutdown;
};
#endif

void CSoundManager::ScriptingInit()
{
	JAmbientSound::ScriptingInit();
	JMusicSound::ScriptingInit();
	JSound::ScriptingInit();
	JSoundPlayer::ScriptingInit();
}


#if CONFIG2_AUDIO


void CSoundManager::CreateSoundManager()
{
	g_SoundManager = new CSoundManager();
}

void CSoundManager::SetEnabled(bool doEnable)
{
	if ( g_SoundManager && !doEnable ) 
	{
		SAFE_DELETE(g_SoundManager);
	}
	else if ( ! g_SoundManager && doEnable ) 
	{
		CSoundManager::CreateSoundManager();
	}
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

CSoundManager::CSoundManager()
{
	m_CurrentEnvirons	= 0;
	m_ALSourceBuffer	= NULL;
	m_CurrentTune		= 0;
	m_SourceCOunt		= 0;
	m_Gain				= 1;
	m_MusicGain			= 1;
	m_AmbientGain		= 1;
	m_ActionGain		= 1;
	m_BufferCount		= 50;
	m_BufferSize		= 65536;
	m_MusicEnabled		= true;
	m_DistressTime	= 0;
	m_DistressErrCount = 0;

	m_Enabled			= AlcInit() == INFO::OK;
	m_ItemsMap 			= new ItemsMap;
	InitListener();

	m_Worker = new CSoundManagerWorker();
	m_Worker->SetEnabled( true );
}

CSoundManager::~CSoundManager()
{	
	if (m_Worker->Shutdown())
		delete m_Worker;

	alcDestroyContext(m_Context);
	alcCloseDevice(m_Device);
}



Status CSoundManager::AlcInit()
{	
	Status ret = INFO::OK;

	m_Device = alcOpenDevice(NULL);
	if(m_Device)
	{
		ALCint attribs[] = {ALC_STEREO_SOURCES, 16, 0};
		m_Context = alcCreateContext(m_Device, &attribs[0]);

		if(m_Context)
		{
			alcMakeContextCurrent(m_Context);
			m_ALSourceBuffer = new ALSourceHolder[SOURCE_NUM];
			ALuint* sourceList = new ALuint[SOURCE_NUM];

			alGenSources( SOURCE_NUM, sourceList);
			ALCenum err = alcGetError(m_Device);


			if ( err == ALC_NO_ERROR )
			{
				for ( int x=0; x<SOURCE_NUM;x++)
				{
					m_ALSourceBuffer[x].ALSource 	= sourceList[x];
					m_ALSourceBuffer[x].SourceItem 	= NULL;

				}
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

	if(err == ALC_NO_ERROR && m_Device && m_Context)
		debug_printf(L"Sound: AlcInit success, using %hs\n", dev_name);
	else
	{
		LOGERROR(L"Sound: AlcInit failed, m_Device=%p m_Context=%p dev_name=%hs err=%d\n", m_Device, m_Context, dev_name, err);



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

	if ( m_DistressTime == 0 )
		return false;
	else if ( (timer_Time() - m_DistressTime) > 10 )
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



ALuint CSoundManager::GetALSource( ISoundItem* anItem)
{
	for ( int x=0; x<SOURCE_NUM;x++)
	{
		if ( ! m_ALSourceBuffer[x].SourceItem )
		{
			m_SourceCOunt++;
			m_ALSourceBuffer[x].SourceItem = anItem;
			return m_ALSourceBuffer[x].ALSource;
		}
	}
	SetDistressThroughShortage();
	return 0;
}

void CSoundManager::ReleaseALSource(ALuint theSource)
{
	for ( int x=0; x<SOURCE_NUM;x++)
	{
		if ( m_ALSourceBuffer[x].ALSource == theSource )
		{
			m_SourceCOunt--;
			m_ALSourceBuffer[x].SourceItem = NULL;
			return;
		}
	}
}

void CSoundManager::SetMemoryUsage(long bufferSize, int bufferCount)
{
	m_BufferCount = bufferCount;
	m_BufferSize = bufferSize;
}
long CSoundManager::GetBufferCount()
{
	return m_BufferCount;
}
long CSoundManager::GetBufferSize()
{
	return m_BufferSize;
}

void CSoundManager::SetMasterGain(float gain)
{
	m_Gain = gain;
	alListenerf( AL_GAIN, m_Gain);
	AL_CHECK
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


ISoundItem* CSoundManager::LoadItem(const VfsPath& itemPath)
{	
	AL_CHECK

	CSoundData* itemData = CSoundData::SoundDataFromFile(itemPath);

	AL_CHECK
	if ( itemData )
		return CSoundManager::ItemForData( itemData );

	return NULL;
}

ISoundItem* CSoundManager::ItemForData(CSoundData* itemData)
{	
	AL_CHECK
	ISoundItem* answer = NULL;

	AL_CHECK
	
	if (itemData != NULL)
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

		if ( answer && m_Worker )
			m_Worker->addItem( answer );
	}

	
	return answer;
}

void CSoundManager::IdleTask()
{
	AL_CHECK
	if (m_CurrentTune)
		m_CurrentTune->EnsurePlay();
	AL_CHECK
	if (m_CurrentEnvirons)
		m_CurrentEnvirons->EnsurePlay();
	AL_CHECK
	if (m_Worker)
		m_Worker->CleanupItems();
	AL_CHECK
}

ISoundItem*	CSoundManager::ItemForEntity( entity_id_t source, CSoundData* sndData)
{
	ISoundItem*		currentItem = NULL;
	if ( false ) 
	{
		ItemsMap::iterator		itemFound = m_ItemsMap->find( source );
		if ( itemFound != m_ItemsMap->end() )
		{
			currentItem = itemFound->second;
			if ( currentItem->CanAttach( sndData ) )
			{
				currentItem->Attach( sndData );
				LOGERROR(L"did REUSE items source = %d", m_SourceCOunt);
			}
			else
			{
				m_ItemsMap->erase( itemFound );
				currentItem->StopAndDelete();
				LOGERROR(L"item UNREUSABLE for data = %d", m_SourceCOunt);
				currentItem = NULL;
			}
		}
	}
	if ( currentItem == NULL )
	{
		currentItem = ItemForData( sndData );
		if ( currentItem )
			m_ItemsMap->insert(std::make_pair( source, currentItem));		
	}

	return currentItem;
}


void CSoundManager::InitListener()
{
	ALfloat listenerPos[]={0.0,0.0,0.0};
	ALfloat listenerVel[]={0.0,0.0,0.0};
	ALfloat	listenerOri[]={0.0,0.0,-1.0, 0.0,1.0,0.0};

	alListenerfv(AL_POSITION,listenerPos);
	alListenerfv(AL_VELOCITY,listenerVel);
	alListenerfv(AL_ORIENTATION,listenerOri);

	alDistanceModel(AL_LINEAR_DISTANCE);
}

void CSoundManager::PlayActionItem(ISoundItem* anItem)
{
	if (anItem)
	{
		if (m_Enabled && (m_ActionGain > 0))
		{
			anItem->SetGain( m_ActionGain );
			anItem->Play();
			AL_CHECK
		}
	}
}
void CSoundManager::PlayGroupItem(ISoundItem* anItem, ALfloat groupGain )
{
	if (anItem)
	{
		if (m_Enabled && (m_ActionGain > 0)) {
			anItem->SetGain(m_ActionGain * groupGain);
			anItem->PlayAndDelete();
			AL_CHECK
		}
	}
}

void CSoundManager::SetMusicEnabled (bool isEnabled)
{
	if (m_CurrentTune && !isEnabled)
	{
		m_CurrentTune->FadeAndDelete(1.00);
		m_CurrentTune = 0L;
	}
	m_MusicEnabled = isEnabled;
}

void CSoundManager::SetMusicItem(ISoundItem* anItem)
{
	AL_CHECK
	if (m_CurrentTune)
	{
		m_CurrentTune->FadeAndDelete(2.00);
		m_CurrentTune = 0L;
	}

	IdleTask();

	if (anItem)
	{
		if (m_MusicEnabled && m_Enabled)
		{
			m_CurrentTune = anItem;
			m_CurrentTune->SetIsManaged( true );
			m_CurrentTune->SetGain(0);
			m_CurrentTune->PlayLoop();
			m_CurrentTune->FadeToIn( m_MusicGain, 1.00);
		}
		else
		{
			anItem->StopAndDelete();
		}
	}
	AL_CHECK
}

void CSoundManager::SetAmbientItem(ISoundItem* anItem)
{
	if (m_CurrentEnvirons)
	{
		m_CurrentEnvirons->FadeAndDelete(3.00);
		m_CurrentEnvirons = 0L;
	}
	IdleTask();
	
	if (anItem)
	{
		if (m_Enabled && (m_AmbientGain > 0))
		{
			m_CurrentEnvirons = anItem;
			m_CurrentEnvirons->SetIsManaged( true );
			m_CurrentEnvirons->SetGain(0);
			m_CurrentEnvirons->PlayLoop();
			m_CurrentEnvirons->FadeToIn( m_AmbientGain, 2.00);
		}
	}
	AL_CHECK
}

#endif // CONFIG2_AUDIO

