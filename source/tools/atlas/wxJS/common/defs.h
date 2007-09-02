/*
 * wxJavaScript - defs.h
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
 * $Id: defs.h 812 2007-07-16 19:38:58Z fbraem $
 */
#ifndef _wxjs_defs_h
#define _wxjs_defs_h

#define wxJS_MAJOR_VERSION      0
#define wxJS_MINOR_VERSION      9
#define wxJS_RELEASE_NUMBER     71
#define wxJS_STR_VERSION        wxT("0.9.71")

// Encoding used internally. SpiderMonkey uses UTF-16
#define wxJS_INTERNAL_ENCODING wxT("UTF-16")
// Default encoding to use when reading files, ...
#define wxJS_EXTERNAL_ENCODING wxT("UTF-8")

#endif // _wxjs_defs_h
