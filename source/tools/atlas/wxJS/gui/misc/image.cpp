#include "precompiled.h"

/*
 * wxJavaScript - image.cpp
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
 * $Id: image.cpp 810 2007-07-13 20:07:05Z fbraem $
 */

#include <wx/wx.h>

#include "../../common/main.h"
#include "../../common/jsutil.h"
#include "../../ext/wxjs_ext.h"

#include "image.h"
#include "size.h"
#include "rect.h"
#include "colour.h"
#include "imghand.h"

#include "../../io/jsstream.h"

using namespace wxjs;
using namespace wxjs::gui;

/***
 * <file>misc/image</file>
 * <module>gui</module>
 * <class name="wxImage">
 *  This class encapsulates a platform-independent image. An image can be loaded from a 
 *  file in a variety of formats.
 * </class>
 */
WXJS_INIT_CLASS(Image, "wxImage", 0)

/***
 * <properties>
 *  <property name="hasMask" type="Boolean" readonly="Y">
 *   Returns true when a mask is active.
 *  </property>
 *  <property name="hasPalette" type="Boolean" readonly="Y">
 *   Returns true when a palette is used.
 *  </property>
 *  <property name="height" type="Integer" readonly="Y">
 *   Gets the height of the image in pixels.
 *  </property>
 *  <property name="mask" type="Boolean">
 *   Specifies whether there is a mask or not. The area of the mask is determined 
 *   by the current mask colour.
 *  </property>
 *  <property name="maskBlue" type="Integer">
 *   Returns the blue value of the mask colour.
 *  </property>
 *  <property name="maskGreen" type="Integer">
 *   Returns the green value of the mask colour.
 *  </property>
 *  <property name="maskRed" type="Integer">
 *  Returns the red value of the mask colour.
 *  </property>
 *  <property name="ok" type="Boolean" readonly="Y">
 *  Returns true when the image data is available.
 *  </property>
 *  <property name="size" type="@wxSize" readonly="Y">
 *   Returns the size of the image in pixels.
 *  </property>
 *  <property name="width" type="Integer" readonly="Y">
 *   Returns the width of the image in pixels.
 *  </property>
 * </properties>
 */
WXJS_BEGIN_PROPERTY_MAP(Image)
    WXJS_READONLY_PROPERTY(P_OK, "ok")
    WXJS_PROPERTY(P_MASK_RED, "maskRed")
    WXJS_PROPERTY(P_MASK_GREEN, "maskGreen")
    WXJS_PROPERTY(P_MASK_BLUE, "maskBlue")
    WXJS_PROPERTY(P_WIDTH, "width")
    WXJS_PROPERTY(P_HEIGHT, "height")
    WXJS_PROPERTY(P_MASK, "mask")
    WXJS_READONLY_PROPERTY(P_HAS_MASK, "hasMask")
    WXJS_READONLY_PROPERTY(P_HAS_PALETTE, "hasPalette")
    WXJS_PROPERTY(P_PALETTE, "palette")
    WXJS_READONLY_PROPERTY(P_SIZE, "size")
WXJS_END_PROPERTY_MAP()

bool Image::GetProperty(wxImage *p, JSContext *cx, JSObject *obj, int id, jsval *vp)
{
    switch (id)
    {
    case P_OK:
        *vp = ToJS(cx, p->Ok());
        break;
    case P_MASK_RED:
        *vp = ToJS(cx, (int) p->GetMaskRed());
        break;
    case P_MASK_GREEN:
        *vp = ToJS(cx, (int) p->GetMaskGreen());
        break;
    case P_MASK_BLUE:
        *vp = ToJS(cx, (int) p->GetMaskBlue());
        break;
    case P_WIDTH:
        *vp = ToJS(cx, p->GetWidth());
        break;
    case P_HEIGHT:
        *vp = ToJS(cx, p->GetHeight());
        break;
    case P_MASK:
    case P_HAS_MASK:
        *vp = ToJS(cx, p->HasMask());
        break;
    case P_HAS_PALETTE:
        *vp = ToJS(cx, p->HasPalette());
        break;
    case P_PALETTE:
        *vp = JSVAL_VOID;
        break;
    case P_SIZE:
        *vp = Size::CreateObject(cx, new wxSize(p->GetWidth(), p->GetHeight()));
        break;
    }
    return true;
}

bool Image::SetProperty(wxImage *p, JSContext *cx, JSObject *obj, int id, jsval *vp)
{
    switch (id)
    {
    case P_MASK_RED:
        {
            int maskRed;
            if ( FromJS(cx, *vp, maskRed) )
            {
                unsigned char g = p->GetMaskGreen();
                unsigned char b = p->GetMaskBlue();
                p->SetMaskColour((unsigned char) maskRed, g, b);
            }
            break;
        }
    case P_MASK_GREEN:
        {
            int maskGreen;
            if ( FromJS(cx, *vp, maskGreen) )
            {
                unsigned char r = p->GetMaskRed();
                unsigned char b = p->GetMaskBlue();
                p->SetMaskColour(r, (unsigned char) maskGreen, b);
            }
            break;
        }
    case P_MASK_BLUE:
        {
            int maskBlue;
            if ( FromJS(cx, *vp, maskBlue) )
            {
                unsigned char r = p->GetMaskRed();
                unsigned char g = p->GetMaskGreen();
                p->SetMaskColour(r, g, (unsigned char) maskBlue);
            }
            break;
        }
    case P_MASK:
        {
            bool mask;
            if ( FromJS(cx, *vp, mask) )
                p->SetMask(mask);
            break;
        }
    case P_PALETTE:
        break;
    }
    return true;
}

/***
 * <class_properties>
 *  <property name="handlers" type="Array" readonly="Y">
 *   Array of @wxImageHandler elements. Get the available list of image handlers.
 *  </property>
 * </class_properties>
 */
WXJS_BEGIN_STATIC_PROPERTY_MAP(Image)
    WXJS_READONLY_PROPERTY(P_HANDLERS, "handlers")
