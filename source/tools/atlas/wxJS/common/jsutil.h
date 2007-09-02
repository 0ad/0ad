/*
 * wxJavaScript - jsutil.h
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
 * $Id: jsutil.h 801 2007-07-02 20:54:19Z fbraem $
 */
#ifndef _wxjs_util_h
#define _wxjs_util_h

namespace wxjs
{
    // Returns a function from a property
    JSBool GetFunctionProperty(JSContext *cx, JSObject *obj, const char *propertyName, jsval *property);
    JSBool CallFunctionProperty(JSContext *cx, JSObject *obj, const char *propertyName, uintN argc, jsval* args, jsval *rval);

    // Returns the JSClass structure for the given classname
    JSClass *GetClass(JSContext *cx, const char* className);

    // Returns true when the object has class as its prototype
    bool HasPrototype(JSContext *cx, JSObject *obj, const char *className);
    bool HasPrototype(JSContext *cx, jsval v, const char *className);

    bool GetScriptRoot(JSContext *cx, jsval *v);

    // Property functions using wxString / UTF-16
    JSBool wxDefineProperty(JSContext *cx, 
                            JSObject *obj, 
                            const wxString &propertyName, 
                            jsval *propertyValue);
    JSBool wxGetProperty(JSContext *cx, 
                         JSObject *obj, 
                         const wxString &propertyName, 
                         jsval *propertyValue);
    JSBool wxSetProperty(JSContext *cx, 
                         JSObject *obj, 
                         const wxString &propertyName, 
                         jsval *propertyValue);
};
#endif //wxjs_util_h
