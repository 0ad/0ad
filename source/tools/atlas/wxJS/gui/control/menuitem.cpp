#include "precompiled.h"

/*
 * wxJavaScript - menuitem.cpp
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
 * $Id: menuitem.cpp 688 2007-04-27 20:45:09Z fbraem $
 */
// menuitem.cpp

#ifndef WX_PRECOMP
	#include <wx/wx.h>
#endif

#include "../../common/main.h"

#include "menu.h"
#include "menuitem.h"

#include "../misc/accentry.h"
#include "../misc/font.h"
#include "../misc/bitmap.h"
#include "../misc/colour.h"

using namespace wxjs;
using namespace wxjs::gui;

/***
 * <file>control/menuitem</file>
 * <module>gui</module>
 * <class name="wxMenuItem">
 *	A menu item represents an item in a popup menu. 
 *	Note that the majority of this class is only implemented under Windows so 
 *  far, but everything except fonts, colours and bitmaps can be achieved via 
 *  @wxMenu on all platforms.
 * </class>
 */
WXJS_INIT_CLASS(MenuItem, "wxMenuItem", 1)

/***
 * <properties>
 *	<property name="accel" type="@wxAcceleratorEntry">
 *	 Get/Set the accelerator.
 *  </property>
 *  <property name="backgroundColour" type="@wxColour">
 *  Get/Set the background colour of the item (Windows only)
 *  </property>
 *  <property name="bitmap" type="@wxBitmap">
 *	Get/Set the checked bitmap. (Windows only)
 *  </property>
 *  <property name="check" type="Boolean">
 *	 Check or uncheck the menu item.
 *  </property>
 *  <property name="checkable" type="Boolean">
 *	 Returns true if the item is checkable.
 *  </property>
 *  <property name="enable" type="Boolean">
 *	 Enables or disables the menu item.
 *  </property>
 *  <property name="font" type="@wxFont">
 *	 Get/Set the font. (Windows only)
 *  </property>
 *  <property name="help" type="String">
 *	 Get/Set the helpstring shown in the statusbar.
 *  </property>
 *  <property name="id" type="Integer">
 *	 Get/Set the id of the menu item
 *  </property>
 *  <property name="label" type="String" readonly="Y">
 *	 Gets the text associated with the menu item without any accelerator
 *	 characters it might contain.
 *  </property>
 *  <property name="marginWidth" type="Integer">
 *	 Get/Set the width of the menu item checkmark bitmap (Windows only).
 *  </property>
 *  <property name="menu" type="@wxMenu" readonly="Y">
 *   Gets the menu that owns this item.
 *  </property>
 *  <property name="separator" type="Boolean">
 *	 Returns true if the item is a separator.
 *  </property>
 *  <property name="subMenu" type="@wxMenu" readonly="Y">
 *	 Gets the submenu associated with the menu item
 *  </property>
 *  <property name="text" type="String">
 *	 Get/Set the text associated with the menu item with any accelerator
 *   characters it may contain.
 *  </property>
 *  <property name="textColour"	type="@wxColour">
 *	 Get/Set the text colour. (Windows Only)
 *  </property>
 * </properties>
 */
WXJS_BEGIN_PROPERTY_MAP(MenuItem)
  WXJS_READONLY_PROPERTY(P_LABEL, "label")
  WXJS_PROPERTY(P_ACCEL, "accel")
  WXJS_PROPERTY(P_TEXT, "text")
  WXJS_PROPERTY(P_CHECK, "check")
  WXJS_READONLY_PROPERTY(P_CHECKABLE, "checkable")
  WXJS_PROPERTY(P_ENABLE, "enable")
  WXJS_PROPERTY(P_HELP, "help")
  WXJS_PROPERTY(P_ID, "id")
  WXJS_PROPERTY(P_FONT, "font")
  WXJS_PROPERTY(P_TEXT_COLOUR, "textColour")
  WXJS_PROPERTY(P_BITMAP, "bitmap")
  WXJS_PROPERTY(P_MARGIN_WIDTH, "marginWidth")
  WXJS_READONLY_PROPERTY(P_SUB_MENU, "subMenu")
  WXJS_PROPERTY(P_BG_COLOUR, "backgroundColour")
  WXJS_READONLY_PROPERTY(P_MENU, "menu")
  WXJS_READONLY_PROPERTY(P_SEPARATOR, "separator")
WXJS_END_PROPERTY_MAP()

