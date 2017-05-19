/*
  Copyright (c) 2008-2017 by Jakob Schröter <js@camaya.net>
  This file is part of the gloox library. http://camaya.net/gloox

  This software is distributed under a license. The full license
  agreement can be found in the file LICENSE in this distribution.
  This software may not be copied, modified, sold or distributed
  other than expressed in the named license agreement.

  This software is distributed without any warranty.
*/


#ifndef JINGLESESSIONHANDLER_H__
#define JINGLESESSIONHANDLER_H__

#include "macros.h"
#include "jinglesession.h"

namespace gloox
{

  namespace Jingle
  {

    /**
     * @brief A Jingle session handler.
     *
     * @author Jakob Schröter <js@camaya.net>
     * @since 1.0.5
     */
    class GLOOX_API SessionHandler
    {
      public:
        /**
         * Virtual destructor.
         */
        virtual ~SessionHandler() {}

        /**
         * This function is called when the remote party requests an action to be taken.
         * @param action The requested action. A convenience parameter, identical to jingle->action().
         * @param session The affected session.
         * @param jingle The complete Jingle.
         * @note Note that an action can cause a session state change. You may check using session->state().
         * @note Also note that you have to reply to most actions, usually with the *Accept or *Reject counterpart,
         * using the similarly-named functions that Session offers.
         */
        virtual void handleSessionAction( Action action, Session* session, const Session::Jingle* jingle ) = 0;

        /**
         * This function is called when a request to a remote entity returns an error.
         * @param action The Action that failed.
         * @param session The affected session.
         * @param error The error. May be 0 in special cases.
         * @note Note that an action can cause a session state change. You may check using session->state().
         */
        virtual void handleSessionActionError( Action action, Session* session, const Error* error ) = 0;

        /**
         * This function is called if a remote entity wants to establish a Jingle session.
         * @param session The new Jingle session.
         * @note Note that you have to explicitely accept or reject the session by calling either of session->sessionAccept() and
         * session->sessionTerminate(), respectively.
         */
        virtual void handleIncomingSession( Session* session ) = 0;

    };

  }

}

#endif // JINGLESESSIONHANDLER_H__
