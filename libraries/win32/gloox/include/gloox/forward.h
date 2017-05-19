/*
  Copyright (c) 2013-2017 by Jakob Schröter <js@camaya.net>
  This file is part of the gloox library. http://camaya.net/gloox

  This software is distributed under a license. The full license
  agreement can be found in the file LICENSE in this distribution.
  This software may not be copied, modified, sold or distributed
  other than expressed in the named license agreement.

  This software is distributed without any warranty.
*/


#ifndef FORWARD_H__
#define FORWARD_H__

#include "stanzaextension.h"

#include <string>

namespace gloox
{

  class DelayedDelivery;
  class Stanza;

  /**
   * @brief This is an implementation of Stanza Forwarding (@xep{0297}) as a StanzaExtension.
   *
   * @note At this point, Forward can only hold forwarded Messages, not IQ or Presence.
   * However, Forward can be used inside any type of stanza (&lt;message&gt;, &lt;iq&gt;,
   * or &lt;presence&gt;).
   * 
   * XEP-Version: 0.5
   *
   * @author Jakob Schröter <js@camaya.net>
   * @author Fernando Sanchez
   * @since 1.0.5
   */
  class GLOOX_API Forward : public StanzaExtension
  {
    public:

      /**
       * Creates a forwarding StanzaExtension, embedding the given Stanza and DelayedDelivery objects.
       * @param stanza The forwarded Stanza. This Forward instance will own the Stanza object.
       * @param delay The date/time the forwarded stanza was received at by the forwarder. This
       * Forward instance will own the DelayedDelivery object.
       */
      Forward( Stanza* stanza, DelayedDelivery* delay );
      
      /**
       * Creates a forwarding Stanza from the given Tag. The original Tag will be ripped off.
       * If a valid Stanza is conatined (as a child) in the Tag it will be parsed, too.
       * It can then be accessed through embeddedStanza(). The Tag that the Stanza was built from
       * is available through embeddedTag().
       * @param tag The Tag to parse.
       */
      Forward( const Tag* tag = 0 );

      /**
       * Virtual destructor.
       */
      virtual ~Forward();

      /**
       * This function returns a pointer to a DelayedDelivery StanzaExtension which indicates
       * when the forwarder originally received the forwarded stanza.
       * 
       * @return A pointer to a DelayedDelivery object. May be 0.
       */
      const DelayedDelivery* when() const { return m_delay; }

      // reimplemented from StanzaExtension
      virtual Stanza* embeddedStanza() const { return m_stanza; }
      
      // reimplemented from StanzaExtension
      virtual Tag* embeddedTag() const { return m_tag; }
      
      // reimplemented from StanzaExtension
      virtual Tag* tag() const;

      // reimplemented from StanzaExtension
      const std::string& filterString() const;
      
      // reimplemented from StanzaExtension
      StanzaExtension* newInstance( const Tag* tag ) const
      {
        return new Forward( tag );
      }

      // reimplemented from StanzaExtension
      StanzaExtension* clone() const;

    private:
      Stanza* m_stanza;
      Tag* m_tag;
      DelayedDelivery* m_delay;

  };

}

#endif // FORWARD_H__
