/*
Network.h
by Simon Brenner
simon.brenner@home.se

OVERVIEW

	Contains the public interfaces to the networking code.
	
	CMessageSocket is a socket that sends and receives messages from the
	network. The global interface for sending and receiving messages is
	an IMessagePipeEnd.
	
	CMessagePipe also uses IMessagePipeEnd as its public interface, meaning that
	a CMessageSocket can be invisibly replaced with a CMessagePipe. Thus, the
	difference between MP and SP games is the source of pipe ends.

	Code that just wants to send messages will most likely only be confronted
	with the message pipe end interface.
	
EXAMPLES

To create a queue pair for IPC communication:

	CMessagePipe pipe;
	StartThread1(pipe[0]);
	StartThread2(pipe[1]);

	The argument type for StartThreadX would be "IMessagePipeEnd &".

NOTES ON THREAD SAFETY

All operations on an IMessagePipeEnd are fully thread-secure. Multiple access
to other interfaces of a CMessageSocket is not secure (but the IMessagePipeEnd
interface to a CMessageSocket is still fully thread secure)

MORE INFO

*/

#ifndef _Network_H
#define _Network_H

//--------------------------------------------------------
//  Includes / Compiler directives
//--------------------------------------------------------

#include "posix.h"
#include "types.h"
#include "Prometheus.h"
#include "ThreadUtil.h"
#include "Singleton.h"

#include "StreamSocket.h"

#include "NetMessage.h"

#include <deque>
#include <map>

//-------------------------------------------------
// Typedefs and Macros
//-------------------------------------------------

typedef CLocker<std::deque <CNetMessage *> > CLockedMessageDeque;

//-------------------------------------------------
// Error Codes
//-------------------------------------------------

DECLARE_ERROR( CONFLICTING_OP_IN_PROGRESS );

//-------------------------------------------------
// Declarations
//-------------------------------------------------

class IMessagePipeEnd;
class CMessagePipe;
class CMessageSocket;

class IMessagePipeEnd
{
public:
	/**
	 * Push a message on the output queue. It will be freed when popped of the
	 * queue, not by the caller. The pointer must point to memory that can be
	 * safely freed by delete.
	 */
	virtual void Push(CNetMessage *msg)=0;

	/**
	 * Try to pop a message from the input queue
	 *
	 * @return A pointer to the popped message, or NULL if the queue was empty
	 */
	virtual CNetMessage *TryPop()=0;

	/**
	 * Wait for a message on the input queue
	 * 
	 * Inputs
	 *	pMsg: A pointer to a message struct to store the popped message
	 *
	 * Returns
	 *	Void. The function returns successfully or blocks indefinitely.
	 */
//	virtual void WaitPop(CNetMessage *)=0;
};

/**
 * A message pipe with two ends, communication flowing in both directions
 * The two ends are indexed with the [] operator or the GetEnd() method
 * Each end has two associated queues, one input and one output queue. The
 * input queue of one End is the output queue of the other End and vice versa.
 */
class CMessagePipe
{
private:
	friend struct End;
	
	struct End: public IMessagePipeEnd
	{
		CMessagePipe *m_pPipe;
		CLockedMessageDeque *m_pIn;
		CLockedMessageDeque *m_pOut;

		inline End()
		{}
		
		inline End(CMessagePipe *pPipe, CLockedMessageDeque *pIn, CLockedMessageDeque *pOut):
			m_pPipe(pPipe), m_pIn(pIn), m_pOut(pOut)
		{}

		virtual void Push(CNetMessage *);
		virtual CNetMessage *TryPop();
		//virtual void WaitPop(CNetMessage *);
	};

	CLockedMessageDeque m_Queues[2];
	End m_Ends[2];
//	pthread_cond_t m_CondVar;
	pthread_mutex_t m_CondMutex;
	
public:
	CMessagePipe();
	
	/**
	 * Return one of the two ends of the pipe
	 */
	inline IMessagePipeEnd &operator [] (int idx)
	{
		return GetEnd(idx);
	}
	
	/**
	 * Return one of the two ends of the pipe
	 */
	inline IMessagePipeEnd &GetEnd(int idx)
	{
		assert(idx==1 || idx==0);
		return m_Ends[idx];
	}
};

class CServerSocket: public CSocketBase
{
protected:
	/**
	 * The default implementation of this method accepts an incoming connection
	 * and calls OnAccept() with the accepted internal socket instance.
	 *
	 * NOTE: Subclasses should never overload this method, overload OnAccept()
	 * instead.
	 */
	virtual void OnRead();

	virtual void OnWrite();
	virtual void OnClose(PS_RESULT errorCode);

public:
	virtual ~CServerSocket();

	/**
	 * There is an incoming connection in the queue. Examine the SocketAddress
	 * and call Accept() or Reject() to accept or reject the incoming
	 * connection
	 *
	 * @see CSocketBase::Accept()
	 * @see CSocketBase::Reject()
	 */
	virtual void OnAccept(const SocketAddress &)=0;
};

/**
 * Implements a Message Pipe over an Async IO stream socket.
 */
class CMessageSocket: public CStreamSocket, public IMessagePipeEnd
{
	bool m_IsWriting;
	u8 *m_pWrBuffer;
	uint m_WrBufferSize;
	bool m_ReadingData;
	u8 *m_pRdBuffer;
	uint m_RdBufferSize;

	CLockedMessageDeque m_InQ; // Messages read from socket
	CLockedMessageDeque m_OutQ;// Messages to write to socket
//	pthread_cond_t m_InCond;
//	pthread_cond_t m_OutCond;

	void StartWriteNextMessage();
	void StartReadHeader();
	void StartReadMessage();
protected:
	virtual void ReadComplete(PS_RESULT);
	virtual void WriteComplete(PS_RESULT);
	
public:
	inline CMessageSocket(CSocketInternal *pInt):
		CStreamSocket(pInt),
		m_IsWriting(false),
		m_pWrBuffer(NULL),
		m_WrBufferSize(0),
		m_ReadingData(false),
		m_pRdBuffer(NULL),
		m_RdBufferSize(0)
	{}
	inline CMessageSocket():
		CStreamSocket(),
		m_IsWriting(false),
		m_pWrBuffer(NULL),
		m_WrBufferSize(0),
		m_ReadingData(false),
		m_pRdBuffer(NULL),
		m_RdBufferSize(0)
	{}
	virtual ~CMessageSocket();

	/**
	 * Beware! If you subclass and override this method, you must call this
	 * implementation from the subclass
	 */
	virtual void ConnectComplete(PS_RESULT errorCode);
	
	virtual void Push(CNetMessage *);
	virtual CNetMessage *TryPop();
};

#endif
