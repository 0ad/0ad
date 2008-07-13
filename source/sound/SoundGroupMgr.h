/**
* =========================================================================
* File        : SoundGroupMgr.h
* Project     : 0 A.D.
* Description : Manages and updates SoundGroups        
* =========================================================================
*/

// license: GPL; see sound/license.txt

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
	size_t AddGroup(const char *XMLFile);  

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
