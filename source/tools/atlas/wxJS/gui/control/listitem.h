/*
 * wxJavaScript - listitem.h
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
 * $Id: listitem.h 598 2007-03-07 20:13:28Z fbraem $
 */
#ifndef _WXJSListItem_H
#define _WXJSListItem_H

/////////////////////////////////////////////////////////////////////////////
// Name:        listitem.h
// Purpose:     ListItem ports wxListItem
//              ListItemAttr ports wxListItemAttr
// Author:      Franky Braem
// Modified by:
// Created:     30.09.2002
// Copyright:   (c) 2001-2002 Franky Braem
// Licence:     LGPL
/////////////////////////////////////////////////////////////////////////////

namespace wxjs
{
    namespace gui
    {

        class ListObjectData;

        class ListItem : public ApiWrapper<ListItem, wxListItem>
        {
        public:

            static bool GetProperty(wxListItem *p, JSContext *cx, JSObject *obj, int id, jsval *vp);
            static bool SetProperty(wxListItem *p, JSContext *cx, JSObject *obj, int id, jsval *vp);

            static wxListItem *Construct(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, bool constructing);

            WXJS_DECLARE_PROPERTY_MAP()

            enum
            {
                P_MASK = WXJS_START_PROPERTY_ID
                , P_ID
                , P_COLUMN
                , P_STATE
                , P_STATE_MASK
                , P_TEXT
                , P_IMAGE
                , P_DATA
                , P_WIDTH
                , P_ALIGN
                , P_TEXT_COLOUR
                , P_BG_COLOUR
                , P_FONT
                , P_HAS_ATTR
            };
        };
    }; // namespace gui
}; // namespace wxjs

#endif //_WXJSListItem_H
