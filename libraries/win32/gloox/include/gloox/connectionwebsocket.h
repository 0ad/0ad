/*
 * Copyright (c) 2017-2019 by Jakob Schröter <js@camaya.net>
 * This file is part of the gloox library. http://camaya.net/gloox
 *
 * This software is distributed under a license. The full license
 * agreement can be found in the file LICENSE in this distribution.
 * This software may not be copied, modified, sold or distributed
 * other than expressed in the named license agreement.
 *
 * This software is distributed without any warranty.
 */

#if !defined( GLOOX_MINIMAL ) || defined( WANT_WEBSOCKET )

#ifndef CONNECTIONWEBSOCKET_H__
#define CONNECTIONWEBSOCKET_H__

#include <string>
#include <list>
#include <ctime>

#if defined(_WIN32) || defined (_WIN32_WCE)
#include <windows.h>
#endif

#include "gloox.h"
#include "connectionbase.h"
#include "logsink.h"
#include "taghandler.h"
#include "pinghandler.h"
#include "parser.h"
#include "base64.h"
#include "sha.h"


#ifndef SYSTEM_RANDOM_FILEPATH
#define SYSTEM_RANDOM_FILEPATH "/dev/urandom"
#endif

namespace gloox
{

  /**
   * @brief This class implements XMPP over Websocket (RFC7395).
   *
   * @author Didier Perrotey
   * @author Olivier Tchilinguirian
   * @author Jakob Schröter <js@camaya.net>
   * @since 1.1
   */
  class GLOOX_API ConnectionWebSocket : public ConnectionBase, public ConnectionDataHandler, public TagHandler
  {
    public:
      /**
       * Creates a new WebSocket connection. Usually, you would feed it a ConnectionTLS
       * and/or ConnectionTCPClient as transport connection (1st parameter).
       * The default path part of the URI is @c /xmpp. Use setPath() to change it.
       * @param connection The transport connection.
       * @param logInstance The LogSink to use for logging.
       * @param wsHost The WebSocket host. This will be put in the HTTP request's Host: header. It
       * is not used to resolve hostnames or make an actual connection.
       * @param ph The PingHandler to receive notifications about WebSocket Pings. Not required
       * for full functionality.
       */
      ConnectionWebSocket( ConnectionBase* connection, const LogSink& logInstance, const std::string& wsHost,
                           PingHandler *ph = 0 );

      /**
       * Creates a new WebSocket connection. Usually, you would feed it a ConnectionTLS
       * and/or ConnectionTCPClient as transport connection (2nd parameter).
       * The default path part of the URI is @c /xmpp. Use setPath() to change it.
       * @param cdh The data handler for incoming data.
       * @param connection The transport connection.
       * @param logInstance The LogSink to use for logging.
       * @param wsHost The WebSocket host. This will be put in the HTTP request's Host: header. It
       * is not used to resolve hostnames or make an actual connection.
       * @param ph The PingHandler to receive notifications about WebSocket Pings. Not required
       * for full functionality.
       */
      ConnectionWebSocket( ConnectionDataHandler* cdh, ConnectionBase* connection,
                           const LogSink& logInstance, const std::string& wsHost,
                           PingHandler *ph = 0 );

      /**
       * Virtual destructor.
       */
      virtual ~ConnectionWebSocket();

      /**
       * Sets the WebSocket host.
       * @param wsHost The WebSocket host. This will be put in the HTTP request's Host: header. It
       * is not used to resolve hostnames or make an actual connection.
       */
      void setServer( const std::string& wsHost ) { m_server = wsHost; }

      /**
       * Sets the path part of the URI that is the WebSocket endpoint.
       * @param path The path.
       */
      void setPath( const std::string& path ) { m_path = path; }

      void setMasked( bool bMasked ) { m_bMasked = bMasked; }

      void setFlags( int flags ) { m_flags = flags; }

      // reimplemented from ConnectionBase
      virtual ConnectionError connect( int timeout = -1 );

      // reimplemented from ConnectionBase
      virtual ConnectionError recv( int timeout = -1 );

      // reimplemented from ConnectionBase
      virtual bool send( const std::string& data );

      // reimplemented from ConnectionBase
      virtual bool send( const char* data, const size_t len ) ;

      // reimplemented from ConnectionBase
      virtual ConnectionError receive();

      // reimplemented from ConnectionBase
      virtual void disconnect();

      // reimplemented from ConnectionBase
      virtual void cleanup();

      // reimplemented from ConnectionBase
      virtual void getStatistics( long int& totalIn, long int& totalOut );

      // reimplemented from ConnectionBase
      virtual std::string header( std::string to, std::string xmlns, std::string xmllang );

      // reimplemented from ConnectionDataHandler
      virtual void handleReceivedData( const ConnectionBase* connection, const std::string& data );

      // reimplemented from ConnectionDataHandler
      virtual void handleConnect( const ConnectionBase* connection );

      // reimplemented from ConnectionDataHandler
      virtual void handleDisconnect( const ConnectionBase* connection, ConnectionError reason );

      // reimplemented from ConnectionDataHandler
      virtual ConnectionBase* newInstance() const;

