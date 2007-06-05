/*
 * wxJavaScript - listbox.h
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
 * $Id: listbox.h 598 2007-03-07 20:13:28Z fbraem $
 */
#ifndef _WXJSListBox_H
#define _WXJSListBox_H

/////////////////////////////////////////////////////////////////////////////
// Name:        listbox.h
// Purpose:		ListBox ports wxListBox to JavaScript.
// Author:      Franky Braem
// Modified by:
// Created:     01.05.02
// Copyright:   (c) 2001-2002 Franky Braem
// Licence:     LGPL
/////////////////////////////////////////////////////////////////////////////

namespace wxjs
{
    namespace gui
    {
        class ListBox : public wxListBox
                          , public ApiWrapper<ListBox, wxListBox>
                          , public Object
        {
        public:

	        ListBox(JSContext *cx, JSObject *obj);

	        virtual ~ListBox();

	        static bool GetProperty(wxListBox *p, JSContext *cx, JSObject *obj, int id, jsval *vp);
	        static wxListBox* Construct(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, bool constructing);
            // Empty to avoid deleting. (It will be deleted by wxWindows).
            static void Destruct(JSContext *cx, wxListBox *p)
            {
            }

	        static JSBool insert_items(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval);
	        static JSBool set_first_item(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval);

	        WXJS_DECLARE_PROPERTY_MAP()
	        WXJS_DECLARE_METHOD_MAP()
	        WXJS_DECLARE_CONSTANT_MAP()

	        enum
	        {
		        P_SELECTIONS
	        };

            void OnListBox(wxCommandEvent &event);
	        void OnDoubleClick(wxCommandEvent &event);
	        DECLARE_EVENT_TABLE()

        };
    }; // namespace gui
}; // namespace wxjs

#endif //_WXJSListBox_H
