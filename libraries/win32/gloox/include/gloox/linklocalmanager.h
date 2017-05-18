/*
  Copyright (c) 2012-2017 by Jakob Schröter <js@camaya.net>
  This file is part of the gloox library. http://camaya.net/gloox

  This software is distributed under a license. The full license
  agreement can be found in the file LICENSE in this distribution.
  This software may not be copied, modified, sold or distributed
  other than expressed in the named license agreement.

  This software is distributed without any warranty.
*/

#ifndef LINKLOCALMANAGER_H___
#define LINKLOCALMANAGER_H___

#include "config.h"

#ifdef HAVE_MDNS

#include "linklocal.h"
#include "macros.h"
#include "gloox.h"
#include "util.h"
#include "logsink.h"
#include "connectiontcpserver.h"
#include "mutex.h"
#include "jid.h"

#include <string>

#include <dns_sd.h>

namespace gloox
{

  class ConnectionHandler;
  class ConnectionTCPClient;

  namespace LinkLocal
  {

    class Handler;

    /**
     * @brief This is a manager for server-less messaging (@xep{0174}).
     *
     * Enable compilation of this code with the @c \-\-enable-mdns switch to @c configure, or add
     * @c \#define @c HAVE_MDNS to your platform's @c config.h. @c dns_sd.h, @c libdns_sd.so, as well
     * as the @c mdnsd daemon from Apple's bonjour distribution are required. The @c mdnsd daemon has
     * to be running on the local host.
     *
     * ### Browsing the local network for XMPP services
     *
     * You can use the Manager to browse the local network for XMPP services.
     * First, create a new instance, register a LinkLocal::Handler, and call startBrowsing().
     * @code
     * m_mdns = new LinkLocal::Manager( ... );
     * m_mdns->registerLinkLocalHandler( yourHandler );
     * m_mdns->startBrowsing();
     * @endcode
     *
     * Then you will need to call @c recv() periodcally. The handler will then receive lists of available
     * or removed services. Check the @c flag member of the Service struct.
     *
     * @code
     * void MyClass::handleBrowseReply( const Service& service )
     * {
     *   LinkLocal::ServiceList::const_iterator it = services.begin();
     *   for( ; it != services.end(); ++it )
     *   {
     *     if( (*it).flag == LinkLocal::AddService )
     *     {
     *       // new service
     *     }
     *     else
     *     {
     *       // service removed
     *     }
     *   }
     * }
     * @endcode
     *
     * @note Note that your own service may show up in the list, too.
     *
     * ### Connecting to an XMPP service
     *
     * Using the info from the Service struct you can initiate a connection to the remote entity.
     * First, create a new instance of LinkLocal::Client and register some basic handlers like you
     * would with a normal gloox::Client:
     *
     * @code
     * LinkLocal::Client c( JID( "romeo@montague.net" ) );
     * c.logInstance().registerLogHandler( LogLevelDebug, LogAreaAll, this ); // optional
     * c.registerConnectionListener( yourConnectionListener );
     * @endcode
     *
     * Then call @link gloox::LinkLocal::Client::connect( const std::string&, const std::string&, const std::string&, int ) connect() @endlink
     * and pass the paramters from the Service struct that you received in handleBrowseReply().
     *
     * @code
     * c.connect( "juliet@laptop", "_presence._tcp", ".local", 4 ); // don't use literal values
     * @endcode
     *
     * Put your LinkLocal::Client instance in your event loop (or in a separate thread) and call
     * @link gloox::LinkLocal::Client::recv() recv() @endlink periodically.
     *
     * ### Advertising an XMPP service on the local network
     *
     * To advertise your own XMPP service you can (re-)use the same Manager instance from 'browsing the local network'
     * above.
     *
     * You can publish some basic info about your service in a DNS TXT record. The Manager offers the addTXTData() function
     * for that. See http://xmpp.org/registrar/linklocal.html for a list of official parameters.
     *
     * @code
     * m_mdns->addTXTData("nick","July");
     * m_mdns->addTXTData("1st","Juliet");
     * m_mdns->addTXTData("last","Capulet");
     * m_mdns->addTXTData("msg","Hanging out");
     * m_mdns->addTXTData("jid","julia@capulet.com");
     * m_mdns->addTXTData("status","avail");
     * @endcode
     *
     * Then, to start publishing the availability of your service as well as the TXT record with the additional info
     * you just call @c registerService().
     *
     * @code
     * m_mdns->registerService();
     * @endcode
     *
     * Other entities on the network will now be informed about the availability of your service.
     *
     * ### Listening for incoming connections
     *
     * The second argument to Manager's constructor is a ConnectionHandler-derived class that
     * will receive incoming connections.
     *
     * When registerService() gets called, the Manager will also start a local server that will
     * accept incoming connections. By default, it will listen on port 5562.
     *
     * In @link gloox::ConnectionHandler::handleIncomingConnection() handleIncomingConnection() @endlink
     * you should create a new LinkLocal::Client and register some basic handlers:
     *
     * @code
     * LinkLocal::Client c( JID( "romeo@desktop" ) );
     * c.logInstance().registerLogHandler( LogLevelDebug, LogAreaAll, this );
     * c.registerMessageHandler( this );
     * c.registerConnectionListener( this );
     * @endcode
     *
     * Finally you have to attach the incoming connection to the Client instance, and call connect().
     *
     * @code
     * connection->registerConnectionDataHandler( &c );
     * c.setConnectionImpl( connection );
     * c.connect();
     * @endcode
     *
     * Add the Client to your event loop to call recv() periodically.
     *
     * @see @c linklocal_example.cpp in @c src/examples/ for a (very) simple implementation of a bot
     * handling both incoming and outgoing connections.
     *
     * @author Jakob Schröter <js@camaya.net>
     * @since 1.0.x
     */
    class GLOOX_API Manager
    {

