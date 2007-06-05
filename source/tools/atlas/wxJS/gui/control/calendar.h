/*
 * wxJavaScript - calendar.h
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
 * $Id: calendar.h 598 2007-03-07 20:13:28Z fbraem $
 */
#ifndef _WXJSCalendarCtrl_H
#define _WXJSCalendarCtrl_H

/////////////////////////////////////////////////////////////////////////////
// Name:        calendar.h
// Purpose:		CalendarCtrl ports wxCalendar to JavaScript.
// Author:      Franky Braem
// Modified by:
// Created:     02.07.02
// Copyright:   (c) 2001-2002 Franky Braem
// Licence:     LGPL
/////////////////////////////////////////////////////////////////////////////

namespace wxjs
{
    namespace gui
    {
        class CalendarCtrl : public wxCalendarCtrl
                               , public ApiWrapper<CalendarCtrl, wxCalendarCtrl>
                               , public Object
        {
        public:
	        /**
	         * Constructor
	         */
	        CalendarCtrl(JSContext *cx, JSObject *obj);

	        /**
	         * Destructor
	         */
	        virtual ~CalendarCtrl();

	        static bool GetProperty(wxCalendarCtrl *p, JSContext *cx, JSObject *obj, int id, jsval *vp);
	        static bool SetProperty(wxCalendarCtrl *p, JSContext *cx, JSObject *obj, int id, jsval *vp);

	        static wxCalendarCtrl* Construct(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, bool constructing);
            // Empty to avoid deleting. (It will be deleted by wxWindows).
            static void Destruct(JSContext *cx, wxCalendarCtrl *p)
            {
            }
        	
	        static JSBool setDateRange(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval);
	        static JSBool setHeaderColours(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval);
	        static JSBool setHighlightColours(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval);
	        static JSBool setHolidayColours(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval);

	        WXJS_DECLARE_PROPERTY_MAP()
	        WXJS_DECLARE_CONSTANT_MAP()
	        WXJS_DECLARE_METHOD_MAP()

	        /**
	         * Property Ids.
	         */
	        enum
	        {
		        P_DATE = WXJS_START_PROPERTY_ID
		        , P_LOWER_DATE_LIMIT
		        , P_UPPER_DATE_LIMIT
		        , P_ENABLE_HOLIDAY_DISPLAY
		        , P_ENABLE_YEAR_CHANGE
		        , P_ENABLE_MONTH_CHANGE
		        , P_HEADER_COLOUR_BG
		        , P_HEADER_COLOUR_FG
		        , P_HIGHLIGHT_COLOUR_BG
		        , P_HIGHLIGHT_COLOUR_FG
		        , P_HOLIDAY_COLOUR_BG
		        , P_HOLIDAY_COLOUR_FG
		        , P_ATTR
	        };

            DECLARE_EVENT_TABLE()

	        void OnCalendar(wxCalendarEvent &event);
	        void OnCalendarSelChanged(wxCalendarEvent &event);
	        void OnCalendarDay(wxCalendarEvent &event);
	        void OnCalendarMonth(wxCalendarEvent &event);
	        void OnCalendarYear(wxCalendarEvent &event);
	        void OnCalendarWeekDayClicked(wxCalendarEvent &event);
        };
    }; // namespace gui
}; // namespace wxjs

#endif //_WXJSCalendarCtrl_H
