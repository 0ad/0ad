/**
* =========================================================================
* File        : SoundGroupMgr.h
* Project     : 0 A.D.
* Description : Manages and updates SoundGroups        
* =========================================================================
*/


/*
 * Copyright (c) 2005-2006 Gavin Fowler
 *
 * Redistribution and/or modification are also permitted under the
 * terms of the GNU General Public License as published by the
 * Free Software Foundation (version 2 or later, at your option).
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 */

#include "SoundGroup.h"
#include <vector>
using namespace std;

class CSoundGroupMgr
{
public:
	vector <CSoundGroup *> m_Groups;  // a collection of sound groups
	static CSoundGroupMgr *m_pInstance; // our static instance of the manager
	static CSoundGroupMgr *GetInstance(); 
	static void DeleteInstance();
	
	///////////////////////////////////////////
	// UpdateSoundGroups()
	// updates all soundgroups, call in Frame()
	///////////////////////////////////////////
	void UpdateSoundGroups(float TimeSinceLastFrame);  
	
	///////////////////////////////////////////
	// PlayNext()
	// in:  size_t index - index into m_Groups
	// Plays the next queued sound in an indexed group 
	///////////////////////////////////////////
	void PlayNext(size_t index); 

	///////////////////////////////////////////
	// AddGroup()
	// in:  const char *XMLFile - the filename of the SoundGroup.xml to open
	// out: size_t index into m_Groups
	// Loads the given XML file and returns an index for later use
	///////////////////////////////////////////
	size_t AddGroup(const char *XMLFile);  

	///////////////////////////////////////////
	// RemoveGroup()
	// in: size_t index into m_Groups
	// out: vector<CSoundGroup *>::iterator - one past the index removed (sometimes useful)
	// Removes and Releases a given soundgroup
	///////////////////////////////////////////
	vector<CSoundGroup *>::iterator RemoveGroup(size_t index);

	///////////////////////////////////////////
	// RemoveGroup()
	// in: vector<CSoundGroup *>::iterator - item to remove
	// out: vector<CSoundGroup *>::iterator - one past the index removed (sometimes useful)
	// Removes and Releases a given soundgroup
	///////////////////////////////////////////
	vector<CSoundGroup *>::iterator RemoveGroup(vector<CSoundGroup *>::iterator iter);


private:
	CSoundGroupMgr();
	CSoundGroupMgr(const CSoundGroupMgr &ref);
	CSoundGroupMgr &operator=(const CSoundGroupMgr &ref);

};
