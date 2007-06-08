#include "precompiled.h"

/*
 * wxJavaScript - imagelst.cpp
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
 * $Id: imagelst.cpp 734 2007-06-06 20:09:13Z fbraem $
 */
// imagelst.cpp

#ifndef WX_PRECOMP
    #include <wx/wx.h>
#endif

/**
 * @if JS
 *  @page wxImageList wxImageList
 *  @since version 0.5
 * @endif
 */

#include "../../common/main.h"

#include "imagelst.h"
#include "bitmap.h"
#include "colour.h"
#include "icon.h"
#include "size.h"

using namespace wxjs;
using namespace wxjs::gui;

ImageList::ImageList() : wxClientDataContainer()
{
}

/***
 * <file>control/imagelst</file>
 * <module>gui</module>
 * <class name="wxImageList">
 *  A wxImageList contains a list of images, which are stored in an unspecified form.
 *  Images can have masks for transparent drawing, and can be made from a variety of 
 *  sources including bitmaps and icons.
 *  <br /><br />
 *  wxImageList is used principally in conjunction with @wxTreeCtrl and 
 *  @wxListCtrl classes.
 * </class>
 */
WXJS_INIT_CLASS(ImageList, "wxImageList", 2)

/***
 * <properties>
 *  <property name="imageCount" type="Integer" readonly="Y">
 *   Returns the number of the images.
 *  </property>
 * </properties>
 */
WXJS_BEGIN_PROPERTY_MAP(ImageList)
    WXJS_READONLY_PROPERTY(P_IMAGE_COUNT, "imageCount")
WXJS_END_PROPERTY_MAP()

bool ImageList::GetProperty(wxImageList *p,
                            JSContext *cx,
                            JSObject* WXUNUSED(obj),
                            int id,
                            jsval *vp)
{
    if ( id == P_IMAGE_COUNT )
        *vp = ToJS(cx, p->GetImageCount());
    return true;
}

/***
 * <ctor>
 *  <function />
 *  <function>
 *   <arg name="Width" type="Integer">
 *    The width of the images in the list
 *   </arg>
 *   <arg name="Height" type="Integer">
 *    The height of the images in the list.
 *   </arg>
 *   <arg name="Mask" type="Boolean" default="true">
 *    When true (default) a mask is created for each image.
 *   </arg>
 *   <arg name="Count" type="Integer" default="1">
 *    The size of the list.
 *   </arg>
 *  </function>
 *  <desc>
 *   Constructs a new wxImageList object.
 *  </desc>
 * </ctor>
 */
ImageList* ImageList::Construct(JSContext *cx,
                                JSObject *obj,
                                uintN argc,
                                jsval *argv,
                                bool WXUNUSED(constructing))
{
  ImageList *p = new ImageList();
  SetPrivate(cx, obj, p);

  if ( argc > 0 )
  {
    jsval rval;
    create(cx, obj, argc, argv, &rval);
  }
  return p;
}

WXJS_BEGIN_METHOD_MAP(ImageList)
    WXJS_METHOD("create", create, 2)
    WXJS_METHOD("add", add, 1)
    WXJS_METHOD("draw", draw, 4)
    WXJS_METHOD("getSize", getSize, 2)
    WXJS_METHOD("remove", remove, 1)
    WXJS_METHOD("removeAll", removeAll, 0)
    WXJS_METHOD("replace", replace, 2)
WXJS_END_METHOD_MAP()

/***
 * <method name="create">
 *  <function>
 *   <arg name="Width" type="Integer">
 *    The width of the images in the list
 *   </arg>
 *   <arg name="Height" type="Integer">
 *    The height of the images in the list.
 *   </arg>
 *   <arg name="Mask" type="Boolean" default="true">
 *    When true (default) a mask is created for each image.
 *   </arg>
 *   <arg name="Count" type="Integer" default="1">
 *    The size of the list.
 *   </arg>
 *  </function>
 *  <desc>
 *   Creates an image list. Width and Height specify the size of the images
 *   in the list (all the same). Mask specifies whether the images have masks or not.
 *   Count is the initial number of images to reserve.
 *  </desc>
 * </method>
 */