      public:

        /**
         * Creates a new Link-local Manager instance. You can call @c registerService() and/or @c startBrowsing()
         * immediately on a new Manager object, it will use sane defaults.
         * @param user The username to advertise, preferably (as per @xep{0174}) the locally
         * logged-in user. This is just the local part of the local JID.
         * @param connHandler A pointer to a ConnectionHandler that will receive incoming connections.
         * @param logInstance The log target. Obtain it from ClientBase::logInstance().
         */
        Manager( const std::string& user, ConnectionHandler* connHandler, const LogSink &logInstance );

        /**
         * Virtual destructor.
         * @note @c deregisterService() and @c stopBrowsing() will be called automatically if necessary.
         */
        virtual ~Manager();

        /**
         * Lets you add additional data to the published TXT record.
         * @note The @c txtvers=1 parameter is included by default and cannot be changed.
         * @param key The key of a key=value parameter pair. Must be non-empty. If the given key
         * has been set before, its value will be overwritten by the new value.
         * @param value The value of a @c key=value parameter pair. Must be non-empty.
         * @note If either parameter is empty, this function is a NOOP.
         * @note The additional data will not be automatically published if you have already called
         * @c registerService(). You will have to call @c registerService() again to update the
         * TXT record.
         */
        void addTXTData( const std::string& key, const std::string& value );

        /**
         * Lets you remove TXT record data by key.
         * @note The @c txtvers=1 parameter is included by default and cannot be removed.
         * @param key The key of the @c key=value parameter pair that should be removed. Must be non-empty.
         * @note A published TXT record will not be automatically updated if you have already called
         * @c registerService(). You will have to call @c registerService() again to update the TXT record.
         */
        void removeTXTData( const std::string& key );

        /**
         * Starts advertising link-local messaging capabilities by publishing a number of DNS records,
         * as per @xep{0174}.
         * You can call this function again to publish any values you updated after the first call.
         */
        void registerService();

        /**
         * Removes the published DNS records and thereby stops advertising link-local messaging
         * capabilities.
         */
        void deregisterService();

        /**
         * Lets you specify a new username.
         * @param user The new username.
         * @note The new username will not be automatically advertised if you have already called
         * @c registerService(). You will have to call @c registerService() again to update the username.
         */
        void setUser( const std::string& user ) { m_user = user; }

        /**
         * Lets you specify an alternate host name to advertise. By default the local machine's hostname
         * as returned by @c gethostname() will be used.
         * @param host The hostname to use.
         * @note The new hostname will not be automatically advertised if you have already called
         * @c registerService(). You will have to call @c registerService() again to update the hostname.
         */
        void setHost( const std::string& host ) { m_host = host; }

