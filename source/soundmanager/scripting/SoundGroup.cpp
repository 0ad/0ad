/* Copyright (C) 2018 Wildfire Games.
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

/**
* =========================================================================
* File		  : SoundGroup.cpp
* Project	  : 0 A.D.
* Description : Loads up a group of sound files with shared properties,
*				and provides a simple interface for playing them.
* =========================================================================
*/

#include "precompiled.h"

#include "SoundGroup.h"
#include "graphics/Camera.h"
#include "graphics/GameView.h"
#include "lib/rand.h"
#include "ps/Game.h"
#include "ps/CLogger.h"
#include "ps/CStr.h"
#include "ps/Filesystem.h"
#include "ps/Util.h"
#include "ps/XML/Xeromyces.h"
#include "soundmanager/items/ISoundItem.h"
#include "soundmanager/SoundManager.h"

#include <algorithm>


extern CGame *g_Game;

#define PI 3.14126f


static const bool DISABLE_INTENSITY = true; // disable for now since it's broken

void CSoundGroup::SetGain(float gain)
{
	gain = std::min(gain, 1.0f);
	m_Gain = gain;
}

void CSoundGroup::SetDefaultValues()
{
	m_index = 0;
	m_Flags = 0;
	m_Intensity = 0;
	m_CurTime = 0.0f;

	// sane defaults; will probably be replaced by the values read during LoadSoundGroup.
	SetGain(0.7f);
	m_Pitch = 1.0f;
	m_Priority = 60;
	m_PitchUpper = 1.1f;
	m_PitchLower = 0.9f;
	m_GainUpper = 1.0f;
	m_GainLower = 0.8f;
	m_ConeOuterGain = 0.0f;
	m_ConeInnerAngle = 360.0f;
	m_ConeOuterAngle = 360.0f;
	m_Decay = 3.0f;
	m_IntensityThreshold = 3;
	// WARNING: m_TimeWindow is currently unused and uninitialized
}

CSoundGroup::CSoundGroup()
{
	SetDefaultValues();
}

CSoundGroup::CSoundGroup(const VfsPath& pathnameXML)
{
	SetDefaultValues();
	LoadSoundGroup(pathnameXML);
}

CSoundGroup::~CSoundGroup()
{
	// clean up all the handles from this group.
	ReleaseGroup();
}

#if CONFIG2_AUDIO
static float RandFloat(float min, float max)
{
	return float(rand(min*100.0f, max*100.0f) / 100.0f);
}
#endif // CONFIG2_AUDIO

float CSoundGroup::RadiansOffCenter(const CVector3D& position, bool& onScreen, float& itemRollOff)
{
	float x, y;
	float answer = 0.0;
	const size_t screenWidth = g_Game->GetView()->GetCamera()->GetViewPort().m_Width;
	const size_t screenHeight = g_Game->GetView()->GetCamera()->GetViewPort().m_Height;
	float bufferSize = screenWidth * 0.10;
	float yBufferSize = 15;
	const size_t audioWidth = screenWidth;
	float radianCap = PI / 3;

	g_Game->GetView()->GetCamera()->GetScreenCoordinates(position, x, y);

	onScreen = true;

	if (x < -bufferSize)
	{
		onScreen = false;
		answer = -radianCap;
	}
	else if (x > screenWidth + bufferSize)
	{
		onScreen = false;
		answer = radianCap;
	}
	else
	{
		if ((x < 0) || (x > screenWidth))
		{
			itemRollOff = 0.5;
		}
		float pixPerRadian = audioWidth / (radianCap * 2);
		answer = (x - (screenWidth/2)) / pixPerRadian;
	}

	if (y < -yBufferSize)
	{
		onScreen = false;
	}
	else if (y > screenHeight + yBufferSize)
	{
		onScreen = false;
	}
	else if ((y < 0) || (y > screenHeight))
	{
		itemRollOff = 0.5;
	}

	return answer;
}