WXJS_END_PROPERTY_MAP()

bool Image::GetStaticProperty(JSContext *cx, int id, jsval *vp)
{
	if ( id == P_HANDLERS )
	{
        wxList handlers = wxImage::GetHandlers();
		jsint count = handlers.GetCount();
		JSObject *objHandlers = JS_NewArrayObject(cx, count, NULL);

		*vp = OBJECT_TO_JSVAL(objHandlers);

        jsint i = 0;
        for ( wxNode *node = handlers.GetFirst(); node; node = node->GetNext() )
        {
            wxImageHandler *handler = (wxImageHandler*) node->GetData();
            ImageHandlerPrivate *priv = new ImageHandlerPrivate(handler, false);
            jsval element = ImageHandler::CreateObject(cx, priv, NULL);
			JS_SetElement(cx, objHandlers, i++, &element);
        }
    }
    return true;
}

/***
 * <ctor>
 *  <function />
 *  <function>
 *   <arg name="Width" type="Integer">
 *    The width of the image.
 *   </arg>
 *   <arg name="Height" type="Integer">
 *    The height of the image.
 *   </arg>
 *  </function>
 *  <function>
 *   <arg name="size" type="@wxSize">
 *    The size of the image.
 *   </arg>
 *  </function>
 *  <function>
 *   <arg name="Name" type="String">
 *    The name of a file from which to load the image.
 *   </arg>
 *   <arg name="Type" type="Integer" default="wxBitmapType.ANY">
 *    The type of the image.
 *   </arg>
 *  </function>
 *  <function>
 *   <arg name="Name" type="String">
 *    The name of a file from which to load the image.
 *   </arg>
 *   <arg name="MimeType" type="String">
 *    The MIME type of the image.
 *   </arg>
 *  </function>
 * </ctor>
 */
wxImage* Image::Construct(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, bool constructing)
{
    if ( argc == 0 )
        return new wxImage();

    if ( Size::HasPrototype(cx, argv[0]) )
    {
        wxSize *size = Size::GetPrivate(cx, argv[0], false);
        return new wxImage(size->GetWidth(), size->GetHeight());
    }

    if ( JSVAL_IS_INT(argv[0]) )
    {
        int width;
        int height;

        if (    FromJS(cx, argv[0], width)
             && FromJS(cx, argv[1], height) )
             return new wxImage(width, height);

        return NULL;
    }

    wxString name;
    FromJS(cx, argv[0], name);

    long type = wxBITMAP_TYPE_ANY;

    if ( argc > 1 )
    {
        if ( JSVAL_IS_NUMBER(argv[1]) )
        {
            if ( ! FromJS(cx, argv[1], type) )
                return NULL;
        }
        else
        {
            wxString mimetype;
            FromJS(cx, argv[1], mimetype);
            return new wxImage(name, mimetype);
        }
    }
    
    return new wxImage(name, type);
}

WXJS_BEGIN_METHOD_MAP(Image)
    WXJS_METHOD("create", create, 1)
    WXJS_METHOD("destroy", destroy, 0)
    WXJS_METHOD("copy", copy, 0)
    WXJS_METHOD("getSubImage", getSubImage, 1)
    WXJS_METHOD("paste", paste, 3)
    WXJS_METHOD("scale", scale, 1)
    WXJS_METHOD("rescale", rescale, 1)
    WXJS_METHOD("rotate", rotate, 2)
    WXJS_METHOD("rotate90", rotate90, 0)
    WXJS_METHOD("mirror", mirror, 0)
    WXJS_METHOD("replace", replace, 2)
    WXJS_METHOD("convertToMono", convertToMono, 1)
    WXJS_METHOD("setRGB", setRGB, 3)
    WXJS_METHOD("getRed", getRed, 2)
    WXJS_METHOD("getGreen", getGreen, 2)
    WXJS_METHOD("getBlue", getBlue, 2)
    WXJS_METHOD("getColour", getColour, 2)
    WXJS_METHOD("findFirstUnusedColour", findFirstUnusedColour, 2)
    WXJS_METHOD("setMaskFromImage", setMaskFromImage, 2)
    WXJS_METHOD("loadFile", loadFile, 1)
    WXJS_METHOD("saveFile", saveFile, 2)
    WXJS_METHOD("setMaskColour", setMaskColour, 1)
    WXJS_METHOD("setOption", setOption, 2)
    WXJS_METHOD("getOption", getOption, 1)
    WXJS_METHOD("getOptionInt", getOptionInt, 1)
    WXJS_METHOD("hasOption", hasOption, 1)
WXJS_END_METHOD_MAP()

/***
 * <method name="create">
 *  <function>
 *   <arg name="Width" type="Integer">
 *    The width of the image.
 *   </arg>
 *   <arg name="Height" type="Integer">
 *    The height of the image.
 *   </arg>
 *  </function>
 *  <function>
 *   <arg name="Size" type="@wxSize">
 *    The size of the image.
 *   </arg>
 *  </function>
 *  <desc>
 *   Creates a fresh image with the given size.
 *  </desc>
 * </method>
 */
JSBool Image::create(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
    wxImage *p = GetPrivate(cx, obj);
    if ( p == NULL )
        return JS_FALSE;

    if ( Size::HasPrototype(cx, argv[0]) )
    {
        wxSize *size = Size::GetPrivate(cx, argv[0], false);
        p->Create(size->GetWidth(), size->GetHeight());
        return JS_TRUE;
    }
    else if ( argc == 2 )
    {
        int width;
        int height;
        if (    FromJS(cx, argv[0], width)
             && FromJS(cx, argv[1], height) )
        {
            p->Create(width, height);
            return JS_TRUE;
        }
    }
    return JS_FALSE;
}

/***
 * <method name="destroy">
 *  <function />
 *  <desc>
 *   Destroys the image data.
 *  </desc>
 * </method>
 */
