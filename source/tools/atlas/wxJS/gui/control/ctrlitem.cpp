#include "precompiled.h"

/*
 * wxJavaScript - ctrlitem.cpp
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
 * $Id: ctrlitem.cpp 598 2007-03-07 20:13:28Z fbraem $
 */
#ifndef WX_PRECOMP
    #include <wx/wx.h>
#endif

#include "../../common/main.h"
#include "../../common/index.h"
#include "ctrlitem.h"
#include "item.h"

using namespace wxjs;
using namespace wxjs::gui;

/***
 * <file>control/ctrlitem</file>
 * <module>gui</module>
 * <class name="wxControlWithItems" prototype="@wxControl">
 *  This class is a prototype for some wxWidgets controls which contain several items,
 *  such as @wxListBox, @wxCheckListBox, @wxChoice and @wxComboBox.
 *  <br /><br />
 *  It defines the methods for accessing the controls items.
 * </class>
 */
WXJS_INIT_CLASS(ControlWithItems, "wxControlWithItems", 0)

/***
 * <properties>
 *  <property name="count" type="Integer" readonly="Y">
 *   The number of items
 *  </property>
 *  <property name="empty" type="Boolean" readonly="Y">
 *   Returns true when the control has no items
 *  </property>
 *  <property name="item" type="@wxControlItem">
 *   This is an 'array' property. This means that you have to specify an index 
 *	 to retrieve the actual item. An example: 
 *   <code class="whjs">
 *    choice.item[0].value = "BMW";
 *   </code>
 *  </property>
 *  <property name="selection" type="Integer">
 *   Get/Set the selected item
 *  </property>
 *  <property name="stringSelection" type="String">
 *   Get the label of the selected item or an empty string when no item is selected.
 *   Or select the item with the given string.
 *  </property>
 * </properties>
 */
WXJS_BEGIN_PROPERTY_MAP(ControlWithItems)
    WXJS_PROPERTY(P_COUNT, "count")
	WXJS_PROPERTY(P_SELECTION, "selection")
	WXJS_READONLY_PROPERTY(P_ITEM, "item")
	WXJS_PROPERTY(P_STRING_SELECTION, "stringSelection")
	WXJS_READONLY_PROPERTY(P_EMPTY, "empty")
WXJS_END_PROPERTY_MAP()

bool ControlWithItems::GetProperty(wxControlWithItems *p, JSContext *cx, JSObject *obj, int id, jsval *vp)
{
    switch(id)
	{
	case P_COUNT:
        *vp = ToJS(cx, p->GetCount());
		break;
	case P_SELECTION:
		*vp = ToJS(cx, p->GetSelection());
		break;
	case P_ITEM:
        *vp = ControlItem::CreateObject(cx, NULL, obj);
		break;
	case P_STRING_SELECTION:
		*vp = ToJS(cx, p->GetStringSelection());
		break;
	case P_EMPTY:
		*vp = ToJS(cx, p->IsEmpty());
		break;
	}
    return true;
}

bool ControlWithItems::SetProperty(wxControlWithItems *p, JSContext *cx, JSObject *obj, int id, jsval *vp)
{
	switch (id) 
	{
	case P_SELECTION:
		{
			int selection;
			if ( FromJS(cx, *vp, selection) )
				p->SetSelection(selection);
		}
		break;
	case P_STRING_SELECTION:
		{
			wxString selection;
			FromJS(cx, *vp, selection);
			p->SetStringSelection(selection);
		}
	}
    return true;
}

void ControlWithItems::Destruct(JSContext *cx, wxControlWithItems* p)
{
}

WXJS_BEGIN_METHOD_MAP(ControlWithItems)
  WXJS_METHOD("append", append, 1)
  WXJS_METHOD("clear", clear, 0)
  WXJS_METHOD("deleteItem", delete_item, 1)
  WXJS_METHOD("findString", findString, 1)
  WXJS_METHOD("insert", insert, 2)
WXJS_END_METHOD_MAP()

/***
 * <method name="append">
 *  <function returns="Integer">
 *   <arg name="Item" type="String" />
 *  </function>
 *  <function>
 *   <arg name="Items" type="Array" />
 *  </function>
 *  <desc>
 *   Adds the item or all elements of the array to the end of the control.
 *   When only one item is appended, the return value is the index
 *   of the newly added item.
 *  </desc>
 * </method>
 */
 JSBool ControlWithItems::append(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
    wxControlWithItems *p = GetPrivate(cx, obj);
    if ( p == NULL )
        return JS_FALSE;

	if (    JSVAL_IS_OBJECT(argv[0]) 
		 && JS_IsArrayObject(cx, JSVAL_TO_OBJECT(argv[0])) )
	{
		wxArrayString strings;
		if ( FromJS(cx, argv[0], strings) )
		{
			p->Append(strings);
		}
	}
	else
	{
		wxString item;
		FromJS(cx, argv[0], item);
		*rval = ToJS(cx, p->Append(item));
	}

	return JS_TRUE;
}

/***
 * <method name="clear">
 *  <function />
 *  <desc>
 *   Removes all items from the control.
 *  </desc>
 * </method>
 */
 JSBool ControlWithItems::clear(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
    wxControlWithItems *p = GetPrivate(cx, obj);
    if ( p == NULL )
        return JS_FALSE;

	p->Clear();
	return JS_TRUE;
 }

/***
 * <method name="deleteItem">
 *  <function>
 *   <arg name="Index" type="Integer" />
 *	</function>
 *	<desc>
 *	 Removes the item with the given index (zero-based).
 *	</desc>
 * </method>
 */
JSBool ControlWithItems::delete_item(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
	wxControlWithItems *p = GetPrivate(cx, obj);
	if ( p == NULL )
		return JS_FALSE;

	int idx;
	if ( FromJS(cx, argv[0], idx) )
	{
		p->Delete(idx);
		return JS_TRUE;
	}

	return JS_FALSE;
}

/***
 * <method name="findString">
 *	<function returns="Integer">
 *	 <arg name="Search" type="String" />
 *	</function>
 *	<desc>
 *	 Returns the zero-based index of the item with the given search text.
 *   -1 is returned when the string was not found.
 *  </desc>
 * </method>
 */
JSBool ControlWithItems::findString(JSContext *cx, JSObject *obj,
							  uintN argc, jsval *argv, jsval *rval)
{
	wxControlWithItems *p = GetPrivate(cx, obj);
	if ( p == NULL )
		return JS_FALSE;

	wxString search;
	FromJS(cx, argv[0], search);
	*rval = ToJS(cx, p->FindString(search));
	return JS_TRUE;
}

/***
 * <method name="insert">
 *  <function returns="Integer">
 *   <arg name="Item" type="String" />
 *   <arg name="Pos" type="Integer" />
 *  </function>
 *  <desc>
 *   Inserts the item into the list before pos. Not valid
 *   for wxListBox.SORT or wxComboBox.SORT styles, use Append instead.
 *   The returned value is the index of the new inserted item.
 *  </desc>
 * </method>
 */
 JSBool ControlWithItems::insert(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
    wxControlWithItems *p = GetPrivate(cx, obj);
    if ( p == NULL )
        return JS_FALSE;

	int pos;
	if ( ! FromJS(cx, argv[1], pos) )
		return JS_FALSE;

	wxString item;
	FromJS(cx, argv[0], item);
	*rval = ToJS(cx, p->Insert(item, pos));

	return JS_TRUE;
}
