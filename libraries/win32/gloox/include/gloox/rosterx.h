/*
  Copyright (c) 2015-2019 by Jakob Schröter <js@camaya.net>
  This file is part of the gloox library. http://camaya.net/gloox

  This software is distributed under a license. The full license
  agreement can be found in the file LICENSE in this distribution.
  This software may not be copied, modified, sold or distributed
  other than expressed in the named license agreement.

  This software is distributed without any warranty.
*/

#if !defined( GLOOX_MINIMAL ) || defined( WANT_ROSTER_ITEM_EXCHANGE )

#ifndef ROSTERX_H__
#define ROSTERX_H__

#include "gloox.h"
#include "stanzaextension.h"
#include "rosterxitemdata.h"

#include <string>
#include <list>

namespace gloox
{

  class Tag;

  /**
   * A list of Roster Item Exchange items.
   */
  typedef std::list <RosterXItemData*> RosterXItemList;

  /**
   * @brief A Roster Item Exchange (@xep{0144}) abstraction implemented as a StanzaExtension.
   *
   * @xep{0144} defines a protocol for exchanging roster items between entities. To receive
   * items proposed for addition, deletion, or modification by another entity, @link
   * RosterListener::handleRosterItemExchange() RosterListener's handleRosterItemExchange() @endlink
   * has to be implemented. Incoming items should then be treated according to the rules established
   * in @xep{0144}.
   *
   * To send roster items to another entity, an instance of this RosterX class should be created and
   * items added using setItems(), again following the rules established in @xep{0144}.
   * This RosterX should then be added to an IQ or Message stanza (see @xep{0144}) and sent off.
   * 
   * XEP Version: 1.1.1
   *
   * @author Jakob Schröter <js@camaya.net>
   * @since 1.1
   */
  class GLOOX_API RosterX : public StanzaExtension
  {
    public:
      /**
       * Constructs a new object from the given Tag.
       * @param tag The Tag to parse.
       */
      RosterX( const Tag* tag = 0 );

      /**
       * Virtual destructor.
       */
      virtual ~RosterX();

      /**
       * Returns the list of exchanged items.
       * @return The list of exchanged items.
       */
      const RosterXItemList& items() const { return m_list; }

      /**
       * Sets a list of roster items. The objects pointed to by the list are owned by RosterX
       * and will be deleted upon destruction or on any future call of setItems().
       * @param items The list of roster items to set.
       */
      void setItems( const RosterXItemList& items );

      // reimplemented from StanzaExtension
      virtual const std::string& filterString() const;

      // reimplemented from StanzaExtension
      virtual StanzaExtension* newInstance( const Tag* tag ) const
      {
        return new RosterX( tag );
      }

      // reimplemented from StanzaExtension
      virtual Tag* tag() const;

      // reimplemented from StanzaExtension
      virtual StanzaExtension* clone() const;

    private:
      RosterXItemList m_list;

  };

}

#endif // ROSTERX_H__

#endif // GLOOX_MINIMAL