JSBool Image::destroy(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
    wxImage *p = GetPrivate(cx, obj);
    if ( p == NULL )
        return JS_FALSE;
    
    p->Destroy();
    return JS_TRUE;
}

/***
 * <method name="copy">
 *  <function returns="wxImage" />
 *  <desc>
 *   Returns an identical copy of image.
 *  </desc>
 * </method>
 */
JSBool Image::copy(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
    wxImage *p = GetPrivate(cx, obj);
    if ( p == NULL )
        return JS_FALSE;
    
    *rval = CreateObject(cx, new wxImage(p->Copy()));
    return JS_TRUE;
}

/***
 * <method name="getSubImage">
 *  <function returns="wxImage">
 *   <arg name="Rect" type="@wxRect" />
 *  </function>
 *  <desc>
 *   Returns a sub image of the current one as long as the rect belongs entirely to the image.
 *  </desc>
 * </method>
 */
JSBool Image::getSubImage(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
    wxImage *p = GetPrivate(cx, obj);
    if ( p == NULL )
        return JS_FALSE;
    
    wxRect *rect = Rect::GetPrivate(cx, argv[0]);
    if ( rect != NULL )
    {
        *rval = CreateObject(cx, new wxImage(p->GetSubImage(*rect)));
        return JS_TRUE;
    }
    return JS_FALSE;
}

/***
 * <method name="paste">
 *  <function>
 *   <arg name="Image" type="wxImage">
 *    The image to paste.
 *   </arg>
 *   <arg name="X" type="Integer">
 *    The x position.
 *   </arg>
 *   <arg name="Y" type="Integer">
 *    The y position.
 *   </arg>
 *  </function>
 *  <desc>
 *   Pastes image into this instance and takes care of
 *   the mask colour and out of bounds problems.
 *  </desc>
 * </method>
 */
JSBool Image::paste(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
    wxImage *p = GetPrivate(cx, obj);
    if ( p == NULL )
        return JS_FALSE;

    wxImage *img = GetPrivate(cx, argv[0]);
    if ( img == NULL )
        return JS_FALSE;

    int x;
    int y;

    if (    FromJS(cx, argv[1], x)
         && FromJS(cx, argv[2], y) )
    {
        p->Paste(*img, x, y);
        return JS_TRUE;
    }
    return JS_FALSE;
}

/***
 * <method name="scale">
 *  <function returns="wxImage">
 *   <arg name="Width" type="Integer">
 *    The width of the new image.
 *   </arg>
 *   <arg name="Height" type="Integer">
 *    The height of the new image.
 *   </arg>
 *  </function>
 *  <function returns="wxImage">
 *   <arg name="Size" type="@wxSize">
 *    The size of the new image.
 *   </arg>
 *  </function>
 *  <desc>
 *   Returns a scaled version of the image with size Width * Height.
 *  </desc>
 * </method>
 */
JSBool Image::scale(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
    wxImage *p = GetPrivate(cx, obj);
    if ( p == NULL )
        return JS_FALSE;

    if ( Size::HasPrototype(cx, argv[0]) )
    {
        wxSize *size = Size::GetPrivate(cx, argv[0], false);
        *rval = CreateObject(cx, new wxImage(p->Scale(size->GetWidth(), size->GetHeight())));
        return JS_TRUE;
    }
    else if ( argc == 2 )
    {
        int width;
        int height;
        if (    FromJS(cx, argv[0], width)
             && FromJS(cx, argv[1], height) )
        {
            *rval = CreateObject(cx, new wxImage(p->Scale(width, height)));
            return JS_TRUE;
        }
    }
    return JS_FALSE;
}

/***
 * <method name="rescale">
 *  <function returns="wxImage">
 *   <arg name="Width" type="Integer">
 *    The width of the new image.
 *   </arg>
 *   <arg name="Height" type="Integer">
 *    The height of the new image.
 *   </arg>
 *  </function>
 *  <function returns="wxImage">
 *   <arg name="Size" type="@wxSize">
 *    The size of the new image.
 *   </arg>
 *  </function>
 *  <desc>
 *   Changes the size of the image in-place: after a call to this function, 
 *   the image will have the given width and height.
 *   Returns the (modified) image itself.
 *  </desc>
 * </method>
 */
JSBool Image::rescale(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
    wxImage *p = GetPrivate(cx, obj);
    if ( p == NULL )
        return JS_FALSE;

    if ( Size::HasPrototype(cx, argv[0]) )
    {
        wxSize *size = Size::GetPrivate(cx, argv[0], false);
        *rval = CreateObject(cx, new wxImage(p->Rescale(size->GetWidth(), size->GetHeight())));
        return JS_TRUE;
    }
    else if ( argc == 2 )
    {
        int width;
        int height;
        if (    FromJS(cx, argv[0], width)
             && FromJS(cx, argv[1], height) )
        {
            *rval = CreateObject(cx, new wxImage(p->Rescale(width, height)));
            return JS_TRUE;
        }
    }
    return JS_FALSE;
}

/***
 * <method name="rotate">
 *  <function returns="wxImage">
 *   <arg name="Angle" type="Double">
 *    The rotation angle.
 *   </arg>
 *   <arg name="Center" type="@wxPoint">
 *    The height of the new image.
 *   </arg>
 *   <arg name="InterPol" type="Boolean" default="true">
 *    Interpolates the new image.
 *   </arg>
 *   <arg name="Offset" type="@wxPoint" default="null" />
 *  </function>
 *  <desc>
 *   Rotates the image about the given point, by angle radians. Passing true for interpolating 
 *   results in better image quality, but is slower. If the image has a mask, then the mask colour 
 *   is used for the uncovered pixels in the rotated image background. Else, black will be used.
 *   Returns the rotated image, leaving this image intact.
 *  </desc>
 * </method>
 */
