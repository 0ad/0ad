/*
 * wxJavaScript - choice.h
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
 * $Id: choice.h 678 2007-04-19 20:12:31Z fbraem $
 */
#ifndef _WXJSChoice_H
#define _WXJSChoice_H

#include "../../common/evtconn.h"

namespace wxjs
{
    namespace gui
    {
        class Choice : public ApiWrapper<Choice, wxChoice>
        {
        public:

          static void InitClass(JSContext* cx, 
                                JSObject* obj, 
                                JSObject* proto);
          static bool AddProperty(wxChoice *p, 
                                  JSContext *cx, 
                                  JSObject *obj, 
                                  const wxString &prop, 
                                  jsval *vp);
          static bool DeleteProperty(wxChoice *p, 
                                     JSContext* cx, 
                                     JSObject* obj, 
                                     const wxString &prop);

          static bool GetProperty(wxChoice *p,
                                  JSContext *cx,
                                  JSObject *obj,
                                  int id,
                                  jsval *vp);
          static bool SetProperty(wxChoice *p,
                                  JSContext *cx,
                                  JSObject *obj,
                                  int id,
                                  jsval *vp);

          static wxChoice* Construct(JSContext *cx,
                                     JSObject *obj,
                                     uintN argc,
                                     jsval *argv,
                                     bool constructing);

          WXJS_DECLARE_PROPERTY_MAP()

          enum
          {
	          P_COLUMNS
          };

          WXJS_DECLARE_METHOD_MAP()
          WXJS_DECLARE_METHOD(create)
        };

        class ChoiceEventHandler : public EventConnector<wxChoice>
                                 , public wxEvtHandler
        {
        public:
          // Events
            void OnChoice(wxCommandEvent &event);
            static void InitConnectEventMap();
        private:
            static void ConnectChoice(wxChoice *p, bool connect);
        };
    }; // namespace gui
}; // namespace wxjs
#endif //_WXJSChoice_H
