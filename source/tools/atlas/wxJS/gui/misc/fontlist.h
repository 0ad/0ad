/*
 * wxJavaScript - fontlist.h
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
 * $Id: fontlist.h 598 2007-03-07 20:13:28Z fbraem $
 */
#ifndef _WXJSFontList_H
#define _WXJSFontList_H

/////////////////////////////////////////////////////////////////////////////
// Name:        fontlist.h
// Purpose:		FontList ports wxFontList to JavaScript.
// Author:      Franky Braem
// Modified by:
// Created:     08.08.02
// Copyright:   (c) 2001-2002 Franky Braem
// Licence:     LGPL
/////////////////////////////////////////////////////////////////////////////

namespace wxjs
{
    namespace gui
    {
        class FontList : public ApiWrapper<FontList, wxFontList>
        {
        public:

	        static JSBool findOrCreate(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval);

            static void Destruct(JSContext *cx, wxFontList *p);

	        WXJS_DECLARE_METHOD_MAP()
        };
    }; // namespace gui
}; // namespace wxjs

#endif //_WXJSFontList_H