JSBool Image::rotate(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
    wxImage *p = GetPrivate(cx, obj);
    if ( p == NULL )
        return JS_FALSE;

    if ( argc > 4 )
        argc = 4;

    wxPoint *offset = NULL;
    bool interpol = true;

    switch(argc)
    {
    case 4:
      offset = wxjs::ext::GetPoint(cx, argv[3]);
        if ( offset == NULL )
            break;
        // Fall through
    case 3:
        if ( ! FromJS(cx, argv[2], interpol) )
            break;
    default:
      wxPoint *center = wxjs::ext::GetPoint(cx, argv[1]);
        if ( center == NULL )
            break;

        double angle;
        if ( ! FromJS(cx, argv[0], angle) )
            break;

        *rval = CreateObject(cx, new wxImage(p->Rotate(angle, *center, interpol, offset)));
        return JS_TRUE;
    }

    return JS_FALSE;
}

/***
 * <method name="rotate90">
 *  <function returns="wxImage">
 *   <arg name="ClockWise" type="Boolean" default="true">
 *    The direction of the rotation. Default is true.
 *   </arg>
 *  </function>
 *  <desc>
 *   Returns a copy of the image rotated 90 degrees in the direction indicated by clockwise.
 *  </desc>
 * </method>
 */
JSBool Image::rotate90(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
    wxImage *p = GetPrivate(cx, obj);
    if ( p == NULL )
        return JS_FALSE;

    bool clockwise = true;
    if (    argc > 0 
         && ! FromJS(cx, argv[0], clockwise) )
        return JS_FALSE;

    *rval = CreateObject(cx, new wxImage(p->Rotate90(clockwise)));
    return JS_TRUE;
}

/***
 * <method name="mirror">
 *  <function returns="wxImage">
 *   <arg name="Horizontally" type="Boolean" default="Y">
 *    The orientation.
 *   </arg>
 *  </function>
 *  <desc>
 *   Returns a mirrored copy of the image. The parameter horizontally indicates the orientation.
 *  </desc>
 * </method>
 */
JSBool Image::mirror(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
    wxImage *p = GetPrivate(cx, obj);
    if ( p == NULL )
        return JS_FALSE;

    bool hor = true;
    if (    argc > 0 
         && ! FromJS(cx, argv[0], hor) )
        return JS_FALSE;

    *rval = CreateObject(cx, new wxImage(p->Mirror(hor)));
    return JS_TRUE;
}

/***
 * <method name="replace">
 *  <function>
 *   <arg name="R1" type="Integer" />
 *   <arg name="G1" type="Integer" />
 *   <arg name="B1" type="Integer" />
 *   <arg name="R2" type="Integer" />
 *   <arg name="G2" type="Integer" />
 *   <arg name="B2" type="Integer" />
 *  </function>
 *  <function>
 *   <arg name="Colour1" type="@wxColour" />
 *   <arg name="Colour2" type="@wxColour" />
 *  </function>
 *  <desc>
 *   Replaces the colour with another colour.
 *  </desc>
 * </method>
 */
JSBool Image::replace(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
    wxImage *p = GetPrivate(cx, obj);
    if ( p == NULL )
        return JS_FALSE;

    if ( argc == 2 )
    {
        wxColour *colour1;
        wxColour *colour2;
        
        if (    (colour1 = Colour::GetPrivate(cx, argv[0])) != NULL
             && (colour2 = Colour::GetPrivate(cx, argv[1])) != NULL )
        {
            p->Replace(colour1->Red(), colour1->Green(), colour1->Blue(),
                       colour2->Red(), colour2->Green(), colour2->Blue());
            return JS_TRUE;
        }
    }
    else if ( argc == 6 )
    {
        int r1, g1, b1, r2, g2, b2;
        if (    FromJS(cx, argv[0], r1)
             && FromJS(cx, argv[1], g1)
             && FromJS(cx, argv[2], b1)
             && FromJS(cx, argv[3], r2)
             && FromJS(cx, argv[4], g2)
             && FromJS(cx, argv[5], b2) )
        {
            p->Replace(r1, g1, b1, r2, g2, b2);
            return JS_TRUE;
        }
    }

    return JS_FALSE;
}

/***
 * <method name="convertToMono">
 *  <function returns="wxImage">
 *   <arg name="R" type="Integer" />
 *   <arg name="G" type="Integer" />
 *   <arg name="B" type="Integer" />
 *  </function>
 *  <function returns="wxImage">
 *   <arg name="Colour" type="@wxColour" />
 *  </function>
 *  <desc>
 *   Converts to a monochrome image. The returned image has white colour where the original has (r,g,b) 
 *   colour and black colour everywhere else.
 *  </desc>
 * </method>
 */
JSBool Image::convertToMono(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
    wxImage *p = GetPrivate(cx, obj);
    if ( p == NULL )
        return JS_FALSE;

    if ( argc == 1 )
    {
        wxColour *colour = Colour::GetPrivate(cx, argv[0]);
        
        if ( colour != NULL )
        {
            *rval = CreateObject(cx, new wxImage(p->ConvertToMono(colour->Red(), colour->Green(), colour->Blue())));
            return JS_TRUE;
        }
    }
    else if ( argc == 3 )
    {
        int r, g, b;
        if (    FromJS(cx, argv[0], r)
             && FromJS(cx, argv[1], g)
             && FromJS(cx, argv[2], b) )
        {
            *rval = CreateObject(cx, new wxImage(p->ConvertToMono(r, g, b)));
            return JS_TRUE;
        }
    }

    return JS_FALSE;
}

/***
 * <method name="setRGB">
 *  <function>
 *   <arg name="X" type="Integer">The x position</arg>
 *   <arg name="Y" type="Integer">The y position</arg>
 *   <arg name="R" type="Integer" />
 *   <arg name="G" type="Integer" />
 *   <arg name="B" type="Integer" />
 *  </function>
 *  <function>
 *   <arg name="X" type="Integer">The x position</arg>
 *   <arg name="Y" type="Integer">The y position</arg>
 *   <arg name="Colour" type="@wxColour" />
 *  </function>
 *  <desc>
 *   Sets the colour of the pixel at the given coordinate. 
 *   This routine performs bounds-checks for the coordinate so it can be considered a safe way 
 *   to manipulate the data, but in some cases this might be too slow.
 *  </desc>
 * </method>
 */
