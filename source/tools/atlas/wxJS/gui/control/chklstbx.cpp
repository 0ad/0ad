#include "precompiled.h"

/*
 * wxJavaScript - chklstbx.cpp
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
 * $Id: chklstbx.cpp 810 2007-07-13 20:07:05Z fbraem $
 */

#ifndef WX_PRECOMP
    #include <wx/wx.h>
#endif

#include "../../common/main.h"
#include "../../common/strsptr.h"
#include "../../common/index.h"
#include "../../ext/wxjs_ext.h"

#include "../event/jsevent.h"
#include "../event/command.h"

#include "chklstbx.h"
#include "chklstbxchk.h"
#include "window.h"
#include "listbox.h"

#include "../misc/size.h"
#include "../misc/validate.h"
#include "../errors.h"

using namespace wxjs;
using namespace wxjs::gui;

/***
 * <file>control/chklstbx</file>
 * <module>gui</module>
 * <class name="wxCheckListBox" prototype="@wxListBox">
 *  A checklistbox is like a listbox, but allows items to be checked
 *  or unchecked.
 * </class>
 */
WXJS_INIT_CLASS(CheckListBox, "wxCheckListBox", 2)

void CheckListBox::InitClass(JSContext* WXUNUSED(cx),
                             JSObject* WXUNUSED(obj), 
                             JSObject* WXUNUSED(proto))
{
    CheckListBoxEventHandler::InitConnectEventMap();
}

/***
 * <properties>
 *  <property name="checked" type="Array" readonly="Y">
 *   Array with @wxCheckListBoxItem elements. 
 *   Use it to check/uncheck a specific item.
 *  </property>
 * </properties>
 */
WXJS_BEGIN_PROPERTY_MAP(CheckListBox)
  WXJS_PROPERTY(P_CHECKED, "checked")
WXJS_END_PROPERTY_MAP()

bool CheckListBox::GetProperty(wxCheckListBox* WXUNUSED(p),
                               JSContext *cx,
                               JSObject *obj,
                               int id,
                               jsval *vp)
{
    if ( id == P_CHECKED )
	{
        *vp = CheckListBoxItem::CreateObject(cx, new Index(0), obj);
	}
    return true;
}

bool CheckListBox::AddProperty(wxCheckListBox *p, 
                               JSContext* WXUNUSED(cx), 
                               JSObject* WXUNUSED(obj), 
                               const wxString &prop, 
                               jsval* WXUNUSED(vp))
{
    if ( WindowEventHandler::ConnectEvent(p, prop, true) )
        return true;
    
    if ( ListBoxEventHandler::ConnectEvent(p, prop, true) )
        return true;

    CheckListBoxEventHandler::ConnectEvent(p, prop, true);
    return true;
}


bool CheckListBox::DeleteProperty(wxCheckListBox *p, 
                                  JSContext* WXUNUSED(cx), 
                                  JSObject* WXUNUSED(obj), 
                                  const wxString &prop)
{
    if ( WindowEventHandler::ConnectEvent(p, prop, false) )
        return true;
    
    if ( ListBoxEventHandler::ConnectEvent(p, prop, false) )
        return true;

    CheckListBoxEventHandler::ConnectEvent(p, prop, false);
    return true;
}

/***
 * <ctor>
 *  <function />
 *  <function>
 *   <arg name="Parent" type="@wxWindow">The parent of this control</arg>
 *   <arg name="Id" type="Integer">
 *    A window identifier. Use -1 when you don't need it.
 *   </arg>
 *   <arg name="Position" type="@wxPoint" default="wxDefaultPosition">
 *    The position of the CheckListBox control on the given parent.
 *   </arg>
 *   <arg name="Size" type="wxSize" default="wxDefaultSize">
 *    The size of the CheckListBox control.
 *   </arg>
 *   <arg name="Items" type="Array" default="null">
 *    An array of Strings to initialize the control
 *   </arg>
 *   <arg name="Style" type="Integer" default="0">
 *    The wxCheckListBox style.
 *   </arg>
 *   <arg name="Validator" type="@wxValidator" default="null">A validator</arg>
 *  </function>
 *  <desc>
 *   Constructs a new wxCheckListBox object.
 *  </desc>
 * </ctor>
 */
wxCheckListBox* CheckListBox::Construct(JSContext *cx,
                                        JSObject *obj,
                                        uintN argc,
                                        jsval *argv,
                                        bool WXUNUSED(constructing))
{
  wxCheckListBox *p = new wxCheckListBox();
  SetPrivate(cx, obj, p);

  if ( argc > 0 )
  {
    jsval rval;
    if ( ! create(cx, obj, argc, argv, &rval) )
      return NULL;
  }
  return p;
}