      // reimplemented from TagHandler
      virtual void handleTag( Tag* tag );

//       virtual ConnectionBase* connectionImpl() const { return m_connection ; };

      /**
       * Sends a WebSocket Ping.
       * @param body An optional ping body.
       */
      virtual bool sendPingWebSocketFrame( std::string body = EmptyString );

    private:

      class FrameWebSocket
      {
        public:
          enum wsFrameType
          {
            wsFrameContinuation,
            wsFrameText,
            wsFrameBinary,
            wsFrameClose = 8,
            wsFramePing,
            wsFramePong
          };

          /**
           * Constructs a new FrameWebSocket object
           */
          FrameWebSocket( const LogSink& logInstance, int flags );

          FrameWebSocket( const LogSink& logInstance, const std::string& payload, wsFrameType frameType,
                          bool bMasked, int flags );
     
          /**
           * Virtual destructor.
           */
          virtual ~FrameWebSocket()
          {
            if( m_encodedFrame )
            {
                delete[] m_encodedFrame;
                m_encodedFrame = 0;
            }
          };
      
          void encode( bool addCRLF = false, int reason = 0 );

          void decode( std::string& frameContent );

          unsigned char* getEncodedBuffer() { return m_encodedFrame; };

          bool isMasked() { return m_MASK; };

          unsigned long long getFrameLen();

          unsigned long long getEffectiveLen();

          unsigned long long getOffsetLen();

          /**
           * Check if frame content is full or partial.
           * @return @b True if this frame content is full (no data missing), @b false otherwise.
           */
          bool isFull();

          std::string getPayload() { return m_payload; };

          void getMask( std::string& mask );

          wsFrameType getType() { return m_opcode; }

          bool isUnfragmented() { return m_FIN; }

          void dump();

        private:

          bool m_FIN;
          bool m_RSV1;
          bool m_RSV2;
          bool m_RSV3;
          wsFrameType m_opcode;
          bool m_MASK;
          unsigned long long m_lenPayload;
          unsigned long long m_len;
          unsigned int m_payloadOffset;
          std::string m_payload;
          std::string m_maskingKey;
          unsigned char* m_encodedFrame;
          const LogSink& m_logInstance;
          int m_flags;

      }; // ~FrameWebSocket
      
    enum WebSocketState
    {
      wsStateDisconnected,
      wsStateDisconnecting,
      wsStateConnecting,
      wsStateClientOpeningHandshake,
      wsStateServerOpeningHandshake,
      wsStateClientOpeningXmpp,
      wsStateClientClosingXmpp,
      wsStateServerOpeningXmpp,
      wsStateClientOpeningXmppFragmented,
      wsStateConnected,
      wsStateConnectedFragmented

    };
    
    enum WebSocketStatusCode
    {
      wsStatusNormal = 1000,
      wsStatusAway,
      wsStatusProtocolError,
      wsStatusUnsupportedData,
      wsStatusNotReceived = 1005,
      wsStatusAbnormalClosure,
      wsStatusInvalidData,
      wsStatusPolicyViolation,
      wsStatusMessageTooBig,
      wsStatusMandatoryExtension,
      wsStatusInternalServerError,
      wsStatusTLSHandshakeError = 1015
    };

    //ConnectionWebSocket& operator=( const ConnectionWebSocket& );
    int getRandom( unsigned char* buf, int len );
    void initInstance( ConnectionBase* connection );
    const std::string getHTTPField( const std::string& field );

    bool sendFrame( FrameWebSocket& frame );

    bool sendOpeningHandshake();
    
    bool sendCloseWebSocketFrame( const WebSocketStatusCode status, std::string reason = EmptyString );
    bool sendPongWebSocketFrame( std::string body = EmptyString );

    bool sendOpenXmppStream();
    bool sendCloseXmppStream( ConnectionError err=ConnUserDisconnected );
    bool sendStartXmppStream();
    bool parse( const std::string &data );

    bool checkStreamVersion( const std::string& version );

    ConnectionBase *m_connection;
    const LogSink& m_logInstance;
    
    WebSocketState m_wsstate;
    
    PingHandler* m_pinghandler;

    std::string m_path;   // The path part of the URL that we need to request
    std::string m_buffer;   // Buffer of received data
    std::string m_fragmentBuffer;   // Buffer of fragmented received data
    std::string m_bufferHeader;   // HTTP header of data currently in buffer // FIXME doens't need to be member
    std::string::size_type m_bufferContentLength;   // Length of the data in the current response
    std::string m_sendBuffer;   // Data waiting to be sent
    std::string m_clientOpeningHandshake;

    Parser m_parser;   // Used for parsing XML section of responses
    bool m_streamInitiationValidated;
    bool m_streamClosureValidated;
    bool m_bMasked;
    int m_flags;

#if defined( _WIN32 ) || defined ( _WIN32_WCE )
    HCRYPTPROV m_hCryptProv;
#else
    int m_fd_random;
#endif

  };

}

#endif // CONNECTIONWEBSOCKET_H__

#endif // GLOOX_MINIMAL