void CSoundGroup::UploadPropertiesAndPlay(size_t theIndex, const CVector3D& position, entity_id_t source)
{
#if CONFIG2_AUDIO
	if (!g_SoundManager)
		return;

	bool isOnscreen = false;
	ALfloat itemRollOff = 0.1f;
	float offset = RadiansOffCenter(position, isOnscreen, itemRollOff);
	
	if (!isOnscreen && !TestFlag(eDistanceless) && !TestFlag(eOmnipresent))
		return;

	if (snd_group.size() == 0)
		Reload();

	if (snd_group.size() <= theIndex)
		return;

	CSoundData* sndData = snd_group[theIndex];
	if (sndData == nullptr)
		return;

	ISoundItem* hSound = static_cast<CSoundManager*>(g_SoundManager)->ItemForEntity(source, sndData);
	if (hSound == nullptr)
		return;

	if (!TestFlag(eOmnipresent))
	{
		CVector3D origin = g_Game->GetView()->GetCamera()->GetOrientation().GetTranslation();
		float sndDist = origin.Y;
		float itemDist = (position - origin).Length();

		if ((sndDist * 2) < itemDist)
			sndDist = itemDist;

		if (TestFlag(eDistanceless))
			itemRollOff = 0;

		if (sndData->IsStereo())
			LOGWARNING("OpenAL: stereo sounds can't be positioned: %s", sndData->GetFileName().string8());

		hSound->SetLocation(CVector3D((sndDist * sin(offset)), 0, -sndDist * cos(offset)));
		hSound->SetRollOff(itemRollOff);
	}

	if (TestFlag(eRandPitch))
		hSound->SetPitch(RandFloat(m_PitchLower, m_PitchUpper));
	else
		hSound->SetPitch(m_Pitch);

	if (TestFlag(eRandGain))
		m_Gain = RandFloat(m_GainLower, m_GainUpper);

	hSound->SetCone(m_ConeInnerAngle, m_ConeOuterAngle, m_ConeOuterGain);
	static_cast<CSoundManager*>(g_SoundManager)->PlayGroupItem(hSound, m_Gain);

#else // !CONFIG2_AUDIO
	UNUSED2(theIndex);
	UNUSED2(position);
	UNUSED2(source);
#endif // !CONFIG2_AUDIO
}


static void HandleError(const CStrW& message, const VfsPath& pathname, Status err)
{
	if (err == ERR::AGAIN)
		return;	// open failed because sound is disabled (don't log this)
	LOGERROR("%s: pathname=%s, error=%s", utf8_from_wstring(message), pathname.string8(), utf8_from_wstring(ErrorString(err)));
}

void CSoundGroup::PlayNext(const CVector3D& position, entity_id_t source)
{
	// if no sounds, return
	if (filenames.size() == 0)
		return;

	m_index = rand(0, (size_t)filenames.size());
	UploadPropertiesAndPlay(m_index, position, source);
}

void CSoundGroup::Reload()
{
	m_index = 0; // reset our index

#if CONFIG2_AUDIO
	ReleaseGroup();

	if ( g_SoundManager ) {
		for (size_t i = 0; i < filenames.size(); i++)
		{
			VfsPath  thePath =  m_filepath/filenames[i];
			CSoundData* itemData = CSoundData::SoundDataFromFile(thePath);

			if (itemData == NULL)
				HandleError(L"error loading sound", thePath, ERR::FAIL);
			else
				snd_group.push_back(itemData->IncrementCount());
		}

		if (TestFlag(eRandOrder))
			random_shuffle(snd_group.begin(), snd_group.end());
	}
#endif // CONFIG2_AUDIO
}

void CSoundGroup::ReleaseGroup()
{
#if CONFIG2_AUDIO
	for (size_t i = 0; i < snd_group.size(); i++)
	{
		CSoundData::ReleaseSoundData( snd_group[i] );
	}
	snd_group.clear();
#endif // CONFIG2_AUDIO
}

void CSoundGroup::Update(float UNUSED(TimeSinceLastFrame))
{
}

