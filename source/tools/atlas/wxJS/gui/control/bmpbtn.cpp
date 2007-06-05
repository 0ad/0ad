#include "precompiled.h"

/*
 * wxJavaScript - bmpbtn.cpp
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
 * $Id: bmpbtn.cpp 598 2007-03-07 20:13:28Z fbraem $
 */
// bmpbtn.cpp

#ifndef WX_PRECOMP
    #include <wx/wx.h>
#endif

/***
 * <file>control/bmpbtn</file>
 * <module>gui</module>
 * <class name="wxBitmapButton" prototype="@wxButton">
 *  A button that contains a bitmap.
 * </class>
 */

#include "../../common/main.h"

#include "../event/evthand.h"
#include "../event/jsevent.h"
#include "../event/command.h"

#include "../misc/size.h"
#include "../misc/point.h"
#include "bmpbtn.h"
#include "../misc/bitmap.h"
#include "window.h"

using namespace wxjs;
using namespace wxjs::gui;

WXJS_INIT_CLASS(BitmapButton, "wxBitmapButton", 3)

BitmapButton::BitmapButton(JSContext *cx, JSObject *obj)
	: wxBitmapButton()
	, Object(obj, cx)
{
	PushEventHandler(new EventHandler(this));
}

BitmapButton::~BitmapButton()
{
	PopEventHandler(true);
}


/***
 * <properties>
 *  <property name="bitmapDisabled" type="@wxBitmap">Bitmap to show when the button is disabled.</property>
 *  <property name="bitmapFocus" type="@wxBitmap">Bitmap to show when the button has the focus.</property>
 *  <property name="bitmapLabel" type="@wxBitmap">The default bitmap.</property>
 *  <property name="bitmapSelected" type="@wxBitmap">Bitmap to show when the button is selected.</property>
 * </properties>
 */
WXJS_BEGIN_PROPERTY_MAP(BitmapButton)
  WXJS_PROPERTY(P_BITMAP_DISABLED, "bitmapDisabled")
  WXJS_PROPERTY(P_BITMAP_FOCUS, "bitmapFocus")
  WXJS_PROPERTY(P_BITMAP_LABEL, "bitmapLabel")
  WXJS_PROPERTY(P_BITMAP_SELECTED, "bitmapSelected")
WXJS_END_PROPERTY_MAP()

bool BitmapButton::GetProperty(wxBitmapButton *p, JSContext *cx, JSObject *obj, int id, jsval *vp)
{
    switch (id) 
	{
	case P_BITMAP_DISABLED:
		*vp = Bitmap::CreateObject(cx, new wxBitmap(p->GetBitmapDisabled()));
		break;
	case P_BITMAP_FOCUS:
		*vp = Bitmap::CreateObject(cx, new wxBitmap(p->GetBitmapFocus()));
		break;
	case P_BITMAP_LABEL:
		*vp = Bitmap::CreateObject(cx, new wxBitmap(p->GetBitmapLabel()));
		break;
	case P_BITMAP_SELECTED:
		*vp = Bitmap::CreateObject(cx, new wxBitmap(p->GetBitmapSelected()));
		break;
    }
    return true;
}

bool BitmapButton::SetProperty(wxBitmapButton *p, JSContext *cx, JSObject *obj, int id, jsval *vp)
{
    switch (id) 
	{
	case P_BITMAP_DISABLED:
		{
			wxBitmap *bitmap = Bitmap::GetPrivate(cx, *vp);
			if ( bitmap != NULL )
				p->SetBitmapDisabled(*bitmap);
			break;
		}
	case P_BITMAP_FOCUS:
		{
			wxBitmap *bitmap = Bitmap::GetPrivate(cx, *vp);
			if ( bitmap != NULL )
				p->SetBitmapFocus(*bitmap);
			break;
		}
	case P_BITMAP_LABEL:
		{
			wxBitmap *bitmap = Bitmap::GetPrivate(cx, *vp);
			if ( bitmap != NULL )
				p->SetBitmapLabel(*bitmap);
			break;
		}
	case P_BITMAP_SELECTED:
		{
			wxBitmap *bitmap = Bitmap::GetPrivate(cx, *vp);
			if ( bitmap != NULL )
				p->SetBitmapSelected(*bitmap);
			break;
		}
	}
    return true;
}

/***
 * <ctor>
 *  <function>
 *   <arg name="Parent" type="@wxWindow">The parent window</arg>
 *   <arg name="Id" type="Integer">A windows identifier. Use -1 when you don't need it.</arg>
 *   <arg name="Bitmap" type="@wxBitmap">The bitmap to display</arg>
 *   <arg name="Position" type="@wxPoint" default="wxDefaultPosition">The position of the control on the given parent</arg>
 *   <arg name="Size" type="@wxSize" default="wxDefaultSize">The size of the control</arg>
 *   <arg name="Style" type="Integer" default="wxButton.AUTO_DRAW">The style of the control</arg>
 *  </function>
 *  <desc>
 *   Constructs a new wxBitmapButton object.
 *  </desc>
 * </ctor>
 */
wxBitmapButton* BitmapButton::Construct(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, bool constructing)
{
	const wxPoint *pt = &wxDefaultPosition;
	const wxSize *size = &wxDefaultSize;
	int style = wxBU_AUTODRAW;

	if ( argc > 6 )
        argc = 6;

	switch(argc)
	{
	case 6:
		if ( ! FromJS(cx, argv[5], style) )
			break;
		// Walk through
	case 5:
		size = Size::GetPrivate(cx, argv[4]);
		if ( size == NULL )
			break;
		// Walk through
	case 4:
		pt = Point::GetPrivate(cx, argv[3]);
		if ( pt == NULL )
			break;
		// Walk through
	default:
		wxBitmap *bmp = Bitmap::GetPrivate(cx, argv[2]);
		if ( bmp == NULL )
			break;

		int id;
		if ( ! FromJS(cx, argv[1], id) )
			break;

        wxWindow *parent = Window::GetPrivate(cx, argv[0]);
		if ( parent == NULL )
            return NULL;

        Object *wxjsParent = dynamic_cast<Object *>(parent);
        JS_SetParent(cx, obj, wxjsParent->GetObject());

		BitmapButton *p = new BitmapButton(cx, obj);
		p->Create(parent, id, *bmp, *pt, *size, style);
		return p;
	}

	return NULL;
}

/***
 * <events>
 *  <event name="onClicked">
 *   This event is triggered when the button is clicked. The function receives
 *   a @wxCommandEvent as argument.
 *  </event>
 * </events>
 */
void BitmapButton::OnClicked(wxCommandEvent &event)
{
	PrivCommandEvent::Fire<CommandEvent>(this, event, "onClicked");
}

BEGIN_EVENT_TABLE(BitmapButton, wxBitmapButton)
	EVT_BUTTON(-1, BitmapButton::OnClicked)
END_EVENT_TABLE()
