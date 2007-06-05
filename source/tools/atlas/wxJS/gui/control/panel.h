/*
 * wxJavaScript - panel.h
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
 * $Id: panel.h 598 2007-03-07 20:13:28Z fbraem $
 */
#ifndef _WXJSPanel_H
#define _WXJSPanel_H

/////////////////////////////////////////////////////////////////////////////
// Name:        panel.h
// Purpose:     Panel ports wxPanel to JavaScript
// Author:      Franky Braem
// Modified by:
// Created:     
// Copyright:   (c) 2001-2002 Franky Braem
// Licence:     LGPL
/////////////////////////////////////////////////////////////////////////////

namespace wxjs
{
    namespace gui
    {
        class Panel : public wxPanel
                        , public ApiWrapper<Panel, wxPanel>
                        , public Object
        {
        public:
	        Panel(JSContext *cx, JSObject *obj);

	        /**
	         * Destructor
	         */
	        virtual ~Panel();

	        static bool GetProperty(wxPanel *p, JSContext *cx, JSObject *obj, int id, jsval *vp);
	        static bool SetProperty(wxPanel *p, JSContext *cx, JSObject *obj, int id, jsval *vp);

	        static wxPanel* Construct(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, bool constructing);
            // Empty to avoid deleting. (It will be deleted by wxWindows).
            static void Destruct(JSContext *cx, wxPanel *p)
            {
            }
        	
            WXJS_DECLARE_PROPERTY_MAP()

	        /**
	         * Property Ids.
	         */
	        enum
	        {
		        P_DEFAULT_ITEM
	        };

            DECLARE_EVENT_TABLE()
	        void OnSysColourChanged(wxSysColourChangedEvent &event);
        };
    }; // namespace gui
}; // namespace wxjs
#endif //_WXJSPanel_H
