#ifndef _SocketBase_H
#define _SocketBase_H

//--------------------------------------------------------
//  Includes / Compiler directives
//--------------------------------------------------------

#include "posix.h"
#include "types.h"
#include "Prometheus.h"
#include <string.h>

//-------------------------------------------------
// Error Codes
//-------------------------------------------------

DECLARE_ERROR( CONNECT_TIMEOUT );
DECLARE_ERROR( CONNECT_REFUSED );
DECLARE_ERROR( NO_SUCH_HOST );
DECLARE_ERROR( NO_ROUTE_TO_HOST );
DECLARE_ERROR( CONNECTION_BROKEN );
DECLARE_ERROR( WAIT_ABORTED );
DECLARE_ERROR( PORT_IN_USE );
DECLARE_ERROR( INVALID_PORT );
DECLARE_ERROR( WAIT_LOOP_FAIL );
DECLARE_ERROR( CONNECT_IN_PROGRESS );
DECLARE_ERROR( INVALID_PROTOCOL );

//-------------------------------------------------
// Declarations
//-------------------------------------------------

class CSocketInternal;

/**
 * An enumeration of all supported protocols, and the special value UNSPEC,
 * which represents an invalid address.
 */
// Modifiers Note: Each value in the enum should correspond to a sockaddr_*
// struct and a PF_* value
enum SocketProtocol
{
	UNSPEC=-1, // This should be an invalid value
	IPv4=PF_INET,
#ifdef USE_INET6
	IPv6=PF_INET6,
#endif
	/* More protocols */
};

/**
 * A protocol-independent representation of a socket address. All protocols
 * in the SocketProtocol enum should have a corresponding member in this union.
 */
// Modifiers Note: Each member must contain a first field, compatible with the
// sin_family field of sockaddr_in. The field contains the SocketProtocol value
// for the address, and it is returned by GetProtocol()
union SocketAddress
{
	sockaddr_in m_IPv4;
#ifdef USE_INET6
	sockaddr_in6 m_IPv6;
#endif

	inline SocketProtocol GetProtocol() const
	{
		return (SocketProtocol)m_IPv4.sin_family;
	}

	inline SocketAddress()
	{
		memset(this, 0, sizeof(SocketAddress));
		m_IPv4.sin_family=UNSPEC;
	}

	/**
	 * Create a wildcard address for the specified protocol with a specified
	 * port.
	 *
	 * @param port The port number, in local byte order
	 * @param proto The protocol to use; default IPv4
	 */
	explicit SocketAddress(int port, SocketProtocol proto=IPv4);

	/**
	 * Create an address from a numerical IPv4 address and port, port in local
	 * byte order, IPv4 address as a byte array in written order. The Protocol
	 * of the resulting SocketAddress will be IPv4
	 *
	 * @param address An IPv4 address as a byte array (in written order)
	 * @param port A port number (0-65535) in local byte order.
	 */
	SocketAddress(u8 address[4], int port);

	/**
	 * Resolve the name using the systems name resolution service (i.e. DNS),
	 * and store the resulting address. When multiple addresses are found, the
	 * first result is returned.
	 *
	 * @param name The name to resolve
	 * @param addr A reference to the variable to hold the address
	 *
	 * @return An error code; PS_OK for success
	 */
	static PS_RESULT Resolve(const char *name, int port, SocketAddress &addr);
};

/**
 * An enumeration of the three socket states
 *
 * @see CSocketBase::GetState()
 */
enum SocketState
{
	/**
	 * The socket is unconnected. Use GetError() to see if it is due to a
	 * failure, a clean close, or it was never connected.
	 *
	 * @see CSocketBase::GetError()
	 */
	SS_UNCONNECTED=0,
	/**
	 * A connect attempt has started on a non-blocking socket. The error state
	 * will be CONNECTION_BROKEN.
	 *
	 * @see CSocketBase::OnWrite()
	 */
	SS_CONNECT_STARTED,
	/**
	 * The socket is connected. The error state will be set to PS_OK.
	 */
	SS_CONNECTED
};

