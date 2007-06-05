/*
 * wxJavaScript - font.h
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
 * $Id: font.h 598 2007-03-07 20:13:28Z fbraem $
 */
#ifndef _WXJSFont_H
#define _WXJSFont_H

/////////////////////////////////////////////////////////////////////////////
// Name:        font.h
// Purpose:		Font ports wxFont to JavaScript.
// Author:      Franky Braem
// Modified by:
// Created:     05.07.02
// Copyright:   (c) 2001-2002 Franky Braem
// Licence:     LGPL
/////////////////////////////////////////////////////////////////////////////
namespace wxjs
{
    namespace gui
    {
        class Font : public ApiWrapper<Font, wxFont>
        {
        public:
	        static bool GetProperty(wxFont *p, JSContext *cx, JSObject *obj, int id, jsval *vp);
	        static bool SetProperty(wxFont *p, JSContext *cx, JSObject *obj, int id, jsval *vp);

            static bool GetStaticProperty(JSContext *cx, int id, jsval *vp);
            static bool SetStaticProperty(JSContext *cx, int id, jsval *vp);

	        static wxFont* Construct(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, bool constructing);

	        WXJS_DECLARE_PROPERTY_MAP()
	        WXJS_DECLARE_STATIC_PROPERTY_MAP()
	        WXJS_DECLARE_CONSTANT_MAP()

	        /**
	         * Property Ids.
	         */
	        enum
	        {
		        P_DEFAULT_ENCODING
		        , P_FACE_NAME
		        , P_FAMILY
		        , P_POINT_SIZE
		        , P_STYLE
		        , P_UNDERLINED
		        , P_WEIGHT
                , P_OK
	        };
        };
    }; // namespace gui
}; // namespace wxjs

#endif //_WXJSFont_H
