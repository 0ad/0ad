#include "precompiled.h"

/*
 * wxJavaScript - imghand.cpp
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
 * $Id: imghand.cpp 598 2007-03-07 20:13:28Z fbraem $
 */
// imghand.cpp

#ifndef WX_PRECOMP
    #include <wx/wx.h>
#endif

#include "../../common/main.h"
#include "../../common/type.h"
#include "../../common/jsutil.h"

#include "../../io/jsstream.h"

#include "image.h"
#include "imghand.h"

using namespace wxjs;
using namespace wxjs::gui;

/***
 * <file>misc/imghand</file>
 * <module>gui</module>
 * <class name="wxImageHandler">
 *  wxImageHandler is the prototype for several image handlers.
 *  See @wxImage, @wxBMPHandler, @wxGIFHandler, @wxICOHandler,
 *  @wxJPEGHandler, @wxPCXHandler, @wxPNGHandler, @wxPNMHandler,
 *  @wxTIFFHandler, @wxXPMHandler
 * </class>
 */
WXJS_INIT_CLASS(ImageHandler, "wxImageHandler", 0)

/***
 * <properties>
 *  <property name="extension" type="String">
 *   Get/Set the handler extension.
 *  </property>
 *  <property name="mimeType" type="String">
 *   Get/Set the handler MIME type.
 *  </property>
 *  <property name="name" type="String">
 *   Get/Set the handler name.
 *  </property>
 *  <property name="type" type="Integer">
 *   Get/Set the handler type.
 *  </property>
 * </properties>
 */
WXJS_BEGIN_PROPERTY_MAP(ImageHandler)
    WXJS_PROPERTY(P_NAME, "name")
    WXJS_PROPERTY(P_EXTENSION, "extension")
    WXJS_PROPERTY(P_TYPE, "type")
    WXJS_PROPERTY(P_MIME_TYPE, "mimeType")
WXJS_END_PROPERTY_MAP()

bool ImageHandler::GetProperty(ImageHandlerPrivate *p, JSContext *cx, JSObject *obj, int id, jsval *vp)
{
    wxImageHandler *handler = p->GetHandler();
    switch (id)
    {
    case P_NAME:
        *vp = ToJS(cx, handler->GetName());
        break;
    case P_EXTENSION:
        *vp = ToJS(cx, handler->GetExtension());
        break;
    case P_TYPE:
        *vp = ToJS(cx, handler->GetType());
        break;
    case P_MIME_TYPE:
        *vp = ToJS(cx, handler->GetMimeType());
        break;
    }
    return true;
}

bool ImageHandler::SetProperty(ImageHandlerPrivate *p, JSContext *cx, JSObject *obj, int id, jsval *vp)
{
    wxImageHandler *handler = p->GetHandler();

    switch (id)
    {
    case P_NAME:
        {
            wxString name;
            FromJS(cx, *vp, name);
            handler->SetName(name);
            break;
        }
    case P_EXTENSION:
        {
            wxString ext;
            FromJS(cx, *vp, ext);
            handler->SetName(ext);
            break;
        }
    case P_TYPE:
        {
            long type;
            if ( FromJS(cx, *vp, type) )
                handler->SetType(type);
            break;
        }
    case P_MIME_TYPE:
        {
            wxString mime;
            FromJS(cx, *vp, mime);
            handler->SetName(mime);
            break;
        }
    }
    return true;
}

WXJS_BEGIN_METHOD_MAP(ImageHandler)
    WXJS_METHOD("getImageCount", getImageCount, 1)
    WXJS_METHOD("loadFile", loadFile, 2)
    WXJS_METHOD("saveFile", saveFile, 2)
WXJS_END_METHOD_MAP()

/***
 * <method name="getImageCount">
 *  <function returns="Integer">
 *   <arg name="Stream" type="@wxInputStream">
 *    Opened input stream for reading image data. Currently, the stream must support seeking.
 *   </arg>
 *  </function>
 *  <desc>
 *   If the image file contains more than one image and the image handler is capable 
 *   of retrieving these individually, this function will return the number of available images.
 *   Most imagehandlers return 1.
 *  </desc>
 * </method>
 */
