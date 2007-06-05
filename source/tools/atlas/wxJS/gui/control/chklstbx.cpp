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
 * $Id: chklstbx.cpp 598 2007-03-07 20:13:28Z fbraem $
 */
// chklstbx.cpp

#ifndef WX_PRECOMP
    #include <wx/wx.h>
#endif

#include "../../common/main.h"
#include "../../common/strsptr.h"
#include "../../common/index.h"

#include "../event/evthand.h"
#include "../event/jsevent.h"
#include "../event/command.h"

#include "chklstbx.h"
#include "chklstbxchk.h"
#include "window.h"

#include "../misc/point.h"
#include "../misc/size.h"
#include "../misc/validate.h"

using namespace wxjs;
using namespace wxjs::gui;

CheckListBox::CheckListBox(JSContext *cx, JSObject *obj)
	:	wxCheckListBox()
	  , Object(obj, cx)
{
	PushEventHandler(new EventHandler(this));
}

CheckListBox::~CheckListBox()
{
	PopEventHandler(true);
}


/***
 * <file>control/chklstbx</file>
 * <module>gui</module>
 * <class name="wxCheckListBox" prototype="@wxListBox">
 *  A checklistbox is like a listbox, but allows items to be checked or unchecked.
 * </class>
 */
WXJS_INIT_CLASS(CheckListBox, "wxCheckListBox", 2)

/***
 * <properties>
 *  <property name="checked" type="Array" readonly="Y">
 *   Array with @wxCheckListBoxChecked elements. Use it to check/uncheck a specific item.
 *  </property>
 * </properties>
 */
WXJS_BEGIN_PROPERTY_MAP(CheckListBox)
  WXJS_PROPERTY(P_CHECKED, "checked")
WXJS_END_PROPERTY_MAP()

bool CheckListBox::GetProperty(wxCheckListBox *p, JSContext *cx, JSObject *obj, int id, jsval *vp)
{
    if ( id == P_CHECKED )
	{
        *vp = CheckListBoxChecked::CreateObject(cx, new Index(0), obj);
	}
    return true;
}

/***
 * <ctor>
 *  <function>
 *   <arg name="Parent" type="@wxWindow">The parent of this control</arg>
 *   <arg name="Id" type="Integer">A window identifier. Use -1 when you don't need it.</arg>
 *   <arg name="Position" type="@wxPoint" default="wxDefaultPosition">
 *    The position of the CheckListBox control on the given parent.
 *   </arg>
 *   <arg name="Size" type="wxSize" default="wxDefaultSize">
 *    The size of the CheckListBox control.
 *   </arg>
 *   <arg name="Items" type="Array" default="null">An array of Strings to initialize the control</arg>
 *   <arg name="Style" type="Integer" default="0">The wxCheckListBox style.</arg>
 *   <arg name="Validator" type="@wxValidator" default="null">A validator</arg>
 *  </function>
 *  <desc>
 *   Constructs a new wxCheckListBox object.
 *  </desc>
 * </ctor>
 */
wxCheckListBox* CheckListBox::Construct(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, bool constructing)
{
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
            break;

        Object *wxjsParent = dynamic_cast<Object *>(parent);
        JS_SetParent(cx, obj, wxjsParent->GetObject());

	    CheckListBox *p = new CheckListBox(cx, obj);
	    // Don't forget the wxLB_OWNERDRAW, because Create is called on wxListBox
	    p->Create(parent, id, *pt, *size, items.GetCount(), items.GetStrings(), style | wxLB_OWNERDRAW, *val);

        return p;
    }
	
    return NULL;
}

/***
 * <events>
 *  <event name="onCheckListBox">
 *   Called when an item is checked or unchecked. The function that is called gets a @CommandEvent
 *   object.</event>
 * </events>
 */
void CheckListBox::OnCheckListBox(wxCommandEvent &event)
{
	PrivCommandEvent::Fire<CommandEvent>(this, event, "onCheckListBox");
}

BEGIN_EVENT_TABLE(CheckListBox, wxCheckListBox)
  EVT_CHECKLISTBOX(-1, CheckListBox::OnCheckListBox)
END_EVENT_TABLE()
