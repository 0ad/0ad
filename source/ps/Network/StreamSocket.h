#ifndef _StreamSocket_H
#define _StreamSocket_H

#include "types.h"
#include "Prometheus.h"
#include "Network.h"
#include "SocketBase.h"

/**
 * A class implementing Async I/O on top of the non-blocking event-driven
 * CSocketBase
 */
class CStreamSocket: public CSocketBase
{
	pthread_mutex_t m_Mutex;
	char *m_pConnectHost;
	int m_ConnectPort;
	
	struct SOperationContext
	{
		bool m_Valid;
		void *m_pBuffer;
		uint m_Length;
		uint m_Completed;

		inline SOperationContext():
			m_Valid(false)
		{}
	};
	SOperationContext m_ReadContext;
	SOperationContext m_WriteContext;

protected:
	friend void *CStreamSocket_ConnectThread(void *);

	CStreamSocket(CSocketInternal *pInt);

	/**
	 * Set the required socket options on the socket.
	 */
	void SetSocketOptions();

	/**
	 * The destructor will disconnect the socket and free any OS resources.
	 */
	virtual ~CStreamSocket();

	virtual void OnRead();
	virtual void OnWrite();

public:
	CStreamSocket();

	/**
	 * The Lock function locks a mutex stored in the CSocket object. None of
	 * the CSocket methods actually use the mutex, it is just there as a
	 * convenience for the user.
	 */
	void Lock();
	/**
	 * The Unlock function unlocks a mutex stored in the CSocket object. None
	 * of the CSocket methods actually use the mutex, it is just there as a
	 * convenience for the user.
	 */
	void Unlock();
	
	/**
	 * Begin a connect operation to the specified host and port. The connect
	 * attempt and name resolution is done in the background and the OnConnect
	 * callback is called when the connect is complete (or failed)
	 *
	 * Note that a PS_OK return only means that the connect operation has been
	 * initiated, not that it is successful.
	 *
	 * @param hostname A hostname or an IP address of the remote host
	 * @param port The TCP port number in host byte order
	 *
	 * @return PS_OK - The connect has been initiated
	 */
	PS_RESULT BeginConnect(const char *hostname, int port);

	/**
	 * Close the socket. No more data can be sent over the socket, but any data
	 * pending from the remote host will still be received, and the OnRead
	 * callback called (if the socket's op mask has the READ bit set). Note
	 * that the socket isn't actually closed until the remote end calls
	 * Close on the corresponding remote socket, upon which the OnClose
	 * callback is called.
	 */
	void Close();
	
	/**
	 * Start a read operation. The function call will return immediately and
	 * complete the I/O in the background. OnRead() will be called when it is
	 * complete. Until the Read is complete, the buffer should not be touched.
	 * There can only be one read operation in progress at one time.
	 *
	 * Inputs
	 *	buf		A pointer to the buffer where the data should be written
	 *	len		The length of the buffer. The amount of data the function should
	 *			try to read.
	 *
	 * Returns
	 *	PS_OK	Some or all data was successfully read.
	 *	CONFLICTING_OP_IN_PROGRESS Another Read operation is alread in progress
	 *	CONNECTION_BROKEN	The socket is not connected or a server socket
	 */	
	PS_RESULT Read(void *buf, uint len);
	
	/**
	 * Start a Write operation. The function call will return immediately and
	 * the I/O complete in the background. OnWrite() will be called when i has
	 * completed. Until the Write is complete, the buffer shouldn't be touched.
	 * There can only be one write operation in progress at one time.
	 *
	 * @param buf A pointer to the buffer of data to write
	 * @param len The length of the buffer.
	 *
	 * Returns
	 *	PS_OK	Some or all data was successfully read.
	 *	CONFLICTING_OP_IN_PROGRESS	Another Write operation is in progress
	 *	CONNECTION_BROKEN	The socket is not connected or a server socket
	 */	
	PS_RESULT Write(void *buf, uint len);
	
	/**
	 * Get the address of the remote host connected to this socket.
	 *
	 * Inputs
	 *	address	The IP address of the remote host, in written order
	 *	port	The remote port number, in local byte order
	 *
	 * Returns
	 *	PS_OK	The remote address was successfully retrieved
	 *	CONNECTION_BROKEN	The socket is not connected
	 */	
	//PS_RESULT GetRemoteAddress(u8 (&address)[4], int &port);

	virtual void ConnectComplete(PS_RESULT errorCode);
	virtual void ReadComplete(PS_RESULT errorCode);
	virtual void WriteComplete(PS_RESULT errorCode);
	virtual void OnClose(PS_RESULT errorCode);
};

#endif
