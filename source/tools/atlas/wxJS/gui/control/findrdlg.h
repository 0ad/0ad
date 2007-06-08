/*
 * wxJavaScript - findrdlg.h
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
 * $Id: findrdlg.h 682 2007-04-24 20:38:18Z fbraem $
 */
#ifndef _WXJSFindReplaceDialog_H
#define _WXJSFindReplaceDialog_H

#include <wx/fdrepdlg.h>
#include "../../common/evtconn.h"

namespace wxjs
{
    namespace gui
    {
      class FindReplaceClientData : public JavaScriptClientData
      {
      public:
        FindReplaceClientData(JSContext *cx,
                              JSObject *obj, 
                              bool protect, 
                              bool owner = true)
                              : JavaScriptClientData(cx, obj, protect, owner)
        {
        }
        virtual ~FindReplaceClientData() {}

        // Keep our own data. This is because the wxFindReplaceData object
        // can be gc'd by the engine, which results in memory problems.
        wxFindReplaceData m_data;
      };

      class FindReplaceDialog : public ApiWrapper<FindReplaceDialog, 
                                                  wxFindReplaceDialog>
      {
      public:
        static void InitClass(JSContext* cx, 
                              JSObject* obj, 
                              JSObject* proto);

        static bool AddProperty(wxFindReplaceDialog *p, 
                                JSContext *cx, 
                                JSObject *obj, 
                                const wxString &prop, 
                                jsval *vp);
        static bool DeleteProperty(wxFindReplaceDialog *p, 
                                   JSContext* cx, 
                                   JSObject* obj, 
                                   const wxString &prop);
        static bool GetProperty(wxFindReplaceDialog *p,
                                JSContext *cx,
                                JSObject *obj,
                                int id,
                                jsval *vp);

        static wxFindReplaceDialog* Construct(JSContext *cx,
                                              JSObject *obj,
                                              uintN argc,
                                              jsval *argv,
                                              bool constructing);
      	

        WXJS_DECLARE_METHOD_MAP()
        WXJS_DECLARE_METHOD(create)

        WXJS_DECLARE_CONSTANT_MAP()

        WXJS_DECLARE_PROPERTY_MAP()
        enum 
        {
	        P_DATA
        };

      };

      class FindReplaceEventHandler 
            : public EventConnector<wxFindReplaceDialog>
            , public wxEvtHandler
      {
      public:
        // Events
        void OnFind(wxFindDialogEvent& event);
        void OnFindNext(wxFindDialogEvent& event);
        void OnReplace(wxFindDialogEvent& event);
        void OnReplaceAll(wxFindDialogEvent& event);
        void OnFindClose(wxFindDialogEvent& event);
        static void InitConnectEventMap();
      private:
        static void ConnectFind(wxFindReplaceDialog *p, bool connect);
        static void ConnectFindNext(wxFindReplaceDialog *p, bool connect);
        static void ConnectReplace(wxFindReplaceDialog *p, bool connect);
        static void ConnectReplaceAll(wxFindReplaceDialog *p, bool connect);
        static void ConnectFindClose(wxFindReplaceDialog *p, bool connect);
      };
    }; // namespace gui
}; // namespace wxjs

#endif //_WXJSFindReplaceDialog_H
