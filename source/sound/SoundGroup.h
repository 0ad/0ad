/**
* =========================================================================
* File        : SoundGroup.h
* Project     : 0 A.D.
* Description : Loads up a group of sound files with shared properties, 
*				and provides a simple interface for playing them.        
*
* Author      : Gavin Fowler
* =========================================================================
*/
/*
Example usage:

CSoundGroup s;
s.LoadSoundGroup("SoundGroup.xml");  // only needs to be called once (or not at all if filename is passed to ctor)
s.PlayNext(); // call each time you want to play another sound from the group. 
s.ReleaseGroup();  // If you want to free up the resources early, but this happens in dtor.

Example SoundGroup.xml
	<?xml version="1.0" encoding="utf-8" standalone="no" ?> 
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





#ifndef SOUNDGROUP_H_
#define SOUNDGROUP_H_


#include "lib/res/handle.h"
#include "ps/cstr.h"
#include "lib/res/sound/snd_mgr.h"

#include <vector>
using namespace std;

enum eSndGrpFlags{ eRandOrder = 0x01, eRandGain = 0x02, eRandPitch = 0x04, eLoop = 0x08 }; 

class CSoundGroup
{

	static size_t m_index;  // index of the next sound to play
	
	vector<Handle> snd_group;  // we store the handles so we can load now and play later
	vector<CStr> filenames; // we need the filenames so we can reload when necessary.
	CStr m_filepath;

	unsigned char m_Flags; // up to eight individual parameters, use with eSndGrpFlags.

	float m_Gain;  
	float m_Pitch;
	float m_Priority;
	float m_ConeOuterGain; 
	float m_PitchUpper;
	float m_PitchLower;
	float m_GainUpper;
	float m_GainLower;
	int m_ConeInnerAngle;
	int m_ConeOuterAngle;

public:
	CSoundGroup(const char *XMLfile);
	CSoundGroup(void);
	~CSoundGroup(void);
	
	// Play next sound in group
	void PlayNext();

	// Load a group
	bool LoadSoundGroup(const char *XMLfile);

	void Reload();
	
	// Release all remaining loaded handles
	void ReleaseGroup();
	
	// Set a flag using a value from eSndGrpFlags
	inline void SetFlag(int flag){ m_Flags |= flag; }  

	// Test flag, returns true if flag is set.
	inline bool TestFlag(int flag) { return (m_Flags & flag) == flag;}
	

};



#endif //#ifndef SOUNDGROUP_H_