/**
 * Contains the basic socket I/O abstraction and event callback methods.
 * A CSocketBase can only be instantiated as a subclass, none of the functions
 * are meant to exist as anything other than helper functions for socket
 * classes
 *
 * Any CSocket subclass that can be Accept:ed by a CServerSocket should
 * provide a constructor that takes a CSocketInternal pointer, and follows
 * the semantics of the CSocket::CSocket(CSocketInternal *) constructor
 */
class CSocketBase
{
private:
	CSocketInternal *m_pInternal;
	SocketState m_State;
	PS_RESULT m_Error;
	SocketProtocol m_Proto;
	bool m_NonBlocking;

	/**
	 * Initialize any data needed to communicate to the RunWaitLoop(). After
	 * the call to InitWaitLoop, it should be safe to call any IPC function
	 * that expects to talk to the wait loop.
	 */
	static void InitWaitLoop();

	/**
	 * Loop forever, waiting for events and calling the callbacks on sockets,
	 * according to their Op mask.
	 */
	static void RunWaitLoop();

	/**
	 * The network thread entry point. Simply calls RunWaitLoop()
	 */
	friend void *WaitLoopThreadMain(void *);
	
	/**
	 * An internal utility function used by the UNIX select loop
	 */
	friend bool ConnectError(CSocketBase *, CSocketInternal *);
		
	/**
	 * Abort the call to RunWaitLoop(), if one is currently running.
	 */
	static void AbortWaitLoop();

	/**
	 * Tell the running wait loop to abort. This is the platform-dependent
	 * implementation of AbortWaitLoop()
	 */
	static void SendWaitLoopAbort();
	void SendWaitLoopUpdate();

protected:
	// These values are bitwise or-ed to produce op masks
	enum Ops
	{
		// Call OnRead() on a stream socket when there is data to read from the
		// socket, or OnAccept() on a server socket when there are incoming
		// connections pending
		READ=1,
		// Call OnWrite() when there is space available in the socket's output
		// buffer. Has no effect on server sockets.
		WRITE=2
	};

	/**
	 * Initialize a CSocketBase from a CSocketInternal pointer. Use in OnAccept
	 * callbacks to create an object of your subclass. This constructor should
	 * be overloaded protected by any subclass that may be Accept:ed.
	 */
	CSocketBase(CSocketInternal *pInt);
	virtual ~CSocketBase();

	/**
	 * Get the op mask for the socket.
	 */
	uint GetOpMask();
	
	/**
	 * Set the op mask for the socket, specifying which callbacks should be
	 * called by the WaitLoop. The initial op mask is zero, which means that
	 * this method must be called explicitly for any callbacks to be called.
	 * Note that before the call to BeginConnect or Bind, any call to this
	 * method is a no-op.
	 *
	 * It is safe to call this function while a RunWaitLoop is running.
	 *
	 * The wait loop guarantees that the callbacks specified in ops will be 
	 * called when appropriate, but does not make the opposite guarantee for 
	 * unset bits; i.e. any callback may be called even with a zero op mask.
	 */
	void SetOpMask(uint ops);

public:
	/**
	 * Constructs a CSocketBase. The OS socket object is not created by the
	 * constructor, but by the protected Initialize method, which is called by
	 * Connect and Bind.
	 *
	 * @see Connect
	 * @see Bind
	 */
	CSocketBase();

	/**
	 * Returns the protocol set by Initialize. All SocketAddresses used with
	 * the socket must have the same SocketProtocol
	 */
	inline SocketProtocol GetProtocol() const
	{ return m_Proto; }

	/**
	 * Destroy the OS socket. If the socket is not cleanly closed before, it
	 * will be forcefully closed by calling this method.
	 */
	void Destroy();

	/**
	 * Create the OS socket for the specified protocol type.
	 */
	PS_RESULT Initialize(SocketProtocol proto=IPv4);

	/**
	 * Connect the socket to the specified address. The socket must be
	 * initialized for the protocol of the address.
	 *
	 * @param addr The address to connect to
	 * @see SocketAddress::Resolve
	 */
	PS_RESULT Connect(const SocketAddress &addr);

	/**
	 * Bind the socket to the specified address and start listening for
	 * incoming connections. You must initialize the socket for the correct
	 * SocketProtocol before calling Bind.
	 *
	 * @param addr The address to bind to
	 * @see SocketAddress::SocketAddress(int,SocketProtocol)
	 */
	PS_RESULT Bind(const SocketAddress &addr);

