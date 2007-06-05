/*
 * wxJavaScript - textctrl.h
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
 * $Id: textctrl.h 598 2007-03-07 20:13:28Z fbraem $
 */
#ifndef _WXJSTEXTCTRL_H
#define _WXJSTEXTCTRL_H

/////////////////////////////////////////////////////////////////////////////
// Name:        textctrl.h
// Purpose:     TextCtrl ports wxTextCtrl to JavaScript
// Author:      Franky Braem
// Modified by:
// Created:     16.12.2002
// Copyright:   (c) 2001-2002 Franky Braem
// Licence:     LGPL
/////////////////////////////////////////////////////////////////////////////
namespace wxjs
{
    namespace gui
    {
        class TextCtrl : public wxTextCtrl
                           , public ApiWrapper<TextCtrl, wxTextCtrl>
                           , public Object
        {
        public:

	        TextCtrl(JSContext *cx, JSObject *obj);
	        virtual ~TextCtrl();

	        static bool GetProperty(wxTextCtrl *p, JSContext *cx, JSObject *obj, int id, jsval *vp);
	        static bool SetProperty(wxTextCtrl *p, JSContext *cx, JSObject *obj, int id, jsval *vp);

	        static wxTextCtrl* Construct(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, bool constructing);
            // Empty to avoid deleting. (It will be deleted by wxWindows).
            static void Destruct(JSContext *cx, wxTextCtrl *p)
            {
            }
        	
	        static JSBool appendText(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval);
	        static JSBool clear(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval);
	        static JSBool cut(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval);
	        static JSBool discardEdits(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval);
	        static JSBool getLineLength(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval);
	        static JSBool getLineText(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval);
	        static JSBool setSelection(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval);
	        static JSBool loadFile(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval);
	        static JSBool paste(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval);
	        static JSBool redo(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval);
	        static JSBool replace(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval);
	        static JSBool remove(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval);
	        static JSBool saveFile(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval);

            WXJS_DECLARE_PROPERTY_MAP()
	        WXJS_DECLARE_METHOD_MAP()
	        WXJS_DECLARE_CONSTANT_MAP()
         
	        enum
	        {
		        P_CAN_COPY
		        , P_CAN_PASTE
		        , P_CAN_CUT
		        , P_CAN_REDO
		        , P_CAN_UNDO
		        , P_INSERTION_POINT
		        , P_NUMBER_OF_LINES
		        , P_SELECTION_FROM
		        , P_SELECTION_TO
		        , P_VALUE
		        , P_MODIFIED
		        , P_LAST_POSITION
		        , P_EDITABLE
	        };

            void OnText(wxCommandEvent &event);
	        void OnTextEnter(wxCommandEvent &event);
	        DECLARE_EVENT_TABLE()
        };
    }; // namespace gui
}; // namespace wxjs

#endif //_WXJSTEXTCTRL_H
