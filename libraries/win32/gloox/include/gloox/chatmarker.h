/*
  Copyright (c) 2018-2019 by Jakob Schröter <js@camaya.net>
  This file is part of the gloox library. http://camaya.net/gloox

  This software is distributed under a license. The full license
  agreement can be found in the file LICENSE in this distribution.
  This software may not be copied, modified, sold or distributed
  other than expressed in the named license agreement.

  This software is distributed without any warranty.
*/


#if !defined( GLOOX_MINIMAL ) || defined( WANT_CHATMARKER )

#ifndef CHATMARKER_H__
#define CHATMARKER_H__

#include "gloox.h"
#include "stanzaextension.h"

#include <string>

namespace gloox
{

  class Tag;

  /**
   * @brief An implementation of Chat Markers (@xep{0333}) as a StanzaExtension.
   *
   * XEP Version: 0.3
   *
   * @author Jakob Schröter <js@camaya.net>
   * @since 1.1
   */
  class GLOOX_API ChatMarker : public StanzaExtension
  {
    public:

      /**
       * Constructs a new object from the given Tag.
       * @param tag A Tag to parse.
       */
      ChatMarker( const Tag* tag );

      /**
       * Constructs a new object of the given type.
       * @param type The chat type.
       */
      ChatMarker( ChatMarkerType type )
        : StanzaExtension( ExtChatMarkers ), m_marker( type )
      {}

      /**
       * Virtual destructor.
       */
      virtual ~ChatMarker() {}

      /**
       * Returns the object's type.
       * @return The object's type.
       */
      ChatMarkerType marker() const { return m_marker; }

      // reimplemented from StanzaExtension
      virtual const std::string& filterString() const;

      // reimplemented from StanzaExtension
      virtual StanzaExtension* newInstance( const Tag* tag ) const
      {
        return new ChatMarker( tag );
      }

      // reimplemented from StanzaExtension
      Tag* tag() const;

      // reimplemented from StanzaExtension
      virtual StanzaExtension* clone() const
      {
        return new ChatMarker( *this );
      }

    private:
      ChatMarkerType m_marker;

  };

}

#endif // CHATMARKER_H__

#endif // GLOOX_MINIMAL
