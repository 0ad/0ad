#include "precompiled.h"

/*
 * wxJavaScript - listbox.cpp
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
 * $Id: listbox.cpp 746 2007-06-11 20:58:21Z fbraem $
 */

#ifndef WX_PRECOMP
	#include <wx/wx.h>
#endif

#include "../../common/main.h"
#include "../../common/index.h"


#include "../event/jsevent.h"
#include "../event/command.h"

#include "../misc/point.h"
#include "../misc/size.h"
#include "../misc/validate.h"

#include "listbox.h"
#include "item.h"
#include "window.h"
#include "../errors.h"

using namespace wxjs;
using namespace wxjs::gui;

/***
 * <file>control/listbox</file>
 * <module>gui</module>
 * <class name="wxListBox" prototype="@wxControlWithItems">
 *	A listbox is used to select one or more of a list of strings. 
 *	The strings are displayed in a scrolling box, with the selected 
 *	string(s) marked in reverse video. A listbox can be single selection 
 *	(if an item is selected, the previous selection is removed) or 
 *	multiple selection (clicking an item toggles the item on or off 
 *  independently of other selections).
 * </class>
 */
WXJS_INIT_CLASS(ListBox, "wxListBox", 2)

void ListBox::InitClass(JSContext* WXUNUSED(cx),
                        JSObject* WXUNUSED(obj), 
                        JSObject* WXUNUSED(proto))
{
  ListBoxEventHandler::InitConnectEventMap();
}

/***
 * <properties>
 *	<property name="selections" type="Array" readonly="Y">
 *	 An array with all the indexes of the selected items
 *  </property>
 * </properties>
 */
WXJS_BEGIN_PROPERTY_MAP(ListBox)
  WXJS_READONLY_PROPERTY(P_SELECTIONS, "selections")
WXJS_END_PROPERTY_MAP()

bool ListBox::GetProperty(wxListBox* p,
                          JSContext *cx,
                          JSObject* WXUNUSED(obj),
                          int id,
                          jsval *vp)
{
	switch (id) 
	{
	case P_SELECTIONS:
		{
			wxArrayInt selections;
			jsint count = p->GetSelections(selections);
			JSObject *objSelections = JS_NewArrayObject(cx, count, NULL);
			*vp = OBJECT_TO_JSVAL(objSelections);
			for(jsint i = 0; i < count; i++)
			{
				jsval element = ToJS(cx, selections[i]);
				JS_SetElement(cx, objSelections, i, &element);
			}
			break;
		}
	}
	return true;
}

bool ListBox::AddProperty(wxListBox *p, 
                          JSContext* WXUNUSED(cx), 
                          JSObject* WXUNUSED(obj), 
                          const wxString &prop, 
                          jsval* WXUNUSED(vp))
{
    if ( WindowEventHandler::ConnectEvent(p, prop, true) )
        return true;
    
    ListBoxEventHandler::ConnectEvent(p, prop, true);

    return true;
}

bool ListBox::DeleteProperty(wxListBox *p, 
                             JSContext* WXUNUSED(cx), 
                             JSObject* WXUNUSED(obj), 
                             const wxString &prop)
{
  if ( WindowEventHandler::ConnectEvent(p, prop, false) )
    return true;
  
  ListBoxEventHandler::ConnectEvent(p, prop, false);
  return true;
}

/***
 * <constants>
 *	<type name="Style">
 *	 <constant name="SINGLE" />
 *	 <constant name="MULTIPLE" />
 *   <constant name="EXTENDED" />
 *   <constant name="HSCROLL" />
 *   <constant name="ALWAYS_SB" />
 *   <constant name="NEEDED_SB" />
 *   <constant name="SORT" />
 *  </type>
 * </constants>
 */
WXJS_BEGIN_CONSTANT_MAP(ListBox)
  WXJS_CONSTANT(wxLB_, SINGLE)
  WXJS_CONSTANT(wxLB_, MULTIPLE)
  WXJS_CONSTANT(wxLB_, EXTENDED)
  WXJS_CONSTANT(wxLB_, HSCROLL)
  WXJS_CONSTANT(wxLB_, ALWAYS_SB)
  WXJS_CONSTANT(wxLB_, NEEDED_SB)
  WXJS_CONSTANT(wxLB_, SORT)	
WXJS_END_CONSTANT_MAP()

