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
 * $Id: htmlwin.h 598 2007-03-07 20:13:28Z fbraem $
 */
#ifndef _WXJSHtmlWindow_H
#define _WXJSHtmlWindow_H

#include <wx/html/htmlwin.h>

#include "../../common/evtconn.h"

namespace wxjs
{
    namespace gui
    {
        class HtmlWindow : public wxHtmlWindow
                         , public ApiWrapper<HtmlWindow, wxHtmlWindow>
                         , public EventConnector<wxHtmlWindow>
                         , public Object
        {
        public:
	        /**
	         * Constructor
	         */
	        HtmlWindow(JSContext *cx, JSObject *obj);

	        /**
	         * Destructor
	         */
	        virtual ~HtmlWindow();

            static void InitClass(JSContext* cx, JSObject* obj, JSObject* proto);

            static bool AddProperty(wxHtmlWindow *p, JSContext *cx, JSObject *obj, const wxString &prop, jsval *vp);
	        static bool GetProperty(wxHtmlWindow *p, JSContext *cx, JSObject *obj, int id, jsval *vp);
	        static bool SetProperty(wxHtmlWindow *p, JSContext *cx, JSObject *obj, int id, jsval *vp);

	        static wxHtmlWindow* Construct(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, bool constructing);
            
            // Empty to avoid deleting. (It will be deleted by wxWidgets).
            static void Destruct(JSContext* WXUNUSED(cx), wxHtmlWindow* WXUNUSED(p))
            {
            }
        	
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
            static JSBool appendToPage(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval);
            static JSBool historyBack(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval);
            static JSBool historyForward(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval);
            static JSBool historyClear(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval);
            static JSBool loadFile(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval);
            static JSBool loadPage(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval);
            static JSBool setPage(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval);
            static JSBool setRelatedFrame(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval);
            static JSBool setRelatedStatusBar(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval);
            static JSBool selectAll(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval);
            static JSBool selectLine(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval);
            static JSBool selectWord(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval);
            static JSBool setBorders(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval);
            static JSBool setFonts(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval);

	        WXJS_DECLARE_CONSTANT_MAP()

            // Events
            void OnLinkClicked(wxHtmlLinkEvent &event);

        private:
            static void ConnectLinkClicked(wxHtmlWindow *p);
            static void InitConnectEventMap();
        };
    }; // namespace gui
}; //namespace wxjs
#endif //_WXJSHtmlWindow_H
