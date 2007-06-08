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
 * $Id: splitwin.h 694 2007-05-03 21:14:13Z fbraem $
 */
#ifndef _WXJSSplitterWindow_H
#define _WXJSSplitterWindow_H

#include <wx/splitter.h>
#include "../../common/evtconn.h"

namespace wxjs
{
    namespace gui
    {
        class SplitterWindow : public ApiWrapper<SplitterWindow,
                                                 wxSplitterWindow>
        {
        public:

          static bool AddProperty(wxSplitterWindow *p, 
                                  JSContext *cx, 
                                  JSObject *obj, 
                                  const wxString &prop, 
                                  jsval *vp);
          static bool DeleteProperty(wxSplitterWindow *p, 
                                     JSContext* cx, 
                                     JSObject* obj, 
                                     const wxString &prop);

          static bool GetProperty(wxSplitterWindow *p,
                                  JSContext *cx,
                                  JSObject *obj,
                                  int id,
                                  jsval *vp);

            static bool SetProperty(wxSplitterWindow *p,
                                    JSContext *cx,
                                    JSObject *obj,
                                    int id,
                                    jsval *vp);

            static wxSplitterWindow* Construct(JSContext *cx,
                                               JSObject *obj,
                                               uintN argc,
                                               jsval *argv,
                                               bool constructing);

            static void InitClass(JSContext *cx,
                                  JSObject *obj,
                                  JSObject *proto);

            WXJS_DECLARE_CONSTANT_MAP()
            
            WXJS_DECLARE_METHOD_MAP()
            WXJS_DECLARE_METHOD(create)
            WXJS_DECLARE_METHOD(setSashPosition)
            WXJS_DECLARE_METHOD(initialize)
            WXJS_DECLARE_METHOD(replaceWindow)
            WXJS_DECLARE_METHOD(splitHorizontally)
            WXJS_DECLARE_METHOD(splitVertically)
            WXJS_DECLARE_METHOD(unsplit)

            WXJS_DECLARE_PROPERTY_MAP()
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

        class SplitterEventHandler : public EventConnector<wxSplitterWindow>
                                   , public wxEvtHandler
        {
        public:
          // Events
            void OnSashPosChanging(wxSplitterEvent &event);
            void OnSashPosChanged(wxSplitterEvent &event);
            void OnUnsplit(wxSplitterEvent &event);
            void OnDClick(wxSplitterEvent &event);
            static void InitConnectEventMap();
        private:
            static void ConnectSashPosChanging(wxSplitterWindow *p,
                                               bool connect);
            static void ConnectSashPosChanged(wxSplitterWindow *p,
                                              bool connect);
            static void ConnectUnsplit(wxSplitterWindow *p, bool connect);
            static void ConnectDClick(wxSplitterWindow *p, bool connect);
        };
    }; // namespace gui
}; // namespace wxjs

#endif //_WXJSSplitterWindow_H
