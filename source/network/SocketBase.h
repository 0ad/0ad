#ifndef INCLUDED_NETWORK_SOCKETBASE
#define INCLUDED_NETWORK_SOCKETBASE

//--------------------------------------------------------
//  Includes / Compiler directives
//--------------------------------------------------------

#include "lib/posix/posix_sock.h"
#include "ps/Pyrogenesis.h"
#include <string.h>
#include "ps/CStr.h"

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
enum ESocketProtocol
{
	// This should be a value that's invalid for most socket functions, so that
	// you don't accidentally use an UNSPEC SocketAddress
	// PF_UNSPEC does not work, since it is accepted by many implementations as
	// a "default" protocol family - whatever that may be
	UNSPEC=((sa_family_t)-1),
	IPv4=PF_INET,
	IPv6=PF_INET6,
	/* More protocols */
};

/**
 * A protocol-independent representation of a socket address. All protocols
 * in the ESocketProtocol enum should have a corresponding member in this union.
 */
// Modifiers Note: Each member must contain a first field, compatible with the
// sin_family field of sockaddr_in. The field contains the ESocketProtocol value
// for the address, and it is returned by GetProtocol()
struct CSocketAddress
{
	union
	{
		sa_family_t m_Family;
		sockaddr_in m_IPv4;
		sockaddr_in6 m_IPv6;
	} m_Union;

	inline ESocketProtocol GetProtocol() const
	{
		return (ESocketProtocol)m_Union.m_Family;
	}

	inline CSocketAddress()
	{
		memset(&m_Union, 0, sizeof(m_Union));
		m_Union.m_Family=UNSPEC;
	}

	/**
	 * Create a wildcard address for the specified protocol with a specified
	 * port.
	 *
	 * @param port The port number, in local byte order
	 * @param proto The protocol to use; default IPv4
	 */
	explicit CSocketAddress(int port, ESocketProtocol proto=IPv4);

	/**
	 * Create an address from a numerical IPv4 address and port, port in local
	 * byte order, IPv4 address as a byte array in written order. The Protocol
	 * of the resulting SocketAddress will be IPv4
	 *
	 * @param address An IPv4 address as a byte array (in written order)
	 * @param port A port number (0-65535) in local byte order.
	 */
	CSocketAddress(u8 address[4], int port);

	/**
	 * Resolve the name using the system name resolution service (i.e. DNS) and
	 * store the resulting address. When multiple addresses are found, the
	 * first result is returned.
	 *
	 * Note that this call will block until the name resolution attempt is
	 * either completed successfully or timed out.
	 *
	 * @param name The name to resolve
	 * @param addr A reference to the variable to hold the address
	 *
	 * @return The result of the operation
	 * @retval PS_OK The hostname was successfully retrieved
	 * @retval NO_SUCH_HOST The hostname was not found
	 */
	static PS_RESULT Resolve(const char *name, int port, CSocketAddress &addr);

	/**
	 * Returns the string representation of the address, i.e. the IP (v4 or v6)
	 * address. Note that the port is not included in the string (mostly due to
	 * the fact that the port representation differs wildly between address
	 * families, and that Resolve does not take port as part of the hostname)
	 */
	CStr GetString() const;

	/**
	 * Returns the port number part of the address
	 */
	int GetPort() const;

	/*
		Create an address pointing to the loopback, with the specified port and
		protocol. Use this with Bind to only listen on the loopback interface.
	*/
	static CSocketAddress Loopback(int port, ESocketProtocol proto=IPv4);
};

/**
 * An enumeration of socket states
 *
 * @see CSocketBase::GetState()
 */
enum ESocketState
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
	SS_CONNECTED,
	/**
	 * The connection has been closed on this end, but the other end might have
	 * sent data that we haven't received yet. The error state will be set to
	 * PS_OK.
	 */
	SS_CLOSED_LOCALLY
};

/**
 * Contains the basic socket I/O abstraction and event callback methods.
 * A CSocketBase can only be instantiated as a subclass, none of the functions
 * are meant to exist as anything other than helper functions for socket
 * classes
 *
 * Any CSocket subclass that can be Accept:ed by a CServerSocket should
 * provide a constructor that takes a CSocketInternal pointer, and hands it to
 * the base class constructor.
 */
class CSocketBase
{
private:
	CSocketInternal *m_pInternal;
	ESocketState m_State;
	PS_RESULT m_Error;
	ESocketProtocol m_Proto;
	bool m_NonBlocking;

	/**
	 * Loop forever, waiting for events and calling the callbacks on sockets,
	 * according to their Op mask. This loop may be aborted by calling
	 * AbortWaitLoop.
	 *
	 * The global lock must be held when calling this function, and will be held
	 * upon return from it.
	 */
	static void RunWaitLoop();

	/**
	 * The network thread entry point. Simply locks the global lock and calls
	 * RunWaitLoop.
	 */
	friend void *WaitLoopThreadMain(void *);

#if OS_WIN
	/**
	 * Used by the winsock AsyncSelect windowproc
	 */
	friend void WaitLoop_SocketUpdateProc(int fd, int error, int eventmask);

#else
	// These are utility functions for the unix select loop. Dox can be found in
	// the source file.
	static bool ConnectError(CSocketBase *);
	static void SocketWritable(CSocketBase *);
	static void SocketReadable(CSocketBase *);
#endif
		
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
	/**
	 * Initialize a CSocketBase from a CSocketInternal pointer. Use in OnAccept
	 * callbacks to create an object of your subclass. This constructor should
	 * be overloaded by any subclass that may be Accept:ed.
	 */
	CSocketBase(CSocketInternal *pInt);
	virtual ~CSocketBase();