bool MenuItem::GetProperty(wxMenuItem *p,
                           JSContext *cx,
                           JSObject* WXUNUSED(obj),
                           int id,
                           jsval *vp)
{
	switch (id) 
	{
	case P_ACCEL:
      *vp = AcceleratorEntry::CreateObject(cx, p->GetAccel());
      break;
	case P_BG_COLOUR:
	  #ifdef __WXMSW__
	    *vp = Colour::CreateObject(cx, new wxColour(p->GetBackgroundColour()));
	  #endif
	  break;
	case P_LABEL:
      *vp = ToJS(cx, p->GetLabel());
      break;
	case P_TEXT:
      *vp = ToJS(cx, p->GetText());
      break;
	case P_CHECK:
      *vp = ToJS(cx, p->IsChecked());
      break;
	case P_CHECKABLE:
      *vp = ToJS(cx, p->IsCheckable());
      break;
	case P_ENABLE:
      *vp = ToJS(cx, p->IsEnabled());
      break;
	case P_HELP:
      *vp = ToJS(cx, p->GetHelp());
      break;
	case P_ID:
      *vp = ToJS(cx, p->GetId());
      break;
	case P_MARGIN_WIDTH:
      #ifdef __WXMSW__
        *vp = ToJS(cx, p->GetMarginWidth());
      #endif
      break;
	case P_SUB_MENU:
        {
          wxMenu *subMenu = p->GetSubMenu();
          JavaScriptClientData *data 
            = dynamic_cast<JavaScriptClientData*>(subMenu->GetClientObject());
          *vp = (data->GetObject() == NULL) ? JSVAL_NULL 
                                            : OBJECT_TO_JSVAL(data->GetObject());
          break;
	    }
	case P_MENU:
        {
          wxMenu *menu = p->GetMenu();
          JavaScriptClientData *data 
            = dynamic_cast<JavaScriptClientData*>(menu->GetClientObject());
          *vp = (data->GetObject() == NULL) ? JSVAL_NULL 
                                            : OBJECT_TO_JSVAL(data->GetObject());
          break;
        }
	case P_FONT:
		#ifdef __WXMSW__
	        *vp = Font::CreateObject(cx, new wxFont(p->GetFont()));
	    #endif
		break;
	case P_BITMAP:
		*vp = Bitmap::CreateObject(cx, new wxBitmap(p->GetBitmap()));
		break;
	case P_TEXT_COLOUR:
		#ifdef __WXMSW__
			*vp = Colour::CreateObject(cx, new wxColour(p->GetTextColour()));
		#endif
		break;
	case P_SEPARATOR:
		  *vp = ToJS(cx, p->IsSeparator());
		  break;
	}
	return true;
}

bool MenuItem::SetProperty(wxMenuItem *p,
                           JSContext *cx,
                           JSObject* WXUNUSED(obj),
                           int id,
                           jsval *vp)
{
	switch(id) 
	{
	case P_ACCEL:
		{
			wxAcceleratorEntry *entry = AcceleratorEntry::GetPrivate(cx, *vp);
            if ( entry != NULL )
                p->SetAccel(entry);
			break;
		}
	case P_BG_COLOUR:
		{
			#ifdef __WXMSW__
				wxColour *colour = Colour::GetPrivate(cx, *vp);
				if ( colour != NULL )
					p->SetBackgroundColour(*colour);
			#endif
			break;
		}
	case P_TEXT:
		{
			wxString str;
			FromJS(cx, *vp, str);
			p->SetText(str);
			break;
		}
	case P_CHECK:
		{
			bool value;
			if ( FromJS(cx, *vp, value) )
			  p->Check(value);
			break;
		}
	case P_ENABLE:
		{
			bool value;
			if ( FromJS(cx, *vp, value) )
				p->Enable(value);
			break;
		}
	case P_HELP:
		{
			wxString str;
			FromJS(cx, *vp, str);
			p->SetHelp(str);
			break;
		}
	case P_MARGIN_WIDTH:
		{
			#ifdef __WXMSW__
				int value;
				if ( FromJS(cx, *vp, value) )
					p->SetMarginWidth(value);
			#endif
			break;
		}
	case P_FONT:
		{
			#ifdef __WXMSW__
	            wxFont *font = Font::GetPrivate(cx, *vp);
				if ( font )
					p->SetFont(*font);
			#endif
			break;
		}
	case P_BITMAP:
		{
			#ifdef __WXMSW__
				wxBitmap *bmp = Bitmap::GetPrivate(cx, *vp);
				if ( bmp )
					p->SetBitmaps(*bmp, wxNullBitmap);
			#endif
			break;
		}
	case P_TEXT_COLOUR:
		{
			#ifdef __WXMSW__
				wxColour *colour = Colour::GetPrivate(cx, *vp);
				if ( colour != NULL )
					p->SetTextColour(*colour);
			#endif
			break;
		}
	}
	return true;
}

