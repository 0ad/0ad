#ifndef INCLUDED_NETWORK_SESSIONMANAGER
#define INCLUDED_NETWORK_SESSIONMANAGER

#include "ps/ThreadUtil.h"

class CNetSession;

/*
	NAME: CSessionManager

	The central nexus of network message handling. Contains the entry point
	called from the main thread.
	CNetSession's are registered and when the Poll method finds that the session
	has a pending message, the session object's HandleMessage method is	called
	to handle it. Any unhandled messages (HandleMessage returns false) are
	logged to the system log.
*/
class CSessionManager: public Singleton<CSessionManager>
{
	typedef std::map <CNetSession *, CNetSession *> SessionMap;
	
	SessionMap m_Sessions;
	SessionMap m_AddQueue;
	SessionMap m_RemoveQueue;
	CMutex m_Mutex;
	
public:
	
	/*
		Poll all registered sessions and pass all messages to their
		message handlers.

		THREADS: Call from Main Thread only
	*/
	void Poll();

	/*
		Register a network session with the session manager. Future calls to
		Poll() will poll this session's socket and pass any messages to
		its message handler function.

		THREADS: Safe from all threads
	*/
	void Register(CNetSession *pSession);

	/*
		Delete the protocol context associated with the specified socket.

		THREADS: Safe from all threads
	*/
	void Deregister(CNetSession *pSession);
};
#define g_SessionManager (CSessionManager::GetSingleton())

#endif
