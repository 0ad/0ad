/*
 * wxJavaScript - panel.h
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
 * $Id: panel.h 688 2007-04-27 20:45:09Z fbraem $
 */
#ifndef _WXJSPanel_H
#define _WXJSPanel_H

#include "../../common/evtconn.h"

namespace wxjs
{
    namespace gui
    {
      class Panel : public ApiWrapper<Panel, wxPanel>
        {
        public:
          static void InitClass(JSContext* cx, 
                                JSObject* obj, 
                                JSObject* proto);
          static bool AddProperty(wxPanel *p, 
                                  JSContext *cx, 
                                  JSObject *obj, 
                                  const wxString &prop, 
                                  jsval *vp);
          static bool DeleteProperty(wxPanel *p, 
                                     JSContext* cx, 
                                     JSObject* obj, 
                                     const wxString &prop);
          static bool GetProperty(wxPanel *p,
                                  JSContext *cx,
                                  JSObject *obj,
                                  int id,
                                  jsval *vp);
          static bool SetProperty(wxPanel *p,
                                  JSContext *cx,
                                  JSObject *obj,
                                  int id,
                                  jsval *vp);

          static wxPanel* Construct(JSContext *cx,
                                      JSObject *obj,
                                      uintN argc,
                                      jsval *argv,
                                      bool constructing);
        	
          WXJS_DECLARE_PROPERTY_MAP()

          /**
           * Property Ids.
           */
          enum
          {
	          P_DEFAULT_ITEM
          };

          WXJS_DECLARE_METHOD_MAP()
          WXJS_DECLARE_METHOD(create)
        };

      class PanelEventHandler : public EventConnector<wxPanel>
                              , public wxEvtHandler
      {
      public:
        // Events
          void OnSysColourChanged(wxSysColourChangedEvent &event);
          static void InitConnectEventMap();
      private:
          static void ConnectSysColourChanged(wxPanel *p, bool connect);
      };
    }; // namespace gui
}; // namespace wxjs
#endif //_WXJSPanel_H
