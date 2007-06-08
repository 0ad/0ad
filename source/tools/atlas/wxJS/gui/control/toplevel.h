/*
 * wxJavaScript - toplevel.h
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
 * $Id: toplevel.h 695 2007-05-04 20:51:28Z fbraem $
 */
#ifndef _WXJSTopLevel_H
#define _WXJSTopLevel_H

namespace wxjs
{
    namespace gui
    {
        class TopLevelWindow : public ApiWrapper<TopLevelWindow,
                                                 wxTopLevelWindow>
        {
        public:
	        static bool GetProperty(wxTopLevelWindow *p,
                                    JSContext *cx,
                                    JSObject *obj,
                                    int id,
                                    jsval *vp);
	        static bool SetProperty(wxTopLevelWindow *p,
                                    JSContext *cx, 
                                    JSObject *obj,
                                    int id,
                                    jsval *vp);
	        static void InitClass(JSContext *cx,
                                  JSObject *obj,
                                  JSObject *proto);

	        WXJS_DECLARE_METHOD(requestUserAttention)
	        WXJS_DECLARE_METHOD(setLeftMenu)
	        WXJS_DECLARE_METHOD(setRightMenu)
	        WXJS_DECLARE_METHOD(showFullScreen)

	        enum
	        {
                P_DEFAULT_ITEM
		        , P_FULL_SCREEN
		        , P_ICON
		        , P_ICONS
		        , P_ACTIVE
		        , P_ICONIZED
		        , P_MAXIMIZED
		        , P_TITLE
	        };

	        WXJS_DECLARE_PROPERTY_MAP()
	        WXJS_DECLARE_METHOD_MAP()
        };
    }; // namespace gui
}; // namespace wxjs

#endif //_WXJSTopLevel_H
