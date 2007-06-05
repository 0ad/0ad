#include "precompiled.h"

/*
 * wxJavaScript - combobox.cpp
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
 * $Id: combobox.cpp 598 2007-03-07 20:13:28Z fbraem $
 */
// combobox.cpp

#ifndef WX_PRECOMP
	#include <wx/wx.h>
#endif

#include "../../common/main.h"
#include "../../common/index.h"

#include "../event/evthand.h"
#include "../event/jsevent.h"
#include "../event/command.h"

#include "combobox.h"
#include "window.h"

#include "../misc/point.h"
#include "../misc/size.h"
#include "../misc/validate.h"

using namespace wxjs;
using namespace wxjs::gui;

ComboBox::ComboBox(JSContext *cx, JSObject *obj)
	:	wxComboBox()
	  , Object(obj, cx)
{
	PushEventHandler(new EventHandler(this));
}

ComboBox::~ComboBox()
{
	PopEventHandler(true);
}

/***
 * <file>control/combobox</file>
 * <module>gui</module>
 * <class name="wxComboBox" prototype="@wxControlWithItems">
 *	A combobox is like a combination of an edit control and a listbox.
 *	It can be displayed as static list with editable or read-only text field; 
 *	or a drop-down list with text field; or a drop-down list without a text field.
 * </class>
 */
WXJS_INIT_CLASS(ComboBox, "wxComboBox", 2)

/***
 * <properties>
 *  <property name="canCopy" type="Boolean" readonly="Y">
 *   Returns true if the combobox is editable and there is a text selection to copy to the clipboard. Only available on Windows.
 *  </property>
 *  <property name="canCut" type="Boolean" readonly="Y">
 *   Returns true if the combobox is editable and there is a text selection to cut to the clipboard. Only available on Windows.
 *  </property>
 *  <property name="canPaste" type="Boolean" readonly="Y">
 *   Returns true if the combobox is editable and there is text to paste from the clipboard. Only available on Windows.
 *  </property>
 *  <property name="canRedo" type="Boolean" readonly="Y">
 *   Returns true if the combobox is editable and the last undo can be redone. Only available on Windows.
 *  </property>
 *  <property name="canUndo" type="Boolean" readonly="Y">
 *   Returns true if the combobox is editable and the last edit can be undone. Only available on Windows.
 *  </property>
 *	<property name="value" type="String">
 *	 Gets/Sets the text field
 *  </property>
 *  <property name="insertionPoint" type="Integer">
 *	 Gets/Sets the insertion point of the text field
 *  </property>
 *	<property name="lastPosition" type="Integer" readonly="Y">
 *	 Gets the last position of the text field
 *  </property>
 * </properties>
 */
WXJS_BEGIN_PROPERTY_MAP(ComboBox)
  WXJS_PROPERTY(P_VALUE, "value")
  WXJS_PROPERTY(P_INSERTION_POINT, "insertionPoint")
  WXJS_READONLY_PROPERTY(P_LAST_POSITION, "lastPosition")
  WXJS_READONLY_PROPERTY(P_CAN_COPY, "canCopy")
  WXJS_READONLY_PROPERTY(P_CAN_CUT, "canCut")
  WXJS_READONLY_PROPERTY(P_CAN_PASTE, "canPaste")
  WXJS_READONLY_PROPERTY(P_CAN_REDO, "canRedo")
  WXJS_READONLY_PROPERTY(P_CAN_UNDO, "canUndo")
WXJS_END_PROPERTY_MAP()

bool ComboBox::GetProperty(wxComboBox *p, JSContext *cx, JSObject *obj, int id, jsval *vp)
{
	switch (id) 
	{
	case P_CAN_COPY:
		*vp = ToJS(cx, p->CanCopy());
		break;
	case P_CAN_CUT:
		*vp = ToJS(cx, p->CanCut());
		break;
	case P_CAN_PASTE:
		*vp = ToJS(cx, p->CanPaste());
		break;
	case P_CAN_REDO:
		*vp = ToJS(cx, p->CanRedo());
		break;
	case P_CAN_UNDO:
		*vp = ToJS(cx, p->CanUndo());
		break;
	case P_VALUE:
		*vp = ToJS(cx, p->GetValue());
		break;
	case P_INSERTION_POINT:
		*vp = ToJS(cx, p->GetInsertionPoint());
		break;
	case P_LAST_POSITION:
		*vp = ToJS(cx, p->GetLastPosition());
		break;
	}
	return true;
}

