/*
 * wxJavaScript - coldata.h
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
 * $Id: coldata.h 672 2007-04-12 20:29:39Z fbraem $
 */
#ifndef _WXJSColourData_H
#define _WXJSColourData_H

namespace wxjs
{
    namespace gui
    {
        class ColourData : public ApiWrapper<ColourData, wxColourData>
        {
        public:

	        static bool GetProperty(wxColourData *p,
                                    JSContext *cx, 
                                    JSObject *obj, 
                                    int id, 
                                    jsval *vp);
	        static bool SetProperty(wxColourData *p, 
                                    JSContext *cx,
                                    JSObject *obj,
                                    int id,
                                    jsval *vp);

	        static wxColourData* Construct(JSContext *cx,
                                           JSObject *obj,
                                           uintN argc,
                                           jsval *argv,
                                           bool constructing);

	        WXJS_DECLARE_PROPERTY_MAP()

	        /**
	         * Property Ids.
	         */
	        enum
	        {
		        P_CHOOSE_FULL
		        , P_COLOUR
		        , P_CUSTOM_COLOUR
	        };
        };

        class CustomColour : public ApiWrapper<CustomColour, Index>
        {
        public:

            static bool GetProperty(Index *p, 
                                    JSContext *cx,
                                    JSObject *obj,
                                    int id,
                                    jsval *vp);
	        static bool SetProperty(Index *p,
                                    JSContext *cx,
                                    JSObject *obj,
                                    int id,
                                    jsval *vp);
        };
    }; // namespace gui
}; // namespace wxjs
#endif //_WXJSColourData_H