WXJS_BEGIN_METHOD_MAP(MenuItem)
  WXJS_METHOD("setBitmaps", setBitmaps, 2)
WXJS_END_METHOD_MAP()

/***
 * <ctor>
 *	<function>
 *	 <arg name="Menu" type="@wxMenu">Menu that owns the item</arg>
 *   <arg name="Id" type="Integer" default="0">Identifier for this item</arg>
 *   <arg name="Text" type="String" default="">The text for the menu item</arg>
 *   <arg name="Help" type="String" default="">The help message shown in
*     the statusbar</arg>
 *   <arg name="Checkable" type="Boolean" default="false">
 *    Indicates if the menu item can be checked or not.
 *   </arg>
 *   <arg name="SubMenu" type="@wxMenu" default="null">
 *    Indicates that the menu item is a submenu</arg>
 *  </function>
 *  <desc>
 *	 Constructs a new wxMenuItem object
 *  </desc>
 * </ctor>
 */
wxMenuItem* MenuItem::Construct(JSContext *cx,
                                JSObject* WXUNUSED(obj),
                                uintN argc,
                                jsval *argv,
                                bool WXUNUSED(constructing))
{
    if ( argc > 6 )
        argc = 6;

    int id = 0;
    wxString text = wxEmptyString;
    wxString help = wxEmptyString;
    bool checkable = false;
    wxMenu *subMenu = NULL;

    switch(argc)
    {
    case 6:
        if ( (subMenu = Menu::GetPrivate(cx, argv[5])) == NULL )
            break;
        // Fall through
    case 5:
        if ( ! FromJS(cx, argv[4], checkable) )
            break;
        // Fall through
    case 4:
        FromJS(cx, argv[3], help);
        // Fall through
    case 3:
        FromJS(cx, argv[2], text);
        // Fall through
    case 2:
        if ( ! FromJS(cx, argv[1], id) )
            break;
        // Fall through
    default:
        wxMenu *menu = Menu::GetPrivate(cx, argv[0]);
        if ( menu != NULL )
        {
            wxItemKind itemkind = wxITEM_NORMAL;
            if ( checkable )
                itemkind = wxITEM_CHECK; 
        #if wxCHECK_VERSION(2,7,0)
            return new wxMenuItem(menu, id, text, help, itemkind, subMenu);
		#else
            return new wxMenuItem(menu, id, text, help, checkable, subMenu);
		#endif
        }
    }

    return NULL;
}

void MenuItem::Destruct(JSContext* WXUNUSED(cx), wxMenuItem* WXUNUSED(p))
{
/*	if ( p->GetMenu() == NULL ) 
	{
		delete p;
		p = NULL;
	}
*/
}

/***
 * <method name="setBitmaps">
 *	<function>
 *	 <arg name="BitmapChecked" type="@wxBitmap" />
 *   <arg name="BitmapUnchecked" type="@wxBitmap" default="null" />
 *  </function>
 *  <desc>
 *   Sets the checked/unchecked bitmaps for the menu item (Windows only).
 *   The first bitmap is also used as the single bitmap for uncheckable menu items.
 *  </desc>
 * </method>
 */
JSBool MenuItem::setBitmaps(JSContext *cx,
                            JSObject *obj,
                            uintN argc,
                            jsval *argv,
                            jsval* WXUNUSED(rval))
{
    wxMenuItem *p = MenuItem::GetPrivate(cx, obj);
	if ( p == NULL )
		return JS_FALSE;

	#ifdef __WXMSW__

		wxBitmap *bmp1 = Bitmap::GetPrivate(cx, argv[0]);
		if ( bmp1 != NULL )
		{
			const wxBitmap *bmp2 = &wxNullBitmap;
			if ( argc > 1 )
			{
				bmp2 = Bitmap::GetPrivate(cx, argv[1]);
				if ( bmp2 == NULL )
				{
					return JS_FALSE;
				}
			}
	
			p->SetBitmaps(*bmp1, *bmp2);
			return JS_TRUE;
		}
	#else
		return JS_TRUE;
	#endif

	return JS_FALSE;
}
