/*
 * wxJavaScript - gridszr.h
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
 * $Id: gridszr.h 598 2007-03-07 20:13:28Z fbraem $
 */
#ifndef _WXJSGridSizer_H
#define _WXJSGridSizer_H

/////////////////////////////////////////////////////////////////////////////
// Name:        gridszr.h
// Purpose:		GridSizer ports wxGridSizer to JavaScript.
// Author:      Franky Braem
// Modified by:
// Created:     18.04.02
// Copyright:   (c) 2001-2002 Franky Braem
// Licence:     LGPL
/////////////////////////////////////////////////////////////////////////////

namespace wxjs
{
    class Object;

    namespace gui
    {

        class GridSizer : public wxGridSizer
					        , public ApiWrapper<GridSizer, GridSizer>
					        , public Object
					        , public AttachedSizer
        {
        public:
            /**
             * Constructor
             */
            GridSizer(JSContext *cx, JSObject *obj, int cols, int rows, int vgap, int hgap);
            GridSizer(JSContext *cx, JSObject *obj, int cols, int vgap = 0, int hgap = 0);

            /**
             * Destructor
             */
            virtual ~GridSizer()
            {
                if ( IsAttached() )
                {
                    JS_SetPrivate(GetContext(), GetObject(), NULL);
                }
            }

            /**
             * Callback for when a wxGridSizer object is created
             */
            static GridSizer* Construct(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, bool constructing);

            static void Destruct(JSContext *cx, GridSizer *p);
        };
    }; // namespace gui
}; // namespace wxjs

#endif //_WXJSGridSizer_H
