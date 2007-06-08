#include "precompiled.h"

/*
 * wxJavaScript - choice.cpp
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
 * $Id: choice.cpp 708 2007-05-14 15:30:45Z fbraem $
 */

#ifndef WX_PRECOMP
	#include <wx/wx.h>
#endif

#include "../../common/main.h"
#include "../../common/index.h"

#include "../event/jsevent.h"
#include "../event/command.h"

#include "window.h"
#include "choice.h"
#include "item.h"

#include "../misc/point.h"
#include "../misc/size.h"
#include "../misc/validate.h"
#include "../errors.h"

using namespace wxjs;
using namespace wxjs::gui;

/***
 * <file>control/choice</file>
 * <module>gui</module>
 * <class name="wxChoice" prototype="@wxControlWithItems">
 *   A choice item is used to select one of a list of strings. 
 *	 Unlike a listbox, only the selection is visible until the 
 *	 user pulls down the menu of choices. An example:
 *   <pre><code class="whjs">
 *	 var items = new Array();
 *	 items[0] = "Opel";
 *	 items[1] = "Ford";
 *	 items[2] = "BMW";
 *	 // dlg is a wxDialog
 *	 var choice = new wxChoice(dlg, -1, wxDefaultPosition, wxDefaultSize, items);
 *   </code></pre>
 * </class>
 */
WXJS_INIT_CLASS(Choice, "wxChoice", 2)

void Choice::InitClass(JSContext* WXUNUSED(cx),
                       JSObject* WXUNUSED(obj), 
                       JSObject* WXUNUSED(proto))
{
  ChoiceEventHandler::InitConnectEventMap();
}

/***
 * <properties>
 *	<property name="columns" type="Integer">
 *	 Gets/Sets the number of columns
 *  </property>
 * </properties>
 */

WXJS_BEGIN_PROPERTY_MAP(Choice)
  WXJS_PROPERTY(P_COLUMNS, "columns")
WXJS_END_PROPERTY_MAP()

bool Choice::GetProperty(wxChoice *p,
                         JSContext *cx,
                         JSObject* WXUNUSED(obj),
                         int id,
                         jsval *vp)
{
	switch (id) 
	{
	case P_COLUMNS:
		*vp = ToJS(cx, p->GetColumns());
		break;
	}
	return true;
}

bool Choice::SetProperty(wxChoice *p,
                         JSContext *cx,
                         JSObject* WXUNUSED(obj),
                         int id,
                         jsval *vp)
{
	switch (id) 
	{
	case P_COLUMNS:
		{
			int columns;
			if ( FromJS(cx, *vp, columns) )
				p->SetColumns(columns);
			break;
		}
	}
	return true;
}

bool Choice::AddProperty(wxChoice *p, 
                         JSContext* WXUNUSED(cx), 
                         JSObject* WXUNUSED(obj), 
                         const wxString &prop, 
                         jsval* WXUNUSED(vp))
{
    if ( WindowEventHandler::ConnectEvent(p, prop, true) )
        return true;
    
    ChoiceEventHandler::ConnectEvent(p, prop, true);

    return true;
}

bool Choice::DeleteProperty(wxChoice *p, 
                            JSContext* WXUNUSED(cx), 
                            JSObject* WXUNUSED(obj), 
                            const wxString &prop)
{
  if ( WindowEventHandler::ConnectEvent(p, prop, false) )
    return true;
  
  ChoiceEventHandler::ConnectEvent(p, prop, false);
  return true;
}

