#ifndef _Network_NetSession_H
#define _Network_NetSession_H

#include <Network/Network.h>
#include <Network/SessionManager.h>

/*
	CNetSession

	DESCRIPTION:
		The representation of a network session (on both the client and server
		side)
*/
class CNetSession: public CMessageSocket
{
protected:
	/*
		The MessageHandler callback follows the contract of HandleMessage, see
		the documentation for that function for more information.
	*/
	typedef bool (MessageHandler)(CNetMessage *, CNetSession *);
	MessageHandler *m_pMessageHandler;

	CStrW m_Name;

public:
	/*
		Note: This constructer takes over ownership of the passed pipe end. It
		should not be freed by the caller, and should be heap-allocated.
	 */
	inline CNetSession(MessageHandler *pMsgHandler=NULL):
		m_pMessageHandler(pMsgHandler)
	{
		g_SessionManager.Register(this);
	}
	inline CNetSession(CSocketInternal *pInt, MessageHandler *pMsgHandler=NULL):
		CMessageSocket(pInt),
		m_pMessageHandler(pMsgHandler)
	{
		g_SessionManager.Register(this);
	}

	virtual ~CNetSession();

	/*
		Handle an incoming message.

		THREADS:
			When used with the session manager, this method will only be
			called from the main thread.

		ARGUMENTS:
			pMsg		The incoming message
			pSocket		The socket the message came from

		RETURNS:
			TRUE if the message was handled by this class or another protocol
			that this class knows of. FALSE if the message was not handled.
	*/
	inline bool HandleMessage(CNetMessage *pMsg)
	{
		if (m_pMessageHandler)
			return (m_pMessageHandler)(pMsg, this);
		else
			return false;
	}

	inline CStrW GetName()
	{
		return m_Name;
	}
};

#endif //_Network_NetSession_H
