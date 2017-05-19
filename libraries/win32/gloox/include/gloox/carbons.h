/*
  Copyright (c) 2013-2017 by Jakob Schröter <js@camaya.net>
  This file is part of the gloox library. http://camaya.net/gloox

  This software is distributed under a license. The full license
  agreement can be found in the file LICENSE in this distribution.
  This software may not be copied, modified, sold or distributed
  other than expressed in the named license agreement.

  This software is distributed without any warranty.
*/


#ifndef CARBONS_H__
#define CARBONS_H__

#include "macros.h"

#include "stanzaextension.h"
#include "tag.h"

namespace gloox
{

  class Forward;

  /**
   * @brief An implementation of Message Carbons (@xep{0280}) as a StanzaExtension.
   *
   * @section enable Enable Mesage Carbons
   *
   * Before using Message Carbons you have to check your server for support of the extension.
   * You can do so using Disco::getDiscoInfo(). You can check the result (in DiscoHandler::handleDiscoInfo())
   * for a feature of @c XMLNS_MESSAGE_CARBONS (use Disco::Info::hasFeature()).
   *
   * If the feature exists, you can enable Message Carbons with the server.
   *
   * @code
   * Client cb( ... );
   * // ...
   *
   * // setup
   * cb.registerStanzaExtension( new Forward() ); // required for Message Carbons support
   * cb.registerStanzaExtension( new Carbons() );
   * // ...
   *
   * // enable Message Carbons
   * IQ iq( IQ::Set, JID() ); // empty JID
   * iq.addExtension( new Carbons( Carbons::Enable ) );
   * cb.send( iq, MyIqHandler, 1 ); // myIqHandler will be notified of the result with the given context ('1' in this case).
   * @endcode
   *
   * @note Once enabled, the server will automatically send all received and sent messages @b of @b type @c Chat to all other Carbons-enabled resources of
   * the current account. You have to make sure that you actually send messages of type @c Chat. The default is currently @c Normal.
   *
   * @section disable Disable Message Carbons
   *
   * Once enabled, you can easily disable Message carbons. The code is almost identical to the code used to enable the feature,
   * except that you use a Carbons::Type of Carbons::Disable when you add the Carbons extension to the IQ:
   * @code
   * iq.addExtension( new Carbons( Carbons::Disable ) );
   * @endcode
   *
   * @section private Prevent carbon copies for a single message
   *
   * To disable carbon copies for a single message, add a Carbons extension of type Private:
   *
   * @code
   * Message msg( Message::Chat, ... );
   * // ...
   * msg.addExtension( new Carbons( Carbons::Private ) );
   * @endcode
   *
   * The server will not copy this message to your other connected resources.
   *
   * @section access Access received carbon copies
   *
   * When receiving a message (sent by either another connected client of the current user, or by a 3rd party), a carbon copy will
   * have the following characteristics:
   * @li The message's @c from attribute will be the @b bare JID of the @b receiving entity.
   * @li The message's @c from attribute will be the @b full JID of the @b receiving entity.
   * @li The message contains a Carbons StanzaExtension. This extension contains the original message with the @b original
   * @c from/to attributes.
   *
   * Some sample code:
   * @code
   * bool Myclass::handleMessage( const Message& msg, MessageSession* )
   * {
   *   if( msg.hasEmbeddedStanza() ) // optional, saves some processing time when there is no Carbons extension
   *   {
   *     const Carbons* carbon = msg.findExtension<const Carbons>( ExtCarbons );
   *     if( carbon && carbon->embeddedStanza() )
   *     {
   *       Message* embeddedMessage = static_cast<Message *>( carbon->embeddedStanza() );
   *     }
   *   }
   * }
   * @endcode
   *
   * You can also determine whether a carbon was sent by a 3rd party or a different client of the current user by checking the return value of Carbons::type().
   * @code
   * Carbons* c = msg.findExtension<...>( ... );
   * // check that c is valid
   *
   * if( c->type() == Carbons::Received )
   *   // Message was sent by a 3rd party
   * else if( c->type() == Carbons::Sent )
   *   // Message was sent by a different client of the current user
   * @endcode
   *
   * XEP Version: 0.8
   *
   * @author Jakob Schröter <js@camaya.net>
   * @since 1.0.5
   */
  class GLOOX_API Carbons : public StanzaExtension
  {
    public:
      /**
       * The types of Message Carbons stanza extensions.
       */
      enum Type
      {
        Received,                   /**< Indicates that the message received has been sent by a third party. */
        Sent,                       /**< Indicates that the message received has been sent by one of the user's own resources. */
        Enable,                     /**< Indicates that the sender wishes to enable carbon copies. */
        Disable,                    /**< Indicates that the sender wishes to disable carbon copies. */
        Private,                    /**< Indicates that the sender does not want carbon copies to be sent for this message. */
        Invalid                     /**< Invalid type. */
      };

      /**
       * Constructs a new Carbons instance of the given type.
       * You should only use the @c Enable, @c Disable and @c Private types.
       * @param type The Carbons type to create.
       */
      Carbons( Type type );

      /**
       * Constructs a new Carbons instance from the given tag.
       * @param tag The Tag to create the Carbons instance from.
       */
      Carbons( const Tag* tag = 0 );

      /**
       * Virtual destructor.
       */
      virtual ~Carbons();

      /**
       * Returns the current instance's type.
       * @return The intance's type.
       */
      Type type() const { return m_type; }

      // reimplemented from StanzaExtension
      virtual Stanza* embeddedStanza() const;

      // reimplemented from StanzaExtension
      virtual Tag* embeddedTag() const;

      // reimplemented from StanzaExtension
      virtual const std::string& filterString() const;

      // reimplemented from StanzaExtension
      virtual StanzaExtension* newInstance( const Tag* tag ) const
      {
        return new Carbons( tag );
      }

      // reimplemented from StanzaExtension
      virtual Tag* tag() const;

      // reimplemented from StanzaExtension
      virtual StanzaExtension* clone() const;

    private:
      Forward* m_forward;
      Type m_type;

  };

}

#endif // CARBONS_H__
