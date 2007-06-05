/*
 * wxJavaScript - radiobox.h
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
 * $Id: radiobox.h 598 2007-03-07 20:13:28Z fbraem $
 */
#ifndef _WXJSRadioBox_H
#define _WXJSRadioBox_H

/////////////////////////////////////////////////////////////////////////////
// Name:        RadioBox.h
// Purpose:     RadioBox ports wxRadioBox to JavaScript
// Author:      Franky Braem
// Modified by:
// Created:     17.08.2002
// Copyright:   (c) 2001-2002 Franky Braem
// Licence:     LGPL
/////////////////////////////////////////////////////////////////////////////
namespace wxjs
{
    namespace gui
    {
        class RadioBox : public wxRadioBox
                           , public ApiWrapper<RadioBox, wxRadioBox>
                           , public Object
        {
        public:
	        /**
	         * Constructor
	         */
	        RadioBox(JSContext *cx, JSObject *obj);

	        /**
	         * Destructor
	         */
	        virtual ~RadioBox();

	        static bool GetProperty(wxRadioBox *p, JSContext *cx, JSObject *obj, int id, jsval *vp);
	        static bool SetProperty(wxRadioBox *p, JSContext *cx, JSObject *obj, int id, jsval *vp);

	        static wxRadioBox* Construct(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, bool constructing);
            // Empty to avoid deleting. (It will be deleted by wxWindows).
            static void Destruct(JSContext *cx, wxRadioBox *p)
            {
            }
        	
	        static JSBool setString(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval);
	        static JSBool findString(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval);
	        static JSBool enable(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval);
	        static JSBool show(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval);

	        WXJS_DECLARE_PROPERTY_MAP()
	        WXJS_DECLARE_CONSTANT_MAP()
	        WXJS_DECLARE_METHOD_MAP()

	        /**
	         * Property Ids.
	         */
	        enum
	        {
		        P_SELECTION
		        , P_STRING_SELECTION
		        , P_ITEM
		        , P_COUNT
	        };

            void OnRadioBox(wxCommandEvent &event);
	        DECLARE_EVENT_TABLE()
        };
    }; // namespace gui
}; // namespace wxjs

#endif //_WXJSRadioBox_H
