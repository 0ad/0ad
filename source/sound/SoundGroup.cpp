/**
* =========================================================================
* File        : SoundGroup.cpp
* Project     : 0 A.D.
* Description : Loads up a group of sound files with shared properties, 
*				and provides a simple interface for playing them.        
*
* Author      : Gavin Fowler
* =========================================================================
*/
#include "precompiled.h"

#include "SoundGroup.h"

#include "ps/XML/Xeromyces.h"

#include "ps/CLogger.h"

#include "lib/lib.h"

#define LOG_CATEGORY "audio"

size_t CSoundGroup::m_index = 0;

CSoundGroup::CSoundGroup()
{
	m_Flags = 0;

}

CSoundGroup::CSoundGroup(const char *XMLfile)
{
	m_Flags = 0;
	LoadSoundGroup(XMLfile);
}

CSoundGroup::~CSoundGroup()
{
	// clean up all the handles from this group.
	ReleaseGroup();

}

void CSoundGroup::PlayNext()
{
	//check for randomization of pitch and gain
	if(TestFlag(eRandPitch))
		snd_set_pitch(snd_group[m_index], (float)((rand(m_PitchLower * 100.0f, m_PitchUpper * 100.0f) / 100.0f))); 
	if(TestFlag(eRandGain))
		snd_set_gain(snd_group[m_index], (float)((rand(m_GainLower * 100.0f, m_GainUpper * 100.0f) / 100.0f)));	
	
	snd_play(snd_group[m_index], m_Priority);
	m_index++;
	if(m_index >= snd_group.size())
		Reload();
}

void CSoundGroup::Reload()
{
	m_index = 0; // reset our static index
	// get rid of the used handles
	snd_group.clear();
	//Reload the sounds
	for(size_t i = 0; i < filenames.size(); i++)
	{
		Handle temp = snd_open(m_filepath + filenames[i]);
		snd_set_gain(temp, m_Gain);	
		snd_set_pitch(temp, m_Pitch);
		snd_group.push_back(temp);			
	}
	if(TestFlag(eRandOrder))
		random_shuffle(snd_group.begin(), snd_group.end());
	



}

void CSoundGroup::ReleaseGroup()
{

	for(size_t i = m_index; i<snd_group.size(); i++)
		snd_free(snd_group[i]);
	snd_group.clear();

}

bool CSoundGroup::LoadSoundGroup(const char *XMLfile)
{

	CXeromyces XeroFile;
	if (XeroFile.Load(XMLfile) != PSRETURN_OK)
		return false;

	// adjust the path name for resources if necessary
	//m_Name = XMLfile + directorypath;

	//Define elements used in XML file
	#define EL(x) int el_##x = XeroFile.getElementID(#x)
	#define AT(x) int at_##x = XeroFile.getAttributeID(#x)
	EL(soundgroup);
	EL(gain);
	EL(looping);
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
	#undef AT
	#undef EL

	XMBElement root = XeroFile.getRoot();

	if (root.getNodeName() != el_soundgroup)
	{
		LOG(ERROR, LOG_CATEGORY, "Invalid SoundGroup format (unrecognised root element '%s')", XeroFile.getElementString(root.getNodeName()).c_str());
		return false;
	}
	
	XERO_ITER_EL(root, child)
	{
	
		int child_name = child.getNodeName();			
		
		if(child_name == el_gain)
		{
			this->m_Gain = CStr(child.getText()).ToFloat();
		}

		if(child_name == el_looping)
		{			
			if(CStr(child.getText()).ToInt() == 1)
				SetFlag(eLoop);
		}

		if(child_name == el_pitch)
		{
			this->m_Pitch = CStr(child.getText()).ToFloat();
		}

		if(child_name == el_priority)
		{
			this->m_Priority = CStr(child.getText()).ToFloat();
		}

		if(child_name == el_randorder)
		{
			if(CStr(child.getText()).ToInt() == 1)
				SetFlag(eRandOrder);
		}

		if(child_name == el_randgain)
		{
			if(CStr(child.getText()).ToInt() == 1)
				SetFlag(eRandGain);
		}

		if(child_name == el_gainupper)
		{
			this->m_GainUpper = CStr(child.getText()).ToFloat();
		}

		if(child_name == el_gainlower)
		{
			this->m_GainLower = CStr(child.getText()).ToFloat();
		}



		if(child_name == el_randpitch)
		{
			if(CStr(child.getText()).ToInt() == 1)
				SetFlag(eRandPitch);
		}


		if(child_name == el_pitchupper)
		{
			this->m_PitchUpper = CStr(child.getText()).ToFloat();
		}

		if(child_name == el_pitchlower)
		{
			this->m_PitchLower = CStr(child.getText()).ToFloat();
		}

		if(child_name == el_conegain)
		{
			this->m_ConeOuterGain = CStr(child.getText()).ToFloat();
		}

		if(child_name == el_coneinner)
		{
			this->m_ConeInnerAngle = CStr(child.getText()).ToInt();
		}

		if(child_name == el_coneouter)
		{
			this->m_ConeOuterAngle = CStr(child.getText()).ToInt();
		}

		if(child_name == el_sound)
		{
			CStr szTemp(child.getText());
			//Handle temp = snd_open(szTemp);
			//this->snd_group.push_back(temp);
			this->filenames.push_back(szTemp);	
			
		}
		if(child_name == el_path)
		{
			m_filepath = child.getText();
		
		}
	}


	Reload();
	return true;

}
