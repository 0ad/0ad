/*
 * wxJavaScript - helpbtn.h
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
 * $Id: helpbtn.h 598 2007-03-07 20:13:28Z fbraem $
 */
#ifndef _WXJSContextHelpButton_H
#define _WXJSContextHelpButton_H

/////////////////////////////////////////////////////////////////////////////
// Name:        helpbtn.h
// Purpose:		ContextHelpButton ports wxContextHelpButton to JavaScript.
// Author:      Franky Braem
// Modified by:
// Created:     27.09.02
// Copyright:   (c) 2001-2002 Franky Braem
// Licence:     LGPL
/////////////////////////////////////////////////////////////////////////////

#include <wx/cshelp.h>

namespace wxjs
{
    namespace gui
    {
        class ContextHelpButton : public wxContextHelpButton
                                    , public ApiWrapper<ContextHelpButton, wxContextHelpButton>
                                    , public Object
        {
        public:
	        /**
	         * Constructor
	         */
	        ContextHelpButton(wxWindow *parent, JSContext *cx, JSObject *obj);

	        /**
	         * Destructor
	         */
	        virtual ~ContextHelpButton();

	        static wxContextHelpButton* Construct(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, bool constructing);
            
            // Empty to avoid deleting. (It will be deleted by wxWindows).
            static void Destruct(JSContext *cx, wxContextHelpButton *p)
            {
            }
        };
    }; // namespace gui
}; // namespace wxjs
#endif //_WXJSContextHelpButton_H
