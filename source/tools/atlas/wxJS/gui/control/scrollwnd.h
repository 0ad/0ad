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
 * $Id: scrollwnd.h 692 2007-05-02 21:30:16Z fbraem $
 */
#ifndef _WXJSScrolledWindow_H
#define _WXJSScrolledWindow_H

#include <wx/scrolwin.h>

namespace wxjs
{
    namespace gui
    {
      class ScrolledWindow : public ApiWrapper<ScrolledWindow, wxScrolledWindow>
      {
      public:

        static bool AddProperty(wxScrolledWindow *p, 
                                JSContext *cx, 
                                JSObject *obj, 
                                const wxString &prop, 
                                jsval *vp);
        static bool DeleteProperty(wxScrolledWindow *p, 
                                   JSContext* cx, 
                                   JSObject* obj, 
                                   const wxString &prop);
        static bool GetProperty(wxScrolledWindow *p,
                                JSContext *cx,
                                JSObject *obj,
                                int id,
                                jsval *vp);

        static wxScrolledWindow* Construct(JSContext *cx,
                                           JSObject *obj,
                                           uintN argc,
                                           jsval *argv,
                                           bool constructing);
        WXJS_DECLARE_PROPERTY_MAP()
        enum
        {
          P_SCROLL_PIXELS_PER_UNIT = WXJS_START_PROPERTY_ID
          , P_VIEW_START
          , P_VIRTUAL_SIZE
          , P_RETAINED
        };

        WXJS_DECLARE_METHOD_MAP()
        WXJS_DECLARE_METHOD(create)
        WXJS_DECLARE_METHOD(calcScrolledPosition)
        WXJS_DECLARE_METHOD(calcUnscrolledPosition)
        WXJS_DECLARE_METHOD(enableScrolling)
        WXJS_DECLARE_METHOD(getScrollPixelsPerUnit)
        WXJS_DECLARE_METHOD(scroll)
        WXJS_DECLARE_METHOD(setScrollbars)
        WXJS_DECLARE_METHOD(setScrollRate)
        WXJS_DECLARE_METHOD(setTargetWindow)
      };
    }; // namespace gui
}; //namespace wxjs
#endif //_WXJSScrolledWindow_H