JSBool Image::setRGB(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
    wxImage *p = GetPrivate(cx, obj);
    if ( p == NULL )
        return JS_FALSE;

    int x = 0;
    int y = 0;

    if (    ! FromJS(cx, argv[0], x)
         && ! FromJS(cx, argv[1], y) )
         return JS_FALSE;

    if ( Colour::HasPrototype(cx, argv[2]) )
    {
        wxColour *colour = Colour::GetPrivate(cx, argv[2], false);
        p->SetRGB(x, y, colour->Red(), colour->Green(), colour->Blue());
        return JS_TRUE;
    }
    else if ( argc == 5 )
    {
        int r, g, b;
        if (    FromJS(cx, argv[2], r)
             && FromJS(cx, argv[3], g)
             && FromJS(cx, argv[4], b) )
        {
            p->SetRGB(x, y, r, g, b);
            return JS_TRUE;
        }
    }

    return JS_FALSE;
}

/***
 * <method name="getRed">
 *  <function returns="Integer">
 *   <arg name="X" type="Integer">The x position</arg>
 *   <arg name="Y" type="Integer">The y position</arg>
 *  </function>
 *  <desc>
 *   Returns the red intensity at the given coordinate.
 *  </desc>
 * </method>
 */
JSBool Image::getRed(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
    wxImage *p = GetPrivate(cx, obj);
    if ( p == NULL )
        return JS_FALSE;

    int x = 0;
    int y = 0;

    if (    ! FromJS(cx, argv[0], x)
         && ! FromJS(cx, argv[1], y) )
         return JS_FALSE;

    *rval = ToJS(cx, (int) p->GetRed(x, y));
    return JS_TRUE;
}

/***
 * <method name="getGreen">
 *  <function returns="Integer">
 *   <arg name="X" type="Integer">The x position</arg>
 *   <arg name="Y" type="Integer">The y position</arg>
 *  </function>
 *  <desc>
 *   Returns the green intensity at the given coordinate.
 *  </desc>
 * </method>
 */
JSBool Image::getGreen(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
    wxImage *p = GetPrivate(cx, obj);
    if ( p == NULL )
        return JS_FALSE;

    int x = 0;
    int y = 0;

    if (    ! FromJS(cx, argv[0], x)
         && ! FromJS(cx, argv[1], y) )
         return JS_FALSE;

    *rval = ToJS(cx, (int) p->GetGreen(x, y));
    return JS_TRUE;
}

/***
 * <method name="getBlue">
 *  <function returns="Integer">
 *   <arg name="X" type="Integer">The x position</arg>
 *   <arg name="Y" type="Integer">The y position</arg>
 *  </function>
 *  <desc>
 *   Returns the blue intensity at the given coordinate.
 *  </desc>
 * </method>
 */
JSBool Image::getBlue(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
    wxImage *p = GetPrivate(cx, obj);
    if ( p == NULL )
        return JS_FALSE;

    int x = 0;
    int y = 0;

    if (    ! FromJS(cx, argv[0], x)
         && ! FromJS(cx, argv[1], y) )
         return JS_FALSE;

    *rval = ToJS(cx, (int) p->GetBlue(x, y));
    return JS_TRUE;
}

/***
 * <method name="getColour">
 *  <function returns="@wxColour">
 *   <arg name="X" type="Integer">The x position</arg>
 *   <arg name="Y" type="Integer">The y position</arg>
 *  </function>
 *  <desc>
 *   Returns the colour of the given coordinate.
 *  </desc>
 * </method>
 */
JSBool Image::getColour(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
    wxImage *p = GetPrivate(cx, obj);
    if ( p == NULL )
        return JS_FALSE;

    int x = 0;
    int y = 0;

    if (    ! FromJS(cx, argv[0], x)
         && ! FromJS(cx, argv[1], y) )
         return JS_FALSE;

    *rval = Colour::CreateObject(cx, new wxColour(p->GetRed(x, y), p->GetGreen(x, y), p->GetBlue(x, y)));
    return JS_TRUE;
}

/***
 * <method name="findFirstUnusedColour">
 *  <function returns="Boolean">
 *   <arg name="Colour" type="@wxColour">
 *    The colour found. Only updated when a colour is found.
 *   </arg>
 *   <arg name="StartColour" type="@wxColour">
 *    The start colour.
 *   </arg>
 *  </function>
 *  <desc>
 *   Finds the first colour that is never used in the image. The search begins at given start 
 *   colour and continues by increasing R, G and B components (in this order) by 1 until an 
 *   unused colour is found or the colour space exhausted. 
 *   Returns true on success. False when there are no colours left.
 *   JavaScript can't pass primitive values by reference, so this method uses @wxColour
 *   instead of separate r, g, b arguments.
 *  </desc>
 * </method>
 */
JSBool Image::findFirstUnusedColour(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
    wxImage *p = GetPrivate(cx, obj);
    if ( p == NULL )
        return JS_FALSE;

    wxColour *colour = Colour::GetPrivate(cx, argv[0]);
    if ( colour == NULL )
        return JS_FALSE;

    wxColour *start = Colour::GetPrivate(cx, argv[1]);
    if ( start == NULL )
        return JS_FALSE;

    unsigned char r, g, b;

    if ( p->FindFirstUnusedColour(&r, &g, &b, start->Red(), start->Green(), start->Blue()) )
    {
        *rval = JSVAL_TRUE;
        colour->Set(r, g, b);
    }
    else
    {
        *rval = JSVAL_FALSE;
    }
    return JS_TRUE;
}

