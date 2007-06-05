/*
 * wxJavaScript - slider.h
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
 * $Id: slider.h 598 2007-03-07 20:13:28Z fbraem $
 */
#ifndef _WXJSSlider_H
#define _WXJSSlider_H

/////////////////////////////////////////////////////////////////////////////
// Name:        slider.h
// Purpose:     Slider ports wxSlider to JavaScript
// Author:      Franky Braem
// Modified by:
// Created:     19.08.2002
// Copyright:   (c) 2001-2002 Franky Braem
// Licence:     LGPL
/////////////////////////////////////////////////////////////////////////////
namespace wxjs
{
    namespace gui
    {
        class Slider : public wxSlider
                         , public ApiWrapper<Slider, wxSlider>
                         , public Object
        {
        public:
	        /**
	         * Constructor
	         */
	        Slider(JSContext *cx, JSObject *obj);

	        /**
	         * Destructor
	         */
	        virtual ~Slider();

	        /**
	         * Callback for retrieving properties
	         */
	        static bool GetProperty(wxSlider *p, JSContext *cx, JSObject *obj, int id, jsval *vp);
	        static bool SetProperty(wxSlider *p, JSContext *cx, JSObject *obj, int id, jsval *vp);

	        static wxSlider* Construct(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, bool constructing);
            // Empty to avoid deleting. (It will be deleted by wxWindows).
            static void Destruct(JSContext *cx, wxSlider *p)
            {
            }
        	
	        static JSBool clearSel(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval);
	        static JSBool clearTicks(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval);
	        static JSBool setRange(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval);
	        static JSBool setSelection(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval);
	        static JSBool setTickFreq(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval);

	        WXJS_DECLARE_PROPERTY_MAP()
	        WXJS_DECLARE_CONSTANT_MAP()
	        WXJS_DECLARE_METHOD_MAP()

	        /**
	         * Property Ids.
	         */
	        enum
	        {
		        P_LINESIZE
		        , P_MAX
		        , P_MIN
		        , P_PAGESIZE
		        , P_SEL_END
		        , P_SEL_START
		        , P_THUMB_LENGTH
		        , P_TICK
		        , P_VALUE
	        };
        	
            DECLARE_EVENT_TABLE()

	        void OnScroll(wxScrollEvent& event);
	        void OnScrollTop(wxScrollEvent& event);
	        void OnScrollBottom(wxScrollEvent& event);
	        void OnScrollLineUp(wxScrollEvent& event);
	        void OnScrollLineDown(wxScrollEvent& event);
	        void OnScrollPageUp(wxScrollEvent& event);
	        void OnScrollPageDown(wxScrollEvent& event);
	        void OnScrollThumbTrack(wxScrollEvent& event);
	        void OnScrollThumbRelease(wxScrollEvent& event);
        };
    }; // namespace gui
}; // namespace wxjs

#endif //_WXJSSlider_H
