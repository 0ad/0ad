/*
 *  Copyright (c) 2019 by Jakob Schröter <js@camaya.net>
 *  This file is part of the gloox library. http://camaya.net/gloox
 *
 *  This software is distributed under a license. The full license
 *  agreement can be found in the file LICENSE in this distribution.
 *  This software may not be copied, modified, sold or distributed
 *  other than expressed in the named license agreement.
 *
 *  This software is distributed without any warranty.
 */


#if !defined( GLOOX_MINIMAL ) || defined( WANT_DATAFORM ) || defined( WANT_ADHOC )

#ifndef DATAFORMMEDIA_H__
#define DATAFORMMEDIA_H__

#include "macros.h"

#include <string>

namespace gloox
{

  class Tag;

  /**
   * An implementation of Data Forms Media Element (@xep{0221}).
   * @note So far only a single URI is supported.
   *
   * XEP version: 1.0
   *
   * @author Jakob Schröter <js@camaya.net>
   * @since 1.1
   */
  class GLOOX_API DataFormMedia
  {
    public:
      /**
       * Constructs a new object.
       * @param uri The mandatory URI to the media.
       * @param type The MIME type of the media.
       * @param height Optionally, the display height of the media.
       * @param width Optionally, the display width of the media.
       */
      DataFormMedia( const std::string& uri, const std::string& type, int height = 0, int width = 0 );

      /**
       * Constructs a new object from the given Tag.
       * @param tag The Tag to parse.
       */
      DataFormMedia( const Tag* tag );

      /**
       * Virtual destructor.
       */
      virtual ~DataFormMedia() {}

      /**
       * Creates the XML representation of the object.
       * @return The XML.
       */
      Tag* tag() const;

      /**
       * Returns the URI.
       * @return The URI.
       */
      const std::string& uri() const { return m_uri; }

      /**
       * Returns the MIME type.
       * @return The MIME type.
       */
      const std::string& type() const { return m_type; }

      /**
       * Returns the height.
       * @return The height.
       */
      const int height() const { return m_height; }

      /**
       * Returns the width.
       * @return The width.
       */
      const int width() const { return m_width; }

    private:
      std::string m_uri;
      std::string m_type;
      int m_height;
      int m_width;

  };

}

#endif // DATAFORMMEDIA_H__

#endif // GLOOX_MINIMAL
