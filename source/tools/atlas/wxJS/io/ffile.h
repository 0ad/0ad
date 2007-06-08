/*
 * wxJavaScript - ffile.h
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
 * $Id: ffile.h 598 2007-03-07 20:13:28Z fbraem $
 */
#ifndef _WXJS_FFILE_H
#define _WXJS_FFILE_H

/////////////////////////////////////////////////////////////////////////////
// Name:        file.h
// Purpose:		Ports wxFFile to JavaScript
// Author:      Franky Braem
// Modified by:
// Created:     18.10.02
// Copyright:   (c) 2001-2002 Franky Braem
// Licence:     LGPL
/////////////////////////////////////////////////////////////////////////////

#include <wx/ffile.h>
namespace wxjs
{
    namespace io
    {
        class FFile : public ApiWrapper<FFile, wxFFile>
        {
        public:
        	
	        static bool GetProperty(wxFFile *f, JSContext *cx, JSObject *obj, int id, jsval *vp);

	        enum
	        { 
	          P_EOF
	          , P_KIND
	          , P_OPENED
	          , P_LENGTH
	          , P_TELL
	        };

	        WXJS_DECLARE_PROPERTY_MAP()
	        WXJS_DECLARE_METHOD_MAP()
	        WXJS_DECLARE_CONSTANT_MAP()

	        static wxFFile *Construct(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, bool constructing);

	        static JSBool close(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval);
	        static JSBool flush(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval);
	        static JSBool open(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval);
	        static JSBool read(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval);
	        static JSBool readAll(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval);
	        static JSBool seek(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval);
	        static JSBool seekEnd(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval);
	        static JSBool write(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval);
        };
    }; // namespace io
}; // namespace wxjs
#endif
