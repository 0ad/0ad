/* Copyright (C) 2010 Wildfire Games.
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
* File        : SoundGroup.cpp
* Project     : 0 A.D.
* Description : Loads up a group of sound files with shared properties, 
*				and provides a simple interface for playing them.        
* =========================================================================
*/

#include "precompiled.h"
#include "SoundGroup.h"

#include <algorithm>

#include "ps/XML/Xeromyces.h"
#include "ps/CLogger.h"
#include "lib/rand.h"

#define LOG_CATEGORY L"audio"


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
	SetGain(0.5f);
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

static float RandFloat(float min, float max)
{
	return float(rand(min*100.0f, max*100.0f) / 100.0f);
}

void CSoundGroup::UploadPropertiesAndPlay(Handle hSound, const CVector3D& position)
{
	// interface/UI sounds should always be played at the listener's
	// position, which is achieved by setting position to 0 and
	// having that treated as relative to the listener.
	float x = 0.0f, y = 0.0f, z = 0.0f;
	bool relative = true;
	if(!TestFlag(eOmnipresent))
	{
		x = position.X;
		y = position.Y;
		z = position.Z;
		relative = false;
	}

	snd_set_pos(hSound, x, y, z, relative);

	float gain = TestFlag(eRandGain)? RandFloat(m_GainLower, m_GainUpper) : m_Gain;
	gain = std::min(gain, 1.0f);	// guard against roundoff error in RandFloat or too high m_GainUpper
	snd_set_gain(hSound, gain);

	const float pitch = TestFlag(eRandPitch)? RandFloat(m_PitchLower, m_PitchUpper) : m_Pitch;
	snd_set_pitch(hSound, pitch);

	snd_play(hSound, m_Priority);
}


void CSoundGroup::PlayNext(const CVector3D& position)
{
	if(m_Intensity >= m_IntensityThreshold)
	{
		if(!is_playing(m_hReplacement))
		{
			// load up replacement file
			m_hReplacement = snd_open(m_filepath/m_intensity_file);
			if(m_hReplacement < 0)	// one cause: sound is disabled
				return;
			
			UploadPropertiesAndPlay(m_hReplacement, position);
		}
	}
	else
	{
		// if no sounds, return
		if (filenames.size() == 0)
			return;

		// try loading on the fly only when we need the sound to see if that fixes release problems...
		if(TestFlag(eRandOrder))
			m_index = (size_t)rand(0, (size_t)filenames.size());
		// (note: previously snd_group[m_index] was used in place of hs)
		Handle hs = snd_open(m_filepath/filenames[m_index]);
		if(hs < 0)	// one cause: sound is disabled
			return;

		UploadPropertiesAndPlay(hs, position);
	}
	
	playtimes.at(m_index) = 0.0f;
	m_index++;
	m_Intensity++;
	if(m_Intensity > m_IntensityThreshold)
		m_Intensity = m_IntensityThreshold;

	if(m_index >= filenames.size())
		Reload();
}

void CSoundGroup::Reload()
{
	m_index = 0; // reset our index
	// get rid of the used handles
	snd_group.clear();
	// clear out the old timers
	playtimes.clear();
	//Reload the sounds
	/*for(size_t i = 0; i < filenames.size(); i++)
	{
		string szTemp = m_filepath + filenames[i];
		Handle temp = snd_open(m_filepath + filenames[i]);
		snd_set_gain(temp, m_Gain);	
		snd_set_pitch(temp, m_Pitch);
		snd_set_cone(temp, m_ConeInnerAngle, m_ConeOuterAngle, m_ConeOuterGain);
		snd_group.push_back(temp);

	}*/
	while(playtimes.size() < filenames.size())
		playtimes.push_back(-1.0f);
	//if(TestFlag(eRandOrder))
		//random_shuffle(snd_group.begin(), snd_group.end());
}

void CSoundGroup::ReleaseGroup()
{
	for(size_t i = m_index; i<snd_group.size(); i++)
	{
		//if(!is_playing(snd_group[i]))	
			snd_free(snd_group[i]);
	}
	snd_group.clear();
	playtimes.clear();
	//if(is_playing(m_hReplacement))
	//	snd_free(m_hReplacement);
	m_index = 0;

}

