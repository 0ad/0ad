/*
 * wxJavaScript - dialog.h
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
 * $Id: dialog.h 682 2007-04-24 20:38:18Z fbraem $
 */
#ifndef _WXJSDialog_H
#define _WXJSDialog_H

#include "../../common/evtconn.h"

namespace wxjs
{
    namespace gui
    {
        class Dialog : public ApiWrapper<Dialog, wxDialog>
        {
        public:

          static void InitClass(JSContext* cx, 
                                JSObject* obj, 
                                JSObject* proto);

          static bool AddProperty(wxDialog *p, 
                                  JSContext *cx, 
                                  JSObject *obj, 
                                  const wxString &prop, 
                                  jsval *vp);
          static bool DeleteProperty(wxDialog *p, 
                                     JSContext* cx, 
                                     JSObject* obj, 
                                     const wxString &prop);
          static bool GetProperty(wxDialog *p, 
                                  JSContext *cx, 
                                  JSObject *obj, 
                                  int id, 
                                  jsval *vp);

          static wxDialog* Construct(JSContext *cx, 
                                     JSObject *obj, 
                                     uintN argc, 
                                     jsval *argv, 
                                     bool constructing);

          WXJS_DECLARE_PROPERTY_MAP()
          enum
          {
              P_RETURN_CODE
            , P_TITLE
            , P_MODAL
          };

          WXJS_DECLARE_CONSTANT_MAP()

          WXJS_DECLARE_METHOD_MAP()
          static JSBool end_modal(JSContext *cx, 
                                  JSObject *obj, 
                                  uintN argc, 
                                  jsval *argv, 
                                  jsval *rval);
          static JSBool show_modal(JSContext *cx, 
                                   JSObject *obj, 
                                   uintN argc, 
                                   jsval *argv, 
                                   jsval *rval);
          static JSBool create(JSContext *cx, 
                               JSObject *obj, 
                               uintN argc, 
                               jsval *argv, 
                               jsval *rval);
        };

        class DialogEventHandler : public EventConnector<wxDialog>
                                 , public wxEvtHandler
        {
        public:
          void OnClose(wxCloseEvent &event);
          void OnInitDialog(wxInitDialogEvent &event);
          static void InitConnectEventMap();
        private:
          static void ConnectClose(wxDialog *p, bool connect);
          static void ConnectInitDialog(wxDialog *p, bool connect);
        };
    }; // namespace gui
}; // namespace wxjs

#endif //_WXJSDialog_H
