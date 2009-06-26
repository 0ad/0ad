/*
 * wxJavaScript - sizer.h
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
 * $Id: sizer.h 733 2007-06-05 21:17:25Z fbraem $
 */
#ifndef _WXJSSIZER_H
#define _WXJSSIZER_H

namespace wxjs
{
    namespace gui
    {
        class Sizer : public ApiWrapper<Sizer, wxSizer>
        {
        public:
	        /**
	         * Callback for retrieving properties.
	         * @param vp Contains the value of the property.
	         */
	        static bool GetProperty(wxSizer *p, JSContext *cx, JSObject *obj, int id, jsval *vp);

	        /**
	         * Callback for setting properties
	         * @param vp Contains the new value of the property.
	         */
	        static bool SetProperty(wxSizer *p, JSContext *cx, JSObject *obj, int id, jsval *vp);

	        static JSBool add(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval);
	        static JSBool layout(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval);
	        static JSBool prepend(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval);
	        static JSBool remove(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval);
	        static JSBool setDimension(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval);
	        static JSBool setMinSize(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval);
	        static JSBool setItemMinSize(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval);
	        static JSBool clear(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval);

	        WXJS_DECLARE_PROPERTY_MAP()
	        WXJS_DECLARE_METHOD_MAP()

	        enum
	        { 
	          P_MIN_SIZE
	          , P_POSITION
	          , P_SIZE
	        };
        };
    }; // namespace gui
}; // namespace wxjs

#endif //_WXJSSIZER_H
