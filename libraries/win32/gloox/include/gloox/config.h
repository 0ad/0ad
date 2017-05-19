/*
  Copyright (c) 2009-2017 by Jakob Schröter <js@camaya.net>
  This file is part of the gloox library. http://camaya.net/gloox

  This software is distributed under a license. The full license
  agreement can be found in the file LICENSE in this distribution.
  This software may not be copied, modified, sold or distributed
  other than expressed in the named license agreement.

  This software is distributed without any warranty.
*/


#ifndef CONFIG_H__
#define CONFIG_H__

#if ( defined _WIN32 ) && !defined( __SYMBIAN32__ )
# include "../config.h.win"
#elif defined( _WIN32_WCE )
# include "../config.h.win"
#elif defined( __SYMBIAN32__ )
# include "../config.h.symbian"
#else
# include "config.h.unix" // run ./configure to create config.h.unix
#endif

#endif // CONFIG_H__
