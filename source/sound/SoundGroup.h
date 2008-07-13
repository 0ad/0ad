/**
* =========================================================================
* File        : SoundGroup.h
* Project     : 0 A.D.
* Description : Loads up a group of sound files with shared properties, 
*				and provides a simple interface for playing them.        
* =========================================================================
*/

// license: GPL; see sound/license.txt

/*
Example usage: (SEE SOUNDGROUPMGR.H)


Example SoundGroup.xml
	<?xml version="1.0" encoding="utf-8"?>
	<SoundGroup>
		<Gain>1.0</Gain>
		<Looping>0</Looping>
		<Pitch>1.0</Pitch>
		<Priority>100</Priority>
		<RandOrder>0</RandOrder>
		<RandGain>0</RandGain>
		<RandPitch>0</RandPitch>
		<ConeGain>1.0</ConeGain>
		<ConeInner>360</ConeInner>
		<ConeOuter>360</ConeOuter>
		<Sound>audio/voice/hellenes/soldier/Attack_Attackx.ogg</Sound>
		<Sound>audio/voice/hellenes/soldier/Attack_Chargex.ogg</Sound>
		<Sound>audio/voice/hellenes/soldier/Attack_Engagex.ogg</Sound>
		<Sound>audio/voice/hellenes/soldier/Attack_ForMyFamily.ogg</Sound>
	</SoundGroup>

*/

#ifndef INCLUDED_SOUNDGROUP
#define INCLUDED_SOUNDGROUP

#include "lib/res/handle.h"
#include "ps/CStr.h"
#include "maths/Vector3D.h"
#include "lib/res/sound/snd_mgr.h"

#include <vector>

enum eSndGrpFlags
{
	eRandOrder     = 0x01,
	eRandGain      = 0x02,
	eRandPitch     = 0x04,
	eLoop          = 0x08,
	eOmnipresent      = 0x10
};


class CSoundGroup
{
	size_t m_index;  // index of the next sound to play
	
	Handle m_hReplacement;
	
	std::vector<Handle> snd_group;  // we store the handles so we can load now and play later
	std::vector<CStr> filenames; // we need the filenames so we can reload when necessary.
	std::vector<float> playtimes; // it would be better to store this in with the Handles perhaps?
	CStr m_filepath; // the file path for the list of sound file resources
	CStr m_intensity_file; // the replacement aggregate 'intense' sound

	float m_CurTime; // Time elapsed since soundgroup was created
	float m_TimeWindow; // The Intensity Threshold Window
	size_t m_IntensityThreshold; // the allowable intensity before a sound switch	
	size_t m_Intensity;  // our current intensity(number of sounds played since m_CurTime - m_TimeWindow) 
	float m_Decay; // 
	unsigned char m_Flags; // up to eight individual parameters, use with eSndGrpFlags.
	
	float m_Gain;  
	float m_Pitch;
	float m_Priority;
	float m_ConeOuterGain; 
	float m_PitchUpper;
	float m_PitchLower;
	float m_GainUpper;
	float m_GainLower;
	float m_ConeInnerAngle;
	float m_ConeOuterAngle;

public:
	CSoundGroup(const char *XMLfile);
	CSoundGroup(void);
	~CSoundGroup(void);
	
	// Play next sound in group
	// @param position world position of the entity generating the sound
	// (ignored if the eOmnipresent flag is set)
	void PlayNext(const CVector3D& position);

	// Load a group
	bool LoadSoundGroup(const char *XMLfile);

	void Reload();
	
	// Release all remaining loaded handles
	void ReleaseGroup();

	// Update SoundGroup, remove dead sounds from intensity count
	void Update(float TimeSinceLastFrame);
	
	// Set a flag using a value from eSndGrpFlags
	inline void SetFlag(int flag){ m_Flags |= flag; }  

	// Test flag, returns true if flag is set.
	inline bool TestFlag(int flag) { return (m_Flags & flag) != 0;}

};

#endif //#ifndef INCLUDED_SOUNDGROUP
