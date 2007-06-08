/*
 * wxJavaScript - dirtrav.h
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
 * $Id: dirtrav.h 715 2007-05-18 20:38:04Z fbraem $
 */
#ifndef _WXJSDirTraverser_H
#define _WXJSDirTraverser_H

/////////////////////////////////////////////////////////////////////////////
// Name:        dirtrav.h
// Purpose:     wxJSDirTraverser ports wxDirTraverser to JavaScript
// Author:      Franky Braem
// Modified by:
// Created:
// Copyright:   (c) 2001-2002 Franky Braem
// Licence:     LGPL
/////////////////////////////////////////////////////////////////////////////

#include <wx/dir.h>

namespace wxjs
{
    namespace io
    {
        class DirTraverser : public wxDirTraverser
                           , public ApiWrapper<DirTraverser, DirTraverser>
                           , public wxClientDataContainer
        {
        public:

            DirTraverser();

            virtual ~DirTraverser()
            {
            }

            wxDirTraverseResult OnFile(const wxString& name);
	        wxDirTraverseResult OnDir(const wxString& name);
            
	        WXJS_DECLARE_CONSTANT_MAP()

            /**
             * Callback for when a wxDirTraverser object is created
             */
            static DirTraverser* Construct(JSContext *cx,
                                           JSObject *obj,
                                           uintN argc,
                                           jsval *argv,
                                           bool constructing);
        };
    }; // namespace io
}; // namespace wxjs
#endif //_WXJSDirTraverser_H
