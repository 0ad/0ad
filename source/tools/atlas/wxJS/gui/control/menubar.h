/*
 * wxJavaScript - menubar.h
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
 * $Id: menubar.h 675 2007-04-17 21:37:49Z fbraem $
 */
#ifndef _WXJSMENUBAR_H
#define _WXJSMENUBAR_H

namespace wxjs
{
    namespace gui
    {
      class MenuBar : public ApiWrapper<MenuBar, wxMenuBar>
      {
      public:

        static bool GetProperty(wxMenuBar *p,
                                JSContext *cx,
                                JSObject *obj,
                                int id,
                                jsval *vp);
        static wxMenuBar* Construct(JSContext *cx,
                                    JSObject *obj,
                                    uintN argc,
                                    jsval *argv,
                                    bool constructing);

        WXJS_DECLARE_METHOD_MAP()
        WXJS_DECLARE_METHOD(append)
        WXJS_DECLARE_METHOD(get_menu)
        WXJS_DECLARE_METHOD(check)
        WXJS_DECLARE_METHOD(insert)
        WXJS_DECLARE_METHOD(enable)
        WXJS_DECLARE_METHOD(enableTop)
        WXJS_DECLARE_METHOD(findMenu)
        WXJS_DECLARE_METHOD(findMenuItem)
        WXJS_DECLARE_METHOD(getHelpString)
        WXJS_DECLARE_METHOD(getLabel)
        WXJS_DECLARE_METHOD(getLabelTop)
        WXJS_DECLARE_METHOD(isChecked)
        WXJS_DECLARE_METHOD(isEnabled)
        WXJS_DECLARE_METHOD(refresh)
        WXJS_DECLARE_METHOD(remove)
        WXJS_DECLARE_METHOD(replace)
        WXJS_DECLARE_METHOD(setHelpString)
        WXJS_DECLARE_METHOD(setLabel)
        WXJS_DECLARE_METHOD(setLabelTop)

        WXJS_DECLARE_CONSTANT_MAP()

        WXJS_DECLARE_PROPERTY_MAP()
        enum
        {
	        P_MENUCOUNT
	        , P_MENUS
        };
      };
    }; // namespace gui
}; // namespace wxjs

#endif // _WXJSMENUBAR_H