/***
 * <method name="setMaskFromImage">
 *  <function returns="Boolean">
 *   <arg name="Image" type="wxImage">
 *    The mask image to extract mask shape from. Must have same dimensions as the image.
 *   </arg>
 *   <arg name="R" type="Integer" />
 *   <arg name="G" type="Integer" />
 *   <arg name="B" type="Integer" />
 *  </function>
 *  <function returns="Boolean">
 *   <arg name="Image" type="wxImage">
 *    The mask image to extract mask shape from. Must have same dimensions as the image.
 *   </arg>
 *   <arg name="Colour" type="@wxColour" />
 *  </function>
 *  <desc>
 *   Sets image's mask so that the pixels that have RGB value of r,g,b (or colour) in mask
 *   will be masked in the image. This is done by first finding an unused colour in the image,
 *   setting this colour as the mask colour and then using this colour to draw all pixels in 
 *   the image whose corresponding pixel in mask has given RGB value.
 *   Returns false if mask does not have same dimensions as the image or if there is no unused 
 *   colour left. Returns true if the mask was successfully applied.
 *  </desc>
 * </method>
 */
JSBool Image::setMaskFromImage(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
    wxImage *p = GetPrivate(cx, obj);
    if ( p == NULL )
        return JS_FALSE;

    wxImage *mask = GetPrivate(cx, argv[0]);
    if ( mask == NULL )
        return JS_FALSE;

    if ( argc == 2 )
    {
        wxColour *colour = Colour::GetPrivate(cx, argv[1]);
        if ( colour != NULL )
        {
            *rval = ToJS(cx, p->SetMaskFromImage(*mask, colour->Red(), colour->Green(), colour->Blue()));
            return JS_TRUE;
        }
    }
    else if ( argc == 4 ) 
    {
        int r, g, b;
        if (    FromJS(cx, argv[1], r)
             && FromJS(cx, argv[2], g)
             && FromJS(cx, argv[3], b) )
        {
            *rval = ToJS(cx, p->SetMaskFromImage(*mask, r, g, b));
            return JS_TRUE;
        }
    }

    return JS_FALSE;
}

/***
 * <method name="loadFile">
 *  <function returns="Boolean">
 *   <arg name="Name" type="String">The name of a file</arg>
 *   <arg name="Type" type="Integer" default="wxBitmapType.ANY">
 *    The type of the image.
 *   </arg>
 *  </function>
 *  <function returns="Boolean">
 *   <arg name="Name" type="String">The name of a file</arg>
 *   <arg name="MimeType" type="String" default="wxBitmapType.ANY">
 *    The MIME type of the image.
 *   </arg>
 *  </function>
 *  <function returns="Boolean">
 *   <arg name="Stream" type="@wxInputStream">
 *    Opened input stream from which to load the image. Currently, the stream must support seeking.
 *   </arg>
 *   <arg name="Type" type="Integer" default="wxBitmapType.ANY">
 *    The type of the image.
 *   </arg>
 *  </function>
 *  <function returns="Boolean">
 *   <arg name="Stream" type="@wxInputStream">
 *    Opened input stream from which to load the image. Currently, the stream must support seeking.
 *   </arg>
 *   <arg name="MimeType" type="String" default="wxBitmapType.ANY">
 *    The MIME type of the image.
 *   </arg>
 *  </function>
 *  <desc>
 *   Loads an image from a file.
 *  </desc>
 * </method>
 */
JSBool Image::loadFile(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
    wxImage *p = GetPrivate(cx, obj);
    if ( p == NULL )
        return JS_FALSE;

    if ( wxjs::HasPrototype(cx, argv[0], "wxInputStream") )
    {
        if ( argc != 2 )
            return JS_FALSE;

        io::Stream *stream = (io::Stream *) JS_GetPrivate(cx, JSVAL_TO_OBJECT(argv[0]));
        if ( stream == NULL )
            return JS_FALSE;

        wxInputStream *in = dynamic_cast<wxInputStream *>(stream->GetStream());
        if ( in == NULL )
        {
            // TODO: error reporting
            return JS_FALSE;
        }

        if ( JSVAL_IS_NUMBER(argv[1]) )
        {
            long type = wxBITMAP_TYPE_ANY;
            if ( ! FromJS(cx, argv[1], type) )
                return JS_FALSE;

            *rval = ToJS(cx, p->LoadFile(*in, type));
        }
        else
        {
            wxString mimetype;
            FromJS(cx, argv[1], mimetype);
            *rval = ToJS(cx, p->LoadFile(*in, mimetype));
        }
    }
    else
    {
        wxString name;
        FromJS(cx, argv[0], name);

        if ( argc > 1 )
        {
            if ( JSVAL_IS_NUMBER(argv[1]) )
            {
                long type = wxBITMAP_TYPE_ANY;
                if ( ! FromJS(cx, argv[1], type) )
                    return JS_FALSE;
                *rval = ToJS(cx, p->LoadFile(name, type));
            }
            else
            {
                wxString mimetype;
                FromJS(cx, argv[1], mimetype);
                *rval = ToJS(cx, p->LoadFile(name, mimetype));
            }
        }
        else
            *rval = ToJS(cx, p->LoadFile(name));
    }
    return JS_TRUE;
}

/***
 * <method name="saveFile">
 *  <function returns="Boolean">
 *   <arg name="Name" type="String">The name of a file</arg>
 *   <arg name="Type" type="Integer" default="wxBitmapType.ANY">
 *    The type of the image.
 *   </arg>
 *  </function>
 *  <function returns="Boolean">
 *   <arg name="Name" type="String">The name of a file</arg>
 *   <arg name="MimeType" type="String" default="wxBitmapType.ANY">
 *    The MIME type of the image.
 *   </arg>
 *  </function>
 *  <function returns="Boolean">
 *   <arg name="Stream" type="@wxOutputStream">
 *    Opened input stream from which to load the image. Currently, the stream must support seeking.
 *   </arg>
 *   <arg name="Type" type="Integer" default="wxBitmapType.ANY">
 *    The type of the image.
 *   </arg>
 *  </function>
 *  <function returns="Boolean">
 *   <arg name="Stream" type="@wxOutputStream">
 *    Opened input stream from which to load the image. Currently, the stream must support seeking.
 *   </arg>
 *   <arg name="MimeType" type="String" default="wxBitmapType.ANY">
 *    The MIME type of the image.
 *   </arg>
 *  </function>
 *  <desc>
 *   Saves an image to a file.
 *  </desc>
 * </method>
 */