JSBool ImageHandler::getImageCount(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
    ImageHandlerPrivate *p = GetPrivate(cx, obj);
    if ( p == NULL )
        return JS_FALSE;

    io::Stream *stream = NULL;
    if ( wxjs::HasPrototype(cx, argv[0], "wxInputStream") )
        stream = (io::Stream *) JS_GetPrivate(cx, JSVAL_TO_OBJECT(argv[0]));
    else
        return JS_FALSE;

    wxInputStream *in = dynamic_cast<wxInputStream *>(stream->GetStream());
    if ( in == NULL )
    {
        // TODO: error reporting
        return JS_FALSE;
    }

    *rval = ToJS(cx, p->GetHandler()->GetImageCount(*in));

    return JS_FALSE;
}

/***
 * <method name="loadFile">
 *  <function returns="Boolean">
 *   <arg name="Image" type="@wxImage">
 *    The object that gets the loaded image.
 *   </arg>
 *   <arg name="Stream" type="@wxInputStream">
 *   Opened input stream for reading image data.
 *   </arg>
 *   <arg name="Verbose" type="Boolean" default="true">
 *    When true the image handler will report errors using wxLog
 *   </arg>
 *   <arg name="Index" type="Integer" default="0">
 *    The index of the image in the file (zero-based).
 *   </arg>
 *  </function>
 *  <desc>
 *   Loads a image from a stream, putting the resulting data into image. 
 *   If the image file contains more than one image and the image handler is capable 
 *   of retrieving these individually, index indicates which image to read from the stream.
 *   Returns true when successful, false otherwise.
 *  </desc>
 * </method>
 */
JSBool ImageHandler::loadFile(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
    ImageHandlerPrivate *p = ImageHandler::GetPrivate(cx, obj);
    if ( p == NULL )
        return JS_FALSE;

    int idx = 0;
    bool verbose = true;

    switch(argc)
    {
    case 4:
        if ( ! FromJS(cx, argv[3], idx) )
            return JS_FALSE;
    case 3:
        if ( ! FromJS(cx, argv[2], verbose) )
            return JS_FALSE;
    default:
        wxImage *img = Image::GetPrivate(cx, argv[0]);
        if ( img == NULL )
            return JS_FALSE;

        io::Stream *stream = NULL;
        if ( wxjs::HasPrototype(cx, argv[0], "wxInputStream") )
            stream = (io::Stream *) JS_GetPrivate(cx, JSVAL_TO_OBJECT(argv[0]));
        else
            return JS_FALSE;

        wxInputStream *in = dynamic_cast<wxInputStream *>(stream->GetStream());
        if ( in == NULL )
        {
            // TODO: error reporting
            return JS_FALSE;
        }

        *rval = ToJS(cx, p->GetHandler()->LoadFile(img, *in, verbose, idx));
    }

    return JS_TRUE;
}

/***
 * <method name="saveFile">
 *  <function returns="Boolean">
 *   <arg name="Image" type="@wxImage">
 *    The object that gets the loaded image.
 *   </arg>
 *   <arg name="Stream" type="@wxOutputStream">
 *    Opened output stream for writing the image data.
 *   </arg>
 *   <arg name="Verbose" type="Boolean" default="true">
 *    When true the image handler will report errors using wxLog
 *   </arg>
 *  </function>
 *  <desc>
 *   Saves a image in the output stream. Returns true on success, false otherwise.
 *  </desc>
 * </method>
 */
JSBool ImageHandler::saveFile(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
    ImageHandlerPrivate *p = GetPrivate(cx, obj);
    if ( p == NULL )
        return JS_FALSE;

    if ( argc > 3 )
        argc = 3;

    bool verbose = true;

    switch(argc)
    {
    case 3:
        if ( ! FromJS(cx, argv[2], verbose) )
            return JS_FALSE;
    default:
        wxImage *img = Image::GetPrivate(cx, argv[0]);
        if ( img == NULL )
            return JS_FALSE;

        io::Stream *stream = NULL;
        if ( wxjs::HasPrototype(cx, argv[0], "wxOutputStream") )
            stream = (io::Stream *) JS_GetPrivate(cx, JSVAL_TO_OBJECT(argv[0]));
        else
            return JS_FALSE;

        wxOutputStream *out = dynamic_cast<wxOutputStream *>(stream->GetStream());
        if ( out == NULL )
        {
            // TODO: error reporting
            return JS_FALSE;
        }

        *rval = ToJS(cx, p->GetHandler()->SaveFile(img, *out, verbose));
    }

    return JS_TRUE;
}
