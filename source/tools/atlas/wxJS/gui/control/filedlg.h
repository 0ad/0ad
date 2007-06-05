/*
 * wxJavaScript - filedlg.h
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
 * $Id: filedlg.h 598 2007-03-07 20:13:28Z fbraem $
 */
#ifndef _WXJSFileDialog_H
#define _WXJSFileDialog_H

/////////////////////////////////////////////////////////////////////////////
// Name:        filedlg.h
// Purpose:		FileDialog ports wxFileDialog to JavaScript.
// Author:      Franky Braem
// Modified by:
// Created:     02.01.02
// Copyright:   (c) 2001-2002 Franky Braem
// Licence:     LGPL
/////////////////////////////////////////////////////////////////////////////

namespace wxjs
{
    namespace gui
    {
        class FileDialog : public ApiWrapper<FileDialog, wxFileDialog>
        {
        public:
	        static bool GetProperty(wxFileDialog *p, JSContext *cx, JSObject *obj, int id, jsval *vp);
	        static bool SetProperty(wxFileDialog *p, JSContext *cx, JSObject *obj, int id, jsval *vp);

	        static wxFileDialog *Construct(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, bool constructing);
	        static void Destruct(JSContext *cx, wxFileDialog *p);

	        WXJS_DECLARE_PROPERTY_MAP()
	        WXJS_DECLARE_CONSTANT_MAP()

	        enum 
	        {
		        P_DIRECTORY
		        , P_FILENAME
		        , P_FILENAMES
		        , P_FILTER_INDEX
		        , P_MESSAGE
		        , P_PATH
		        , P_PATHS
		        , P_WILDCARD
	        };
        };
    }; // namespace gui
}; // namespace wxjs

#endif //_WXJSFileDialog_H