/***
 * <ctor>
 *  <function />
 *	<function>
 *	 <arg name="Parent" type="@wxWindow">
 *	  The parent of the wxListBox. null is not Allowed.
 *   </arg>
 *	 <arg name="Id" type="Integer">
 *	 The windows identifier. -1 can be used when you don't need a unique id.
 *   </arg>
 *	 <arg name="Position" type="@wxPoint" default="wxDefaultPosition">
 *	 The position of the listbox.
 *   </arg>
 *	 <arg name="Size" type="@wxSize" default="wxDefaultSize">
 *	  The size of the listbox.
 *   </arg>
 *	 <arg name="Items" type="Array" default="null">
 *	  The items for the listbox.
 *   </arg>
 *	 <arg name="Style" type="Integer" default="0">
 *	  The style of listbox. You can use the @wxListBox#style.
 *   </arg>
 *   <arg name="Validator" type="@wxValidator" default="null" />
 *  </function>
 *  <desc>
 *	 Creates a new wxListBox object
 *  </desc>
 * </ctor>
 */
wxListBox* ListBox::Construct(JSContext *cx,
                              JSObject *obj,
                              uintN argc,
                              jsval *argv,
                              bool WXUNUSED(constructing))
{
  wxListBox *p = new wxListBox();
  SetPrivate(cx, obj, p);

  if ( argc > 0 )
  {
    jsval rval;
    if ( ! create(cx, obj, argc, argv, &rval) )
      return NULL;
  }
  return p;
}

WXJS_BEGIN_METHOD_MAP(ListBox)
  WXJS_METHOD("create", create, 2)
  WXJS_METHOD("setFirstItem", set_first_item, 1)
  WXJS_METHOD("insertItems", insert_items, 1)
WXJS_END_METHOD_MAP()

/***
 * <method name="create">
 *	<function returns="Boolean">
 *	 <arg name="Parent" type="@wxWindow">
 *	  The parent of the wxListBox. null is not Allowed.
 *   </arg>
 *	 <arg name="Id" type="Integer">
 *	 The windows identifier. -1 can be used when you don't need a unique id.
 *   </arg>
 *	 <arg name="Position" type="@wxPoint" default="wxDefaultPosition">
 *	 The position of the listbox.
 *   </arg>
 *	 <arg name="Size" type="@wxSize" default="wxDefaultSize">
 *	  The size of the listbox.
 *   </arg>
 *	 <arg name="Items" type="Array" default="null">
 *	  The items for the listbox.
 *   </arg>
 *	 <arg name="Style" type="Integer" default="0">
 *	  The style of listbox. You can use the @wxListBox#style.
 *   </arg>
 *   <arg name="Validator" type="@wxValidator" default="null" />
 *  </function>
 *  <desc>
 *	 Creates a wxListBox
 *  </desc>
 * </method>
 */
JSBool ListBox::create(JSContext *cx,
                       JSObject *obj,
                       uintN argc,
                       jsval *argv,
                       jsval *rval)
{
    wxListBox *p = GetPrivate(cx, obj);
    *rval = JSVAL_FALSE;

    const wxPoint *pt = &wxDefaultPosition;
	const wxSize *size = &wxDefaultSize;
    int style = 0;
	StringsPtr items;
    const wxValidator *val = &wxDefaultValidator;

    if ( argc > 7 )
        argc = 7;

    switch(argc)
    {
    case 7:
        val = Validator::GetPrivate(cx, argv[6]);
        if ( val == NULL )
            break;
    case 6:
        if ( ! FromJS(cx, argv[5], style) )
            break;
        // Fall through
    case 5:
    	if ( ! FromJS(cx, argv[4], items) )
            break;
        // Fall through
    case 4:
		size = Size::GetPrivate(cx, argv[3]);
		if ( size == NULL )
			break;
		// Fall through
	case 3:
		pt = Point::GetPrivate(cx, argv[2]);
		if ( pt == NULL )
			break;
		// Fall through
    default:
        int id;
        if ( ! FromJS(cx, argv[1], id) )
            break;

        wxWindow *parent = Window::GetPrivate(cx, argv[0]);
        if ( parent == NULL )
        {
            JS_ReportError(cx, WXJS_NO_PARENT_ERROR, GetClass()->name);
            return JS_FALSE;
        }
        JavaScriptClientData *clntParent =
              dynamic_cast<JavaScriptClientData *>(parent->GetClientObject());
        if ( clntParent == NULL )
        {
            JS_ReportError(cx, WXJS_NO_PARENT_ERROR, GetClass()->name);
            return JS_FALSE;
        }
        JS_SetParent(cx, obj, clntParent->GetObject());

    	if ( p->Create(parent, id, *pt, *size, 
                       items.GetCount(), items.GetStrings(), style, *val) )
        {
          *rval = JSVAL_TRUE;
          p->SetClientObject(new JavaScriptClientData(cx, obj, true, false));
        }
    }
	
    return JS_TRUE;
}