bool CSoundGroup::LoadSoundGroup(const VfsPath& pathnameXML)
{
	CXeromyces XeroFile;
	if (XeroFile.Load(g_VFS, pathnameXML, "sound_group") != PSRETURN_OK)
	{
		HandleError(L"error loading file", pathnameXML, ERR::FAIL);
		return false;
	}
	// Define elements used in XML file
	#define EL(x) int el_##x = XeroFile.GetElementID(#x)
	#define AT(x) int at_##x = XeroFile.GetAttributeID(#x)
	EL(soundgroup);
	EL(gain);
	EL(looping);
	EL(omnipresent);
	EL(heardby);
	EL(distanceless);
	EL(pitch);
	EL(priority);
	EL(randorder);
	EL(randgain);
	EL(randpitch);
	EL(conegain);
	EL(coneinner);
	EL(coneouter);
	EL(sound);
	EL(gainupper);
	EL(gainlower);
	EL(pitchupper);
	EL(pitchlower);
	EL(path);
	EL(threshold);
	EL(decay);
	#undef AT
	#undef EL

	XMBElement root = XeroFile.GetRoot();

	if (root.GetNodeName() != el_soundgroup)
	{
		LOGERROR("Invalid SoundGroup format (unrecognised root element '%s')", XeroFile.GetElementString(root.GetNodeName()).c_str());
		return false;
	}

	XERO_ITER_EL(root, child)
	{
		int child_name = child.GetNodeName();

		if(child_name == el_gain)
		{
			SetGain(child.GetText().ToFloat());
		}
		else if(child_name == el_looping)
		{
			if(child.GetText().ToInt() == 1)
				SetFlag(eLoop);
		}
		else if(child_name == el_omnipresent)
		{
			if(child.GetText().ToInt() == 1)
				SetFlag(eOmnipresent);
		}
		else if(child_name == el_heardby)
		{
			if(child.GetText().FindInsensitive( "owner" ) == 0 )
				SetFlag(eOwnerOnly);
		}
		else if(child_name == el_distanceless)
		{
			if(child.GetText().ToInt() == 1)
				SetFlag(eDistanceless);
		}
		else if(child_name == el_pitch)
		{
			this->m_Pitch = child.GetText().ToFloat();
		}
		else if(child_name == el_priority)
		{
			this->m_Priority = child.GetText().ToFloat();
		}
		else if(child_name == el_randorder)
		{
			if(child.GetText().ToInt() == 1)
				SetFlag(eRandOrder);
		}
		else if(child_name == el_randgain)
		{
			if(child.GetText().ToInt() == 1)
				SetFlag(eRandGain);
		}
		else if(child_name == el_gainupper)
		{
			this->m_GainUpper = child.GetText().ToFloat();
		}
		else if(child_name == el_gainlower)
		{
			this->m_GainLower = child.GetText().ToFloat();
		}
		else if(child_name == el_randpitch)
		{
			if(child.GetText().ToInt() == 1)
				SetFlag(eRandPitch);
		}
		else if(child_name == el_pitchupper)
		{
			this->m_PitchUpper = child.GetText().ToFloat();
		}
		else if(child_name == el_pitchlower)
		{
			this->m_PitchLower = child.GetText().ToFloat();
		}
		else if(child_name == el_conegain)
		{
			this->m_ConeOuterGain = child.GetText().ToFloat();
		}
		else if(child_name == el_coneinner)
		{
			this->m_ConeInnerAngle = child.GetText().ToFloat();
		}
		else if(child_name == el_coneouter)
		{
			this->m_ConeOuterAngle = child.GetText().ToFloat();
		}
		else if(child_name == el_sound)
		{
			this->filenames.push_back(child.GetText().FromUTF8());
		}
		else if(child_name == el_path)
		{
			m_filepath = child.GetText().FromUTF8();
		}
		else if(child_name == el_threshold)
		{
			m_IntensityThreshold = child.GetText().ToFloat();
		}
		else if(child_name == el_decay)
		{
			m_Decay = child.GetText().ToFloat();
		}
	}
	return true;
}

