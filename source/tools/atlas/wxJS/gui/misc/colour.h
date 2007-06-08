/*
 * wxJavaScript - colour.h
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
 * $Id: colour.h 598 2007-03-07 20:13:28Z fbraem $
 */
#ifndef _WXJSColour_H
#define _WXJSColour_H

/////////////////////////////////////////////////////////////////////////////
// Name:        colour.h
// Purpose:		Ports wxColour to JavaScript
// Author:      Franky Braem
// Modified by:
// Created:     04.07.02
// Copyright:   (c) 2001-2002 Franky Braem
// Licence:     LGPL
/////////////////////////////////////////////////////////////////////////////


namespace wxjs
{
    namespace gui
    {
        class Colour : public ApiWrapper<Colour, wxColour>
        {
        public:

	        static bool GetProperty(wxColour *p, JSContext *cx, JSObject *obj, int id, jsval *vp);
	        static wxColour *Construct(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, bool constructing);

	        enum
	        {
		        P_RED
		        , P_GREEN
		        , P_BLUE
		        , P_OK
	        };

	        WXJS_DECLARE_PROPERTY_MAP()
	        WXJS_DECLARE_METHOD_MAP()

	        static JSBool set(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval);
        };

        void DefineGlobalColours(JSContext *cx, JSObject *obj);
        void DefineGlobalColour(JSContext *cx, JSObject *obj,
						        const char *name, const wxColour *colour);
    }; // namespace gui
}; // namespace wxjs

#endif //_WXJSColour_H