bool ComboBox::SetProperty(wxComboBox *p, JSContext *cx, JSObject *obj, int id, jsval *vp)
{
	switch (id) 
	{
	case P_VALUE:
		{
			wxString value;
			FromJS(cx, *vp, value);
			p->SetValue(value);
			break;
		}
	case P_INSERTION_POINT:
		{
			int point;
			if ( FromJS(cx, *vp, point) )
				p->SetInsertionPoint(point);
			break;
		}
	}
	return true;
}

/***
 * <constants>
 *	<type name="Style">
 *	 <constant>SIMPLE</constant>
 *	 <constant>DROPDOWN</constant>
 *	 <constant>READONLY</constant>
 *	 <constant>SORT</constant>
 *  </type>
 * </constants>
 */
WXJS_BEGIN_CONSTANT_MAP(ComboBox)
  WXJS_CONSTANT(wxCB_, SIMPLE)
  WXJS_CONSTANT(wxCB_, DROPDOWN)
  WXJS_CONSTANT(wxCB_, READONLY)
  WXJS_CONSTANT(wxCB_, SORT)
WXJS_END_CONSTANT_MAP()

/***
 * <ctor>
 *	<function>
 *	 <arg name="Parent" type="@wxWindow">
 *    The parent of the wxComboBox
 *   </arg>
 *	 <arg name="Id" type="Integer">
 *	  A window identifier. Use -1 when you don't need it
 *   </arg>
 *   <arg name="Text" type="String" default="">
 *	  The default text of the text field
 *   </arg>
 *   <arg name="Position" type="@wxPoint" default="wxDefaultPosition">
 *	  The position of the control on the given parent.
 *   </arg>
 *   <arg name="Size" type="@wxSize" default="wxDefaultSize">
 *	  The size of the control.
 *   </arg>
 *   <arg name="Items" type="Array" default="null">
 *	  An array of Strings to initialize the control.
 *   </arg>
 *   <arg name="Style" type="Integer" default="0">
 *	  The window style.
 *   </arg>
 *   <arg name="Validator" type="@wxValidator" default="null">
 *    A validator
 *   </arg>
 *  </function>
 *  <desc>
 *	 Constructs a new wxComboBox object
 *  </desc>
 * </ctor>
 */
wxComboBox *ComboBox::Construct(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, bool constructing)
{
    if ( argc > 8 )
        argc = 8;

    int style = 0;
    StringsPtr items;
    wxString text;
    const wxPoint *pt = &wxDefaultPosition;
    const wxSize *size = &wxDefaultSize;
    const wxValidator *val = &wxDefaultValidator;

    switch(argc)
    {
    case 8:
        val = Validator::GetPrivate(cx, argv[7]);
        if ( val == NULL )
            break;
    case 7:
        if ( ! FromJS(cx, argv[6], style) )
            break;
        // Fall through
    case 6:
    	if ( ! FromJS(cx, argv[5], items) )
            break;
        // Fall through
    case 5:
		size = Size::GetPrivate(cx, argv[4]);
		if ( size == NULL )
			break;
		// Fall through
	case 4:
		pt = Point::GetPrivate(cx, argv[3]);
		if ( pt == NULL )
			break;
		// Fall through
    default:
        
        wxString text;
        FromJS(cx, argv[2], text);
        
        int id;
        if ( ! FromJS(cx, argv[1], id) )
            break;

        wxWindow *parent = Window::GetPrivate(cx, argv[0]);
		if ( parent == NULL )
            break;

        Object *wxjsParent = dynamic_cast<Object *>(parent);
        JS_SetParent(cx, obj, wxjsParent->GetObject());

	    ComboBox *p = new ComboBox(cx, obj);
	    p->Create(parent, id, text, *pt, *size, items.GetCount(), items.GetStrings(), style, *val);

        return p;
    }
	return NULL;
}

WXJS_BEGIN_METHOD_MAP(ComboBox)
  WXJS_METHOD("copy", copy, 0)
  WXJS_METHOD("cut", cut, 0)
  WXJS_METHOD("paste", paste, 0)
  WXJS_METHOD("replace", replace, 3)
  WXJS_METHOD("remove", remove, 2)
  WXJS_METHOD("redo", redo, 0)
  WXJS_METHOD("undo", undo, 0)
WXJS_END_METHOD_MAP()

/***
 * <method name="copy">
 *	<function />
 *  <desc>
 *	 Copies the selected text to the clipboard
 *  </desc>
 * </method>
 */
JSBool ComboBox::copy(JSContext *cx, JSObject *obj,
						  uintN argc, jsval *argv, jsval *rval)
{
    wxComboBox *p = GetPrivate(cx, obj);
	if ( p == NULL )
		return JS_FALSE;

	p->Copy();

	return JS_TRUE;
}

