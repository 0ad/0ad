/*
  Copyright (c) 2012-2017 by Jakob Schröter <js@camaya.net>
  This file is part of the gloox library. http://camaya.net/gloox

  This software is distributed under a license. The full license
  agreement can be found in the file LICENSE in this distribution.
  This software may not be copied, modified, sold or distributed
  other than expressed in the named license agreement.

  This software is distributed without any warranty.
*/

#ifndef LINKLOCALHANDLER_H__
#define LINKLOCALHANDLER_H__

#ifdef HAVE_MDNS

#include "linklocal.h"
#include "macros.h"
#include "gloox.h"

#include <string>

namespace gloox
{

  namespace LinkLocal
  {

//     class Client;

    /**
     * @brief A base class that gets informed about advertised or removed XMPP services on the local network.
     *
     * See @ref gloox::LinkLocal::Manager for more information on how to implement
     * link-local messaging.
     * 
     * @author Jakob Schröter <js@camaya.net>
     * @since 1.0.x
     */
    class GLOOX_API Handler
    {
      public:
        /**
         * Reimplement this function to be notified about services available on (or removed from)
         * the local network.
         * @param services A list of services.
         * @note Make a copy of the service list as the list will not be valid after the function
         * returned.
         */
        virtual void handleBrowseReply( const ServiceList& services ) = 0;

//         /**
//          *
//          */
//         virtual void handleClient( Client* client ) = 0;

    };

  }

}

#endif // HAVE_MDNS

#endif // LINKLOCALHANDLER_H__
