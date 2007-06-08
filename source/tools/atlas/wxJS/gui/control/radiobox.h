/*
 * wxJavaScript - radiobox.h
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
 * $Id: radiobox.h 691 2007-05-01 20:00:49Z fbraem $
 */
#ifndef _WXJSRadioBox_H
#define _WXJSRadioBox_H

#include "../../common/evtconn.h"

namespace wxjs
{
    namespace gui
    {
        class RadioBox : public ApiWrapper<RadioBox, wxRadioBox>
        {
        public:

            static void InitClass(JSContext* cx, 
                                  JSObject* obj, 
                                  JSObject* proto);
            static bool AddProperty(wxRadioBox *p, 
                                    JSContext *cx, 
                                    JSObject *obj, 
                                    const wxString &prop, 
                                    jsval *vp);
            static bool DeleteProperty(wxRadioBox *p, 
                                       JSContext* cx, 
                                       JSObject* obj, 
                                       const wxString &prop);
	        static bool GetProperty(wxRadioBox *p, 
                                    JSContext *cx, 
                                    JSObject *obj, 
                                    int id, 
                                    jsval *vp);
	        static bool SetProperty(wxRadioBox *p,
                                    JSContext *cx, 
                                    JSObject *obj,
                                    int id,
                                    jsval *vp);

	        static wxRadioBox* Construct(JSContext *cx,
                                         JSObject *obj,
                                         uintN argc,
                                         jsval *argv,
                                         bool constructing);

            WXJS_DECLARE_METHOD_MAP()
	        WXJS_DECLARE_METHOD(create)
	        WXJS_DECLARE_METHOD(setString)
	        WXJS_DECLARE_METHOD(findString)
	        WXJS_DECLARE_METHOD(enable)
	        WXJS_DECLARE_METHOD(show)

	        WXJS_DECLARE_CONSTANT_MAP()

	        WXJS_DECLARE_PROPERTY_MAP()
	        enum
	        {
		        P_SELECTION
		        , P_STRING_SELECTION
		        , P_ITEM
		        , P_COUNT
	        };
        };

        class RadioBoxEventHandler : public EventConnector<wxRadioBox>
                                   , public wxEvtHandler
        {
        public:
          // Events
            void OnRadioBox(wxCommandEvent &event);
            static void InitConnectEventMap();
        private:
            static void ConnectRadioBox(wxRadioBox *p, bool connect);
        };
    }; // namespace gui
}; // namespace wxjs

#endif //_WXJSRadioBox_H
