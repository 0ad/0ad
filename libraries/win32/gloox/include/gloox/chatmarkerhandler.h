/*
  Copyright (c) 2005-2019 by Jakob Schröter <js@camaya.net>
  This file is part of the gloox library. http://camaya.net/gloox

  This software is distributed under a license. The full license
  agreement can be found in the file LICENSE in this distribution.
  This software may not be copied, modified, sold or distributed
  other than expressed in the named license agreement.

  This software is distributed without any warranty.
*/


#if !defined( GLOOX_MINIMAL ) || defined( WANT_CHATMARKER )

#ifndef CHATMARKERHANDLER_H__
#define CHATMARKERHANDLER_H__

#include "gloox.h"

namespace gloox
{

  class JID;

  /**
   * @brief A virtual interface that enables an object to be notified about incoming
   * Chat Markers (@xep{0333}).
   *
   * @author Jakob Schröter <js@camaya.net>
   * @since 1.1
   */
  class GLOOX_API ChatMarkerHandler
  {
    public:
      /**
       * Virtual Destructor.
       */
      virtual ~ChatMarkerHandler() {}

      /**
       * Notifies the ChatMarkerHandler that a message has been marked by the remote
       * contact. The occurrence of the 'markable' tag does not cause this handler to be called.
       * @param from The originator of the Event.
       * @param marker The marker type.
       * @param id The marked message's ID.
       */
      virtual void handleChatMarker( const JID& from, ChatMarkerType type, const std::string& id ) = 0;

  };

}

#endif // CHATMARKERHANDLER_H__

#endif // GLOOX_MINIMAL
