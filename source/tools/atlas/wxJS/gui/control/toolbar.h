/*
 * wxJavaScript - toolbar.h
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
 * $Id: toolbar.h 598 2007-03-07 20:13:28Z fbraem $
 */
#ifndef _WXJSToolBar_H
#define _WXJSToolBar_H

/////////////////////////////////////////////////////////////////////////////
// Name:        toolbar.h
// Purpose:     ToolBar ports wxToolBar to JavaScript
// Author:      Franky Braem
// Modified by:
// Created:     31-01-2003
// Copyright:   (c) 2001-2003 Franky Braem
// Licence:     LGPL
/////////////////////////////////////////////////////////////////////////////

namespace wxjs
{
    namespace gui
    {
        class ToolBar : public wxToolBar
                          , public ApiWrapper<ToolBar, wxToolBar>
                          , public Object
        {
        public:

            ToolBar(JSContext *cx, JSObject *obj);
            virtual ~ToolBar();

            /**
             * Callback for retrieving properties of wxToolBar
             */
            static bool GetProperty(wxToolBar *p, JSContext *cx, JSObject *obj, int id, jsval *vp);

            /**
             * Callback for setting properties
             */
            static bool SetProperty(wxToolBar *p, JSContext *cx, JSObject *obj, int id, jsval *vp);

            /**
             * Callback for when a wxToolBar object is created
             */
            static wxToolBar* Construct(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, bool constructing);

            /**
             * Callback for when a wxToolBar object is destroyed
             */
            static void Destruct(JSContext *cx, wxToolBar *p);

            WXJS_DECLARE_PROPERTY_MAP()
            WXJS_DECLARE_CONSTANT_MAP()
            WXJS_DECLARE_METHOD_MAP()
            static JSBool addControl(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval);
            static JSBool addSeparator(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval);
            static JSBool addTool(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval);
            static JSBool addCheckTool(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval);
            static JSBool addRadioTool(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval);
            static JSBool deleteTool(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval);
            static JSBool deleteToolByPos(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval);
            static JSBool enableTool(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval);
            static JSBool findControl(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval);
            static JSBool getToolClientData(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval);
            static JSBool getToolEnabled(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval);
            static JSBool getToolLongHelp(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval);
            static JSBool getToolShortHelp(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval);
            static JSBool getToolState(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval);
            static JSBool insertControl(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval);
            static JSBool insertSeparator(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval);
            static JSBool insertTool(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval);
            static JSBool realize(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval);
            static JSBool setToolClientData(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval);
            static JSBool setToolLongHelp(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval);
            static JSBool setToolShortHelp(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval);
            static JSBool toggleTool(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval);

            /**
             * Property Ids.
             */
            enum
            {
                P_TOOL_SIZE
                , P_TOOL_BITMAP_SIZE
                , P_MARGINS
                , P_TOOL_PACKING
                , P_TOOL_SEPARATION
            };
        };

        class ToolData : public wxObject
        {
        public:
            ToolData(JSContext *cx, jsval v) : m_cx(cx), m_val(v)
            {
                if ( JSVAL_IS_GCTHING(m_val) )
                {
                    JS_AddRoot(m_cx, &m_val);
                }
            }

            virtual ~ToolData()
            {
                if ( JSVAL_IS_GCTHING(m_val) )
                {
                    JS_RemoveRoot(m_cx, &m_val);
                }
            }

            jsval GetJSVal()
            {
                return m_val;
            }
        private:
	        JSContext *m_cx;
            jsval m_val;
        };
    }; // namespace gui
}; // namespace wxjs

#endif //_WXJSToolBar_H
