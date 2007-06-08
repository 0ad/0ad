/*
 * wxJavaScript - app.h
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
 * $Id: app.h 716 2007-05-20 17:57:22Z fbraem $
 */
#ifndef _WXJSApp_H
#define _WXJSApp_H

namespace wxjs
{
    namespace gui
    {
        class App : public wxApp
                  , public ApiWrapper<App, wxApp>
        {
        public:

	        /**
	         * Constructor
	         */
	        App() : wxApp()
	        {
	        }

	        /**
	         * Destructor
	         */
	        virtual ~App();

	        static bool GetProperty(wxApp *p,
                                    JSContext *cx,
                                    JSObject *obj,
                                    int id,
                                    jsval *vp);
	        static bool SetProperty(wxApp *p,
                                    JSContext *cx,
                                    JSObject *obj, 
                                    int id, 
                                    jsval *vp);

            /**
	         * MainLoop is overridden to make sure, we only enter the mainloop
	         * when a window is created
	         */
	        int MainLoop();

	        /**
	         * Runs the mainloop
	         */
	        static JSBool mainLoop(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval);

	        WXJS_DECLARE_PROPERTY_MAP()
	        WXJS_DECLARE_METHOD_MAP()

	        /**
	         * Property Ids.
	         */
	        enum
	        {
		        P_APPLICATION_NAME
		        , P_TOP_WINDOW
		        , P_INITIALIZED
		        , P_CLASS_NAME
		        , P_VENDOR_NAME
	        };

            virtual int OnExit();
	        bool OnInit();

        private:
	        void DestroyTopWindows();
        };
    }; // namespace gui
}; // namespace wxjs

#endif //_WXJSApp_H
