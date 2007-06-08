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
 * $Id: frame.h 704 2007-05-11 19:46:45Z fbraem $
 */
#ifndef _WXJSFRAME_H
#define _WXJSFRAME_H

#include "../../common/evtconn.h"

namespace wxjs
{
    namespace gui
    {
      class Frame : public ApiWrapper<Frame, wxFrame>
                  , public wxFrame
        {
        public:

          Frame() : wxFrame() 
          {
          }

          virtual ~Frame()
          {
          }

          virtual wxToolBar* OnCreateToolBar(long style, 
                                             wxWindowID id,
                                             const wxString& name);

          static void InitClass(JSContext* cx, 
                                JSObject* obj, 
                                JSObject* proto);

          static bool AddProperty(wxFrame *p, 
                                  JSContext *cx, 
                                  JSObject *obj, 
                                  const wxString &prop, 
                                  jsval *vp);
          static bool DeleteProperty(wxFrame *p, 
                                     JSContext* cx, 
                                     JSObject* obj, 
                                     const wxString &prop);

          static bool GetProperty(wxFrame *p,
                                    JSContext *cx,
                                    JSObject *obj,
                                    int id,
                                    jsval *vp);
          static bool SetProperty(wxFrame *p,
                                    JSContext *cx,
                                    JSObject *obj,
                                    int id,
                                    jsval *vp);
         
          static wxFrame* Construct(JSContext *cx,
                                      JSObject *obj,
                                      uintN argc,
                                      jsval *argv,
                                      bool constructing);
        	
          WXJS_DECLARE_METHOD_MAP()
          WXJS_DECLARE_METHOD(create)
          WXJS_DECLARE_METHOD(setStatusText)
          WXJS_DECLARE_METHOD(setStatusWidths)
          WXJS_DECLARE_METHOD(createStatusBar)
          WXJS_DECLARE_METHOD(processCommand)
          WXJS_DECLARE_METHOD(createToolBar)

          WXJS_DECLARE_CONSTANT_MAP()

          WXJS_DECLARE_PROPERTY_MAP()

          enum
          {
	          P_MENUBAR
	          , P_STATUSBAR_FIELDS
	          , P_STATUS_WIDTHS
	          , P_TOOLBAR
                , P_STATUSBAR
                , P_STATUSBAR_PANE
          };
        };

        class FrameEventHandler : public EventConnector<wxFrame>
                                , public wxEvtHandler
        {
        public:
          // Events
	        void OnClose(wxCloseEvent &event);
	        void OnMenu(wxCommandEvent &event);
	        void OnIconize(wxIconizeEvent &event);
	        void OnMaximize(wxMaximizeEvent &event);
            static void InitConnectEventMap();
        private:
            static void ConnectClose(wxFrame *p, bool connect);
            static void ConnectMenu(wxFrame *p, bool connect);
            static void ConnectIconize(wxFrame *p, bool connect);
            static void ConnectMaximize(wxFrame *p, bool connect);
        };

    }; // namespace gui
}; // namespace wxjs

#endif // _WXJSFRAME_H
