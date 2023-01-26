/*
  Copyright (c) 2004-2019 by Jakob Schröter <js@camaya.net>
  This file is part of the gloox library. http://camaya.net/gloox

  This software is distributed under a license. The full license
  agreement can be found in the file LICENSE in this distribution.
  This software may not be copied, modified, sold or distributed
  other than expressed in the named license agreement.

  This software is distributed without any warranty.
*/


#if !defined( GLOOX_MINIMAL ) || defined( WANT_ROSTER_ITEM_EXCHANGE )

#ifndef ROSTERXITEMDATA_H__
#define ROSTERXITEMDATA_H__

#include "gloox.h"
#include "rosteritembase.h"

#include <string>

namespace gloox
{

  class JID;
  class Tag;
  class RosterItemBase;

  /**
   * @brief An bastraction of a @xep{0144} (Roster Item Exchange) roster item.
   *
   * @author Jakob Schröter <js@camaya.net>
   * @since 1.1
   */
  class GLOOX_API RosterXItemData : public RosterItemBase
  {
    public:
      /**
       * A list of supported actions.
       */
      enum XActions
      {
        XAdd,                           /**< Addition suggested. */
        XDelete,                        /**< Deletion suggested. */
        XModify,                        /**< Modification suggested. */
        XInvalid                        /**< Invalid action. */
      };

      /**
       * Creates a new item with the given data.
       * @param action The suggested action.
       * @param jid The contact's JID.
       * @param name The contact's name.
       * @param group A list of groups the contact belongs to.
       */
      RosterXItemData( XActions action, const JID& jid, const std::string& name,
                       const StringList& groups );

      /**
       * Creates a new item from the given Tag.
       * @param tag The Tag to parse.
       */
      RosterXItemData( const Tag* tag );

      /**
       * Virtual destructor.
       */
      virtual ~RosterXItemData() {}

      /**
       * Returns the suggested action.
       * @return The suggested action.
       */
      XActions action() const { return m_action; }

      // reimplemented from RosterItemBase
      virtual Tag* tag() const;

    private:
      XActions m_action;

  };

}

#endif // ROSTERXITEMDATA_H__

#endif // GLOOX_MINIMAL
