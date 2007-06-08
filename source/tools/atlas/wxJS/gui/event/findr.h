/*
 * wxJavaScript - findr.h
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
 * $Id: findr.h 598 2007-03-07 20:13:28Z fbraem $
 */
#ifndef _WXJSFindDialogEvent_H
#define _WXJSFindDialogEvent_H

/////////////////////////////////////////////////////////////////////////////
// Name:        findr.h
// Purpose:     Ports wxFindDialogEvent to JavaScript
// Author:      Franky Braem
// Modified by:
// Created:     06.08.2002
// Copyright:   (c) 2001-2002 Franky Braem
// Licence:     LGPL
/////////////////////////////////////////////////////////////////////////////

#include <wx/fdrepdlg.h>

namespace wxjs
{
    namespace gui
    {
        typedef JSEvent<wxFindDialogEvent> PrivFindDialogEvent;
        class FindDialogEvent : public ApiWrapper<FindDialogEvent, PrivFindDialogEvent>
        {
        public:
	        static bool GetProperty(PrivFindDialogEvent *p, JSContext *cx, JSObject *obj, int id, jsval *vp);

            virtual ~FindDialogEvent()
            {
            }

            enum
            { 
              P_FLAGS
              , P_FINDSTRING
              , P_REPLACESTRING
              , P_DIALOG
            };

            WXJS_DECLARE_PROPERTY_MAP()
        };
    }; // namespace gui
}; // namespace wxjs

#endif //_WXJSFindDialogEvent_H
