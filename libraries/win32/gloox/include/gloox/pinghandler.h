/*
  Copyright (c) 2004-2019 by Jakob Schröter <js@camaya.net>
  This file is part of the gloox library. http://camaya.net/gloox

  This software is distributed under a license. The full license
  agreement can be found in the file LICENSE in this distribution.
  This software may not be copied, modified, sold or distributed
  other than expressed in the named license agreement.

  This software is distributed without any warranty.
*/



#if !defined( GLOOX_MINIMAL ) || defined( WANT_PING )

#ifndef PINGHANDLER_H__
#define PINGHANDLER_H__

#include "gloox.h"

#include <string>

namespace gloox
{

  /**
   * @brief A virtual interface which can be reimplemented to receive ping stanzas.
   *
   * Derived classes can be registered as PingHandlers with the Client.
   * Upon an incoming Ping or Pong packet @ref handlePing() will be called.
   *
   * @author Jakob Schröter <js@camaya.net>
   * @since 1.1
   */
  class GLOOX_API PingHandler
  {
    public:

      /**
       * The supported ping types.
       */
      enum PingType
      {
        websocketPing = 0,          /**< A WebSocket Ping. */
        websocketPong,              /**< A WebSocket Pong (ping reply). */
        whitespacePing              /**< A whitespace ping. */
      };

      /**
      * Virtual Destructor.
      */
      virtual ~PingHandler() {}

      /**
      * Reimplement this function if you want to be updated on
      * incoming ping or pong notifications.
      * @param body The content of the ping or pong stanza.
      * @since 1.1
      */
      virtual void handlePing( const PingType type, const std::string& body ) = 0;
    
  };

}

#endif // PINGHANDLER_H__

#endif // GLOOX_MINIMAL
