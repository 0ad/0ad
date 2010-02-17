/* Copyright (C) 2009 Wildfire Games.
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

/** Manages and updates SoundGroups. */

#ifndef INCLUDED_SOUNDGROUPMGR
#define INCLUDED_SOUNDGROUPMGR

#include "SoundGroup.h"
#include <vector>

class CSoundGroupMgr
{
public:
	std::vector <CSoundGroup *> m_Groups;  // a collection of sound groups
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
	void PlayNext(size_t index, const CVector3D& position); 

	///////////////////////////////////////////
	// AddGroup()
	// in:  const char *XMLFile - the filename of the SoundGroup.xml to open
	// out: size_t index into m_Groups
	// Loads the given XML file and returns an index for later use
	///////////////////////////////////////////
	size_t AddGroup(const VfsPath& XMLFile);  

	///////////////////////////////////////////
	// RemoveGroup()
	// in: size_t index into m_Groups
	// out: std::vector<CSoundGroup *>::iterator - one past the index removed (sometimes useful)
	// Removes and Releases a given soundgroup
	///////////////////////////////////////////
	std::vector<CSoundGroup *>::iterator RemoveGroup(size_t index);

	///////////////////////////////////////////
	// RemoveGroup()
	// in: std::vector<CSoundGroup *>::iterator - item to remove
	// out: std::vector<CSoundGroup *>::iterator - one past the index removed (sometimes useful)
	// Removes and Releases a given soundgroup
	///////////////////////////////////////////
	std::vector<CSoundGroup *>::iterator RemoveGroup(std::vector<CSoundGroup *>::iterator iter);


private:
	CSoundGroupMgr();
	CSoundGroupMgr(const CSoundGroupMgr &ref);
	CSoundGroupMgr &operator=(const CSoundGroupMgr &ref);

};

#define g_soundGroupMgr CSoundGroupMgr::GetInstance()

#endif // INCLUDED_SOUNDGROUPMGR
