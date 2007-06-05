/*
 * wxJavaScript - fontdlg.h
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
 * $Id: fontdlg.h 598 2007-03-07 20:13:28Z fbraem $
 */
#ifndef _WXJSFontDialog_H
#define _WXJSFontDialog_H

/////////////////////////////////////////////////////////////////////////////
// Name:        fontdlg.h
// Purpose:		FontDialog ports wxFontDialog to JavaScript.
// Author:      Franky Braem
// Modified by:
// Created:     06.08.02
// Copyright:   (c) 2001-2002 Franky Braem
// Licence:     LGPL
/////////////////////////////////////////////////////////////////////////////

#include <wx/fontdlg.h>

namespace wxjs
{
    namespace gui
    {
        class FontDialog : public ApiWrapper<FontDialog, wxFontDialog>
        {
        public:

            static bool GetProperty(wxFontDialog *p, JSContext *cx, JSObject *obj, int id, jsval *vp);

	        static wxFontDialog* Construct(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, bool constructing);
	        static void Destruct(JSContext *cx, wxFontDialog *p);

	        WXJS_DECLARE_PROPERTY_MAP()

	        enum 
	        {
		        P_FONT_DATA
	        };
        };
    }; // namespace gui
}; // namespace wxjs

#endif //_WXJSFontDialog_H
