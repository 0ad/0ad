#ifndef _Network_SessionManager_H
#define _Network_SessionManager_H

#include <ThreadUtil.h>

class CNetSession;

/*
	NAME: CSessionManager

	The central nexus of network message handling. Contains the entry point
	called from the main thread. CMessageSocket's are registered with a
	corresponding Protocol pointer (see IPointer, below) which is used to
	handle the incoming messages. The protocol pointer is changed by re-
	registering the message socket with another protocol.

*/
class CSessionManager: public Singleton<CSessionManager>
{
	typedef std::map <CNetSession *, CNetSession *> SessionMap;
	SessionMap m_Sessions;
	CMutex m_Mutex;
public:
	
	/*
		Poll all registered sessions and pass all messages to their
		message handlers.

		THREADS: Call from Main Thread only
	*/
	void Poll();

	/*
		Return the total number of bytes read by all registered sockets
	*/
	uint GetBytesRead();
	/*
		Return the total number of bytes written by all registered sockets
	*/
	uint GetBytesWritten();

	/*
		Register a network session with the session manager. Future calls to
		Poll() will poll this session's socket and pass any messages to
		its message handler function.

		THREADS: Safe from all threads
	*/
	void Register(CNetSession *pSession);

	/*
		Delete the protocol context associated with the specified socket

		THREADS: Safe from all threads

		RETURNS:
			TRUE if the protocol context was found and deleted
			FALSE if no registered context was found for the socket
	*/
	bool Deregister(CNetSession *pSession);
};
#define g_SessionManager (CSessionManager::GetSingleton())

#endif
