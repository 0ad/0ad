/*
 * wxJavaScript - scrollwnd.h
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
 * $Id: scrollwnd.h 598 2007-03-07 20:13:28Z fbraem $
 */
#ifndef _WXJSScrolledWindow_H
#define _WXJSScrolledWindow_H

#include <wx/scrolwin.h>

namespace wxjs
{
    namespace gui
    {
        class ScrolledWindow : public wxScrolledWindow
                             , public ApiWrapper<ScrolledWindow, wxScrolledWindow>
                             , public Object
        {
        public:
	        /**
	         * Constructor
	         */
	        ScrolledWindow(JSContext *cx, JSObject *obj);

	        /**
	         * Destructor
	         */
	        virtual ~ScrolledWindow();

	        static bool GetProperty(wxScrolledWindow *p, JSContext *cx, JSObject *obj, int id, jsval *vp);

	        static wxScrolledWindow* Construct(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, bool constructing);
            
            // Empty to avoid deleting. (It will be deleted by wxWidgets).
            static void Destruct(JSContext* WXUNUSED(cx), wxScrolledWindow* WXUNUSED(p))
            {
            }
        	
	        WXJS_DECLARE_PROPERTY_MAP()

	        /**
	         * Property Ids.
	         */
	        enum
	        {
                P_SCROLL_PIXELS_PER_UNIT = WXJS_START_PROPERTY_ID
                , P_VIEW_START
                , P_VIRTUAL_SIZE
                , P_RETAINED
            };

            WXJS_DECLARE_METHOD_MAP()
            static JSBool calcScrolledPosition(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval);
            static JSBool calcUnscrolledPosition(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval);
            static JSBool enableScrolling(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval);
            static JSBool getScrollPixelsPerUnit(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval);
            static JSBool scroll(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval);
            static JSBool setScrollbars(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval);
            static JSBool setScrollRate(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval);
            static JSBool setTargetWindow(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval);
        };
    }; // namespace gui
}; //namespace wxjs
#endif //_WXJSScrolledWindow_H
