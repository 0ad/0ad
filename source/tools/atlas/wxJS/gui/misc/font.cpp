#include "precompiled.h"

/*
 * wxJavaScript - font.cpp
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
 * $Id: font.cpp 689 2007-04-27 20:45:43Z fbraem $
 */
// font.cpp

#ifndef WX_PRECOMP
    #include <wx/wx.h>
#endif

#include "../../common/main.h"
#include "../../common/apiwrap.h"

#include "font.h"

using namespace wxjs;
using namespace wxjs::gui;

/***
 * <file>misc/font</file>
 * <module>gui</module>
 * <class name="wxFont">
 *  A font is an object which determines the appearance of text. 
 *  Fonts are used for drawing text to a device context, and setting the 
 *  appearance of a window's text.
 * </class>
 */
WXJS_INIT_CLASS(Font, "wxFont", 0)

/***
 * <properties>
 *  <property name="encoding" type="Integer" />
 *  <property name="faceName" type="String">
 *   Get/Set the actual typeface to be used.
 *  </property>
 *  <property name="family" type="Integer">
 *   Font family, a generic way of referring to fonts without specifying an actual facename.
 *  </property>
 *  <property name="ok" type="Boolean">
 *   Returns true when the font is ok.
 *  </property>
 *  <property name="pointSize" type="Integer">
 *   Get/Set size in points
 *  </property>
 *  <property name="style" type="Integer">
 *   Get/Set the font style
 *  </property>
 *  <property name="underlined" type="Boolean">
 *   Get/Set underline.
 *  </property>
 *  <property name="weight" type="Integer">
 *   Get/Set the weight of the font
 *  </property>
 * </properties>
 */
WXJS_BEGIN_PROPERTY_MAP(Font)
  WXJS_PROPERTY(P_FACE_NAME, "faceName")
  WXJS_PROPERTY(P_FAMILY, "family")
  WXJS_PROPERTY(P_POINT_SIZE, "pointSize")
  WXJS_PROPERTY(P_STYLE, "style")
  WXJS_PROPERTY(P_UNDERLINED, "underlined")
  WXJS_PROPERTY(P_WEIGHT, "weight")
  WXJS_PROPERTY(P_OK, "ok")
WXJS_END_PROPERTY_MAP()

bool Font::GetProperty(wxFont *p, JSContext *cx, JSObject *obj, int id, jsval *vp)
{
    switch (id) 
	{
	case P_FACE_NAME:
        if ( p->Ok() )
		    *vp = ToJS(cx, p->GetFaceName());
		break;
	case P_FAMILY:
        if ( p->Ok() )
    		*vp = ToJS(cx, p->GetFamily());
		break;
	case P_POINT_SIZE:
        if ( p->Ok() )
    		*vp = ToJS(cx, p->GetPointSize());
		break;
	case P_STYLE:
        if ( p->Ok() )
    		*vp = ToJS(cx, p->GetStyle());
		break;
	case P_UNDERLINED:
        if ( p->Ok() )
    		*vp = ToJS(cx, p->GetUnderlined());
		break;
	case P_WEIGHT:
        if ( p->Ok() )
    		*vp = ToJS(cx, p->GetWeight());
		break;
    case P_OK:
        *vp = ToJS(cx, p->Ok());
        break;
    }
    return true;
}

bool Font::SetProperty(wxFont *p, JSContext *cx, JSObject *obj, int id, jsval *vp)
{
    switch (id) 
	{
	case P_FACE_NAME:
		{
			wxString name;
			FromJS(cx, *vp, name);
			p->SetFaceName(name);
			break;
		}
	case P_FAMILY:
		{
			int family;
			if ( FromJS(cx, *vp, family) )
				p->SetFamily(family);
			break;
		}
	case P_POINT_SIZE:
		{
			int size;
			if ( FromJS(cx, *vp, size) )
				p->SetPointSize(size);
			break;
		}
	case P_STYLE:
		{
			int style;
			if ( FromJS(cx, *vp, style) )
				p->SetStyle(style);
			break;
		}
	case P_UNDERLINED:
		{
			bool underlined;
			if ( FromJS(cx, *vp, underlined) )
				p->SetUnderlined(underlined);
			break;
		}
	case P_WEIGHT:
		{
			int weight;
			if ( FromJS(cx, *vp, weight) )
				p->SetWeight(weight);
			break;
		}
	}

    return true;
}

/***
 * <class_properties>
 *  <property name="defaultEncoding" type="Integer">
 *	 Get/Set the default encoding
 *  </property>
 * </class_properties>
 */
