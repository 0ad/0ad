#include "precompiled.h"

/*
 * wxJavaScript - pcxhdlr.cpp
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
 * $Id: pcxhdlr.cpp 598 2007-03-07 20:13:28Z fbraem $
 */
#ifndef WX_PRECOMP
    #include <wx/wx.h>
#endif

#include "../../common/main.h"
#include "../../common/type.h"
#include "../../common/jsutil.h"

#include "imghand.h"

using namespace wxjs;
using namespace wxjs::gui;

/***
 * <file>misc/pcxhdlr</file>
 * <module>gui</module>
 * <class name="wxPCXHandler" prototype="@wxImageHandler">
 *  Image handler for PNG images.
 *  When saving in PCX format, wxPCXHandler will count the number of different colours 
 *  in the image; if there are 256 or less colours, it will save as 8 bit, else it will 
 *  save as 24 bit.
 * </class>
 */
WXJS_INIT_CLASS(PCXHandler, "wxPCXHandler", 0)

ImageHandlerPrivate* PCXHandler::Construct(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, bool constructing)
{
    return new ImageHandlerPrivate(new wxPCXHandler(), true);
};

