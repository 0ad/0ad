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
#include "soundmanager/CSoundManager.h"
#include "SMSoundGroup.h"

#include <algorithm>

#include "lib/rand.h"

#include "ps/XML/Xeromyces.h"
#include "ps/CLogger.h"
#include "ps/Filesystem.h"
#include "ps/Util.h"


static const bool DISABLE_INTENSITY = true; // disable for now since it's broken

void CSMSoundGroup::SetGain(float gain)
{
	gain = std::min(gain, 1.0f);
	m_Gain = gain;
}

void CSMSoundGroup::SetDefaultValues()
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

CSMSoundGroup::CSMSoundGroup()
{
	SetDefaultValues();
}

CSMSoundGroup::CSMSoundGroup(const VfsPath& pathnameXML)
{
	SetDefaultValues();
	LoadSoundGroup(pathnameXML);
}

CSMSoundGroup::~CSMSoundGroup()
{
	// clean up all the handles from this group.
	ReleaseGroup();
}

static float RandFloat(float min, float max)
{
	return float(rand(min*100.0f, max*100.0f) / 100.0f);
}

void CSMSoundGroup::UploadPropertiesAndPlay(ISoundItem* hSound, const CVector3D& position)
{
    hSound->play();
}


static void HandleError(const std::wstring& message, const VfsPath& pathname, Status err)
{
	if(err == ERR::AGAIN)
		return;	// open failed because sound is disabled (don't log this)
	LOGERROR(L"%ls: pathname=%ls, error=%ls", message.c_str(), pathname.string().c_str(), ErrorString(err));
}

void CSMSoundGroup::PlayNext(const CVector3D& position)
{
    // if no sounds, return
    if (filenames.size() == 0)
        return;
    
    m_index = (size_t)rand(0, (size_t)filenames.size());
    // (note: previously snd_group[m_index] was used in place of hs)
    
    UploadPropertiesAndPlay( snd_group[m_index], position);
}

void CSMSoundGroup::Reload()
{
    m_index = 0; // reset our index
	// get rid of the used handles
	snd_group.clear();
	// clear out the old timers

	for(size_t i = 0; i < filenames.size(); i++)
    {
        OsPath  thePath = OsPath( m_filepath/filenames[i] );
        ISoundItem* temp = g_SoundManager->loadItem( thePath );
        snd_group.push_back(temp);
    }

    if(TestFlag(eRandOrder))
        random_shuffle(snd_group.begin(), snd_group.end());
}

void CSMSoundGroup::ReleaseGroup()
{
}

void CSMSoundGroup::Update(float TimeSinceLastFrame)
{
}

bool CSMSoundGroup::LoadSoundGroup(const VfsPath& pathnameXML)
{
    LOGERROR(L"loading sound group '%ls'", pathnameXML.string().c_str());

	CXeromyces XeroFile;
	if (XeroFile.Load(g_VFS, pathnameXML) != PSRETURN_OK)
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
		LOGERROR(L"Invalid SoundGroup format (unrecognised root element '%hs')", XeroFile.GetElementString(root.GetNodeName()).c_str());
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

	Reload();
	return true;
}