JSBool Image::saveFile(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
    wxImage *p = GetPrivate(cx, obj);
    if ( p == NULL )
        return JS_FALSE;

    if ( wxjs::HasPrototype(cx, argv[0], "wxOutputStream") )
    {
        if ( argc != 2 )
            return JS_FALSE;

        io::Stream *stream = (io::Stream *) JS_GetPrivate(cx, JSVAL_TO_OBJECT(argv[0]));
        if ( stream == NULL )
            return JS_FALSE;

        wxOutputStream *out = dynamic_cast<wxOutputStream *>(stream->GetStream());
        if ( out == NULL )
        {
            // TODO: error reporting
            return JS_FALSE;
        }

        if ( JSVAL_IS_NUMBER(argv[1]) )
        {
            long type = wxBITMAP_TYPE_ANY;
            if ( ! FromJS(cx, argv[1], type) )
                return JS_FALSE;

            *rval = ToJS(cx, p->SaveFile(*out, type));
        }
        else
        {
            wxString mimetype;
            FromJS(cx, argv[1], mimetype);
            *rval = ToJS(cx, p->SaveFile(*out, mimetype));
        }
    }
    else
    {
        wxString name;
        FromJS(cx, argv[0], name);

        if ( JSVAL_IS_NUMBER(argv[1]) )
        {
            long type;
            if ( ! FromJS(cx, argv[1], type) )
                return JS_FALSE;
            *rval = ToJS(cx, p->SaveFile(name, type));
        }
        else
        {
            wxString mimetype;
            FromJS(cx, argv[1], mimetype);
            *rval = ToJS(cx, p->SaveFile(name, mimetype));
        }
    }
    return JS_TRUE;
}

/***
 * <method name="setMaskColour">
 *  <function>
 *   <arg name="R" type="Integer" />
 *   <arg name="G" type="Integer" />
 *   <arg name="B" type="Integer" />
 *  </function>
 *  <function>
 *   <arg name="Colour" type="@wxColour" />
 *  </function>
 *  <desc>
 *   Sets the mask colour for this image (and tells the image to use the mask).
 *  </desc>
 * </method>
 */
JSBool Image::setMaskColour(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
    wxImage *p = GetPrivate(cx, obj);
    if ( p == NULL )
        return JS_FALSE;

    if ( Colour::HasPrototype(cx, argv[0]) )
    {
        wxColour *colour = Colour::GetPrivate(cx, argv[0], false);
        p->SetMaskColour(colour->Red(), colour->Green(), colour->Blue());
        return JS_TRUE;
    }
    else if ( argc == 3 )
    {
        int r, g, b;
        if (    FromJS(cx, argv[0], r)
             && FromJS(cx, argv[1], g)
             && FromJS(cx, argv[2], b) )
        {
            p->SetMaskColour(r, g, b);
            return JS_TRUE;
        }
    }

    return JS_FALSE;
}

/***
 * <method name="setOption">
 *  <function>
 *   <arg name="Name" type="String">Name of the option</arg>
 *   <arg name="Value" type="String">Value of the option</arg>
 *  </function>
 *  <function>
 *   <arg name="Name" type="String">Name of the option</arg>
 *   <arg name="Value" type="Integer">Value of the option</arg>
 *  </function>
 *  <desc>
 *   Sets a user-defined option. The function is case-insensitive to name.
 *   For example, when saving as a JPEG file, the option quality is used, 
 *   which is a number between 0 and 100 (0 is terrible, 100 is very good).
 *   See @wxImage#getOption, @wxImage#getOptionInt, @wxImage#hasOption.
 *  </desc>
 * </method>
 */
JSBool Image::setOption(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
    wxImage *p = GetPrivate(cx, obj);
    if ( p == NULL )
        return JS_FALSE;

    wxString name;
    FromJS(cx, argv[0], name);

    if ( JSVAL_IS_INT(argv[1]) )
    {
        int value;
        FromJS(cx, argv[1], value);
        p->SetOption(name, value);
        return JS_TRUE;
    }

    wxString value;
    FromJS(cx, argv[1], value);
    p->SetOption(name, value);

    return JS_TRUE;
}

/***
 * <method name="getOption">
 *  <function returns="String">
 *   <arg name="Name" type="String">
 *    The name of the option.
 *   </arg>
 *  </function>
 *  <desc>
 *   Gets a user-defined option. The function is case-insensitive to name.
 *   See @wxImage#setOption, @wxImage#getOptionInt, @wxImage#hasOption
 *  </desc>
 * </method>
 */
JSBool Image::getOption(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
    wxImage *p = GetPrivate(cx, obj);
    if ( p == NULL )
        return JS_FALSE;

    wxString name;
    FromJS(cx, argv[0], name);

    *rval = ToJS(cx, p->GetOption(name));
    return JS_TRUE;
}

/***
 * <method name="getOptionInt">
 *  <function returns="Integer">
 *   <arg name="Name" type="String">
 *    The name of the option.
 *   </arg>
 *  </function>
 *  <desc>
 *   Gets a user-defined option. The function is case-insensitive to name.
 *   See @wxImage#setOption, @wxImage#getOptionString, @wxImage#hasOption
 *  </desc>
 * </method>
 */
JSBool Image::getOptionInt(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
    wxImage *p = GetPrivate(cx, obj);
    if ( p == NULL )
        return JS_FALSE;

    wxString name;
    FromJS(cx, argv[0], name);

    *rval = ToJS(cx, p->GetOptionInt(name));
    return JS_TRUE;
}

