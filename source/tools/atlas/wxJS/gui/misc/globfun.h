/*
 * wxJavaScript - globfun.h
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
 * $Id: globfun.h 598 2007-03-07 20:13:28Z fbraem $
 */
/**
 * (c) 2001-2002 Franky Braem (S.A.W.)
 *
 * This file is part of wxJS. wxJS ports wxWindows to JavaScript
 * 
 * File      : wxJSGlobFun.h
 * Desc.     : This file defines all the global functions.
 * Created   : 24-05-2001
 * L. Update :
 * 
 */

namespace wxjs
{
    namespace gui
    {
        /**
         * Initializes the global functions
         */
        bool InitFunctions(JSContext *cx, JSObject *global);

        /**
         * Implements wxMessageBox
         */
        JSBool MessageBox(JSContext *cx, uintN argc, jsval *vp);
        JSBool InitAllImageHandlers(JSContext *cx, uintN argc, jsval *vp);
        JSBool GetKeyState(JSContext *cx, uintN argc, jsval *vp);

        void DefineGlobals(JSContext *cx, JSObject *global);
    }; // namespace gui
}; // namespace wxjs
