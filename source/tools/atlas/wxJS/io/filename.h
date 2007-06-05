/*
 * wxJavaScript - filename.h
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
 * $Id: filename.h 598 2007-03-07 20:13:28Z fbraem $
 */
#ifndef _WXJSFileName_H
#define _WXJSFileName_H

/////////////////////////////////////////////////////////////////////////////
// Name:        filename.h
// Purpose:     wxJSFileName ports wxFileName to JavaScript
// Author:      Franky Braem
// Modified by:
// Created:     14-01-2003
// Copyright:   (c) 2001-2003 Franky Braem
// Licence:     LGPL
/////////////////////////////////////////////////////////////////////////////

#include <wx/filename.h>

namespace wxjs
{
    namespace io
    {
        class FileName : public ApiWrapper<FileName, wxFileName>
        {
        public:
            /**
             * Callback for retrieving properties of wxFileName
             */
            static bool GetProperty(wxFileName *p, JSContext *cx, JSObject *obj, int id, jsval *vp);

            /**
             * Callback for setting properties
             */
            static bool SetProperty(wxFileName *p, JSContext *cx, JSObject *obj, int id, jsval *vp);

            static bool GetStaticProperty(JSContext *cx, int id, jsval *vp);
            static bool SetStaticProperty(JSContext *cx, int id, jsval *vp);
            
            static void InitClass(JSContext *cx, JSObject *obj, JSObject *proto);
            
            /**
             * Callback for when a wxFileName object is created
             */
            static wxFileName* Construct(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, bool constructing);

            static JSBool appendDir(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval);
            static JSBool assign(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval);
            static JSBool assignCwd(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval);
            static JSBool assignDir(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval);
            static JSBool assignHomeDir(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval);
            static JSBool assignTempFileName(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval);
            static JSBool clear(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval);
            static JSBool getFullPath(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval);
            static JSBool getPath(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval);
            static JSBool getPathSeparator(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval);
            static JSBool getPathSeparators(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval);
            static JSBool getTimes(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval);
            static JSBool setTimes(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval);
            static JSBool getVolumeSeparator(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval);
            static JSBool insertDir(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval);
            static JSBool isAbsolute(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval);
            static JSBool isCaseSensitive(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval);
            static JSBool isPathSeparator(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval);
            static JSBool isRelative(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval);
            static JSBool makeRelativeTo(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval);
            static JSBool mkdir(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval);
            static JSBool normalize(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval);
            static JSBool prependDir(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval);
            static JSBool removeDir(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval);
            static JSBool removeLastDir(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval);
            static JSBool rmdir(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval);
            static JSBool sameAs(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval);
            static JSBool setCwd(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval);
            static JSBool touch(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval);

            WXJS_DECLARE_PROPERTY_MAP()
            WXJS_DECLARE_METHOD_MAP()
	        WXJS_DECLARE_STATIC_PROPERTY_MAP()
            WXJS_DECLARE_STATIC_METHOD_MAP()

            static JSBool createTempFileName(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval);
            static JSBool dirExists(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval);
            static JSBool dirName(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval);
            static JSBool fileExists(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval);
            static JSBool fileName(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval);
            static JSBool getCwd(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval);
            static JSBool getFormat(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval);
            static JSBool smkdir(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval);
            static JSBool srmdir(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval);
            static JSBool splitPath(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval);

            /**
             * Property Ids.
             */
            enum
            {
                P_CWD = -128
                , P_DIRS
                , P_DIR_COUNT
                , P_DIR_EXISTS
                , P_EXT
                , P_HAS_EXT
                , P_HAS_NAME
                , P_HAS_VOLUME
                , P_FILE_EXISTS
                , P_FULL_NAME
                , P_FULL_PATH
                , P_HOME_DIR
                , P_LONG_PATH
                , P_MODIFICATION_TIME
                , P_NAME
                , P_PATH_SEPARATOR
                , P_PATH_SEPARATORS
                , P_SHORT_PATH
                , P_ACCESS_TIME
                , P_CREATE_TIME
                , P_VOLUME
                , P_VOLUME_SEPARATOR
                , P_OK
                , P_IS_DIR
            };

            static bool SetDate(JSContext *cx, jsval v, const wxDateTime &date);
        };
    }; // namespace io
}; // namespace wxjs
#endif //_WXJSFileName_H
