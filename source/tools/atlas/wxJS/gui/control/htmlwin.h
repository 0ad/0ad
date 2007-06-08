/*
 * wxJavaScript - htmlwin.h
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
 * $Id: htmlwin.h 682 2007-04-24 20:38:18Z fbraem $
 */
#ifndef _WXJSHtmlWindow_H
#define _WXJSHtmlWindow_H

#include <wx/html/htmlwin.h>

#include "../../common/evtconn.h"

namespace wxjs
{
    namespace gui
    {
        class HtmlWindow : public ApiWrapper<HtmlWindow, wxHtmlWindow>
        {
        public:

            static void InitClass(JSContext* cx, 
                                  JSObject* obj,
                                  JSObject* proto);

            static bool AddProperty(wxHtmlWindow *p,
                                    JSContext *cx,
                                    JSObject *obj,
                                    const wxString &prop,
                                    jsval *vp);
            static bool DeleteProperty(wxHtmlWindow *p, 
                                       JSContext* cx, 
                                       JSObject* obj, 
                                       const wxString &prop);

	        static bool GetProperty(wxHtmlWindow *p,
                                    JSContext *cx, 
                                    JSObject *obj,
                                    int id,
                                    jsval *vp);
	        static bool SetProperty(wxHtmlWindow *p,
                                    JSContext *cx,
                                    JSObject *obj,
                                    int id,
                                    jsval *vp);

	        static wxHtmlWindow* Construct(JSContext *cx,
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
                  P_OPENED_ANCHOR
                , P_OPENED_PAGE
                , P_OPENED_PAGE_TITLE
                , P_RELATED_FRAME
                , P_HISTORY_CAN_BACK
                , P_HISTORY_CAN_FORWARD
                , P_TEXT
                , P_SELECTION_TEXT
	        };

            WXJS_DECLARE_METHOD_MAP()
            WXJS_DECLARE_METHOD(create);
            WXJS_DECLARE_METHOD(appendToPage);
            WXJS_DECLARE_METHOD(historyBack);
            WXJS_DECLARE_METHOD(historyForward);
            WXJS_DECLARE_METHOD(historyClear);
            WXJS_DECLARE_METHOD(loadFile);
            WXJS_DECLARE_METHOD(loadPage);
            WXJS_DECLARE_METHOD(setPage);
            WXJS_DECLARE_METHOD(setRelatedFrame);
            WXJS_DECLARE_METHOD(setRelatedStatusBar);
            WXJS_DECLARE_METHOD(selectAll);
            WXJS_DECLARE_METHOD(selectLine);
            WXJS_DECLARE_METHOD(selectWord);
            WXJS_DECLARE_METHOD(setBorders);
            WXJS_DECLARE_METHOD(setFonts);

	        WXJS_DECLARE_CONSTANT_MAP()
        };

        class HtmlLinkEventHandler : public EventConnector<wxHtmlWindow>
                                   , public wxEvtHandler
        {
        public:
            // Events
            void OnLinkClicked(wxHtmlLinkEvent &event);
            static void InitConnectEventMap();
        private:
            static void ConnectLinkClicked(wxHtmlWindow *p, bool connect);
        };

    }; // namespace gui
}; //namespace wxjs
#endif //_WXJSHtmlWindow_H