JSBool ImageList::create(JSContext *cx,
                         JSObject *obj,
                         uintN argc,
                         jsval *argv,
                         jsval *rval)
{
    ImageList *p = GetPrivate(cx, obj);
    if ( p == NULL )
        return JS_FALSE;

    if ( argc > 4 )
        argc = 4;

    bool mask = true;
    int count = 1;

    switch(argc)
    {
    case 4:
        if ( ! FromJS(cx, argv[3], count) )
            break;
        // Fall through
    case 3:
        if ( ! FromJS(cx, argv[2], mask) )
            break;
        // Fall through
    default:
        int width;
        int height;

        if (    FromJS(cx, argv[1], height) 
             && FromJS(cx, argv[0], width) )
        {
            p->Create(width, height, mask, count);
            p->SetClientObject(new JavaScriptClientData(cx, obj, false, false));
            return JS_TRUE;
        }
    }
    return JS_FALSE;
}

/***
 * <method name="add">
 *  <function returns="Integer">
 *   <arg name="Bitmap" type="@wxBitmap" />
 *   <arg name="Mask" type="Integer" />
 *  </function>
 *  <function returns="Integer">
 *   <arg name="Bitmap" type="@wxBitmap" />
 *   <arg name="Colour" type="@wxColour" />
 *  </function>
 *  <function returns="Integer">
 *   <arg name="Icon" type="@wxIcon" />
 *  </function>
 *  <desc>
 *   Adds a new image using a bitmap and an optional mask bitmap
 *   or adds a new image using a bitmap and a mask colour
 *   or adds a new image using an icon.
 *   Returns the new zero-based index of the image.
 *  </desc>
 * </method>
 */
JSBool ImageList::add(JSContext *cx,
                      JSObject *obj,
                      uintN argc,
                      jsval *argv,
                      jsval *rval)
{
    wxImageList *p = GetPrivate(cx, obj);
    if ( p == NULL )
        return JS_FALSE;

    if ( Icon::HasPrototype(cx, argv[0]) )
    {
        wxIcon *ico = Icon::GetPrivate(cx, argv[0]);
        if ( ico != NULL )
        {
            *rval = ToJS(cx, p->Add(*ico));
            return JS_TRUE;
        }
    }
    else
    {
        wxBitmap *bmp = Bitmap::GetPrivate(cx, argv[0], false);
        const wxBitmap *mask = &wxNullBitmap;
        if ( argc == 2 )
        {
            if ( Colour::HasPrototype(cx, argv[1]) )
            {
                wxColour *colour = Colour::GetPrivate(cx, argv[1]);
                if ( colour != NULL )
                {
                    *rval = ToJS(cx, p->Add(*bmp, *colour));
                    return JS_TRUE;
                }
            }
            else
            {
                mask = Bitmap::GetPrivate(cx, argv[1]);
                if ( mask == NULL )
                    return JS_FALSE;
            }
        }
        *rval = ToJS(cx, p->Add(*bmp, *mask));
        return JS_TRUE;
    }
    return JS_FALSE;
}

JSBool ImageList::draw(JSContext *cx,
                       JSObject *obj,
                       uintN argc,
                       jsval *argv,
                       jsval *rval)
{
    wxImageList *p = GetPrivate(cx, obj);
    if ( p == NULL )
        return JS_FALSE;

    return JS_FALSE;
}

/***
 * <method name="getSize">
 *  <function returns="Boolean">
 *   <arg name="Index" type="Integer">
 *    The index parameter is ignored as all images in the list have the same size.
 *   </arg>
 *   <arg name="Size" type="@wxSize">
 *    Receives the size of the image.
 *   </arg>
 *  </function>
 *  <desc>
 *   The size is put in the <I>Size</I> argument. This method differs from the wxWindow function
 *   GetSize which uses 3 arguments: index, width and height. wxJS uses @wxSize because JavaScript
 *   can't pass primitive types by reference.
 *  </desc>
 * </method>
 */
