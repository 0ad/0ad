/*
 * wxJavaScript - listbox.h
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
 * $Id: listbox.h 685 2007-04-25 21:14:49Z fbraem $
 */
#ifndef _WXJSListBox_H
#define _WXJSListBox_H

#include "../../common/evtconn.h"

namespace wxjs
{
    namespace gui
    {
        class ListBox : public ApiWrapper<ListBox, wxListBox>
        {
        public:

          static void InitClass(JSContext* cx, 
                                JSObject* obj, 
                                JSObject* proto);

          static bool AddProperty(wxListBox *p, 
                                  JSContext *cx, 
                                  JSObject *obj, 
                                  const wxString &prop, 
                                  jsval *vp);
          static bool DeleteProperty(wxListBox *p, 
                                     JSContext* cx, 
                                     JSObject* obj, 
                                     const wxString &prop);

          static bool GetProperty(wxListBox *p,
                                  JSContext *cx,
                                  JSObject *obj,
                                  int id,
                                  jsval *vp);
	      static wxListBox* Construct(JSContext *cx,
                                      JSObject *obj,
                                      uintN argc,
                                      jsval *argv,
                                      bool constructing);

          WXJS_DECLARE_PROPERTY_MAP()
  
          WXJS_DECLARE_METHOD_MAP()
          WXJS_DECLARE_METHOD(create)
	      WXJS_DECLARE_METHOD(insert_items)
	      WXJS_DECLARE_METHOD(set_first_item)
	        
          WXJS_DECLARE_CONSTANT_MAP()

          enum
          {
	          P_SELECTIONS
          };
        };

        class ListBoxEventHandler : public EventConnector<wxListBox>
                                  , public wxEvtHandler
        {
        public:
          // Events
            void OnListBox(wxCommandEvent &event);
	        void OnDoubleClick(wxCommandEvent &event);
            static void InitConnectEventMap();
        private:
            static void ConnectListBox(wxListBox *p, bool connect);
            static void ConnectDoubleClick(wxListBox *p, bool connect);
        };
    }; // namespace gui
}; // namespace wxjs

#endif //_WXJSListBox_H
