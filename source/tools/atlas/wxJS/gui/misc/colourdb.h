/*
 * wxJavaScript - colourdb.h
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
 * $Id: colourdb.h 598 2007-03-07 20:13:28Z fbraem $
 */
#ifndef _WXJSColourDatabase_H
#define _WXJSColourDatabase_H

/////////////////////////////////////////////////////////////////////////////
// Name:        colourdb.h
// Purpose:     ColourDatabase ports wxColourDatabase to JavaScript
// Author:      Franky Braem
// Modified by:
// Created:     26.09.2002
// Copyright:   (c) 2001-2002 Franky Braem
// Licence:     LGPL
/////////////////////////////////////////////////////////////////////////////

namespace wxjs
{
    namespace gui
    {
        class ColourDatabase : public ApiWrapper<ColourDatabase, wxColourDatabase>
        {
        public:
            WXJS_DECLARE_METHOD_MAP()

            // Empty, to avoid deleting the colour database. 
            // This is automatically done by wxWindows
            static void Destruct(JSContext *cx, wxColourDatabase *p)
            {
            }

            static JSBool find(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval);
            static JSBool findName(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval);
        };
    }; // namespace gui
}; // namespace wxjs

#endif //_WXJSColourDatabase_H