JSBool ImageList::getSize(JSContext *cx,
                          JSObject *obj,
                          uintN argc,
                          jsval *argv,
                          jsval *rval)
{
    wxImageList *p = GetPrivate(cx, obj);
    if ( p == NULL )
        return JS_FALSE;

    int index = 0;
    wxSize *size = Size::GetPrivate(cx, argv[1]);
    if ( size != NULL )
    {
        int width;
        int height;
        *rval = ToJS(cx, p->GetSize(index, width, height));
        size->SetWidth(width);
        size->SetHeight(height);
        return JS_TRUE;
    }

    return JS_FALSE;
}

/***
 * <method name="remove">
 *  <function returns="Boolean">
 *   <arg name="Index" type="Integer">
 *    The index of the image.
 *   </arg>
 *  </function>
 *  <desc>
 *   Removes the image with the given index. Returns true on success.
 *  </desc>
 * </method>
 */
JSBool ImageList::remove(JSContext *cx,
                         JSObject *obj,
                         uintN argc,
                         jsval *argv, 
                         jsval *rval)
{
    wxImageList *p = GetPrivate(cx, obj);
    if ( p == NULL )
        return JS_FALSE;

    int index;
    if ( FromJS(cx, argv[0], index) )
    {
        *rval = ToJS(cx, p->Remove(index));
        return JS_TRUE;
    }

    return JS_FALSE;
}

/***
 * <method name="removeAll">
 *  <function returns="Boolean" />
 *  <desc>
 *   Removes all images. Returns true on success.
 *  </desc>
 * </method>
 */
JSBool ImageList::removeAll(JSContext *cx,
                            JSObject *obj,
                            uintN argc,
                            jsval *argv,
                            jsval *rval)
{
    wxImageList *p = GetPrivate(cx, obj);
    if ( p == NULL )
        return JS_FALSE;

    *rval = ToJS(cx, p->RemoveAll());
    return JS_TRUE;
}

/***
 * <method name="replace">
 *  <function returns="Boolean">
 *   <arg name="Index" type="Integer">
 *    The index of the image to replace.
 *   </arg>
 *   <arg name="Bitmap" type="@wxBitmap">
 *    The new bitmap.
 *   </arg>
 *   <arg name="Mask" type="@wxBitmap">
 *    Monochrome mask bitmap, representing the transparent areas of the image.
 *    Windows only.
 *   </arg>
 *  </function>
 *  <function returns="Boolean">
 *   <arg name="Index" type="Integer">
 *    The index of the image to replace.
 *   </arg>
 *   <arg name="Icon" type="@wxIcon">
 *    Icon to use as image.
 *   </arg>
 *  </function>
 *  <desc>
 *   Replaces the image at the given index with a new image.
 *  </desc>
 * </method>
 */
JSBool ImageList::replace(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
    wxImageList *p = GetPrivate(cx, obj);
    if ( p == NULL )
        return JS_FALSE;

    int index;
    if ( ! FromJS(cx, argv[0], index) )
        return JS_FALSE;

    if ( Bitmap::HasPrototype(cx, argv[1]) )
    {
    	#ifdef __WXMSW__
	        wxBitmap *bmp = Bitmap::GetPrivate(cx, argv[1], false);
	        const wxBitmap *mask = &wxNullBitmap;
	        if (    argc == 3 
	             && (mask = Bitmap::GetPrivate(cx, argv[2])) == NULL )
	            return JS_FALSE;
	        *rval = ToJS(cx, p->Replace(index, *bmp, *mask));
	    #endif
        return JS_TRUE;
    }
    else
    {
        wxIcon *ico = Icon::GetPrivate(cx, argv[0]);
        if ( ico != NULL )
        {
            *rval = ToJS(cx, p->Replace(index, *ico));
            return JS_TRUE;
        }
    }
    return JS_FALSE;
}
