/*
  Copyright (c) 2007-2019 by Jakob Schröter <js@camaya.net>
  This file is part of the gloox library. http://camaya.net/gloox

  This software is distributed under a license. The full license
  agreement can be found in the file LICENSE in this distribution.
  This software may not be copied, modified, sold or distributed
  other than expressed in the named license agreement.

  This software is distributed without any warranty.
*/



#ifndef CONNECTIONBASE_H__
#define CONNECTIONBASE_H__

#include "gloox.h"
#include "connectiondatahandler.h"

#include <string>

namespace gloox
{

  /**
   * @brief An abstract base class for a connection.
   *
   * You should not need to use this class directly.
   *
   * @author Jakob Schröter <js@camaya.net>
   * @since 0.9
   */
  class GLOOX_API ConnectionBase
  {
    public:
      /**
       * Constructor.
       * @param cdh An object derived from @ref ConnectionDataHandler that will receive
       * received data.
       */
      ConnectionBase( ConnectionDataHandler* cdh )
        : m_handler( cdh ), m_state( StateDisconnected ), m_port( -1 ), m_timeout( -1 )
      {}

      /**
       * Virtual destructor.
       */
      virtual ~ConnectionBase() { cleanup(); }

      /**
       * Used to initiate the connection.
       * @param timeout The timeout to use for select() in milliseconds. Default of -1 means blocking.
       * @return Returns the connection state.
       */
      virtual ConnectionError connect( int timeout = -1 ) = 0;

      /**
       * Use this periodically to receive data from the socket.
       * @param timeout The timeout to use for select in microseconds. Default of -1 means blocking.
       * @return The state of the connection.
       */
      virtual ConnectionError recv( int timeout = -1 ) = 0;

      /**
       * Use this function to send a string of data over the wire. The function returns only after
       * all data has been sent.
       * @param data The data to send.
       * @return @b True if the data has been sent (no guarantee of receipt), @b false
       * in case of an error.
       */
      virtual bool send( const std::string& data ) = 0;

      /**
       * Use this function to send an arbitrary chunk of data over the wire. The function returns only after
       * all data has been sent.
       * @param data The data to send.
       * @param len The length of data to send.
       * @return @b True if the data has been sent (no guarantee of receipt), @b false
       * in case of an error.
       */
       virtual bool send( const char* data, const size_t len ) = 0;

      /**
       * Use this function to put the connection into 'receive mode', i.e. this function returns only
       * when the connection is terminated.
       * @return Returns a value indicating the disconnection reason.
       */
      virtual ConnectionError receive() = 0;

      /**
       * Disconnects an established connection. NOOP if no active connection exists.
       */
      virtual void disconnect() = 0;

      /**
       * This function is called after a disconnect to clean up internal state. It is also called by
       * ConnectionBase's destructor.
       */
      virtual void cleanup() {}

      /**
       * Returns the current connection state.
       * @return The state of the connection.
       */
      ConnectionState state() const { return m_state; }

      /**
       * Use this function to register a new ConnectionDataHandler. There can be only one
       * ConnectionDataHandler at any one time.
       * @param cdh The new ConnectionDataHandler.
       */
      void registerConnectionDataHandler( ConnectionDataHandler* cdh ) { m_handler = cdh; }

      /**
       * Sets the server to connect to.
       * @param server The server to connect to. Either IP or fully qualified domain name.
       * @param port The port to connect to.
       */
      void setServer( const std::string &server, int port = -1 ) { m_server = server; m_port = port; }

      /**
       * Returns the currently set server/IP.
       * @return The server host/IP.
       */
      const std::string& server() const { return m_server; }

      /**
       * Returns the currently set port.
       * @return The server port.
       */
      int port() const { return m_port; }

      /**
       * Returns the local port.
       * @return The local port.
       */
      virtual int localPort() const { return -1; }

      /**
       * Returns the open timeout value.
       * If return value is >= 0 this is a non-blocking connection.
       * @return The timeout value.
       */
       int timeout() const { return m_timeout; }

      /**
       * Returns the locally bound IP address.
       * @return The locally bound IP address.
       */
      virtual const std::string localInterface() const { return EmptyString; }

      /**
       * Returns current connection statistics.
       * @param totalIn The total number of bytes received.
       * @param totalOut The total number of bytes sent.
       */
      virtual void getStatistics( long int &totalIn, long int &totalOut ) = 0;

      /**
       * This function returns a new instance of the current ConnectionBase-derived object.
       * The idea is to be able to 'clone' ConnectionBase-derived objects without knowing of
       * what type they are exactly.
       * @return A new Connection* instance.
       */
      virtual ConnectionBase* newInstance() const = 0;

      /**
       * Sends the stream header.
       */
      virtual std::string header( std::string to, std::string xmlns, std::string xmllang )
      {
        std::string head = "<?xml version='1.0' ?>";
        head += "<stream:stream to='" + to + "' xmlns='" + xmlns + "' ";
        head += "xmlns:stream='http://etherx.jabber.org/streams'  xml:lang='" + xmllang + "' ";
        head += "version='" + XMPP_STREAM_VERSION_MAJOR + "." + XMPP_STREAM_VERSION_MINOR + "'>";

        return head;
      }

    protected:
      /** A handler for incoming data and connect/disconnect events. */
      ConnectionDataHandler* m_handler;

      /** Holds the current connection state. */
      ConnectionState m_state;

      /** Holds the server's name/address. */
      std::string m_server;

      /** Holds the port to connect to. */
      int m_port;

       /** If timeout is >= 0 this is a non-blocking connection. */
      int m_timeout;
  };

}

#endif // CONNECTIONBASE_H__
