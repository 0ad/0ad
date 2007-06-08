/*
 * wxJavaScript - treeitem.h
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
 * $Id: treeitem.h 696 2007-05-07 21:16:23Z fbraem $
 */
#ifndef _WXJSTreeItem_H
#define _WXJSTreeItem_H

namespace wxjs
{
    namespace gui
    {
        class TreeItem : public ApiWrapper<TreeItem, wxTreeItemId>
        {
        public:
            static bool GetProperty(wxTreeItemId *p,
                                    JSContext *cx,
                                    JSObject *obj,
                                    int id,
                                    jsval *vp);
            static bool SetProperty(wxTreeItemId *p,
                                    JSContext *cx,
                                    JSObject *obj,
                                    int id,
                                    jsval *vp);

            WXJS_DECLARE_PROPERTY_MAP()
            enum
            {
                P_TEXT
                , P_DATA
                , P_TEXT_COLOUR
                , P_BACKGROUND_COLOUR
                , P_FONT
                , P_HAS_CHILDREN
                , P_BOLD
                , P_VISIBLE
                , P_EXPANDED
                , P_SELECTED
                , P_CHILDREN_COUNT
                , P_ALL_CHILDREN_COUNT
                , P_PARENT
                , P_LAST_CHILD
                , P_NEXT_SIBLING
                , P_PREV_SIBLING
            };

            WXJS_DECLARE_METHOD_MAP()
            WXJS_DECLARE_METHOD(getImage)
            WXJS_DECLARE_METHOD(setImage)
            WXJS_DECLARE_METHOD(getFirstChild)
            WXJS_DECLARE_METHOD(getNextChild)
        };
    }; // namespace gui
}; // namespace wxjs

#endif //_WXJSTreeItem_H
