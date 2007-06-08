/*
 * wxJavaScript - menu.h
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
 * $Id: menu.h 688 2007-04-27 20:45:09Z fbraem $
 */
#ifndef _WXJSMENU_H
#define _WXJSMENU_H

namespace wxjs
{
    namespace gui
    {
        class Menu : public ApiWrapper<Menu, wxMenu>
        {
        public:

	        static bool GetProperty(wxMenu *p,
                                    JSContext *cx,
                                    JSObject *obj,
                                    int id,
                                    jsval *vp);
	        static bool SetProperty(wxMenu *p,
                                    JSContext *cx,
                                    JSObject *obj,
                                    int id,
                                    jsval *vp);

	        static wxMenu* Construct(JSContext *cx,
                                     JSObject *obj,
                                     uintN argc,
                                     jsval *argv,
                                     bool constructing);

//	        static void Destruct(JSContext *cx, wxMenu *p);

	        ////////////////////////
	        // JavaScript methods
	        ////////////////////////

	        WXJS_DECLARE_METHOD_MAP()
	        WXJS_DECLARE_METHOD(append)
	        WXJS_DECLARE_METHOD(append_separator)
	        WXJS_DECLARE_METHOD(new_column)
	        WXJS_DECLARE_METHOD(check)
	        WXJS_DECLARE_METHOD(delete_item)
	        WXJS_DECLARE_METHOD(enable)
	        WXJS_DECLARE_METHOD(find_item)
	        WXJS_DECLARE_METHOD(getHelpString)
	        WXJS_DECLARE_METHOD(getItem)
	        WXJS_DECLARE_METHOD(getLabel)
	        WXJS_DECLARE_METHOD(destroy)
	        WXJS_DECLARE_METHOD(insert)
	        WXJS_DECLARE_METHOD(remove)
	        WXJS_DECLARE_METHOD(isChecked)
            WXJS_DECLARE_METHOD(isEnabled)
	        WXJS_DECLARE_METHOD(setHelpString)
	        WXJS_DECLARE_METHOD(setLabel)

	        WXJS_DECLARE_CONSTANT_MAP()

	        WXJS_DECLARE_PROPERTY_MAP()
	        enum
	        {
		        P_MENU_ITEM_COUNT
		        , P_MENU_ITEMS
		        , P_TITLE
	        };
        };
    }; // namespace gui
}; // namespace wxjs

#endif //_WXJSMENU_H

