/*
 * wxJavaScript - main.h
 *
 * Copyright (c) 2002-2007 Franky Braem and the wxJavaScript project
 *
 * Project Info: http://www.wxjavascript.net or http://wxjs.sourceforge.net
 *
 * This library is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the License, or
 * (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE. See the GNU Lesser General Public
 * License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301,
 * USA.
 *
 * $Id: main.h 714 2007-05-16 20:24:49Z fbraem $
 */
#ifndef _wxJS_H
#define _wxJS_H

#ifdef _MSC_VER
	// Turn off identifier was truncated warning
	#pragma warning(disable:4786)
	// Turn off deprecated warning
	#pragma warning(disable:4996)
#endif 

#include <js/jsapi.h>

#include "defs.h"
#include "clntdata.h"
#include "apiwrap.h"
#include "type.h"

static const int WXJS_CONTEXT_SIZE = 32768;
static const int WXJS_START_PROPERTY_ID = -128;

#endif //_wxJS_H
