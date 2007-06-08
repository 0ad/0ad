/*
 * wxJavaScript - chklstbx.h
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
 * $Id: chklstbx.h 678 2007-04-19 20:12:31Z fbraem $
 */
#ifndef _WXJSCheckListBox_H
#define _WXJSCheckListBox_H

#include "../../common/evtconn.h"

namespace wxjs
{
    namespace gui
    {
        class CheckListBox : public ApiWrapper<CheckListBox, wxCheckListBox>
        {
        public:

          static void InitClass(JSContext* cx, 
                                JSObject* obj, 
                                JSObject* proto);

          static bool AddProperty(wxCheckListBox *p, 
                                  JSContext *cx, 
                                  JSObject *obj, 
                                  const wxString &prop, 
                                  jsval *vp);
          static bool DeleteProperty(wxCheckListBox *p, 
                                     JSContext* cx, 
                                     JSObject* obj, 
                                     const wxString &prop);

          static bool GetProperty(wxCheckListBox *p,
                                    JSContext *cx,
                                    JSObject *obj,
                                    int id,
                                    jsval *vp);

          static wxCheckListBox* Construct(JSContext *cx,
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
		        P_CHECKED = WXJS_START_PROPERTY_ID
	        };

            WXJS_DECLARE_METHOD_MAP()
            WXJS_DECLARE_METHOD(create)
        };

        class CheckListBoxEventHandler : public EventConnector<wxCheckListBox>
                                       , public wxEvtHandler
        {
        public:
          // Events
          void OnCheckListBox(wxCommandEvent &event);
          static void InitConnectEventMap();
        private:
          static void ConnectCheckListBox(wxCheckListBox *p, bool connect);
        };

    }; // namespace gui
}; // namespace wxjs
#endif //_WXJSCheckListBox_H
