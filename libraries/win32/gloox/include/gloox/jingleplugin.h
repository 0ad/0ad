/*
  Copyright (c) 2008-2017 by Jakob Schröter <js@camaya.net>
  This file is part of the gloox library. http://camaya.net/gloox

  This software is distributed under a license. The full license
  agreement can be found in the file LICENSE in this distribution.
  This software may not be copied, modified, sold or distributed
  other than expressed in the named license agreement.

  This software is distributed without any warranty.
*/


#ifndef JINGLEPLUGIN_H__
#define JINGLEPLUGIN_H__

#include "macros.h"
#include "util.h"

#include <string>
#include <list>

namespace gloox
{

  class Tag;

  namespace Jingle
  {

    class Plugin;
    class PluginFactory;

    /**
     * The type of Jingle plugin.
     */
    enum JinglePluginType
    {
      PluginNone,                   /**< Invalid plugin type. */
      PluginContent,                /**< A plugin abstracting a &lt;content&gt; element. May contain further plugins. */
      PluginFileTransfer,           /**< A plugin for File Transfer. */
      PluginICEUDP,                 /**< A plugin for ICE UDP transport negotiation. */
      PluginReason,                 /**< An abstraction of a Jingle (@xep{0166}) session terminate reason. */
      PluginUser                    /**< User-supplied plugins must use IDs above this. Do
                                     * not hard-code PluginUser's value anywhere, it is subject
                                     * to change. */
    };

    /**
     * A list of Jingle plugins.
     */
    typedef std::list<const Plugin*> PluginList;

    /**
     * @brief An abstraction of a Jingle plugin. This is part of Jingle (@xep{0166} et al.)
     *
     * This is the base class for Content and all other pluggable Jingle-related containers, e.g.
     * session information, such as the 'ringing' info in Jingle Audio, or Jingle DTMF, etc.
     *
     * A Plugin abstracts the XML that gets sent and received as part of a Jingle session negotiation.
     *
     * XEP Version: 1.1
     *
     * @author Jakob Schröter <js@camaya.net>
     * @since 1.0.5
     */
    class GLOOX_API Plugin
    {

      friend class PluginFactory;

      public:
        /**
         * Simple initializer.
         */
        Plugin( JinglePluginType type ) : m_factory( 0 ), m_pluginType( type ) {}

        /**
         * Virtual destructor.
         */
        virtual ~Plugin() { util::clearList( m_plugins ) ; }

        /**
         * Adds another Plugin as child.
         * @param plugin A plugin to be embedded. Will be owned by this instance and deleted in the destructor.
         */
        void addPlugin( const Plugin* plugin ) { if( plugin ) m_plugins.push_back( plugin ); }

        /**
         * Finds a Jingle::Plugin of a particular type.
         * @param type JinglePluginType to search for.
         * @return A pointer to the first Jingle::Plugin of the given type, or 0 if none was found.
         */
        const Plugin* findPlugin( int type ) const
        {
          PluginList::const_iterator it = m_plugins.begin();
          for( ; it != m_plugins.end() && (*it)->pluginType() != type; ++it ) ;
          return it != m_plugins.end() ? (*it) : 0;
        }

        /**
         * Finds a Jingle::Plugin of a particular type.
         * Example:
         * @code
         * const MyPlugin* c = plugin.findPlugin<MyPlugin>( PluginMyPlugin );
         * @endcode
         * @param type The plugin type to look for.
         * @return The static_cast' type, or 0 if none was found.
         */
        template< class T >
        inline const T* findPlugin( int type ) const
        {
          return static_cast<const T*>( findPlugin( type ) );
        }

        /**
         * Returns a reference to a list of embedded plugins.
         * @return A reference to a list of embedded plugins.
         */
        const PluginList& plugins() const { return m_plugins; }

        /**
         * Reimplement this function if your plugin wants to add anything to the list of
         * features announced via Disco.
         * @return A list of additional feature strings.
         */
        virtual const StringList features() const { return StringList(); }

        /**
         * Returns an XPath expression that describes a path to child elements of a
         * jingle element that the plugin handles.
         * The result should be a single Tag.
         *
         * @return The plugin's filter string.
         */
        virtual const std::string& filterString() const = 0;

        /**
         * Returns a Tag representation of the plugin.
         * @return A Tag representation of the plugin.
         */
        virtual Tag* tag() const = 0;

        /**
         * Returns a new instance of the same plugin type,
         * based on the Tag provided.
         * @param tag The Tag to parse and create a new instance from.
         * @return The new plugin instance.
         */
        virtual Plugin* newInstance( const Tag* tag ) const = 0;

        /**
         * Creates an identical deep copy of the current instance.
         * @return An identical deep copy of the current instance.
         */
        virtual Plugin* clone() const = 0;

        /**
         * Returns the plugin type.
         * @return The plugin type.
         */
        JinglePluginType pluginType() const { return m_pluginType; }

      protected:
        PluginList m_plugins;
        PluginFactory* m_factory;

      private:
        void setFactory( PluginFactory* factory ) { m_factory = factory; }

        JinglePluginType m_pluginType;

    };

  }

}

#endif // JINGLEPLUGIN_H__