WXJS_BEGIN_STATIC_PROPERTY_MAP(Font)
  WXJS_STATIC_PROPERTY(P_DEFAULT_ENCODING, "defaultEncoding")
WXJS_END_PROPERTY_MAP()

bool Font::GetStaticProperty(JSContext *cx, int id, jsval *vp)
{
    if ( id == P_DEFAULT_ENCODING )
    {
        *vp = ToJS(cx, (int)wxFont::GetDefaultEncoding());
    }
    return true;
}

bool Font::SetStaticProperty(JSContext *cx, int id, jsval *vp)
{
    if ( id == P_DEFAULT_ENCODING )
    {
        int encoding;
        FromJS(cx, *vp, encoding);
        wxFont::SetDefaultEncoding((wxFontEncoding) encoding);
    }
    return true;
}

/***
 * <constants>
 *  <type name="Family">
 *   <constant name="DEFAULT" />
 *   <constant name="DECORATIVE" />
 *   <constant name="ROMAN" />
 *   <constant name="SCRIPT" />
 *   <constant name="SWISS" />
 *   <constant name="MODERN" />
 *  </type>
 *  <type name="Style">
 *   <constant name="NORMAL" />
 *   <constant name="SLANT" />
 *   <constant name="ITALIC" />
 *  </type>
 *  <type name="Weight">
 *   <constant name="NORMAL" />
 *   <constant name="LIGHT" />
 *   <constant name="BOLD" />
 *  </type>
 * </constants>
 */
WXJS_BEGIN_CONSTANT_MAP(Font)
  // Family constants 
  WXJS_CONSTANT(wx, DEFAULT)
  WXJS_CONSTANT(wx, DECORATIVE)
  WXJS_CONSTANT(wx, ROMAN)
  WXJS_CONSTANT(wx, SCRIPT)
  WXJS_CONSTANT(wx, SWISS)
  WXJS_CONSTANT(wx, MODERN)  
  // Style constants
  WXJS_CONSTANT(wx, NORMAL)
  WXJS_CONSTANT(wx, SLANT)
  WXJS_CONSTANT(wx, ITALIC)
  // Weight constants
  WXJS_CONSTANT(wx, LIGHT)
  WXJS_CONSTANT(wx, BOLD)
WXJS_END_CONSTANT_MAP()

/***
 * <ctor>
 *  <function />
 *  <function>
 *   <arg name="PointSize" type="Integer">
 *    Size in points
 *   </arg>
 *   <arg name="Family" type="Integer">
 *    Font family, a generic way of referring to fonts without specifying actual facename.
 *   </arg>
 *   <arg name="Style" type="Integer">
 *    Font style
 *   </arg>
 *   <arg name="Weight" type="Integer">
 *    Font weight
 *   </arg>
 *   <arg name="Underline" type="Boolean" />
 *   <arg name="FaceName" type="String" default="">
 *    An optional string specifying the actual typeface to be used.
 *    When not specified, a default typeface will chosen based on the family.
 *   </arg>
 *   <arg name="Encoding" type="@wxFontEncoding" default="wxFontEncoding.DEFAULT" />
 *  </function>
 *  <desc>
 *   Constructs a new wxFont object.
 *  </desc>
 * </ctor>
 */
wxFont* Font::Construct(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, bool constructing)
{
	if ( argc == 0 )
	{
		return new wxFont();
	}
	else if ( argc > 7 )
        argc = 7;

    int pointSize = 0;
	int family = 0;
	int style = 0;
	int weight = 0;
	bool underline = false;
	wxString faceName;
	int encoding = wxFONTENCODING_DEFAULT;

    switch(argc)
    {
    case 7:
        if ( ! FromJS(cx, argv[6], encoding) )
            break;
        // Walk through
    case 6:
        FromJS(cx, argv[5], faceName); 
        // Walk through
    case 5:
        if ( ! FromJS(cx, argv[4], underline) )
            break;
        // Walk through
    case 4:
        if ( ! FromJS(cx, argv[3], weight) )
            break;
        // Walk through
    case 3:
        if ( ! FromJS(cx, argv[2], style) )
            break;
        // Walk through
    case 2:
        if ( ! FromJS(cx, argv[1], family) )
            break;
        // Walk through
    case 1:
        if ( ! FromJS(cx, argv[0], pointSize) )
            break;
        return new wxFont(pointSize, family, style, 
						  weight, underline, 
						  faceName, (wxFontEncoding) encoding);
    }
    return NULL;
}
