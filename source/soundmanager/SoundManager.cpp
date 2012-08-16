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

CSoundManager* g_SoundManager;

void CSoundManager::ScriptingInit()
{
	JAmbientSound::ScriptingInit();
	JMusicSound::ScriptingInit();
	JSound::ScriptingInit();
	JSoundPlayer::ScriptingInit();
}

CSoundManager::CSoundManager()
{
	m_Items = new ItemsList;
	m_CurrentEnvirons	= 0;
	m_CurrentTune		= 0;
	m_Gain				= 1;
	m_MusicGain			= 1;
	m_AmbientGain		= 1;
	m_ActionGain		= 1;
	m_Enabled			= true;
	m_BufferCount		= 50;
	m_BufferSize		= 65536;
	m_MusicEnabled		= true;
	AlcInit();
}

CSoundManager::~CSoundManager()
{	
	ItemsList::iterator lstr = m_Items->begin();
	while (lstr != m_Items->end())
	{
		(*lstr)->Stop();
		delete *lstr;
		lstr++;
	}

	alcDestroyContext(m_Context);
	alcCloseDevice(m_Device);

	delete m_Items;
	m_Items = 0L;
	m_CurrentEnvirons = 0;
	m_CurrentTune = 0;
}


Status CSoundManager::AlcInit()
{	
	Status ret = INFO::OK;

	m_Device = alcOpenDevice(NULL);
	if(m_Device)
	{
		m_Context = alcCreateContext(m_Device, 0);	// no attrlist needed
		if(m_Context)
			alcMakeContextCurrent(m_Context);
	}

	const char* dev_name = (const char*)alcGetString(m_Device, ALC_DEVICE_SPECIFIER);

	// check if init succeeded.
	// some OpenAL implementations don't indicate failure here correctly;
	// we need to check if the device and context pointers are actually valid.
	ALCenum err = alcGetError(m_Device);
	if(err == ALC_NO_ERROR && m_Device && m_Context)
		debug_printf(L"Sound: AlcInit success, using %hs\n", dev_name);
	else
	{
		debug_printf(L"Sound: AlcInit failed, m_Device=%p m_Context=%p dev_name=%hs err=%d\n", m_Device, m_Context, dev_name, err);
// FIXME Hack to get around exclusive access to the sound device
#if OS_UNIX
		ret = INFO::OK;
#else
		ret = ERR::FAIL;
#endif
	}

	return ret;
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
	CSoundData* itemData = CSoundData::SoundDataFromFile(itemPath);
	ISoundItem* answer = NULL;
	
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

		if (answer != NULL)
			m_Items->push_back(answer);
	}

	
	return answer;
}

unsigned long CSoundManager::Count()
{
	return m_Items->size();
}

void CSoundManager::IdleTask()
{
	if (m_Items)
	{
		ItemsList::iterator lstr = m_Items->begin();
		ItemsList  deadItemList;
		ItemsList* nextItemList = new ItemsList;


		while (lstr != m_Items->end()) {
			if ((*lstr)->IdleTask())
				nextItemList->push_back(*lstr);
			else
				deadItemList.push_back(*lstr);
			lstr++;
		}
		delete m_Items;
		m_Items = nextItemList;

		ItemsList::iterator deadItems = deadItemList.begin();
		while (deadItems != deadItemList.end())
		{   
			delete *deadItems;
			deadItems++;
		}
	}
	if (m_CurrentTune)
		m_CurrentTune->EnsurePlay();
	if (m_CurrentEnvirons)
		m_CurrentEnvirons->EnsurePlay();
}

void CSoundManager::DeleteItem(long itemNum)
{
	ItemsList::iterator lstr = m_Items->begin();
	lstr += itemNum;
	
	delete *lstr;
	
	m_Items->erase(lstr);
}

ISoundItem* CSoundManager::GetSoundItem(unsigned long itemRow)
{
   return (*m_Items)[itemRow];
}

void CSoundManager::InitListener()
{
	ALfloat listenerPos[]={0.0,0.0,0.0};
	ALfloat listenerVel[]={0.0,0.0,0.0};
	ALfloat	listenerOri[]={0.0,0.0,-1.0, 0.0,1.0,0.0};

	alListenerfv(AL_POSITION,listenerPos);
	alListenerfv(AL_VELOCITY,listenerVel);
	alListenerfv(AL_ORIENTATION,listenerOri);

	alDistanceModel(AL_EXPONENT_DISTANCE);
}

void CSoundManager::SetEnabled(bool doEnable)
{
	m_Enabled = doEnable;
}

void CSoundManager::PlayActionItem(ISoundItem* anItem)
{
	if (anItem)
	{
		if (m_Enabled && (m_ActionGain > 0))
		{
			anItem->SetGain(m_Gain * m_ActionGain);
			anItem->Play();
		}
	}
}
void CSoundManager::PlayGroupItem(ISoundItem* anItem, ALfloat groupGain)
{
	if (anItem)
	{
		if (m_Enabled && (m_ActionGain > 0)) {
			anItem->SetGain(m_Gain * groupGain);
			anItem->Play();
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
	if (m_CurrentTune)
	{
		m_CurrentTune->FadeAndDelete(3.00);
		m_CurrentTune = 0L;
	}
	IdleTask();
	if (anItem)
	{
		if (m_MusicEnabled && m_Enabled)
		{
			m_CurrentTune = anItem;
			m_CurrentTune->SetGain(0);
			m_CurrentTune->PlayLoop();
			m_CurrentTune->FadeToIn(m_Gain * m_MusicGain, 2.00);
		}
		else
		{
			anItem->StopAndDelete();
		}
	}
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
			m_CurrentEnvirons->SetGain(0);
			m_CurrentEnvirons->PlayLoop();
			m_CurrentEnvirons->FadeToIn(m_Gain * m_AmbientGain, 2.00);
		}
	}
}