	/**
	 * Store the address of the next incoming connection in the SocketAddress
	 * pointed to by addr. You must then choose whether to accept or reject the
	 * connection by calling Accept or Reject
	 *
	 * @param addr A pointer to a SocketAddress
	 * @return PS_OK or PS_FAIL
	 * 
	 * @see Accept(SocketAddress&)
	 * @see Reject()
	 */
	PS_RESULT PreAccept(SocketAddress &addr);

	/**
	 * Accept the next incoming connection. You must construct a suitable
	 * CSocketBase subclass using the passed CSocketInternal.
	 * May only be called after a successful PreAccept call
	 */
	CSocketInternal *Accept();
	/**
	 * Reject the next incoming connection.
	 *
	 * May only be called after a successful PreAccept call
	 */
	void Reject();

	/**
	 * Set or reset non-blocking operation. When non-blocking, all socket
	 * operations will return immediately, having done none or parts of
	 * the operation. The default state for a socket is non-blocking
	 *
	 * @see CSocketBase::Read
	 * @see CSocketBase::Write
	 * @see CSocketBase::Connect
	 */
	void SetNonBlocking(bool nonBlocking=true);

	/**
	 * Return the current non-blocking state of the socket.
	 *
	 * @see SetNonBlocking(bool)
	 */
	inline bool IsNonBlocking() const
	{ return m_NonBlocking; }

	/**
	 * Return the error state of the socket. This will be the same value that
	 * was returned by the IO function that failed.
	 *
	 * @see GetState()
	 */
	inline PS_RESULT GetErrorState() const
	{ return m_Error; }

	/**
	 * Return the connection state of the socket. If the connection status is
	 * "unconnected", use GetError() to see if it was disconnected due to an
	 * error, or cleanly closed.
	 *
	 * @see SocketState
	 * @see GetError()
	 */
	inline SocketState GetState() const
	{ return m_State; }

	/**
	 * Disable Nagle's algorithm (enable no-delay working mode)
	 */
	void SetTcpNoDelay(bool tcpNoDelay=true);

	/**
	 * Get the address of the remote end to which the socket is connected.
	 * 
	 * @return A reference to the socket address
	 */
	const SocketAddress &GetRemoteAddress();

	/**
	 * Get the address of the internal pointer. Can be used in an OnAccept
	 * callback to implement address-based protection.
	 *
	 * @return A reference to the socket address
	 */
	static const SocketAddress &GetRemoteAddress(CSocketInternal *pInt);

	/**
	 * Attempt to read data from the socket. Any data available without blocking
	 * will be returned. Note that a successful return does not mean that the
	 * whole buffer was filled.
	 *
	 * Inputs
	 *	buf		A pointer to the buffer where the data should be written
	 *	len		The length of the buffer. The amount of data the function should
	 *			try to read.
	 *	bytesRead	A pointer to an uint where the amount of bytes read should
	 *				be stored
	 *
	 * Returns
	 *	PS_OK	Some or all data was successfully read.
	 *	CONNECTION_BROKEN	The socket is not connected or a server socket
	 */
	PS_RESULT Read(void *buf, uint len, uint *bytesRead);
	
	/**
	 * Attempt to write data to the socket. All data that can be sent without
	 * blocking will be buffered.
	 *
	 * Inputs
	 *	buf				A pointer to the buffer of data to write
	 *	len				The length of the buffer.
	 *	bytesWritten	A pointer to an uint to store the bytes written
	 *
	 * Returns
	 *	PS_OK	Some or all data was successfully read.
	 *	CONNECTION_BROKEN	The socket is not connected or a server socket
	 */	
	PS_RESULT Write(void *buf, uint len, uint *bytesWritten);

// CALLBACKS

	virtual void OnRead()=0;
	virtual void OnWrite()=0;

	/**
	 * The socket has been closed. It is not certain that the error code
	 * provides meaningful diagnostics. CONNECTION_BROKEN is the generic catch-
	 * all for erroneous closures, PS_OK for clean closures.
	 *
	 * Inputs
	 *	errorCode	The reason for closure.
	 */	
	virtual void OnClose(PS_RESULT errorCode)=0;
};

#endif