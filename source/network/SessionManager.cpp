#include "precompiled.h"

#include "Session.h"
#include "Network.h"
#include "ps/CLogger.h"

using namespace std;

void CSessionManager::Register(CNetSession *pSession)
{
	CScopeLock scopeLock(m_Mutex);
	SessionMap::iterator it;
	
	// The total number of references to this session should be 1 and only 1
	// Either there's one in m_Sessions or there's one in m_AddQueue. There
	// shall be no references in the remove queue.
	// m_Sessions is Read-Only
	
	if ((it = m_RemoveQueue.find(pSession)) != m_RemoveQueue.end())
		m_RemoveQueue.erase(it);
	if (m_Sessions.find(pSession) == m_Sessions.end())
		m_AddQueue[pSession]=pSession;
}

void CSessionManager::Deregister(CNetSession *pSession)
{
	CScopeLock scopeLock(m_Mutex);
	SessionMap::iterator it;
	
	// The total number of references to this session should be 2 or 0:
	// One in m_Sessions and one in the remove queue or 0 in both. None in
	// m_AddQueue.
	// m_Sessions is Read-Only
	
	if ((it = m_AddQueue.find(pSession)) != m_AddQueue.end())
		m_AddQueue.erase(it);
	if (m_Sessions.find(pSession) != m_Sessions.end())
		m_RemoveQueue.insert(make_pair(pSession, pSession));
}

void CSessionManager::Poll()
{
	m_Mutex.Lock();
	SessionMap::iterator it;
	
	// De/Register should've made sure that it doesn't matter which order we
	// process the two queues.
	for (it=m_AddQueue.begin();it != m_AddQueue.end();++it)
	{
		m_Sessions.insert(*it);
	}
	m_AddQueue.clear();
	for (it=m_RemoveQueue.begin();it != m_RemoveQueue.end();++it)
	{
		m_Sessions.erase(it->second);
	}
	m_RemoveQueue.clear();
	
	it=m_Sessions.begin();
	while (it != m_Sessions.end())
	{
		CNetMessage *pMsg;
		
		// Extraneous? well, sessions are perfectly able to delete other
		// sessions (or cause them to be deleted) in their message handler
		if (m_RemoveQueue.find(it->second) != m_RemoveQueue.end())
			continue;
		
		while ((pMsg = it->second->TryPop()) != NULL)
		{
			CNetSession *pSess=it->second;
			m_Mutex.Unlock();
			if (!pSess->HandleMessage(pMsg))
			{
				LOG(CLogger::Warning, LOG_CAT_NET, "CSessionManager::Poll(): Unhandled message %s.", pMsg->GetString().c_str());
				delete pMsg;
			}
			m_Mutex.Lock();
			
			if (m_RemoveQueue.find(it->second) != m_RemoveQueue.end())
				break;
		}
		++it;
	}
	m_Mutex.Unlock();
}
