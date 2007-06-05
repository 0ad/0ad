/*
 * wxJavaScript - pwdlg.h
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
 * $Id: pwdlg.h 598 2007-03-07 20:13:28Z fbraem $
 */
#ifndef _WXJSPasswordEntryDialog_H
#define _WXJSPasswordEntryDialog_H

/////////////////////////////////////////////////////////////////////////////
// Name:        pwdlg.h
// Purpose:		PasswordEntryDialog ports wxPasswordEntryDialog to JavaScript.
// Author:      Franky Braem
// Modified by:
// Created:     04.11.05
// Copyright:   (c) 2001-2002 Franky Braem
// Licence:     LGPL
/////////////////////////////////////////////////////////////////////////////
namespace wxjs
{
    namespace gui
    {
        class PasswordEntryDialog : public wxPasswordEntryDialog
							          , public ApiWrapper<PasswordEntryDialog, wxPasswordEntryDialog>
							          , public Object
        {
        public:
	        /**
	         * Constructor
	         */
	        PasswordEntryDialog(JSContext *cx,
							        JSObject *obj, 
							        wxWindow* parent,
							        const wxString& message,
							        const wxString& caption, const wxString& defaultValue, long style,
							        const wxPoint& pos);

	        /**
	         * Destructor
	         */
	        virtual ~PasswordEntryDialog();

	        static wxPasswordEntryDialog* Construct(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, bool constructing);
	        static void Destruct(JSContext *cx, wxPasswordEntryDialog *p);

            DECLARE_EVENT_TABLE()
        };
    }; // namespace gui
}; // namespace wxjs

#endif //_WXJSPasswordEntryDialog_H
