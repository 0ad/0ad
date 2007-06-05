/*
 * wxJavaScript - button.h
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
 * $Id: button.h 598 2007-03-07 20:13:28Z fbraem $
 */
#ifndef _WXJSButton_H
#define _WXJSButton_H

/////////////////////////////////////////////////////////////////////////////
// Name:        button.h
// Purpose:		wxJSButton ports wxButton to JavaScript.
// Author:      Franky Braem
// Modified by:
// Created:     29.01.02
// Copyright:   (c) 2001-2002 Franky Braem
// Licence:     LGPL
/////////////////////////////////////////////////////////////////////////////

namespace wxjs
{
    namespace gui
    {

        class Button : public wxButton
                         , public ApiWrapper<Button, wxButton>
                         , public Object
        {
        public:
	        /**
	         * Constructor
	         */
	        Button(JSContext *cx, JSObject *obj);

	        /**
	         * Destructor
	         */
	        virtual ~Button();

	        /**
	         * Callback for retrieving properties
	         */
	        static bool GetProperty(wxButton *p, JSContext *cx, JSObject *obj, int id, jsval *vp);
	        static bool SetProperty(wxButton *p, JSContext *cx, JSObject *obj, int id, jsval *vp);

	        /**
	         * Callback for retrieving static properties
	         */
	        static bool GetStaticProperty(JSContext *cx, int id, jsval *vp);

	        /**
	         * Callback for when a wxButton object is created
	         */
	        static wxButton *Construct(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, bool constructing);
            // Empty to avoid deleting. (It will be deleted by wxWindows).
            static void Destruct(JSContext *cx, wxButton *p)
            {
            }

	        static JSBool setDefault(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval);

	        WXJS_DECLARE_PROPERTY_MAP()
	        WXJS_DECLARE_STATIC_PROPERTY_MAP()
	        WXJS_DECLARE_CONSTANT_MAP()
	        WXJS_DECLARE_METHOD_MAP()

	        enum
	        {
		        P_LABEL
		        , P_DEFAULT_SIZE
	        };

            void OnClicked(wxCommandEvent &event);
	        DECLARE_EVENT_TABLE()
        };
    }; // namespace gui
}; // namespace wxjs
#endif //_WXJSButton_H