        /**
         * This function lets you set an alternate browse domain. The default domain should work in most cases.
         * @param domain The browse domain to set.
         * @note The new domain will not be automatically used if you have already called
         * @c registerService(). You will have to call @c registerService() again to update the domain.
         * The same applies to @c startBrowsing().
         */
        void setDomain( const std::string& domain ) { m_domain = domain; }

        /**
         * Lets you specifiy a port to listen on for incoming server-less XMPP connections. Default
         * for this implementation is port 5562, but any unused port can be used.
         * @note In addition to the SRV record, the port will also be published in the TXT record
         * automaticlly, you do not have to add it manually. That is, you should not call
         * @c addTXTData() with a key of @c "port.p2pj".
         * @param port The port to use.
         * @note The new port will not be automatically advertised if you have already called
         * @c registerService(). You will have to call @c registerService() again to update the port.
         */
        void setPort( const int port ) { m_port = port; addTXTData( "port.p2pj", util::int2string( m_port ) ); }

        /**
         * This function can be used to set a non-default interface. Use @c if_nametoindex() to
         * find the index for a specific named device.
         * By default DNS records will be broadcast on all available interfaces, and all available
         * interfaces will be used or browsing services.
         * @param iface The interface to use. If you set an interface here, and want to change it
         * back to 'any', use @b 0. Use @b -1 to broadcast only on the local machine.
         * @note The new interface will not be automatically used if you have already called
         * @c registerService(). You will have to call @c registerService() again to use the new
         * interface. The same applies to @c startBrowsing().
         */
        void setInterface( unsigned iface ) { m_interface = iface; }

        /**
         * Starts looking for other entities of type @c _presence. You have to set a handler for
         * results using @c registerLinkLocalHandler() before calling this function.
         * You can call this function again to re-start browsing with updated parameters.
         * @return Returns @b true if browsing could be started successfully, @b false otherwise.
         */
        bool startBrowsing();

        /**
         * Stops the browsing.
         */
        void stopBrowsing();

        /**
         * Exposes the socket used for browsing so you can have it checked in your own event loop,
         * if desired. If your event loop signals new data on the socket, you should NOT
         * try to read from it directly. Instead you should call @c recv().
         * As an alternative to using the raw socket you could also put the Manager in a
         * separate thread and call @c recv() repeatedly, or achieve this in any other way.
         */
        int socket() const { return m_browseFd; }

        /**
         * Checks once for new data on the socket used for browsing.
         * @param timeout The timeout for @c select() in microseconds. Use @b -1 if blocking behavior
         * is acceptable.
         */
        void recv( int timeout );

        /**
         * Registers a handler that will be notfied about entities found on the network that match
         * the given browse criteria.
         * @param handler The handler to register.
         */
        void registerLinkLocalHandler( Handler* handler ) { m_linkLocalHandler = handler; }

//         /**
//          *
//          */
//         static const StringMap decodeTXT( const std::string& txt );

      private:
        static void handleBrowseReply( DNSServiceRef sdRef, DNSServiceFlags flags, unsigned interfaceIndex,
                                       DNSServiceErrorType errorCode, const char* serviceName, const char* regtype,
                                       const char* replyDomain, void* context );

        void handleBrowse( Flag flag, const std::string& service, const std::string& regtype, const std::string& domain, int iface, bool moreComing );

        typedef std::list<ConnectionTCPClient*> ConnectionList;
        typedef std::map<ConnectionTCPClient*, const JID> ConnectionMap;

        DNSServiceRef m_publishRef;
        DNSServiceRef m_browseRef;

        ServiceList m_tmpServices;
//         ServiceList m_services;

        std::string m_user;
        std::string m_host;
        std::string m_domain;
        unsigned m_interface;
        int m_port;

        const LogSink& m_logInstance;

        int m_browseFd;

        StringMap m_txtData;

        ConnectionTCPServer m_server;

        Handler* m_linkLocalHandler;
        ConnectionHandler* m_connectionHandler;

    };

  }

}

#endif // HAVE_MDNS

#endif // LINKLOCALMANAGER_H___
