/*
  Copyright (c) 2019 by Jakob Schroeter <js@camaya.net>
  This file is part of the gloox library. http://camaya.net/gloox

  This software is distributed under a license. The full license
  agreement can be found in the file LICENSE in this distribution.
  This software may not be copied, modified, sold or distributed
  other than expressed in the named license agreement.

  This software is distributed without any warranty.
*/


#if !defined( GLOOX_MINIMAL ) || defined( WANT_BOB )

#ifndef BOB_H__
#define BOB_H__

#include "macros.h"

#include "stanzaextension.h"

// #include "bobhandler.h"

#include <string>

namespace gloox
{

  class Tag;

  /**
   * @brief This is an implementation of Bits of Binary (@xep{0231}) as a StanzaExtension.
   *
   * XEP Version: 1.0
   * @author Adrien Destugues
   * @author Jakob Schröter <js@camaya.net>
   * @since 1.1
   */
  class GLOOX_API BOB : public StanzaExtension
  {

    public:
      /**
       * Constructs a new object from the given Tag.
       * @param tag The Tag to parse.
       */
      BOB( const Tag* tag = 0 );

      /**
       * Constructs a new object with the given namespace and priority.
       * @param cid A Content-ID that matches the data below and can be mapped to a cid: URL as specified
       * in RFC 2111 [10]. The 'cid' value SHOULD be of the form algo+hash@bob.xmpp.org,
       * where the "algo" is the hashing algorithm used (e.g., "sha1" for the SHA-1
       * algorithm as specified in RFC 3174) and the "hash" is the hex
       * output of the algorithm applied to the binary data itself.
       * This will be calculated for you using SHA1 if left empty.
       * @param type The value of the 'type' attribute MUST match the syntax specified in RFC 2045.
       * That is, the value MUST include a top-level media type, the "/" character,
       * and a subtype; in addition, it MAY include one or more optional parameters
       * (e.g., the "audio/ogg" MIME type in the example shown below includes a
       * "codecs" parameter as specified in RFC 4281 [14]). The "type/subtype"
       * string SHOULD be registered in the IANA MIME Media Types Registry,
       * but MAY be an unregistered or yet-to-be-registered value.
       * @param data The binary data. It will be Base64-encoded for you.
       * @param maxage A suggestion regarding how long (in seconds) to cache the data;
       * the meaning matches the Max-Age attribute from RFC 2965.
       */
      BOB( const std::string& cid, const std::string& type,
           const std::string& data, int maxage );

      /**
       * Virtual Destructor.
       */
      virtual ~BOB() {}

      /**
       * Returns the binary data.
       * @return The binary data.
       */
      const std::string data() const;

      /**
       * Returns the content ID.
       * @return The content ID.
       */
      const std::string cid() const { return m_cid; }

      /**
       * Returns the content type.
       * @return The content type.
       */
      const std::string& type() const { return m_type; }

      /**
       * Returns the maximum cache time in seconds.
       * @return The maximum cache time in seconds.
       */
      int maxage() const { return m_maxage; }

      // reimplemented from StanzaExtension
      virtual const std::string& filterString() const;

      // reimplemented from StanzaExtension
      virtual StanzaExtension* newInstance( const Tag* tag ) const
      {
        return new BOB( tag );
      }

      // reimplemented from StanzaExtension
      virtual Tag* tag() const;

      // reimplemented from StanzaExtension
      virtual StanzaExtension* clone() const
      {
        return new BOB( *this );
      }

    private:
      std::string m_cid;
      std::string m_type;
      std::string m_data;
      int m_maxage;

  };

}

#endif // BOB_H__

#endif // GLOOX_MINIMAL
