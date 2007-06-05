/*
 * wxJavaScript - jsmembuf.h
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
 * $Id: jsmembuf.h 598 2007-03-07 20:13:28Z fbraem $
 */
#ifndef _WXJS_MEMBUF_H
#define _WXJS_MEMBUF_H

/////////////////////////////////////////////////////////////////////////////
// Name:        jsmembuf.h
// Purpose:		Ports wxMemoryBuffer to JavaScript
// Author:      Franky Braem
// Modified by:
// Created:     02.12.2005
// Copyright:   (c) 2001-2005 Franky Braem
// Licence:     LGPL
/////////////////////////////////////////////////////////////////////////////

#include <wx/buffer.h>

namespace wxjs
{
    namespace ext
    {
        class MemoryBuffer : public ApiWrapper<MemoryBuffer, wxMemoryBuffer>
        {
        public:
        	
	        static bool GetProperty(wxMemoryBuffer *p, JSContext *cx, JSObject *obj, int id, jsval *vp);
	        static bool SetProperty(wxMemoryBuffer *p, JSContext *cx, JSObject *obj, int id, jsval *vp);

	        enum
	        { 
		        P_DATA_LENGTH = WXJS_START_PROPERTY_ID,
		        P_LENGTH,
		        P_IS_NULL
	        };

	        WXJS_DECLARE_PROPERTY_MAP()
	        WXJS_DECLARE_METHOD_MAP()

	        static wxMemoryBuffer *Construct(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, bool constructing);

	        static JSBool append(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval);
	        static JSBool toString(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval);
        };
    }; // namespace ext
}; // namespace wxjs
#endif
