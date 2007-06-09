/*
 * wxJavaScript - wxjs_ext.h
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
 * $Id: wxjs_ext.h 598 2007-03-07 20:13:28Z fbraem $
 */
#ifndef _wxjs_ext_h
#define _wxjs_ext_h

#include <js/jsapi.h>

#include <wx/dlimpexp.h>

#ifdef WXJSDLL_BUILD
	#define WXJSAPI WXEXPORT
#else 
	#define WXJSAPI WXIMPORT
#endif

WXJSAPI bool wxJS_EXTInit(JSContext *cx, JSObject *global);
WXJSAPI bool wxJS_EXTInitClass(JSContext *cx, JSObject *obj);
WXJSAPI void wxJS_EXTDestroy();

namespace wxjs
{
    namespace ext
    {
        wxMemoryBuffer* NewMemoryBuffer(void *buffer, int size);
        JSObject *CreateMemoryBuffer(JSContext *cx, void *buffer, int size);
        wxMemoryBuffer* GetMemoryBuffer(JSContext *cx, JSObject *obj);
    };
};

#endif // _wxjs_ext_h
