#include "precompiled.h"

/*
 * wxJavaScript - jpghdlr.cpp
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
 * $Id: jpghdlr.cpp 598 2007-03-07 20:13:28Z fbraem $
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
 * <file>misc/jpghdlr</file>
 * <module>gui</module>
 * <class name="wxJPEGHandler" prototype="@wxImageHandler">
 *  Image handler for JPEG images.
 * </class>
 */
WXJS_INIT_CLASS(JPEGHandler, "wxJPEGHandler", 0)

ImageHandlerPrivate* JPEGHandler::Construct(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, bool constructing)
{
    return new ImageHandlerPrivate(new wxJPEGHandler(), true);
};