WXJS_BEGIN_METHOD_MAP(CheckListBox)
  WXJS_METHOD("create", create, 2)
WXJS_END_METHOD_MAP()

/***
 * <method name="create">
 *  <function returns="Boolean">
 *   <arg name="Parent" type="@wxWindow">The parent of this control</arg>
 *   <arg name="Id" type="Integer">
 *    A window identifier. Use -1 when you don't need it.
 *   </arg>
 *   <arg name="Position" type="@wxPoint" default="wxDefaultPosition">
 *    The position of the CheckListBox control on the given parent.
 *   </arg>
 *   <arg name="Size" type="wxSize" default="wxDefaultSize">
 *    The size of the CheckListBox control.
 *   </arg>
 *   <arg name="Items" type="Array" default="null">
 *    An array of Strings to initialize the control
 *   </arg>
 *   <arg name="Style" type="Integer" default="0">
 *    The wxCheckListBox style.
 *   </arg>
 *   <arg name="Validator" type="@wxValidator" default="null">A validator</arg>
 *  </function>
 *  <desc>
 *   Creates wxCheckListBox.
 *  </desc>
 * </method>
 */
JSBool CheckListBox::create(JSContext *cx,
                            JSObject *obj,
                            uintN argc,
                            jsval *argv,
                            jsval *rval)
{
    wxCheckListBox *p = GetPrivate(cx, obj);
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
        {
          JS_ReportError(cx, WXJS_INVALID_ARG_TYPE, 7, "wxValidator");
          return JS_FALSE;
        }
    case 6:
        if ( ! FromJS(cx, argv[5], style) )
        {
          JS_ReportError(cx, WXJS_INVALID_ARG_TYPE, 6, "Integer");
          return JS_FALSE;
        }
    case 5:
    	if ( ! FromJS(cx, argv[4], items) )
        {
          JS_ReportError(cx, WXJS_INVALID_ARG_TYPE, 5, "Array");
          return JS_FALSE;
        }
        // Fall through
    case 4:
		size = Size::GetPrivate(cx, argv[3]);
		if ( size == NULL )
        {
          JS_ReportError(cx, WXJS_INVALID_ARG_TYPE, 4, "wxSize");
          return JS_FALSE;
        }
		// Fall through
	case 3:
        pt = wxjs::ext::GetPoint(cx, argv[2]);
		if ( pt == NULL )
        {
          JS_ReportError(cx, WXJS_INVALID_ARG_TYPE, 3, "wxPoint");
          return JS_FALSE;
        }
		// Fall through
    default:
        int id;
        if ( ! FromJS(cx, argv[1], id) )
        {
          JS_ReportError(cx, WXJS_INVALID_ARG_TYPE, 2, "Integer");
          return JS_FALSE;
        }

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

	    // Don't forget the wxLB_OWNERDRAW, 
        // because Create is called on wxListBox
	    if ( p->Create(parent, id, *pt, *size, 
                       items.GetCount(), items.GetStrings(), 
                       style | wxLB_OWNERDRAW, *val) )
        {
          *rval = JSVAL_TRUE;
          p->SetClientObject(new JavaScriptClientData(cx, obj, true, false));
        }
    }
	
    return JS_TRUE;
}

/***
 * <events>
 *  <event name="onCheckListBox">
 *   Called when an item is checked or unchecked. 
 *   The function that is called gets a @wxCommandEvent
 *   object.</event>
 * </events>
 */

WXJS_INIT_EVENT_MAP(wxCheckListBox)
const wxString WXJS_CHECKLISTBOX_EVENT = wxT("onCheckListBox");

void CheckListBoxEventHandler::OnCheckListBox(wxCommandEvent &event)
{
	PrivCommandEvent::Fire<CommandEvent>(event, WXJS_CHECKLISTBOX_EVENT);
}

void CheckListBoxEventHandler::ConnectCheckListBox(wxCheckListBox *p,
                                                   bool connect)
{
  if ( connect )
  {
    p->Connect(wxEVT_COMMAND_CHECKLISTBOX_TOGGLED, 
               wxCommandEventHandler(CheckListBoxEventHandler::OnCheckListBox));
  }
  else
  {
    p->Disconnect(wxEVT_COMMAND_CHECKLISTBOX_TOGGLED, 
                  wxCommandEventHandler(CheckListBoxEventHandler::OnCheckListBox));
  }
}

void CheckListBoxEventHandler::InitConnectEventMap()
{
    AddConnector(WXJS_CHECKLISTBOX_EVENT, ConnectCheckListBox);
}
