/*
 * wxJavaScript - scrollwin.h
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
 * $Id: scrollwin.h 598 2007-03-07 20:13:28Z fbraem $
 */
#ifndef _WXJSScrollWinEvent_H
#define _WXJSScrollWinEvent_H

/////////////////////////////////////////////////////////////////////////////
// Name:        ScrollWinEvent.h
// Purpose:     ScrollWinEvent ports wxScrollWinEvent to JavaScript
// Author:      Franky Braem
// Modified by:
// Created:     24.08.2002
// Copyright:   (c) 2001-2002 Franky Braem
// Licence:     LGPL
/////////////////////////////////////////////////////////////////////////////

namespace wxjs
{
    namespace gui
    {
        typedef JSEvent<wxScrollWinEvent> PrivScrollWinEvent;

        class ScrollWinEvent : public ApiWrapper<ScrollWinEvent, PrivScrollWinEvent>
        {
        public:
            virtual ~ScrollWinEvent()
            {
            }

            static bool GetProperty(PrivScrollWinEvent *p, JSContext *cx, JSObject *obj, int id, jsval *vp);

            enum
            { 
              P_ORIENTATION
              , P_POSITION
            };

            WXJS_DECLARE_PROPERTY_MAP()
        };
    }; // namespace gui
}; // namespace wxjs

#endif //_WXJSScrollWinEvent_H
