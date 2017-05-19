/*
  Copyright (c) 2013-2017 by Jakob Schröter <js@camaya.net>
  This file is part of the gloox library. http://camaya.net/gloox

  This software is distributed under a license. The full license
  agreement can be found in the file LICENSE in this distribution.
  This software may not be copied, modified, sold or distributed
  other than expressed in the named license agreement.

  This software is distributed without any warranty.
*/


#ifndef JINGLEPLUGINFACTORY_H__
#define JINGLEPLUGINFACTORY_H__

#include "jingleplugin.h"
#include "jinglesession.h"

namespace gloox
{

  class Tag;

  namespace Jingle
  {

    /**
     * @brief A factory for which creates Plugin instances based on Tags. This is part of Jingle (@xep{0166}).
     *
     * Used by Jingle::SessionManager. You should not need to use this class directly.
     *
     * @author Jakob Schröter <js@camaya.net>
     * @since 1.0.7
     */
    class PluginFactory
    {
      friend class SessionManager;

      public:
        /**
         * Virtual destructor.
         */
        virtual ~PluginFactory();

        /**
         * Registers an empty Plugin as a template with the factory.
         * @param plugin The plugin to register.
         */
        void registerPlugin( Plugin* plugin );

        /**
         * Based on the template plugins' filter string, this function checks the supplied tag for
         * supported extensions and adds them as new plugins to the supplied Plugin instance.
         * @param plugin The Plugin-derived object that will have the newly created plugins embedded.
         * @param tag The Tag to check for supported extensions.
         */
        void addPlugins( Plugin& plugin, const Tag* tag );

        /**
         * Based on the template plugins' filter string, this function checks the supplied tag for
         * supported extensions and adds them as new plugins to the supplied Jingle instance.
         * @param jingle The Jingle object that will have the newly created plugins embedded.
         * @param tag The Tag to check for supported extensions.
         */
        void addPlugins( Session::Jingle& jingle, const Tag* tag );

      private:
        /**
         * Creates a new instance.
         */
        PluginFactory();

        PluginList m_plugins;

    };

  }

}

#endif // JINGLEPLUGINFACTORY_H__
