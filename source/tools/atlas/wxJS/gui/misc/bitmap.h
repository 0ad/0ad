/*
 * wxJavaScript - bitmap.h
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
 * $Id: bitmap.h 598 2007-03-07 20:13:28Z fbraem $
 */
#ifndef _WXJSBitmap_H
#define _WXJSBitmap_H

/////////////////////////////////////////////////////////////////////////////
// Name:        bitmap.h
// Purpose:		Ports wxBitmap to JavaScript
// Author:      Franky Braem
// Modified by:
// Created:     09.08.02
// Copyright:   (c) 2001-2002 Franky Braem
// Licence:     LGPL
// Since:       version 0.4
/////////////////////////////////////////////////////////////////////////////

namespace wxjs
{
    namespace gui
    {
        class Bitmap : public ApiWrapper<Bitmap, wxBitmap>
        {
        public:

	        static bool GetProperty(wxBitmap *p, JSContext *cx, JSObject *obj, int id, jsval *vp);
	        static bool SetProperty(wxBitmap *p, JSContext *cx, JSObject *obj, int id, jsval *vp);
	        static wxBitmap *Construct(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, bool constructing);

	        enum
	        {
		        P_DEPTH
		        , P_HEIGHT
		        , P_OK
		        , P_WIDTH
	        };

	        WXJS_DECLARE_PROPERTY_MAP()
	        WXJS_DECLARE_METHOD_MAP()

	        static JSBool create(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval);
	        static JSBool loadFile(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval);
        };
    }; // namespace gui
}; // namespace wxjs

#endif //_WXJSBitmap_H
