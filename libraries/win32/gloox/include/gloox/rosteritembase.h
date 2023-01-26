/*
  Copyright (c) 2015-2019 by Jakob Schröter <js@camaya.net>
  This file is part of the gloox library. http://camaya.net/gloox

  This software is distributed under a license. The full license
  agreement can be found in the file LICENSE in this distribution.
  This software may not be copied, modified, sold or distributed
  other than expressed in the named license agreement.

  This software is distributed without any warranty.
*/


#ifndef ROSTERITEMBASE_H__
#define ROSTERITEMBASE_H__

#include "gloox.h"
#include "jid.h"
#include "tag.h"

#include <string>
#include <list>


namespace gloox
{

  /**
   * @brief A base class holding some roster item data.
   *
   * You should not need to use this class directly.
   *
   * @author Jakob Schröter <js@camaya.net>
   * @since 1.1
   */
  class GLOOX_API RosterItemBase
  {

    public:
      /**
       * Constructs a new item of the roster.
       * @param jid The JID of the contact.
       * @param name The displayed name of the contact.
       * @param groups A list of groups the contact belongs to.
       */
      RosterItemBase( const JID& jid, const std::string& name,
                      const StringList& groups )
        : m_jid( jid ), m_name( name ), m_groups( groups ), m_changed( false )
      {}

      /**
       * Constructs a new item from the given Tag.
       * @param tag The Tag to parse.
       */
      RosterItemBase( const Tag* tag )
      {
        if( !tag || tag->name() != "item" )
          return;

        m_jid.setJID( tag->findAttribute( "jid" ) );
        m_name = tag->findAttribute( "name" );
        const ConstTagList& g = tag->findTagList( "item/group" );
        ConstTagList::const_iterator it = g.begin();
        for( ; it != g.end(); ++it )
          m_groups.push_back( (*it)->cdata() );
      }

      /**
       * Copy constructor.
       * @param right The RosterItemBase to copy.
       */
      RosterItemBase( const RosterItemBase& right )
        : m_jid( right.m_jid ), m_name( right.m_name ),
          m_groups( right.m_groups ), m_changed( right.m_changed )
      {}

      /**
       * Virtual destructor.
       */
      virtual ~RosterItemBase() {}

      /**
       * Returns the contact's bare JID.
       * @return The contact's bare JID.
       */
      const JID& jid() const { return m_jid; }

      /**
       * Sets the displayed name of a contact/roster item.
       * @param name The contact's new name.
       */
      void setName( const std::string& name )
      {
        m_name = name;
        m_changed = true;
      }

      /**
       * Retrieves the displayed name of a contact/roster item.
       * @return The contact's name.
       */
      const std::string& name() const { return m_name; }

      /**
       * Sets the groups this RosterItem belongs to.
       * @param groups The groups to set for this item.
       */
      void setGroups( const StringList& groups )
      {
        m_groups = groups;
        m_changed = true;
      }

      /**
       * Returns the groups this RosterItem belongs to.
       * @return The groups this item belongs to.
       */
      const StringList& groups() const { return m_groups; }

      /**
       * Retruns a Tag representation of the roster item data.
       * @return A Tag representation.
       */
      virtual Tag* tag() const
      {
        Tag* i = new Tag( "item" );
        i->addAttribute( "jid", m_jid.full() );
        i->addAttribute( "name", m_name );
        StringList::const_iterator it = m_groups.begin();
        for( ; it != m_groups.end(); ++it )
          new Tag( i, "group", (*it) );

        return i;
      }

    protected:
      JID m_jid;
      std::string m_name;
      StringList m_groups;
      bool m_changed;

  };

}

#endif // ROSTERITEMBASE_H__