	/**
	 * Get the op mask for the socket.
	 */
	int GetOpMask();
	
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
	void SetOpMask(int ops);

public:
	/**
	 *	These values are bitwise or-ed to produce op masks
	 */
	enum Ops
	{
		/**
		 * Call OnRead() on a stream socket when there is data to read from the
		 * socket, or OnAccept() on a server socket when there are incoming
		 * connections pending
		 */
		READ=1,
		// Call OnWrite() when there is space available in the socket's output
		// buffer. Has no effect on server sockets.
		WRITE=2
	};

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
	 * Forcibly shuts down the network wait loop. This should happen
	 * automatically as soon as all sockets are closed.
	 */
	static void Shutdown();
	
	/**
	 * Returns the protocol set by Initialize. All SocketAddresses used with
	 * the socket must have the same SocketProtocol
	 */
	inline ESocketProtocol GetProtocol() const
	{ return m_Proto; }

	/**
	 * Destroy the OS socket. If the socket is not cleanly closed before, it
	 * will be forcefully closed by calling this method.
	 */
	void Destroy();

	/**
	 * Close the socket. No more data can be sent over the socket, but any data
	 * pending from the remote host will still be received, and the OnRead
	 * callback called (if the socket's op mask has the READ bit set). Note
	 * that the socket isn't actually closed until the remote end calls
	 * Close on the corresponding remote socket, upon which the OnClose
	 * callback is called with a status code of PS_OK.
	 */
	void Close();

	/**
	 * Create the OS socket for the specified protocol type.
	 */
	PS_RESULT Initialize(ESocketProtocol proto=IPv4);

	/**
	 * Connect the socket to the specified address. The socket must be
	 * initialized for the protocol of the address.
	 *
	 * @param addr The address to connect to
	 * @see SocketAddress::Resolve
	 */
	PS_RESULT Connect(const CSocketAddress &addr);

	/** @name Functions for Server Sockets */
	//@{

	/**
	 * Bind the socket to the specified address and start listening for
	 * incoming connections. You must initialize the socket for the correct
	 * SocketProtocol before calling Bind.
	 *
	 * @param addr The address to bind to
	 * @see SocketAddress::SocketAddress(int,SocketProtocol)
	 */
	PS_RESULT Bind(const CSocketAddress &addr);

	/**
	 * Store the address of the next incoming connection in the SocketAddress
	 * pointed to by addr. You must then choose whether to accept or reject the
	 * connection by calling Accept or Reject
	 *
	 * @param addr A pointer to a SocketAddress
	 * @return PS_OK or an error code
	 * 
	 * @see Accept(SocketAddress&)
	 * @see Reject()
	 */
	PS_RESULT PreAccept(CSocketAddress &addr);

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

	//@}
	/** @name Status and Options */
	//@{

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
	inline ESocketState GetState() const
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
	const CSocketAddress &GetRemoteAddress();

	//@}
	/** @name Stream I/O */
	//@{

	/**
	 * Attempt to read data from the socket. Any data available without blocking
	 * will be returned. Note that a successful return does not mean that the
	 * whole buffer was filled.
	 *
	 * @param buf A pointer to the buffer where the data should be written
	 * @param len The amount of data that should be read.
	 * @param bytesRead The number of bytes read will be stored in the variable
	 * pointed to by bytesRead
	 *
	 * @retval PS_OK Some or all data was successfully read.
	 * @retval CONNECTION_BROKEN The socket is not connected or a server socket
	 */
	PS_RESULT Read(void *buf, size_t len, size_t *bytesRead);
	
	/**
	 * Attempt to write data to the socket. All data that can be sent without
	 * blocking will be buffered.
	 *
	 * @param buf A pointer to the data that should be written
	 * @param len The length of the buffer.
	 * @param bytesWritten The number of bytes written will be stored in the
	 * variable pointed to by bytesWritten
	 *
	 * @retval PS_OK Some or all data was successfully read.
	 * @retval CONNECTION_BROKEN The socket is not connected or a server socket
	 */	
	PS_RESULT Write(void *buf, size_t len, size_t *bytesWritten);

	//@}
	/** @name Callbacks */
	//@{

	/**
	 * Called by the Network Thread when data is available for reading. Use
	 * SetOpMask with the READ bit set to enable calling of this function.
	 *
	 * For server sockets, "data is available for reading" means "incoming
	 * connections are pending".
	 */
	virtual void OnRead()=0;
	/**
	 * Called by the Network Thread when data can be written to the socket.
	 * Will only be called when the WRITE bit is set in the Op Mask of the
	 * socket.
	 */
	virtual void OnWrite()=0;

	/**
	 * The socket has been closed. It is not certain that the error code
	 * provides meaningful diagnostics. CONNECTION_BROKEN is the generic catch-
	 * all for erroneous closures, PS_OK for clean closures.
	 *
	 * @param errorCode A result code describing the reason why the socket was
	 * closed
	 */	
	virtual void OnClose(PS_RESULT errorCode)=0;
};

#endif
