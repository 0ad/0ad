/*
 * wxJavaScript - bmpbtn.h
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
 * $Id: bmpbtn.h 598 2007-03-07 20:13:28Z fbraem $
 */
#ifndef _WXJSBitmapButton_H
#define _WXJSBitmapButton_H

/////////////////////////////////////////////////////////////////////////////
// Name:        bmpbtn.h
// Purpose:		BitmapButton ports wxBitmapButton to JavaScript.
// Author:      Franky Braem
// Modified by:
// Created:     01.08.02
// Copyright:   (c) 2001-2002 Franky Braem
// Licence:     LGPL
/////////////////////////////////////////////////////////////////////////////
namespace wxjs
{
    namespace gui
    {
        class BitmapButton : public wxBitmapButton
                               , public ApiWrapper<BitmapButton, wxBitmapButton>
                               , public Object
        {
        public:
	        /**
	         * Constructor
	         */
	        BitmapButton(JSContext *cx, JSObject *obj);

	        /**
	         * Destructor
	         */
	        virtual ~BitmapButton();

	        static bool GetProperty(wxBitmapButton *p, JSContext *cx, JSObject *obj, int id, jsval *vp);
	        static bool SetProperty(wxBitmapButton *p, JSContext *cx, JSObject *obj, int id, jsval *vp);

	        static wxBitmapButton* Construct(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, bool constructing);
            
            // Empty to avoid deleting. (It will be deleted by wxWindows).
            static void Destruct(JSContext *cx, wxBitmapButton *p)
            {
            }
        	
	        WXJS_DECLARE_PROPERTY_MAP()

	        /**
	         * Property Ids.
	         */
	        enum
	        {
		        P_BITMAP_DISABLED
		        , P_BITMAP_FOCUS
		        , P_BITMAP_SELECTED
		        , P_BITMAP_LABEL
	        };

            void OnClicked(wxCommandEvent &event);
	        DECLARE_EVENT_TABLE()

        };
    }; // namespace gui
}; //namespace wxjs
#endif //_WXJSBitmapButton_H
