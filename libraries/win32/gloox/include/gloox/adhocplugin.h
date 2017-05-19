/*
  Copyright (c) 2015-2017 by Jakob Schröter <js@camaya.net>
  This file is part of the gloox library. http://camaya.net/gloox

  This software is distributed under a license. The full license
  agreement can be found in the file LICENSE in this distribution.
  This software may not be copied, modified, sold or distributed
  other than expressed in the named license agreement.

  This software is distributed without any warranty.
*/



#ifndef ADHOCPLUGIN_H__
#define ADHOCPLUGIN_H__

#include "gloox.h"
#include "stanzaextension.h"

namespace gloox
{

  class Tag;

  /**
   * @brief A base class for Adhoc Command plugins (DataForm, IO Data, ...).
   *
   * This is just a common base class for abstractions of protocols that can be embedded into Adhoc Commands.
   * You should not need to use this class directly unless you're extending Adhoc Commands further.
   *
   * This class exists purely as an additional abstraction layer, to limit the type of objects that can be
   * added to an Adhoc Command.
   *
   * @author Jakob Schröter <js@camaya.net>
   * @since 1.0.13
   */
  class GLOOX_API AdhocPlugin : public StanzaExtension
  {
    public:

      /**
       * Creazes a new Adhoc Plugin of the given type.
       * @param type This should be a StanzaExtension type, i.e. the type of StanzaExtension the plugin will
       * hold.
       */
      AdhocPlugin( int type ) : StanzaExtension( type ) {}

      /**
       * Virtual destructor.
       */
      virtual ~AdhocPlugin() {}

      /**
       * Converts to  @b true if the plugin is valid, @b false otherwise.
       */
      virtual operator bool() const = 0;

  };

}

#endif // ADHOCPLUGIN_H__
