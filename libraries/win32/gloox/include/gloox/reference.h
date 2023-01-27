/*
 *  Copyright (c) 2018-2019 by Jakob Schröter <js@camaya.net>
 *  This file is part of the gloox library. http://camaya.net/gloox
 *
 *  This software is distributed under a license. The full license
 *  agreement can be found in the file LICENSE in this distribution.
 *  This software may not be copied, modified, sold or distributed
 *  other than expressed in the named license agreement.
 *
 *  This software is distributed without any warranty.
 */


#if !defined( GLOOX_MINIMAL ) || defined( WANT_REFERENCES )

#ifndef REFERENCE_H__
#define REFERENCE_H__

#include "gloox.h"
#include "stanzaextension.h"

#include <string>
#include <list>

namespace gloox
{
  /**
   * This is an abstraction of References (@xep{0372}).
   *
   * XEP Version: 0.2
   *
   * @author Jakob Schröter <js@camaya.net>
   * @since 1.1
   */
  class GLOOX_API Reference : public StanzaExtension
  {
    public:

      /**
       * The supported Message Markup types according to @xep{0394}.
       */
      enum ReferenceType
      {
        Mention,                    /**< Mentions are a reference to a user's bare JID. */
        Data,                       /**< Data references are a generic reference without additional information. */
        InvalidType                 /**< Invalid/unknown type. Ignored. */
      };

      /**
       * Constructs a new object from the given Tag.
       * @param tag A Tag to parse.
       */
      Reference( const Tag* tag );

      /**
       * Constructs a new Reference object with the given markups.
       * @param type The reference type.
       * @param uri The referencing URI.
       */
      Reference( ReferenceType type, const std::string& uri )
        : StanzaExtension( ExtReference ), m_type( type ), m_uri( uri )
      {}

      /**
       * Virtual destructor.
       */
      virtual ~Reference() {}

      /**
       * Sets the start position of the @c mention. See @xep{0372}.
       * @param begin The start position of the @c mention.
       */
      void setBegin( const int begin ) { m_begin = begin; }

      /**
       * Returns the start position of the mention.
       * @return The start position of the mention.
       */
      int begin() const { return m_begin; }

      /**
       * Sets the end position of the @c mention. See @xep{0372}.
       * @param end The end position of the @c mention.
       */
      void setEnd( const int end ) { m_end = end; }

      /**
       * Returns the end position of the mention. See @xep{0372}.
       * @return The end position of the mention.
       */
      int end() const { return m_end; }

      /**
       * An anchor, i.e. an uri to a previous message. See @xep{0372}.
       * @param anchor The anchor.
       */
      void setAnchor( const std::string& anchor ) { m_anchor = anchor; }

      /**
       * Returns the anchor. See @xep{0372}.
       * @return The anchor.
       */
      const std::string& anchor() const { return m_anchor; }

      /**
       * Returns the URI. See @xep{0372}.
       * @return The URI.
       */
      const std::string& uri() const { return m_uri; }

      /**
       * Returns the reference type. See @xep{0372}.
       * @return The reference type.
       */
      ReferenceType type() const { return m_type; }

      // reimplemented from StanzaExtension
      virtual const std::string& filterString() const;

      // reimplemented from StanzaExtension
      virtual StanzaExtension* newInstance( const Tag* tag ) const
      {
        return new Reference( tag );
      }

      // reimplemented from StanzaExtension
      Tag* tag() const;

      // reimplemented from StanzaExtension
      virtual Reference* clone() const
      {
        return new Reference( *this );
      }

    private:
      ReferenceType m_type;
      std::string m_uri;
      std::string m_anchor;
      int m_begin;
      int m_end;

  };

}

#endif // REFERENCE_H__

#endif // GLOOX_MINIMAL
