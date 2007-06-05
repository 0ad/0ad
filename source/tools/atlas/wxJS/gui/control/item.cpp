#include "precompiled.h"

/*
 * wxJavaScript - item.cpp
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
 * $Id: item.cpp 598 2007-03-07 20:13:28Z fbraem $
 */
#ifndef WX_PRECOMP
    #include <wx/wx.h>
#endif

#include "../../common/main.h"
#include "../../common/index.h"

#include "item.h"
#include "ctrlitem.h"

using namespace wxjs;
using namespace wxjs::gui;

/***
 * <file>control/item</file>
 * <module>gui</module>
 * <class name="wxControlItem">
 *  wxControlItem represents an item in a @wxControlWithItems control.
 *  <br /><br /><b>Remark:</b> This class is a helper class to make
 *  it possible to use the items of a @wxControlWithItems as an array.
 *  It has no corresponding class in wxWidgets.
 * </class>
 */
WXJS_INIT_CLASS(ControlItem, "wxControlItem", 0)

/***
 * <properties>
 *  <property name="value" type="String">
 *   Get/Set the value of the item.
 *  </property>
 * </properties>
 */
WXJS_BEGIN_PROPERTY_MAP(ControlItem)
  WXJS_PROPERTY(P_VALUE, "value")
WXJS_END_PROPERTY_MAP()

WXJS_BEGIN_METHOD_MAP(ControlItem)
  WXJS_METHOD("remove", remove, 0)
  WXJS_METHOD("select", select, 0)
WXJS_END_METHOD_MAP()

bool ControlItem::GetProperty(Index *p, JSContext *cx, JSObject *obj, int id, jsval *vp)
{
	JSObject *parent = JS_GetParent(cx, obj);
	wxASSERT_MSG(parent != NULL, wxT("No parent found for ControlItem"));

    wxControlWithItems *ctrl = ControlWithItems::GetPrivate(cx, parent);
	wxASSERT_MSG(ctrl != NULL, wxT("No private data associated with ControlWithItems"));

	// When propId is greater then 0, then we have an array index.
	if ( id >= 0 )
	{
		SetPrivate(cx, obj, new Index(id));
        *vp = OBJECT_TO_JSVAL(obj);
	}
	else
	{
		if ( p->GetIndex() < ctrl->GetCount() ) // To be sure
		{
			switch(id) 
			{
			case P_VALUE:
				*vp = ToJS(cx, ctrl->GetString(p->GetIndex()));
				break;
			}
		}
	}
	return true;
}

bool ControlItem::SetProperty(Index *p, JSContext *cx, JSObject *obj, int id, jsval *vp)
{
	if ( p == NULL )
		return true;

	JSObject *parent = JS_GetParent(cx, obj);
	wxASSERT_MSG(parent != NULL, wxT("No parent found for ControlItem"));

    wxControlWithItems *ctrl = ControlWithItems::GetPrivate(cx, parent);
	wxASSERT_MSG(ctrl != NULL, wxT("No private data associated with ControlWithItems"));

	if ( p->GetIndex() < ctrl->GetCount() ) // To be sure
	{
		switch (id) 
		{
		case P_VALUE:
			{
				wxString value;
				FromJS(cx, *vp, value);
				ctrl->SetString(p->GetIndex(), value);
				break;
			}
		}
	}
    return true;
}

/***
 * <method name="remove">
 *  <function />
 *  <desc>
 *   removes the item from the control.
 *  </desc>
 * </method>
 */
JSBool ControlItem::remove(JSContext *cx, JSObject *obj, uintN argc,
							  jsval *argv, jsval *rval)
{
	JSObject *parent = JS_GetParent(cx, obj);
	wxASSERT_MSG(parent != NULL, wxT("No parent found for ControlItem"));

    wxControlWithItems *ctrl = ControlWithItems::GetPrivate(cx, parent);
	if ( ctrl == NULL )
		return JS_FALSE;

    Index *item = ControlItem::GetPrivate(cx, obj);
	wxASSERT_MSG(item != NULL, wxT("No private data associated with ChoiceItem"));

	if ( item->GetIndex() < ctrl->GetCount() ) // To be sure.
		ctrl->Delete(item->GetIndex());

	return JS_TRUE;
}

/***
 * <method name="select">
 *  <function />
 *  <desc>
 *   Selects/unselects the item.
 *  </desc>
 * </method>
 */
JSBool ControlItem::select(JSContext *cx, JSObject *obj, uintN argc,
							  jsval *argv, jsval *rval)
{
	JSObject *parent = JS_GetParent(cx, obj);
	wxASSERT_MSG(parent != NULL, wxT("No parent found for ControlItem"));

    wxControlWithItems *ctrl = ControlWithItems::GetPrivate(cx, parent);
	if ( ctrl == NULL )
		return JS_FALSE;

    Index *item = ControlItem::GetPrivate(cx, obj);
	wxASSERT_MSG(item != NULL, wxT("No private data associated with ChoiceItem"));

	if ( item->GetIndex() < ctrl->GetCount() ) // To be sure.
		ctrl->Select(item->GetIndex());

	return JS_TRUE;
}
