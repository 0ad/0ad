/*
 * wxJavaScript - splitwin.h
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
 * $Id: splitwin.h 598 2007-03-07 20:13:28Z fbraem $
 */
#ifndef _WXJSSplitterWindow_H
#define _WXJSSplitterWindow_H

/////////////////////////////////////////////////////////////////////////////
// Name:        splitwin.h
// Purpose:     SplitterWindow ports wxSplitterWindow to JavaScript
// Author:      Franky Braem
// Modified by:
// Created:     22-01-2003
// Copyright:   (c) 2001-2003 Franky Braem
// Licence:     LGPL
/////////////////////////////////////////////////////////////////////////////

#include <wx/splitter.h>
namespace wxjs
{
    namespace gui
    {
        class SplitterWindow : public wxSplitterWindow
                                 , public ApiWrapper<SplitterWindow, wxSplitterWindow>
                                 , public Object
        {
        public:

            SplitterWindow(JSContext *cx, JSObject *obj);
            virtual ~SplitterWindow();

            /**
             * Callback for retrieving properties of wxSplitterWindow
             */
            static bool GetProperty(wxSplitterWindow *p, JSContext *cx, JSObject *obj, int id, jsval *vp);

            /**
             * Callback for setting properties
             */
            static bool SetProperty(wxSplitterWindow *p, JSContext *cx, JSObject *obj, int id, jsval *vp);

            /**
             * Callback for when a wxSplitterWindow object is created
             */
            static wxSplitterWindow* Construct(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, bool constructing);

            /**
             * Callback for when a wxSplitterWindow object is destroyed
             */
            static void Destruct(JSContext *cx, wxSplitterWindow *p);
            static void InitClass(JSContext *cx, JSObject *obj, JSObject *proto);

            WXJS_DECLARE_PROPERTY_MAP()
            WXJS_DECLARE_CONSTANT_MAP()
            
            WXJS_DECLARE_METHOD_MAP()
            static JSBool setSashPosition(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval);
            static JSBool initialize(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval);
            static JSBool replaceWindow(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval);
            static JSBool splitHorizontally(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval);
            static JSBool splitVertically(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval);
            static JSBool unsplit(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval);

            /**
             * Property Ids.
             */
            enum
            {
                P_MIN_PANE_SIZE
                , P_SASH_POS
                , P_WINDOW1
                , P_WINDOW2
                , P_SPLIT_MODE
                , P_IS_SPLIT
            };
        };
    }; // namespace gui
}; // namespace wxjs

#endif //_WXJSSplitterWindow_H
