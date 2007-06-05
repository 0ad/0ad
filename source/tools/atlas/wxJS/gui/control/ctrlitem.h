/*
 * wxJavaScript - ctrlitem.h
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
 * $Id: ctrlitem.h 598 2007-03-07 20:13:28Z fbraem $
 */
#ifndef _WXJSControlWithItems_H
#define _WXJSControlWithItems_H

namespace wxjs
{
    namespace gui
    {
        class ControlWithItems : public ApiWrapper<ControlWithItems, wxControlWithItems>
        {
        public:

	        static bool GetProperty(wxControlWithItems *p, JSContext *cx, JSObject *obj, int id, jsval *vp);
	        static bool SetProperty(wxControlWithItems *p, JSContext *cx, JSObject *obj, int id, jsval *vp);

	        // Empty to avoid deleting. (It will be deleted by wxWindows).
            static void Destruct(JSContext *cx, wxControlWithItems *p);
        	
	        WXJS_DECLARE_PROPERTY_MAP()

	        /**
	         * Property Ids.
	         */
	        enum
	        {
		          P_COUNT = WXJS_START_PROPERTY_ID
		        , P_SELECTION
		        , P_ITEM
		        , P_STRING_SELECTION
		        , P_EMPTY
	        };

	        WXJS_DECLARE_METHOD_MAP()
	        static JSBool append(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval);
	        static JSBool clear(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval);
	        static JSBool delete_item(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval);
	        static JSBool findString(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval);
	        static JSBool insert(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval);
        };
    }; // namespace gui
}; // namespace wxjs

#endif //_WXJSControlWithItems_H
