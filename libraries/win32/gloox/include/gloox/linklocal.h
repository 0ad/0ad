/*
  Copyright (c) 2012-2017 by Jakob Schröter <js@camaya.net>
  This file is part of the gloox library. http://camaya.net/gloox

  This software is distributed under a license. The full license
  agreement can be found in the file LICENSE in this distribution.
  This software may not be copied, modified, sold or distributed
  other than expressed in the named license agreement.

  This software is distributed without any warranty.
*/

#ifndef LINKLOCAL_H__
#define LINKLOCAL_H__

#include "config.h"

#ifdef HAVE_MDNS

#include <string>
#include <list>

namespace gloox
{

  /**
   * @brief Namespace holding all the Link-local-related structures and definitions.
   *
   * See @ref gloox::LinkLocal::Manager for more information on how to implement
   * link-local messaging.
   */
  namespace LinkLocal
  {

    class Client;

    /**
     * Used in conjunction with Service to indicate whether a service has been added (newly advertised) or removed.
     */
    enum Flag
    {
      AddService,            /**< A service has been added. */
      RemoveService          /**< A service has been removed. */
    };

    /**
     * @brief An abstraction of the parameters of a single link-local service.
     *
     * @author Jakob Schröter <js@camaya.net>
     * @since 1.0.x
     */
    struct Service
    {
      friend class Manager;

      private:
        Service( Flag _flag, const std::string& _service, const std::string& _regtype, const std::string& _domain, int _interface )
         : flag( _flag ), service( _service ), regtype( _regtype ), domain( _domain ), iface( _interface ) {}

      public:
        Flag flag;
        std::string service;
        std::string regtype;
        std::string domain;
        int iface;
    };

    /**
     * A list of services.
     */
    typedef std::list<Service> ServiceList;

  }

}

#endif // HAVE_MDNS

#endif // LINKLOCAL_H__
