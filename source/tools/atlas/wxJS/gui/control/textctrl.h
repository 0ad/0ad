/*
 * wxJavaScript - textctrl.h
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
 * $Id: textctrl.h 695 2007-05-04 20:51:28Z fbraem $
 */
#ifndef _WXJSTEXTCTRL_H
#define _WXJSTEXTCTRL_H

#include "../../common/evtconn.h"

namespace wxjs
{
    namespace gui
    {
        class TextCtrl : public ApiWrapper<TextCtrl, wxTextCtrl>
        {
        public:
            static void InitClass(JSContext* cx, 
                                  JSObject* obj, 
                                  JSObject* proto);
            static bool AddProperty(wxTextCtrl *p, 
                                    JSContext *cx, 
                                    JSObject *obj, 
                                    const wxString &prop, 
                                    jsval *vp);
            static bool DeleteProperty(wxTextCtrl *p, 
                                       JSContext* cx, 
                                       JSObject* obj, 
                                       const wxString &prop);
            static bool GetProperty(wxTextCtrl *p,
                                    JSContext *cx,
                                    JSObject *obj,
                                    int id,
                                    jsval *vp);
	        static bool SetProperty(wxTextCtrl *p,
                                    JSContext *cx,
                                    JSObject *obj,
                                    int id,
                                    jsval *vp);

	        static wxTextCtrl* Construct(JSContext *cx,
                                         JSObject *obj,
                                         uintN argc,
                                         jsval *argv,
                                         bool constructing);
        	
	        WXJS_DECLARE_METHOD_MAP()
	        WXJS_DECLARE_METHOD(appendText)
	        WXJS_DECLARE_METHOD(clear)
            WXJS_DECLARE_METHOD(create)
	        WXJS_DECLARE_METHOD(cut)
	        WXJS_DECLARE_METHOD(discardEdits)
	        WXJS_DECLARE_METHOD(getLineLength)
	        WXJS_DECLARE_METHOD(getLineText)
	        WXJS_DECLARE_METHOD(setSelection)
	        WXJS_DECLARE_METHOD(loadFile)
	        WXJS_DECLARE_METHOD(paste)
	        WXJS_DECLARE_METHOD(redo)
	        WXJS_DECLARE_METHOD(replace)
	        WXJS_DECLARE_METHOD(remove)
	        WXJS_DECLARE_METHOD(saveFile)

	        WXJS_DECLARE_CONSTANT_MAP()
         
            WXJS_DECLARE_PROPERTY_MAP()
	        enum
	        {
		        P_CAN_COPY
		        , P_CAN_PASTE
		        , P_CAN_CUT
		        , P_CAN_REDO
		        , P_CAN_UNDO
		        , P_INSERTION_POINT
		        , P_NUMBER_OF_LINES
		        , P_SELECTION_FROM
		        , P_SELECTION_TO
		        , P_VALUE
		        , P_MODIFIED
		        , P_LAST_POSITION
		        , P_EDITABLE
	        };
        };

        class TextCtrlEventHandler : public EventConnector<wxTextCtrl>
                                   , public wxEvtHandler
        {
        public:
          // Events
            void OnText(wxCommandEvent &event);
	        void OnTextEnter(wxCommandEvent &event);
	        void OnTextURL(wxCommandEvent &event);
	        void OnTextMaxLen(wxCommandEvent &event);
            static void InitConnectEventMap();
        private:
            static void ConnectText(wxTextCtrl *p, bool connect);
            static void ConnectTextEnter(wxTextCtrl *p, bool connect);
            static void ConnectTextURL(wxTextCtrl *p, bool connect);
            static void ConnectTextMaxLen(wxTextCtrl *p, bool connect);
        };
    }; // namespace gui
}; // namespace wxjs

#endif //_WXJSTEXTCTRL_H
