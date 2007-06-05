/*
 * wxJavaScript - dialog.h
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
 * $Id: dialog.h 598 2007-03-07 20:13:28Z fbraem $
 */
#ifndef _WXJSDialog_H
#define _WXJSDialog_H

/////////////////////////////////////////////////////////////////////////////
// Name:        dialog.h
// Purpose:		Dialog ports wxDialog to JavaScript.
// Author:      Franky Braem
// Modified by:
// Created:     28.01.02
// Copyright:   (c) 2001-2002 Franky Braem
// Licence:     LGPL
/////////////////////////////////////////////////////////////////////////////

namespace wxjs
{
    namespace gui
    {
        class Dialog : public wxDialog
                         , public ApiWrapper<Dialog, wxDialog>
                         , public Object
        {
        public:
	        /**
	         * Constructor
	         */
	        Dialog(JSContext *cx, JSObject *obj);

	        /**
	         * Destructor
	         */
	        virtual ~Dialog();

	        static bool GetProperty(wxDialog *p, JSContext *cx, JSObject *obj, int id, jsval *vp);

	        static wxDialog* Construct(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, bool constructing);
        	
	        static void Destruct(JSContext *cx, wxDialog *p);

	        static JSBool end_modal(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval);
	        static JSBool show_modal(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval);

	        enum
	        {
		        P_RETURN_CODE
		        , P_TITLE
		        , P_MODAL
	        };

	        WXJS_DECLARE_PROPERTY_MAP()
	        WXJS_DECLARE_METHOD_MAP()
	        WXJS_DECLARE_CONSTANT_MAP()

            DECLARE_EVENT_TABLE()
	        void OnClose(wxCloseEvent &event);
        };
    }; // namespace gui
}; // namespace wxjs

#endif //_WXJSDialog_H
