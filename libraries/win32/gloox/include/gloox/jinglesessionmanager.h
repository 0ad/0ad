/*
  Copyright (c) 2013-2017 by Jakob Schröter <js@camaya.net>
  This file is part of the gloox library. http://camaya.net/gloox

  This software is distributed under a license. The full license
  agreement can be found in the file LICENSE in this distribution.
  This software may not be copied, modified, sold or distributed
  other than expressed in the named license agreement.

  This software is distributed without any warranty.
*/


#ifndef JINGLESESSIONMANAGER_H__
#define JINGLESESSIONMANAGER_H__

#include "macros.h"
#include "iqhandler.h"
#include "jinglepluginfactory.h"

#include <list>

namespace gloox
{

  class ClientBase;

  namespace Jingle
  {

    class Session;
    class SessionHandler;

    /**
     * @brief The SessionManager is responsible for creating and destroying Jingle sessions, as well as for delegating incoming
     * IQs to their respective sessions. This is part of Jingle (@xep{0166}).
     *
     * @note The classes in the Jingle namespace implement the signaling part of Jingle only.
     * Establishing connections to a remote entity or transfering data outside the XMPP channel
     * is out of scope of gloox.
     *
     * To use Jingle with gloox you should first instantiate a Jingle::SessionManager. The SessionManager will
     * let you create new Jingle sessions and notify the respective handler about incoming Jingle session requests.
     * It will also announce generic Jingle support via Disco. You have to register any
     * @link gloox::Jingle::Plugin Jingle plugins @endlink you want to use using registerPlugin().
     * These will automatically announce any additional features via Disco.
     *
     * Use createSession() to create a new Session.
     *
     * Implement SessionHandler::handleIncomingSession() to receive incoming session requests.
     *
     * Use discardSession() to get rid of a session. Do not delete a session manually.
     *
     * There is no limit to the number of concurrent sessions.
     *
     * @author Jakob Schröter <js@camaya.net>
     * @since 1.0.5
     */
    class GLOOX_API SessionManager : public IqHandler
    {
      public:
        /**
         * Creates new instance. There should be only one SessionManager per ClientBase.
         * @param parent A ClientBase instance used for sending and receiving.
         * @param sh A session handler that will be notified about incoming session requests.
         * Only handleIncomingSession() will be called in that handler.
         */
        SessionManager( ClientBase* parent, SessionHandler* sh );

        /**
         * Virtual destructor.
         */
        virtual ~SessionManager();

        /**
         * Registers an empty Plugin as a template with the manager.
         * @param plugin The plugin to register.
         */
        void registerPlugin( Plugin* plugin );

        /**
         * Lets you create a new Jingle session.
         * @param callee The remote entity's JID.
         * @param handler The handler responsible for handling events assicoated with the new session.
         * @return The new session.
         * @note You should not delete a session yourself. Instead, pass it to discardSession().
         */
        Session* createSession( const JID& callee, SessionHandler* handler );

        /**
         * Removes a given session from the nternal queue and deletes it.
         * @param session The session to delete.
         */
        void discardSession( Session* session );


        // reimplemented from IqHandler
        virtual bool handleIq( const IQ& iq );

        // reimplemented from IqHandler
        virtual void handleIqID( const IQ& /*iq*/, int /*context*/ ) {}

      private:
        typedef std::list<Jingle::Session*> SessionList;

        SessionList m_sessions;
        ClientBase* m_parent;
        SessionHandler* m_handler;
        PluginFactory m_factory;

    };

  }

}

#endif // JINGLESESSIONMANAGER_H__
