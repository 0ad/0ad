/*
  Copyright (c) 2008-2019 by Jakob Schröter <js@camaya.net>
  This file is part of the gloox library. http://camaya.net/gloox

  This software is distributed under a license. The full license
  agreement can be found in the file LICENSE in this distribution.
  This software may not be copied, modified, sold or distributed
  other than expressed in the named license agreement.

  This software is distributed without any warranty.
*/


#if !defined( GLOOX_MINIMAL ) || defined( WANT_JINGLE )

#ifndef JINGLETRANSPORT_H__
#define JINGLETRANSPORT_H__

#include "jingleplugin.h"

#include <string>

namespace gloox
{

  namespace Jingle
  {

    /**
     * @brief An abstract base class of a Jingle Transport. This is part of Jingle (@xep{0166}).
     *
     * You should not need to use this class directly unless you are extending gloox. See
     * @link gloox::Jingle::Session Jingle::Session @endlink for more info on Jingle.
     *
     * XEP Version: 1.1
     *
     * @author Jakob Schröter <js@camaya.net>
     * @since 1.0.5
     */
    class GLOOX_API Transport : public Plugin
    {
      public:
        /**
         * Virtual destructor.
         */
        virtual ~Transport() {}

      protected:
        /** The Transport's namespace. */
//         const std::string m_xmlns;

    };

  }

}

#endif // JINGLETRANSPORT_H__

#endif // GLOOX_MINIMAL
