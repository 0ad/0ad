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


#if !defined( GLOOX_MINIMAL ) || defined( WANT_HINTS )

#ifndef HINT_H__
#define HINT_H__

#include "gloox.h"
#include "stanzaextension.h"

#include <string>
#include <list>

namespace gloox
{
  /**
   * This is an abstraction of Message Processing Hints (@xep{0334}).
   *
   * XEP Version: 0.3.0
   *
   * @author Jakob Schröter <js@camaya.net>
   * @since 1.1
   */
  class GLOOX_API Hint : public StanzaExtension
  {
    public:

      /**
       * The supported Message Processing Hints types according to @xep{0334}.
       */
      enum HintType
      {
        NoPermanentStore,           /**< The message shouldn't be stored in any permanent or semi-permanent
                                     * public or private archive or in logs (such as chatroom logs) */
        NoStore,                    /**< A message containing this hint should not be stored by a server either
                                     * permanently (as for NoPermanentStore) or temporarily, e.g. for later
                                     * delivery to an offline client, or to users not currently present in a
                                     * chatroom. */
        NoCopy,                     /**< Messages with this hint should not be copied to addresses other than
                                     * the one to which it is addressed, for example through Message Carbons
                                     * (@xep{0280}). */
        Store,                      /**< A message containing this hint that is not of type 'error' SHOULD be
                                     * stored by the entity. */
        InvalidType                 /**< Invalid type. Ignored. */
      };

      /**
       * Constructs a new object from the given Tag.
       * @param tag A Tag to parse.
       */
      Hint( const Tag* tag );

      /**
       * Constructs a new Hint object of the given type.
       * @param ml The hint type.
       */
      Hint( HintType type )
        : StanzaExtension( ExtHint ), m_type( type )
      {}

      /**
       * Virtual destructor.
       */
      virtual ~Hint() {}

      /**
       * Lets you retrieve the hint's type.
       * @return The hint's type.
       */
      HintType type() const { return m_type; }

      // reimplemented from StanzaExtension
      virtual const std::string& filterString() const;

      // reimplemented from StanzaExtension
      virtual StanzaExtension* newInstance( const Tag* tag ) const
      {
        return new Hint( tag );
      }

      // reimplemented from StanzaExtension
      Tag* tag() const;

      // reimplemented from StanzaExtension
      virtual Hint* clone() const
      {
        return new Hint( *this );
      }

    private:
      HintType m_type;

  };

}

#endif // HINT_H__

#endif // GLOOX_MINIMAL
