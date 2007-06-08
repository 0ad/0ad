/*
 * wxJavaScript - fn.h
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
 * $Id: fn.h 598 2007-03-07 20:13:28Z fbraem $
 */
#ifndef wxjs_io_fn_h
#define wxjs_io_fn_h

// Common functions

namespace wxjs
{
    namespace io
    {
        JSBool concatFiles(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval);
        JSBool copyFile(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval);
        JSBool renameFile(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval);
        JSBool removeFile(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval);
        JSBool fileExists(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval);
        JSBool getCwd(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval);
        JSBool getFreeDiskSpace(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval);
        JSBool getTotalDiskSpace(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval);
        JSBool getOSDirectory(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval);
        JSBool isAbsolutePath(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval);
        JSBool isWild(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval);
        JSBool dirExists(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval);
        JSBool matchWild(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval);
        JSBool mkDir(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval);
        JSBool rmDir(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval);
        JSBool setWorkingDirectory(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval);
        
        // Process functions
        JSBool execute(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval);
        JSBool shell(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval);
    }; // namespace io
}; // namespace wxjs
#endif //wxjs_io_fn_h
