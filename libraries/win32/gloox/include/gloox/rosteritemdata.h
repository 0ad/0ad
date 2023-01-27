/*
  Copyright (c) 2004-2019 by Jakob Schröter <js@camaya.net>
  This file is part of the gloox library. http://camaya.net/gloox

  This software is distributed under a license. The full license
  agreement can be found in the file LICENSE in this distribution.
  This software may not be copied, modified, sold or distributed
  other than expressed in the named license agreement.

  This software is distributed without any warranty.
*/


#ifndef ROSTERITEMDATA_H__
#define ROSTERITEMDATA_H__

#include "rosterxitemdata.h"
#include "rosteritembase.h"
#include "gloox.h"
#include "jid.h"
#include "tag.h"

#include <string>
#include <list>


namespace gloox
{

  /**
   * @brief A class holding (some more) roster item data.
   *
   * You should not need to use this class directly.
   *
   * @author Jakob Schröter <js@camaya.net>
   * @since 1.0
   */
  class GLOOX_API RosterItemData : public RosterItemBase
  {

    public:
      /**
       * Constructs a new item of the roster.
       * @param jid The JID of the contact.
       * @param name The displayed name of the contact.
       * @param groups A list of groups the contact belongs to.
       */
      RosterItemData( const JID& jid, const std::string& name,
                      const StringList& groups )
        : RosterItemBase( jid, name, groups ),
          m_subscription( S10nNone ), m_remove( false )
      {}

      /**
       * Constructs a new item of the roster, scheduled for removal.
       * @param jid The JID of the contact to remove.
       */
      RosterItemData( const JID& jid )
        : RosterItemBase( jid, EmptyString, StringList() ),
          m_subscription( S10nNone ), m_remove( true )
      {}

      /**
       * Copy constructor.
       * @param right The RosterItemData to copy.
       */
      RosterItemData( const RosterItemData& right )
        : RosterItemBase( right ),
          m_subscription( right.m_subscription ), m_remove( right.m_remove )
      {}

      /**
       * Virtual destructor.
       */
      virtual ~RosterItemData() {}

      /**
       * Sets the current subscription status of the contact.
       * @param subscription The current subscription.
       * @param ask Whether a subscription request is pending.
       */
      void setSubscription( const std::string& subscription, const std::string& ask )
      {
        m_sub = subscription.empty() ? "none" : subscription;
        m_ask = ask;

        if( m_sub == "from" && ask.empty() )
          m_subscription = S10nFrom;
        else if( m_sub == "from" && !ask.empty() )
          m_subscription = S10nFromOut;
        else if( m_sub == "to" && ask.empty() )
          m_subscription = S10nTo;
        else if( m_sub == "to" && !ask.empty() )
          m_subscription = S10nToIn;
        else if( m_sub == "none" && ask.empty() )
          m_subscription = S10nNone;
        else if( m_sub == "none" && !ask.empty() )
          m_subscription = S10nNoneOut;
        else if( m_sub == "both" )
          m_subscription = S10nBoth;
      }

      /**
       * Returns the current subscription type between the remote and the local entity.
       * @return The subscription type.
       */
      SubscriptionType subscription() const { return m_subscription; }

      /**
       * Whether the item has unsynchronized changes.
       * @return @b True if the item has unsynchronized changes, @b false otherwise.
       */
      bool changed() const { return m_changed; }

      /**
       * Whether the item is scheduled for removal.
       * @return @b True if the item is subject to a removal or scheduled for removal, @b false
       * otherwise.
       */
      bool remove() const { return m_remove; }

      /**
       * Removes the 'changed' flag from the item.
       */
      void setSynchronized() { m_changed = false; }

      /**
       * Retruns a Tag representation of the roster item data.
       * @return A Tag representation.
       */
      virtual Tag* tag() const
      {
        Tag* i = RosterItemBase::tag();
        if( m_remove )
          i->addAttribute( "subscription", "remove" );
        else
        {
          i->addAttribute( "subscription", m_sub );
          i->addAttribute( "ask", m_ask );
        }
        return i;
      }

    protected:
      SubscriptionType m_subscription;
      std::string m_sub;
      std::string m_ask;
      bool m_remove;

  };

}

#endif // ROSTERITEMDATA_H__
