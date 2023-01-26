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

#ifndef CHATMARKERFILTER_H__
#define CHATMARKERFILTER_H__

#include "messagefilter.h"
#include "gloox.h"

namespace gloox
{

  class Tag;
  class ChatMarkerHandler;
  class MessageSession;
  class Message;

  /**
   * @brief This class adds Chat Markers (@xep{0333}) support to a MessageSession.
   *
   * This implementation of Chat States is fully transparent to the user of the class.
   * If the remote entity does not request chat markers, ChatMarkerFilter will not send
   * any, even if the user requests it. (This is required by the protocol specification.)
   * You MUST annouce this capability by use of Disco (associated namespace is XMLNS_CHAT_STATES).
   * (This is also required by the protocol specification.)
   *
   * @note You must register ChatMarker as a StanzaExtension by calling
   * ClientBase::registerStanzaExtension() for notifications to work.
   *
   * @author Jakob Schröter <js@camaya.net>
   * @since 1.1
   */
  class GLOOX_API ChatMarkerFilter : public MessageFilter
  {
    public:
      /**
       * Constructs a new Chat State filter for a MessageSession.
       * @param parent The MessageSession to decorate.
       */
      ChatMarkerFilter( MessageSession* parent );

      /**
       * Virtual destructor.
       */
      virtual ~ChatMarkerFilter();

      /**
       * Use this function to set a chat marker as defined in @xep{0085}.
       * @note The Spec states that Chat States shall not be sent to an entity
       * which did not request them. Reasonable effort is taken in this function to
       * avoid spurious marker sending. You should be safe to call this even if Message
       * Events were not requested by the remote entity. However,
       * calling setChatState( CHAT_STATE_COMPOSING ) for every keystroke still is
       * discouraged. ;)
       * @param marker The marker to set.
       */
      void setMessageMarkable();

      /**
       * The ChatMarkerHandler registered here will receive Chat States according
       * to @xep{0085}.
       * @param csh The ChatMarkerHandler to register.
       */
      void registerChatMarkerHandler( ChatMarkerHandler* csh )
        { m_chatMarkerHandler = csh; }

      /**
       * This function clears the internal pointer to the ChatMarkerHandler.
       * Chat States will not be delivered anymore after calling this function until another
       * ChatMarkerHandler is registered.
       */
      void removeChatMarkerHandler()
        { m_chatMarkerHandler = 0; }

      // reimplemented from MessageFilter
      virtual void decorate( Message& msg );

      // reimplemented from MessageFilter
      virtual void filter( Message& msg );

    protected:
      /** A handler for incoming chat marker changes. */
      ChatMarkerHandler* m_chatMarkerHandler;

      /** Holds the marker sent last. */
      ChatMarkerType m_lastSent;

      /** Indicates whether or not chat markers are currently enabled. */
      bool m_enableChatMarkers;

  };

}

#endif // CHATMARKERFILTER_H__

#endif // GLOOX_MINIMAL
