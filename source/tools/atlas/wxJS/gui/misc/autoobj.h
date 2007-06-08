/*
 * wxJavaScript - autoobj.h
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
 * $Id: autoobj.h 598 2007-03-07 20:13:28Z fbraem $
 */
#if defined(__WXMSW__)

#ifndef _WXJSAutomationObject_H
#define _WXJSAutomationObject_H

/////////////////////////////////////////////////////////////////////////////
// Name:        autoobj.h
// Purpose:     AutomationObject ports wxAutomationObject to JavaScript
// Author:      Franky Braem
// Modified by:
// Created:     28-12-2002
// Copyright:   (c) 2001-2002 Franky Braem
// Licence:     LGPL
/////////////////////////////////////////////////////////////////////////////

namespace wxjs
{
    namespace gui
    {
        class AutomationObject : public ApiWrapper<AutomationObject, wxAutomationObject>
        {
        public:
            /**
             * Callback for when a wxAutomationObject object is created
             */
            static wxAutomationObject* Construct(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, bool constructing);
            static void Destruct(JSContext *cx, wxAutomationObject *p)
            {
                p->SetDispatchPtr(NULL);
                delete p;
            }
            static wxVariant* CreateVariant(JSContext *cx, jsval v);
            static jsval CreateJSVal(JSContext *cx, const wxVariant &var);

            WXJS_DECLARE_METHOD_MAP()

            static JSBool createInstance(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval);
            static JSBool callMethod(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval);
            static JSBool getInstance(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval);
            static JSBool getObject(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval);
            static JSBool putProperty(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval);
            static JSBool getProperty(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval);
        };
    }; // namespace gui
}; // namespace wxjs

#endif //_WXJSAutomationObject_H

#endif // __WXMSW__