void CSoundGroup::Update(float TimeSinceLastFrame)
{
	for(size_t i = 0; i < playtimes.size(); i++)
	{
		if(playtimes[i] >= 0.0f)
			playtimes[i] += TimeSinceLastFrame;

		if(playtimes[i] >= m_Decay)
		{
			playtimes[i] = -1.0f;
			m_Intensity--;
		}	
	}
}

bool CSoundGroup::LoadSoundGroup(const VfsPath& pathnameXML)
{
	CXeromyces XeroFile;
	if (XeroFile.Load(pathnameXML) != PSRETURN_OK)
		return false;

	// Define elements used in XML file
	#define EL(x) int el_##x = XeroFile.GetElementID(#x)
	#define AT(x) int at_##x = XeroFile.GetAttributeID(#x)
	EL(soundgroup);
	EL(gain);
	EL(looping);
	EL(omnipresent);
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
	EL(replacement);
	#undef AT
	#undef EL

	XMBElement root = XeroFile.GetRoot();

	if (root.GetNodeName() != el_soundgroup)
	{
		LOG(CLogger::Error, LOG_CATEGORY, L"Invalid SoundGroup format (unrecognised root element '%hs')", XeroFile.GetElementString(root.GetNodeName()).c_str());
		return false;
	}
	
	XERO_ITER_EL(root, child)
	{
	
		int child_name = child.GetNodeName();			
		
		if(child_name == el_gain)
		{
			SetGain(CStr(child.GetText()).ToFloat());
		}
		else if(child_name == el_looping)
		{
			if(CStr(child.GetText()).ToInt() == 1)
				SetFlag(eLoop);
		}
		else if(child_name == el_omnipresent)
		{
			if(CStr(child.GetText()).ToInt() == 1)
				SetFlag(eOmnipresent);
		}
		else if(child_name == el_pitch)
		{
			this->m_Pitch = CStr(child.GetText()).ToFloat();
		}
		else if(child_name == el_priority)
		{
			this->m_Priority = CStr(child.GetText()).ToFloat();
		}
		else if(child_name == el_randorder)
		{
			if(CStr(child.GetText()).ToInt() == 1)
				SetFlag(eRandOrder);
		}
		else if(child_name == el_randgain)
		{
			if(CStr(child.GetText()).ToInt() == 1)
				SetFlag(eRandGain);
		}
		else if(child_name == el_gainupper)
		{
			this->m_GainUpper = CStr(child.GetText()).ToFloat();
		}
		else if(child_name == el_gainlower)
		{
			this->m_GainLower = CStr(child.GetText()).ToFloat();
		}
		else if(child_name == el_randpitch)
		{
			if(CStr(child.GetText()).ToInt() == 1)
				SetFlag(eRandPitch);
		}
		else if(child_name == el_pitchupper)
		{
			this->m_PitchUpper = CStr(child.GetText()).ToFloat();
		}
		else if(child_name == el_pitchlower)
		{
			this->m_PitchLower = CStr(child.GetText()).ToFloat();
		}
		else if(child_name == el_conegain)
		{
			this->m_ConeOuterGain = CStr(child.GetText()).ToFloat();
		}
		else if(child_name == el_coneinner)
		{
			this->m_ConeInnerAngle = CStr(child.GetText()).ToFloat();
		}
		else if(child_name == el_coneouter)
		{
			this->m_ConeOuterAngle = CStr(child.GetText()).ToFloat();
		}
		else if(child_name == el_sound)
		{
			CStrW szTemp(child.GetText());
			this->filenames.push_back(szTemp);
		}
		else if(child_name == el_path)
		{
			m_filepath = CStrW(child.GetText());
		}
		else if(child_name == el_threshold)
		{
			m_IntensityThreshold = CStr(child.GetText()).ToFloat();
		}
		else if(child_name == el_decay)
		{
			m_Decay = CStr(child.GetText()).ToFloat();
		}
		else if(child_name == el_replacement)
		{
			m_intensity_file = CStrW(child.GetText());
		}
	}

	Reload();
	return true;
}
