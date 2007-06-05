/*
 * wxJavaScript - item.h
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
 * $Id: item.h 598 2007-03-07 20:13:28Z fbraem $
 */
#ifndef _WXJSControlItem_H
#define _WXJSControlItem_H

namespace wxjs
{
    namespace gui
    {
        class ControlItem : public ApiWrapper<ControlItem, Index>
        {
        public:

	        static bool GetProperty(Index *p, JSContext *cx, JSObject *obj, int id, jsval *vp);
	        static bool SetProperty(Index *p, JSContext *cx, JSObject *obj, int id, jsval *vp);
        	
	        static JSBool remove(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval);
	        static JSBool select(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval);

            WXJS_DECLARE_PROPERTY_MAP()
	        WXJS_DECLARE_METHOD_MAP()

	        enum
	        {
		        P_VALUE = WXJS_START_PROPERTY_ID
	        };
        };
    }; // namespace gui
}; // namespace wxjs
#endif //_WXJSControlItem_H
