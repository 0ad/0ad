/*
 * wxJavaScript - textfile.h
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
 * $Id: textfile.h 598 2007-03-07 20:13:28Z fbraem $
 */
#ifndef _WXJS_TEXTFILE_H
#define _WXJS_TEXTFILE_H

/////////////////////////////////////////////////////////////////////////////
// Name:        file.h
// Purpose:		Ports wxFile to JavaScript
// Author:      Franky Braem
// Modified by:
// Created:     30.08.02
// Copyright:   (c) 2001-2002 Franky Braem
// Licence:     LGPL
/////////////////////////////////////////////////////////////////////////////

#include <wx/textfile.h>

namespace wxjs
{
    namespace io
    {
        class TextFile : public ApiWrapper<TextFile, wxTextFile>
        {
        public:
        	
	        static bool GetProperty(wxTextFile *f, JSContext *cx, JSObject *obj, int id, jsval *vp);

	        enum
	        { 
	          P_CURRENT_LINE
	          , P_EOF
	          , P_OPENED
	          , P_LINE_COUNT
	          , P_FIRST_LINE
	          , P_NEXT_LINE
	          , P_GUESS_TYPE
	          , P_NAME
	          , P_LAST_LINE
	          , P_PREV_LINE
	          , P_LINES
	        };

	        WXJS_DECLARE_PROPERTY_MAP()
	        WXJS_DECLARE_METHOD_MAP()
	        WXJS_DECLARE_STATIC_METHOD_MAP()

	        static void InitClass(JSContext *cx, JSObject *obj, JSObject *proto);

	        static wxTextFile *Construct(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, bool constructing);

	        static JSBool addLine(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval);
	        static JSBool clear(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval);
	        static JSBool close(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval);
	        static JSBool create(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval);
	        static JSBool exists(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval);
	        static JSBool open(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval);
	        static JSBool gotoLine(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval);
	        static JSBool removeLine(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval);
	        static JSBool insertLine(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval);
	        static JSBool write(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval);

	        static JSBool getEOL(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval);
        };
    }; // namespace io
}; // namespace wxjs
#endif  // _WXJS_TEXTFILE