/***
 * <method name="hasOption">
 *  <function returns="Boolean">
 *   <arg name="Name" type="String">
 *    The name of the option.
 *   </arg>
 *  </function>
 *  <desc>
 *   Returns true if the given option is present. The function is case-insensitive to name.
 *   See @wxImage#setOption, @wxImage#getOptionInt, @wxImage#hasOption
 *  </desc>
 * </method>
 */
JSBool Image::hasOption(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
    wxImage *p = GetPrivate(cx, obj);
    if ( p == NULL )
        return JS_FALSE;

    wxString name;
    FromJS(cx, argv[0], name);

    *rval = ToJS(cx, p->HasOption(name));
    return JS_TRUE;
}

WXJS_BEGIN_STATIC_METHOD_MAP(Image)
    WXJS_METHOD("canRead", canRead, 1)
    WXJS_METHOD("addHandler", addHandler, 1)
    WXJS_METHOD("cleanUpHandlers", cleanUpHandlers, 0)
    WXJS_METHOD("removeHandler", removeHandler, 1)
    WXJS_METHOD("findHandler", findHandler, 1)
    WXJS_METHOD("findHandlerMime", findHandlerMime, 1)
    WXJS_METHOD("insertHandler", insertHandler, 1)
WXJS_END_METHOD_MAP()

/***
 * <class_method name="addHandler">
 *  <function>
 *   <arg name="Handler" type="@wxImageHandler" />
 *  </function>
 *  <desc>
 *   Adds a handler to the end of the static list of format handlers.
 *   There is usually only one instance of a given handler class in an application session.
 *   See @wxImage#handlers and @wxInitAllImageHandlers
 *  </desc>
 * </class_method>
 */
JSBool Image::addHandler(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
    ImageHandlerPrivate *p = ImageHandler::GetPrivate(cx, argv[0]);
    if ( p == NULL )
        return JS_FALSE;

    p->SetOurs(false);
    wxImage::AddHandler(p->GetHandler());

    return JS_TRUE;
}

/***
 * <class_method name="canRead">
 *  <function returns="Boolean">
 *   <arg name="Name" type="String">
 *    A filename.
 *   </arg>
 *  </function>
 *  <desc>
 *   Returns true when the file can be read.
 *  </desc>
 * </class_method>
 */
JSBool Image::canRead(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
    wxString name;
    FromJS(cx, argv[0], name);
    *rval = ToJS(cx, wxImage::CanRead(name));
    return JS_TRUE;
}

/***
 * <class_method name="cleanUpHandlers">
 *  <function />
 *  <desc>
 *   Removes all handlers. Is called automatically by wxWindows.
 *  </desc>
 * </class_method>
 */
JSBool Image::cleanUpHandlers(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
    wxImage::CleanUpHandlers();
    return JS_TRUE;
}

/***
 * <class_method name="findHandler">
 *  <function returns="@wxImageHandler">
 *   <arg name="Name" type="String" />
 *  </function>
 *  <function returns="@wxImageHandler">
 *   <arg name="Extension" type="String" />
 *   <arg name="Type" type="Integer" />
 *  </function>
 *  <function returns="@wxImageHandler">
 *   <arg name="Type" type="Integer" />
 *  </function>
 *  <desc>
 *   Searches a handler
 *  </desc>
 * </class_method>
 */
JSBool Image::findHandler(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
    wxImageHandler *handler = NULL;
    if ( JSVAL_IS_STRING(argv[0]) )
    {
        wxString str;
        FromJS(cx, argv[0], str);

        if ( argc == 2 )
        {
            long type;
            if ( FromJS(cx, argv[1], type) )
            {
                handler = wxImage::FindHandler(str, type);
            }
            else
                return JS_FALSE;
        }
        else
            handler = wxImage::FindHandler(str);
    }
    else
    {
        long type;
        if ( FromJS(cx, argv[1], type) )
        {
            handler = wxImage::FindHandler(type);
        }
        else
            return JS_FALSE;
    }

    if ( handler == NULL )
        *rval = JSVAL_VOID;
    else
    {
        *rval = ImageHandler::CreateObject(cx, new ImageHandlerPrivate(handler, false));
    }
    
    return JS_TRUE;
}

/***
 * <class_method name="findHandlerMime">
 *  <function returns="@wxImageHandler">
 *   <arg name="Type" type="String">
 *    A mime type.
 *   </arg>
 *  </function>
 *  <desc>
 *   Finds the handler associated with the given MIME type.
 *  </desc>
 * </class_method>
 */
JSBool Image::findHandlerMime(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
    wxString mime;
    FromJS(cx, argv[0], mime);

    wxImageHandler *handler = wxImage::FindHandlerMime(mime);
    if ( handler == NULL )
        *rval = JSVAL_VOID;
    else
    {
        *rval = ImageHandler::CreateObject(cx, new ImageHandlerPrivate(handler, false));
    }
    return JS_TRUE;
}

/***
 * <class_method name="removeHandler">
 *  <function returns="Boolean">
 *   <arg name="Name" type="String">
 *    The name of the handler.
 *   </arg>
 *  </function>
 *  <desc>
 *   Remove the given handler.
 *  </desc>
 * </class_method>
 */
JSBool Image::removeHandler(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
    wxString name;
    FromJS(cx, argv[0], name);

    *rval = ToJS(cx, wxImage::RemoveHandler(name));
    return JS_TRUE;
}

/***
 * <class_method name="insertHandler">
 *  <function>
 *   <arg name="Handler" type="@wxImageHandler" />
 *  </function>
 *  <desc>
 *   Adds a handler at the start of the static list of format handlers.
 *   There is usually only one instance of a given handler class in an application session.
 *   See @wxImage#handlers
 *  </desc>
 * </class_method>
 */
JSBool Image::insertHandler(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
    ImageHandlerPrivate *p = ImageHandler::GetPrivate(cx, argv[0]);
    if ( p == NULL )
        return JS_FALSE;

    p->SetOurs(false);
    wxImage::InsertHandler(p->GetHandler());

    return JS_TRUE;
}