/***
 * <method name="setFirstItem">
 *  <function>
 *   <arg name="Idx" type="Integer">
 *    The zero-based item index
 *   </arg>
 *  </function>
 *  <function>
 *   <arg name="Str" type="String">
 *    The string that should be visible
 *   </arg>
 *  </function>
 *  <desc>
 *   Set the specified item to be the first visible item. Windows only.
 *  </desc>
 * </method>
 */
JSBool ListBox::set_first_item(JSContext *cx,
                               JSObject *obj,
                               uintN WXUNUSED(argc),
                               jsval *argv,
                               jsval* WXUNUSED(rval))
{
    wxListBox *p = GetPrivate(cx, obj);
	if ( p == NULL )
		return JS_FALSE;

	int idx;
	if ( FromJS(cx, argv[0], idx) )
		p->SetFirstItem(idx);
	else
	{
		wxString item;
		FromJS(cx, argv[0], item);
		p->SetFirstItem(item);
	}
	return JS_TRUE;
}

/***
 * <method name="insertItems">
 *	<function>
 *	 <arg name="Items" type="Array" />
 *	 <arg name="Index" type="Integer">
 *	  Position before which to insert the items: for example, if 
 *	  pos is 0 (= the default) the items will be inserted in the 
 *    beginning of the listbox
 *   </arg>
 *  </function>
 *	<desc>
 *   Inserts all the items
 *  </desc>
 * </method>
 */
JSBool ListBox::insert_items(JSContext *cx,
                             JSObject *obj,
                             uintN argc,
							 jsval *argv,
                             jsval* WXUNUSED(rval))
{
    wxListBox *p = GetPrivate(cx, obj);
	if ( p == NULL )
		return JS_FALSE;

    StringsPtr items;
    if ( FromJS(cx, argv[0], items) )
    {
        int pos = 0;
    	if (    argc > 0 
             && ! FromJS(cx, argv[1], pos) )
             return JS_FALSE;

		p->InsertItems(items.GetCount(), items.GetStrings(), pos);
		return JS_TRUE;
	}

    return JS_FALSE;
}

/***
 * <events>
 *	<event name="onListBox">
 *	 Called when an item is selected. 
 *   The type of the argument that your handler receives is @wxCommandEvent.
 *  </event>
 *  <event name="onDoubleClicked">
 *	 Called when the listbox is double clicked. 
 *   The type of the argument that your handler receives is @wxCommandEvent.
 *  </event>
 * </events>
 */

WXJS_INIT_EVENT_MAP(wxListBox)
const wxString WXJS_LISTBOX_EVENT = wxT("onListBox");
const wxString WXJS_DOUBLECLICK_EVENT = wxT("onDoubleClicked");

void ListBoxEventHandler::OnListBox(wxCommandEvent &event)
{
	PrivCommandEvent::Fire<CommandEvent>(event, WXJS_LISTBOX_EVENT);
}

void ListBoxEventHandler::OnDoubleClick(wxCommandEvent &event)
{
	PrivCommandEvent::Fire<CommandEvent>(event, WXJS_DOUBLECLICK_EVENT);
}

void ListBoxEventHandler::ConnectListBox(wxListBox *p, bool connect)
{
  if ( connect )
  {
    p->Connect(wxEVT_COMMAND_LISTBOX_SELECTED, 
               wxCommandEventHandler(ListBoxEventHandler::OnListBox));
  }
  else
  {
    p->Disconnect(wxEVT_COMMAND_LISTBOX_SELECTED, 
                  wxCommandEventHandler(ListBoxEventHandler::OnListBox));
  }
}

void ListBoxEventHandler::ConnectDoubleClick(wxListBox *p, bool connect)
{
  if ( connect )
  {
    p->Connect(wxEVT_COMMAND_LISTBOX_DOUBLECLICKED, 
               wxCommandEventHandler(ListBoxEventHandler::OnListBox));
  }
  else
  {
    p->Disconnect(wxEVT_COMMAND_LISTBOX_DOUBLECLICKED, 
                  wxCommandEventHandler(ListBoxEventHandler::OnListBox));
  }
}

void ListBoxEventHandler::InitConnectEventMap()
{
    AddConnector(WXJS_LISTBOX_EVENT, ConnectListBox);
    AddConnector(WXJS_DOUBLECLICK_EVENT, ConnectDoubleClick);
}
