#include "precompiled.h"

#include <Network/Session.h>
#include <Network/Network.h>
#include <CLogger.h>

using namespace std;

void CSessionManager::Register(CNetSession *pSession)
{
	CScopeLock scopeLock(m_Mutex);

	m_Sessions[pSession]=pSession;
}

bool CSessionManager::Deregister(CNetSession *pSession)
{
	CScopeLock scopeLock(m_Mutex);

	SessionMap::iterator it=m_Sessions.find(pSession);
	if (it == m_Sessions.end())
		return false;
	else
	{
		m_Sessions.erase(it);
		return true;
	}
}

void CSessionManager::Poll()
{
	m_Mutex.Lock();
	SessionMap::iterator it=m_Sessions.begin();
	while (it != m_Sessions.end())
	{
		CNetMessage *pMsg;
		if ((pMsg = it->second->TryPop()) != NULL)
		{
			CNetSession *pSess=it->second;
			m_Mutex.Unlock();
			if (!pSess->HandleMessage(pMsg))
			{
				LOG(WARNING, LOG_CAT_NET, "CSessionManager::Poll(): Unhandled message %s.", pMsg->GetString().c_str());
				delete pMsg;
			}
			m_Mutex.Lock();
		}
		++it;
	}
	m_Mutex.Unlock();
}
