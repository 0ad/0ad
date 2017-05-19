/*
  Copyright (c) 2012-2017 by Jakob Schröter <js@camaya.net>
  This file is part of the gloox library. http://camaya.net/gloox

  This software is distributed under a license. The full license
  agreement can be found in the file LICENSE in this distribution.
  This software may not be copied, modified, sold or distributed
  other than expressed in the named license agreement.

  This software is distributed without any warranty.
*/


#ifndef LINKLOCALCLIENT_H__
#define LINKLOCALCLIENT_H__

#include "config.h"

#ifdef HAVE_MDNS

#include "client.h"
#include "jid.h"

#include <string>

#include <dns_sd.h>

namespace gloox
{

  class Tag;

  namespace LinkLocal
  {

    /**
     * @brief An implementation of a link-local client.
     *
     * See @ref gloox::LinkLocal::Manager for more information on how to implement
     * link-local messaging.
     *
     * @author Jakob Schröter <js@camaya.net>
     * @since 1.0.x
     */
    class Client : public gloox::Client
    {
      public:
        /**
         * Constructs a new instance.
         * @param jid The local JID to use.
         */
        Client( const JID& jid );

        /**
         * Virtual destructor.
         */
        virtual ~Client();

        /**
         * Internally sets up an already connected connection.
         * @note Use this function only on a Client instance that you created for an @b incoming connection.
         */
        bool connect();

        /**
         * Starts resolving the given service. Use values from Handler::handleBrowseReply().
         * @param service The service to connect to.
         * @param type The service type.
         * @param domain The service's domain.
         * @param iface The network interface the service was found on. May be 0 to try
         * to resolve the service on all available interfaces.
         * @return @b True if resolving the service could be started successfully, @b false otherwise.
         * @note Use this function only for @b outgoing connections.
         */
        bool connect( const std::string& service, const std::string& type, const std::string& domain, int iface = 0 );

        /**
         * Call this periodically to receive data from the underlying socket.
         * @param timeout An optional timeout in microseconds. Default of -1 means blocking
         * until data was available.
         * @return The state of the underlying connection.
         */
        virtual ConnectionError recv( int timeout = -1 );

        // reimplemented from ConnectionDataHandler, overwriting ClientBase::handleConnect()
        virtual void handleConnect( const ConnectionBase* connection );

      protected:
        // reimplemented from ClientBase
        virtual void handleStartNode( const Tag* start );

      private:
        static void handleResolveReply( DNSServiceRef sdRef, DNSServiceFlags flags, uint32_t interfaceIndex,
                                        DNSServiceErrorType errorCode, const char *fullname, const char *hosttarget,
                                        uint16_t port, uint16_t txtLen, const unsigned char *txtRecord, void *context );
        static void handleQueryReply( DNSServiceRef sdRef, DNSServiceFlags flags, uint32_t interfaceIndex,
                                      DNSServiceErrorType errorCode, const char *fullname, uint16_t rrtype,
                                      uint16_t rrclass, uint16_t rdlen, const void *rdata, uint32_t ttl,
                                      void *context );

        bool resolve( const std::string& serviceName, const std::string& regtype, const std::string& replyDomain );
        bool query( const std::string& hostname, int port );
        void handleQuery( const std::string& addr );
        void sendStart( const std::string& to );

        DNSServiceRef m_qRef;
        DNSServiceRef m_rRef;
        DNSServiceRef m_currentRef;

        std::string m_to;

        int m_interface;
        int m_port;

        bool m_streamSent;

    };

  }

}

#endif // HAVE_MDNS

#endif // LINKLOCALCLIENT_H__
