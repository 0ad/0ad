/*
 * wxJavaScript - frame.h
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
 * $Id: frame.h 598 2007-03-07 20:13:28Z fbraem $
 */
#ifndef _WXJSFRAME_H
#define _WXJSFRAME_H

/////////////////////////////////////////////////////////////////////////////
// Name:        frame.h
// Purpose:		Frame ports wxFrame to JavaScript.
// Author:      Franky Braem
// Modified by:
// Created:     16.12.01
// Copyright:   (c) 2001-2002 Franky Braem
// Licence:     LGPL
/////////////////////////////////////////////////////////////////////////////

namespace wxjs
{
    namespace gui
    {
        class Frame : public wxFrame
                        , public ApiWrapper<Frame, wxFrame>
                        , public Object
        {
        public:
        	
	        /**
	         * Constructor
	         */
	        Frame(JSContext *cx, JSObject *obj);

	        /**
	         * Destructor
	         */
	        virtual ~Frame();

	        static bool GetProperty(wxFrame *p, JSContext *cx, JSObject *obj, int id, jsval *vp);
	        static bool SetProperty(wxFrame *p, JSContext *cx, JSObject *obj, int id, jsval *vp);
         
	        static wxFrame* Construct(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, bool constructing);
            // Empty to avoid deleting. (It will be deleted by wxWindows).
            static void Destruct(JSContext* WXUNUSED(cx), wxFrame* WXUNUSED(p) )
            {
            }
        	
	        static JSBool setStatusText(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval);
	        static JSBool setStatusWidths(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval);
	        static JSBool createStatusBar(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval);
	        static JSBool processCommand(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval);
	        static JSBool createToolBar(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval);

	        void OnIconize(wxIconizeEvent &event);
	        void OnMaximize(wxMaximizeEvent &event);

	        WXJS_DECLARE_PROPERTY_MAP()
	        WXJS_DECLARE_METHOD_MAP()
	        WXJS_DECLARE_CONSTANT_MAP()

	        enum
	        {
		        P_MENUBAR
		        , P_STATUSBAR_FIELDS
		        , P_STATUS_WIDTHS
		        , P_TOOLBAR
                , P_STATUSBAR
                , P_STATUSBAR_PANE
	        };

            DECLARE_EVENT_TABLE()
	        void OnClose(wxCloseEvent &event);
	        void OnMenu(wxCommandEvent &event);
	        void OnTool(wxCommandEvent &event);

            // Overrided because wxJS needs a StatusBar
            virtual wxStatusBar* OnCreateStatusBar(int number,
                                                   long style,
                                                   wxWindowID id,
                                                   const wxString& name);
            virtual wxToolBar* OnCreateToolBar(long style,
                                               wxWindowID id,
                                               const wxString& name);
        };
    }; // namespace gui
}; // namespace wxjs

#endif // _WXJSFRAME_H