/***
 * <ctor>
 *  <function />
 *	<function>
 *   <arg name="Parent" type="@wxWindow">
 *    The parent of the control
 *   </arg>
 *   <arg name="Id" type="Integer">
 *	  An window identifier. Use -1 when you don't need it.
 *   </arg>
 *   <arg name="Position" type="@wxPoint" default="wxDefaultPosition">
 *	  The position of the choice control on the given parent.
 *   </arg>
 *   <arg name="Size" type="@wxSize" default="wxDefaultSize">
 *	  The size of the choice control.
 *   </arg>
 *   <arg name="Items" type="Array" default="null">
 *	  An array of Strings to initialize the control.
 *   </arg>
 *   <arg name="Style" type="Integer" default="0">
 *    The style of the control
 *   </arg>
 *   <arg name="Validator" type="@wxValidator" default="null">
 *    A validator
 *   </arg>
 *  </function>
 *  <desc>
 *	 Constructs a new wxChoice object.
 *  </desc>
 * </ctor>
 */
wxChoice* Choice::Construct(JSContext *cx,
                            JSObject *obj,
                            uintN argc,
                            jsval *argv,
                            bool WXUNUSED(constructing))
{
  wxChoice *p = new wxChoice();
  SetPrivate(cx, obj, p);

  if ( argc > 0 )
  {
    jsval rval;
    create(cx, obj, argc, argv, &rval);
  }
  return p;
}

WXJS_BEGIN_METHOD_MAP(Choice)
  WXJS_METHOD("create", create, 2)
WXJS_END_METHOD_MAP()

/***
 * <method name="create">
 *	<function returns="Boolean">
 *   <arg name="Parent" type="@wxWindow">
 *    The parent of the control
 *   </arg>
 *   <arg name="Id" type="Integer">
 *	  An window identifier. Use -1 when you don't need it.
 *   </arg>
 *   <arg name="Position" type="@wxPoint" default="wxDefaultPosition">
 *	  The position of the choice control on the given parent.
 *   </arg>
 *   <arg name="Size" type="@wxSize" default="wxDefaultSize">
 *	  The size of the choice control.
 *   </arg>
 *   <arg name="Items" type="Array" default="null">
 *	  An array of Strings to initialize the control.
 *   </arg>
 *   <arg name="Style" type="Integer" default="0">
 *    The style of the control
 *   </arg>
 *   <arg name="Validator" type="@wxValidator" default="null">
 *    A validator
 *   </arg>
 *  </function>
 *  <desc>
 *	 Creates a wxChoice
 *  </desc>
 * </method>
 */
JSBool Choice::create(JSContext *cx,
                      JSObject *obj,
                      uintN argc,
                      jsval *argv,
                      jsval *rval)
{
    wxChoice *p = GetPrivate(cx, obj);
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
        // Fall through
    case 5:
    	if ( ! FromJS(cx, argv[4], items) )
        {
          JS_ReportError(cx, WXJS_INVALID_ARG_TYPE, 5, "Array");
          return JS_FALSE;
        }
    case 4:
		size = Size::GetPrivate(cx, argv[3]);
		if ( size == NULL )
        {
          JS_ReportError(cx, WXJS_INVALID_ARG_TYPE, 4, "wxSize");
          return JS_FALSE;
        }
		// Fall through
	case 3:
		pt = Point::GetPrivate(cx, argv[2]);
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
 * <events>
 *	<event name="onChoice">
 *	 Called when an item is selected. The type of the argument that your handler receives 
 *	 is @wxCommandEvent.
 *  </event>
 * </events>
 */
WXJS_INIT_EVENT_MAP(wxChoice)
const wxString WXJS_CHOICE_EVENT = wxT("onChoice");

void ChoiceEventHandler::OnChoice(wxCommandEvent &event)
{
	PrivCommandEvent::Fire<CommandEvent>(event, WXJS_CHOICE_EVENT);
}

void ChoiceEventHandler::ConnectChoice(wxChoice *p, bool connect)
{
  if ( connect )
  {
    p->Connect(wxEVT_COMMAND_CHOICE_SELECTED, 
               wxCommandEventHandler(OnChoice));
  }
  else
  {
    p->Disconnect(wxEVT_COMMAND_CHOICE_SELECTED, 
                  wxCommandEventHandler(OnChoice));
  }
}

void ChoiceEventHandler::InitConnectEventMap()
{
    AddConnector(WXJS_CHOICE_EVENT, ConnectChoice);
}
