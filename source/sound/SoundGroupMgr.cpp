/**
* =========================================================================
* File        : SoundGroupMgr.h
* Project     : 0 A.D.
* Description : Manages and updates SoundGroups        
* =========================================================================
*/

// license: GPL; see sound/license.txt

// Example usage: 

// size_t index;
// CSoundGroupMgr *sgm = CSoundGroupMgr::GetInstance();
// index = sgm->AddGroup("SoundGroup.xml");

// sgm->UpdateSoundGroups(TimeSinceLastFrame); // call in Frame()

// sgm->PlayNext(index);  // wash-rinse-repeat


// sgm->RemoveGroup(index);  // Remove the group if you like

// sgm->DeleteInstance();  // Delete instance in shutdown


#include "precompiled.h"
#include "SoundGroupMgr.h"

typedef std::vector<CSoundGroup*> SoundGroups;
typedef SoundGroups::iterator SoundGroupIt;


CSoundGroupMgr *CSoundGroupMgr::m_pInstance = 0;

CSoundGroupMgr::CSoundGroupMgr()
{

}

CSoundGroupMgr *CSoundGroupMgr::GetInstance()
{
	if(!m_pInstance)
		m_pInstance = new CSoundGroupMgr();

	return m_pInstance;

}

void CSoundGroupMgr::DeleteInstance()
{
	if(m_pInstance)
	{
		SoundGroupIt vIter = m_pInstance->m_Groups.begin();
		while(vIter != m_pInstance->m_Groups.end())
			vIter = m_pInstance->RemoveGroup(vIter);		
		
		delete m_pInstance;
	}		
	m_pInstance = 0;

}

///////////////////////////////////////////
// AddGroup()
// in:  const char *XMLFile - the filename of the SoundGroup.xml to open
// out: size_t index into m_Groups
// Loads the given XML file and returns an index for later use
///////////////////////////////////////////
size_t CSoundGroupMgr::AddGroup(const char *XMLFile)
{
	CSoundGroup* newGroup = new CSoundGroup(XMLFile);
	m_Groups.push_back(newGroup);

	return m_Groups.size() - 1;
}

///////////////////////////////////////////
// RemoveGroup()
// in: size_t index into m_Groups
// out: SoundGroupIt - one past the index removed (sometimes useful)
// Removes and Releases a given soundgroup
///////////////////////////////////////////
SoundGroupIt CSoundGroupMgr::RemoveGroup(size_t index)
{
	SoundGroupIt vIter = m_Groups.begin();
	if(index >= m_Groups.size())
		return vIter;
	
	CSoundGroup *temp = (*vIter);
	(*vIter)->ReleaseGroup();
	vIter = m_Groups.erase(vIter+index);
	
	delete temp;

	return vIter;

}

///////////////////////////////////////////
// RemoveGroup()
// in: SoundGroupIt - item to remove
// out: SoundGroupIt - one past the index removed (sometimes useful)
// Removes and Releases a given soundgroup
///////////////////////////////////////////
SoundGroupIt CSoundGroupMgr::RemoveGroup(SoundGroupIt iter)
{

	(*iter)->ReleaseGroup();
	
	CSoundGroup *temp = (*iter);
	
	iter = m_Groups.erase(iter);
	
	delete temp;

	return iter;

}

///////////////////////////////////////////
// UpdateSoundGroups()
// updates all soundgroups, call in Frame()
///////////////////////////////////////////
void CSoundGroupMgr::UpdateSoundGroups(float TimeSinceLastFrame)
{
	SoundGroupIt vIter = m_Groups.begin();	
	while(vIter != m_Groups.end())
	{
		(*vIter)->Update(TimeSinceLastFrame);
		vIter++;
	}
}

///////////////////////////////////////////
// PlayNext()
// in:  size_t index - index into m_Groups
// Plays the next queued sound in an indexed group 
///////////////////////////////////////////
void CSoundGroupMgr::PlayNext(size_t index, const CVector3D& position)
{
	if(index < m_Groups.size())
		m_Groups[index]->PlayNext(position);
	else
		debug_printf("SND: PlayNext(%d) invalid, %d groups defined\n", index, m_Groups.size());
}