/***
 * <method name="cut">
 *	<function />
 *  <desc>
 *	 Copies the selected text to the clipboard and removes the selected text
 *  </desc>
 * </method>
 */
JSBool ComboBox::cut(JSContext *cx, JSObject *obj,
						  uintN argc, jsval *argv, jsval *rval)
{
    wxComboBox *p = GetPrivate(cx, obj);
	if ( p == NULL )
		return JS_FALSE;

	p->Cut();

	return JS_TRUE;
}

/***
 * <method name="paste">
 *	<function />
 *  <desc>
 *	 Pastes the content of the clipboard in the text field.
 *  </desc>
 * </method>
 */
JSBool ComboBox::paste(JSContext *cx, JSObject *obj,
						   uintN argc, jsval *argv, jsval *rval)
{
    wxComboBox *p = GetPrivate(cx, obj);
	if ( p == NULL )
		return JS_FALSE;

	p->Paste();

	return JS_TRUE;
}

/***
 * <method name="redo">
 *	<function />
 *  <desc>
 *   Redoes the last undo in the text field. Windows only.
 *  </desc>
 * </method>
 */
JSBool ComboBox::redo(JSContext *cx, JSObject *obj,
						  uintN argc, jsval *argv, jsval *rval)
{
    wxComboBox *p = GetPrivate(cx, obj);
	if ( p == NULL )
		return JS_FALSE;

	p->Redo();

	return JS_TRUE;
}

/***
 * <method name="remove">
 *	<function>
 *	 <arg name="From" type="Integer" />
 *	 <arg name="To" type="Integer" />
 *  </function>
 *	<desc>
 *   Removes the text between From and To
 *  </desc>
 * </method>
 */
JSBool ComboBox::remove(JSContext *cx, JSObject *obj, 
							uintN argc, jsval *argv, jsval *rval)
{
    wxComboBox *p = GetPrivate(cx, obj);
	if ( p == NULL )
		return JS_FALSE;
	
    long from = 0L;
	long to = 0L;
    if (    FromJS(cx, argv[0], from) 
         && FromJS(cx, argv[1], to) )
	{
		p->Remove(from, to);
		return JS_TRUE;
	}

    return JS_FALSE;
}

/***
 * <method name="replace">
 *	<function>
 *   <arg name="From" type="Integer" />
 *	 <arg name="To" type="Integer" />
 *	 <arg name="Text" type="String" />
 *  </function>
 *  <desc>
 *	 Replaces the text between From and To with the given text
 *  </desc>
 * </method>
 */
JSBool ComboBox::replace(JSContext *cx, JSObject *obj,
							 uintN argc, jsval *argv, jsval *rval)
{
    wxComboBox *p = GetPrivate(cx, obj);
	if ( p == NULL )
		return JS_FALSE;

	int from;
	int to;
	wxString text;

    if (    FromJS(cx, argv[0], from)
         && FromJS(cx, argv[1], to)
         && FromJS(cx, argv[2], text) )
    {
    	p->Replace(from, to, text);
        return JS_TRUE;
    }
	return JS_FALSE;
}

/***
 * <method name="undo">
 *	<function />
 *  <desc>
 *	 Undoes the last edit in the text field. Windows only.
 *  </desc>
 * </method>
 */
JSBool ComboBox::undo(JSContext *cx, JSObject *obj,
						  uintN argc, jsval *argv, jsval *rval)
{
    wxComboBox *p = GetPrivate(cx, obj);
	if ( p == NULL )
		return JS_FALSE;

	p->Undo();

	return JS_TRUE;
}

/***
 * <events>
 *  <event name="onText">
 *	 Called when the text of the textfield is changed. 
 *	 The type of the argument that your handler receives is @wxCommandEvent.
 *  </event>
 *	<event name="onComboBox">
 *	 Called when an item is selected. The type of the argument 
 *	 that your handler receives is @wxCommandEvent.
 *  </event>
 * </events>
 */
void ComboBox::OnText(wxCommandEvent &event)
{
	PrivCommandEvent::Fire<CommandEvent>(this, event, "onText");
}

void ComboBox::OnComboBox(wxCommandEvent &event)
{
	PrivCommandEvent::Fire<CommandEvent>(this, event, "onComboBox");
}

BEGIN_EVENT_TABLE(ComboBox, wxComboBox)
	EVT_TEXT(-1, ComboBox::OnText)
	EVT_COMBOBOX(-1, ComboBox::OnComboBox)
END_EVENT_TABLE()

