/*
 * wxJavaScript - file.h
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
 * $Id: file.h 598 2007-03-07 20:13:28Z fbraem $
 */
#ifndef _WXJS_FILE_H
#define _WXJS_FILE_H

/////////////////////////////////////////////////////////////////////////////
// Name:        file.h
// Purpose:		Ports wxFile to JavaScript
// Author:      Franky Braem
// Modified by:
// Created:     30.08.02
// Copyright:   (c) 2001-2002 Franky Braem
// Licence:     LGPL
/////////////////////////////////////////////////////////////////////////////

#include <wx/file.h>

namespace wxjs
{
    namespace io
    {
    class File : public ApiWrapper<File, wxFile>
    {
    public:
    	
	    static bool GetProperty(wxFile *f, JSContext *cx, JSObject *obj, int id, jsval *vp);

	    enum
	    { 
	      P_EOF
	      , P_OPENED
	      , P_LENGTH
	      , P_TELL
	      , P_KIND
	    };

	    WXJS_DECLARE_PROPERTY_MAP()
	    WXJS_DECLARE_METHOD_MAP()
	    WXJS_DECLARE_CONSTANT_MAP()
	    WXJS_DECLARE_STATIC_METHOD_MAP()

	    static void InitClass(JSContext *cx, JSObject *obj, JSObject *proto);

	    static wxFile *Construct(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, bool constructing);

	    static JSBool attach(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval);
	    static JSBool detach(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval);
	    static JSBool close(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval);
	    static JSBool create(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval);
	    static JSBool flush(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval);
	    static JSBool open(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval);
	    static JSBool read(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval);
	    static JSBool seek(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval);
	    static JSBool seekEnd(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval);
	    static JSBool write(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval);
	    static JSBool exists(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval);
	    static JSBool access(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval);
    };
    }; // namespace io
}; // namespace wxjs
#endif
