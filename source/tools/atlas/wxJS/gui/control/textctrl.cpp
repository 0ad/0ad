#include "precompiled.h"

/*
 * wxJavaScript - textctrl.cpp
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
 * $Id: textctrl.cpp 598 2007-03-07 20:13:28Z fbraem $
 */
// textctrl.cpp

#ifndef WX_PRECOMP
    #include <wx/wx.h>
#endif
#include <wx/file.h>

#include "../../common/main.h"

#include "../event/evthand.h"
#include "../event/jsevent.h"
#include "../event/command.h"

#include "textctrl.h"
#include "window.h"

#include "../misc/point.h"
#include "../misc/size.h"
#include "../misc/validate.h"

using namespace wxjs;
using namespace wxjs::gui;

TextCtrl::TextCtrl(JSContext *cx, JSObject *obj)
	:	wxTextCtrl()
	  , Object(obj, cx)
{
	PushEventHandler(new EventHandler(this));
}

TextCtrl::~TextCtrl()
{
	PopEventHandler(true);
}

/***
 * <file>control/textctrl</file>
 * <module>gui</module>
 * <class name="wxTextCtrl" prototype="@wxControl">
 *	A text control allows text to be displayed and edited. It may be single line or multi-line.
 *	@section wxTextCtrl_proto Prototype
 * </class>
 */
WXJS_INIT_CLASS(TextCtrl, "wxTextCtrl", 2)

/***
 * <properties>
 *	<property name="canCopy" type="Boolean" readonly="Y">
 *	 Returns true if the selection can be copied to the clipboard.
 *  </property>
 *	<property name="canCut" type="Boolean" readonly="Y">
 *	 Returns true when the selection can be cut to the clipboard.
 *  </property>
 *	<property name="canPaste" type="Boolean" readonly="Y">
 *	 Returns true when the contents of clipboard can be pasted into the text control.
 *  </property>
 *	<property name="canRedo" type="Boolean" readonly="Y">
 *	 Returns true if there is a redo facility available and the last operation can be redone.
 *  </property>
 *	<property name="canUndo" type="Boolean" readonly="Y">
 *	 Returns true if there is an undo facility available and the last operation can be undone.
 *  </property>
 *	<property name="insertionPoint" type="Integer">
 *	 Get/Sets the insertion point
 *  </property>
 *	<property name="numberOfLines" type="Integer" readonly="Y">
 * 	 Get the number of lines
 *  </property>
 *	<property name="selectionFrom" type="Integer" readonly="Y">
 *	 Get the start position of the selection
 *  </property>
 *	<property name="selectionTo" type="Integer" readonly="Y">
 *	 Get the end position of the selection
 *  </property>
 *	<property name="value" type="String">
 *	 Get/Set the text
 *  </property>
 *	<property name="modified" type="Boolean" readonly="Y">
 *	 Returns true when the text is changed
 *  </property>
 *	<property name="lastPosition" type="Integer" readonly="Y">
 *	 Get the position of the last character in the text control
 *  </property>
 *	<property name="editable" type="Boolean">
 *	 Enables/Disables the text control
 *  </property>
 * </properties>
 */
WXJS_BEGIN_PROPERTY_MAP(TextCtrl)
  WXJS_READONLY_PROPERTY(P_CAN_COPY, "canCopy")
  WXJS_READONLY_PROPERTY(P_CAN_CUT, "canCut")
  WXJS_READONLY_PROPERTY(P_CAN_PASTE, "canPaste")
  WXJS_READONLY_PROPERTY(P_CAN_REDO, "canRedo")
  WXJS_READONLY_PROPERTY(P_CAN_UNDO, "canUndo")
  WXJS_PROPERTY(P_INSERTION_POINT, "insertionPoint")
  WXJS_READONLY_PROPERTY(P_NUMBER_OF_LINES, "numberOfLines")
  WXJS_READONLY_PROPERTY(P_SELECTION_FROM, "selectionFrom")
  WXJS_READONLY_PROPERTY(P_SELECTION_TO, "selectionTo")
  WXJS_PROPERTY(P_VALUE, "value")
  WXJS_READONLY_PROPERTY(P_MODIFIED, "modified")
  WXJS_READONLY_PROPERTY(P_LAST_POSITION, "lastPosition")
  WXJS_PROPERTY(P_EDITABLE, "editable")
WXJS_END_PROPERTY_MAP()

WXJS_BEGIN_METHOD_MAP(TextCtrl)
  WXJS_METHOD("appendText", appendText, 1)
  WXJS_METHOD("clear", clear, 0)
  WXJS_METHOD("cut", cut, 0)
  WXJS_METHOD("discardEdits", discardEdits, 0)
  WXJS_METHOD("getLineLength", getLineLength, 1)
  WXJS_METHOD("getLineText", getLineText, 1)
  WXJS_METHOD("loadFile", loadFile, 1)
  WXJS_METHOD("paste", paste, 0)
  WXJS_METHOD("setSelection", setSelection, 2)
  WXJS_METHOD("redo", redo, 0)
  WXJS_METHOD("replace", replace, 3)
  WXJS_METHOD("remove", remove, 2)
  WXJS_METHOD("saveFile", saveFile, 2)
WXJS_END_METHOD_MAP()

/***
 * <constants>
 *	<type name="Style">
 *	 <constant name="PROCESS_ENTER" />
 *	 <constant name="PROCESS_TAB" />
 *	 <constant name="MULTILINE" />
 *	 <constant name="PASSWORD" />
 *	 <constant name="READONLY" />
 *	 <constant name="RICH" />
 *  </type>
 * </constants>
 */
WXJS_BEGIN_CONSTANT_MAP(TextCtrl)
  WXJS_CONSTANT(wxTE_, PROCESS_ENTER)
  WXJS_CONSTANT(wxTE_, PROCESS_TAB)
  WXJS_CONSTANT(wxTE_, MULTILINE)
  WXJS_CONSTANT(wxTE_, PASSWORD)
  WXJS_CONSTANT(wxTE_, READONLY)
  WXJS_CONSTANT(wxTE_, RICH)
WXJS_END_CONSTANT_MAP()

bool TextCtrl::GetProperty(wxTextCtrl *p, JSContext *cx, JSObject *obj, int id, jsval *vp)
{
	switch (id) 
	{
	case P_CAN_COPY:
		*vp = ToJS(cx, p->CanCopy());
		break;
	case P_CAN_PASTE:
		*vp = ToJS(cx, p->CanPaste());
		break;
	case P_CAN_CUT:
		*vp = ToJS(cx, p->CanCut());
		break;
	case P_CAN_REDO:
		*vp = ToJS(cx, p->CanRedo());
		break;
	case P_CAN_UNDO:
		*vp = ToJS(cx, p->CanUndo());
		break;
	case P_INSERTION_POINT:
		*vp = ToJS(cx, p->GetInsertionPoint());
		break;
	case P_NUMBER_OF_LINES:
		*vp = ToJS(cx, p->GetNumberOfLines());
		break;
	case P_SELECTION_FROM:
		{
			long from = 0L;
			long to = 0L;
			p->GetSelection(&from, &to);
			*vp = ToJS(cx, from);
			break;
		}
	case P_SELECTION_TO:
		{
			long from = 0L;
			long to = 0L;
			p->GetSelection(&from, &to);
			*vp = ToJS(cx, to);
			break;
		}
	case P_VALUE:
		*vp = ToJS(cx, p->GetValue());
		break;
	case P_MODIFIED:
		*vp = ToJS(cx, p->IsModified());
		break;
	case P_LAST_POSITION:
		*vp = ToJS(cx, p->GetLastPosition());
		break;
	case P_EDITABLE:
		{
			// Need some testing !!!!!!
			long style = p->GetWindowStyleFlag();
			*vp = ToJS(cx, (style & wxTE_READONLY) != wxTE_READONLY);
			break;
		}
	}
    return true;
}

bool TextCtrl::SetProperty(wxTextCtrl *p, JSContext *cx, JSObject *obj, int id, jsval *vp)
{
	switch (id) 
	{
	case P_INSERTION_POINT:
		{
			int pos;
			if ( FromJS(cx, *vp, pos) )
				p->SetInsertionPoint(pos);
			break;
		}
	case P_VALUE:
		{
			wxString value;
			FromJS(cx, *vp, value);
			p->SetValue(value);
			break;
		}
	case P_EDITABLE:
		{
			bool editable;
			if ( FromJS(cx, *vp, editable) )
			{
				p->SetEditable(editable);
			}
			break;
		}
	}
	return true;
}

/***
 * <ctor>
 *	<function>
 *	 <arg name="Parent" type="@wxWindow">
 *	  The parent of the text control.
 *   </arg>
 *	 <arg name="Id" type="Integer">
 *	  The unique id
 *   </arg>
 *	 <arg name="Text" type="String">
 *	  The default text
 *   </arg>
 *	 <arg name="Position" type="@wxPoint" default="wxDefaultPosition">
 *	  The position of the control.
 *   </arg>
 *	 <arg name="Size" type="@wxSize" default="wxDefaultSize">
 *	  The size of the control.
 *   </arg>
 *	 <arg name="Validator" type="@wxValidator" default="null" />
 *  </function>
 *  <desc>
 *	 Constructs a new wxTextCtrl object
 *  </desc>
 * </ctor>
 */
wxTextCtrl* TextCtrl::Construct(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, bool constructing)
{
    if ( argc > 7 )
        argc = 7;

	const wxPoint *pt = &wxDefaultPosition;
	const wxSize *size = &wxDefaultSize;
    int style = 0;
    wxString text = wxEmptyString;
    const wxValidator *val = &wxDefaultValidator;

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
		size = Size::GetPrivate(cx, argv[4]);
		if ( size == NULL )
			break;
		// Fall through
	case 4:
		pt = Point::GetPrivate(cx, argv[3]);
		if ( pt == NULL )
			break;
		// Fall through
    case 3:
        FromJS(cx, argv[2], text);
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

	    wxTextCtrl *p = new TextCtrl(cx, obj);
	    p->Create(parent, id, text, *pt, *size, style, *val);
        return p;
    }

    return NULL;
}

/***
 * <method name="appendText">
 *	<function>
 *	 <arg name="Text" type="String">
 *	  Text to append
 *	 </arg>
 *  </function>
 *  <desc>
 *	 Appends the text to the text of the control.
 *  </desc>
 * </method>
 */
JSBool TextCtrl::appendText(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
    wxTextCtrl *p = GetPrivate(cx, obj);
	if ( p == NULL )
		return JS_FALSE;

	wxString text;
	FromJS(cx, argv[0], text);
	p->AppendText(text);

	return JS_TRUE;
}

/***
 * <method name="clear">
 *	<function />
 *	<desc>
 *	 Removes the text.
 *  </desc>
 * </method>
 */
JSBool TextCtrl::clear(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
    wxTextCtrl *p = GetPrivate(cx, obj);
	if ( p == NULL )
		return JS_FALSE;

    p->Clear();
	
	return JS_TRUE;
}

/***
 * <method name="cut">
 *	<function />
 *	<desc>
 *   Removes the selected text and copies it to the clipboard.
 *  </desc>
 * </method>
 */
JSBool TextCtrl::cut(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
    wxTextCtrl *p = GetPrivate(cx, obj);
	if ( p == NULL )
		return JS_FALSE;

	p->Cut();
	
	return JS_TRUE;
}

/***
 * <method name="discardEdits">
 *	<function />
 *	<desc>
 *	 Resets the modified flag
 *  </desc>
 * </method>
 */
JSBool TextCtrl::discardEdits(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
    wxTextCtrl *p = GetPrivate(cx, obj);
	if ( p == NULL )
		return JS_FALSE;

	p->DiscardEdits();
	
	return JS_TRUE;
}

/***
 * <method name="getLineLength">
 *	<function returns="Integer">
 *	 <arg name="Line" type="Integer">
 *	  The line number
 *   </arg>
 *  </function>
 *  <desc>
 * 	 Returns the length of the given line
 *  </desc>
 * </method>
 */
JSBool TextCtrl::getLineLength(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
    wxTextCtrl *p = GetPrivate(cx, obj);
	if ( p == NULL )
		return JS_FALSE;

	int line;
	if ( FromJS(cx, argv[0], line) )
	{
		*rval = ToJS(cx, p->GetLineLength(line));
	}
	else
	{
		return JS_FALSE;
	}

	return JS_TRUE;
}

/***
 * <method name="getLineText">
 *	<function returns="String">
 *	 <arg name="Line" type="Integer">
 *	  The line number
 *   </arg>
 *  </function>
 *	<desc>
 *	 Returns the text of the given line	
 *  </desc>
 * </method>
 */
JSBool TextCtrl::getLineText(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
    wxTextCtrl *p = GetPrivate(cx, obj);
	if ( p == NULL )
		return JS_FALSE;

	int line;
	if ( FromJS(cx, argv[0], line) )
	{
		*rval = ToJS(cx, p->GetLineText(line));
	}
	else
	{
		return JS_FALSE;
	}

	return JS_TRUE;
}

/***
 * <method name="setSelection">
 *	<function>
 *   <arg name="From" type="Integer" default="0">
 *	  When not specified, 0 is used.
 *   </arg>
 *	 <arg name="To" type="Integer">
 *	  When not specified, the end position is used.
 *   </arg>
 *  </function>
 *  <desc>
 *	 Selects the text between <i>From</i> and <i>To</i>.
 *  </desc>
 * </method>
 */
JSBool TextCtrl::setSelection(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
    wxTextCtrl *p = GetPrivate(cx, obj);
	if ( p == NULL )
		return JS_FALSE;

	long from = 0L;
	long to = p->GetLastPosition();

	if (argc > 0)
	{
		FromJS(cx, argv[0], from);
		if ( argc > 1 )
		{
			FromJS(cx, argv[1], to);
		}
	}
	p->SetSelection(from, to);

	return JS_TRUE;
}

/***
 * <method name="loadFile">
 *	<function>
 *	 <arg name="File" type="String">
 *	  The name of a file
 *   </arg>
 *  </function>
 *  <desc>
 *	 Loads a file into the text control
 *  </desc>
 * </method>
 */
JSBool TextCtrl::loadFile(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
    wxTextCtrl *p = GetPrivate(cx, obj);
	if ( p == NULL )
		return JS_FALSE;

	wxString filename;
	FromJS(cx, argv[0], filename);
	if ( wxFile::Exists(filename) )
	{
		p->LoadFile(filename);
	}
	else
	{
		return JS_FALSE;
	}
	return JS_TRUE;
}

/***
 * <method name="paste">
 *	<function />
 *  <desc>
 *	 Pastes the content of the clipboard in the selection of the text control.
 *  </desc>
 * </method>
 */
JSBool TextCtrl::paste(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
    wxTextCtrl *p = GetPrivate(cx, obj);
	if ( p == NULL )
		return JS_FALSE;

	p->Paste();
	
	return JS_TRUE;
}

/***
 * <method name="redo">
 *	<function />
 *	<desc>
 *	 Tries to redo the last operation
 *  </desc>
 * </method>
 */
JSBool TextCtrl::redo(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
    wxTextCtrl *p = GetPrivate(cx, obj);
	if ( p == NULL )
		return JS_FALSE;

	p->Redo();
	
	return JS_TRUE;
}

/***
 * <method name="replace">
 *  <function>
 *	 <arg name="From" type="Integer" />
 *	 <arg name="To" type="Integer" />
 *	 <arg name="Text" type="String" />
 *	</function>
 *  <desc>
 *	 Replaces the text between <i>From</i> and <i>To</i> with the new text
 *  </desc>
 * </method>
 */
JSBool TextCtrl::replace(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
    wxTextCtrl *p = GetPrivate(cx, obj);
	if ( p == NULL )
		return JS_FALSE;

	long from = 0L;
	long to = 0L;
	wxString text;

	if (    FromJS(cx, argv[0], from)
		 && FromJS(cx, argv[1], to) )
	{
		FromJS(cx, argv[2], text);
		p->Replace(from, to, text);
		return JS_TRUE;
	}
	return JS_FALSE;
}

/***
 * <method name="remove">
 *  <function>
 *	 <arg name="From" type="Integer" />
 *	 <arg name="To" type="Integer" />
 *	</function>
 *  <desc>
 *	 Removes the text between <i>From</i> and <i>To</i>.
 *  </desc>
 * </method>
 */
JSBool TextCtrl::remove(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
    wxTextCtrl *p = GetPrivate(cx, obj);
	if ( p == NULL )
		return JS_FALSE;

	long from = 0L;
	long to = 0L;
	if (    FromJS(cx, argv[0], from)
		 && FromJS(cx, argv[1], to)   )
	{
		p->Remove(from, to);
	}
	else
	{
		return JS_FALSE;
	}
	return JS_TRUE;
}

/***
 * <method name="saveFile">
 *	<function returns="Boolean">
 *	 <arg name="File" type="String" />
 *  </function>
 *  <desc>
 *	 Saves the content of the text control to the given file
 *  </desc>
 * </method>
 */
JSBool TextCtrl::saveFile(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
    wxTextCtrl *p = GetPrivate(cx, obj);
	if ( p == NULL )
		return JS_FALSE;

	wxString filename;
	FromJS(cx, argv[0], filename);
	*rval = ToJS(cx, p->SaveFile(filename));

	return JS_TRUE;
}

/***
 * <events>
 *  <event name="onText">
 *	 Triggered when the text is changed. The argument of the function is @wxCommandEvent
 *  </event>
 *	<event name="onTextEnter">
 *	 Triggered when the enter key is pressed in a single-line text control. 
 *	 The argument of the function is @wxCommandEvent
 *  </event>
 * </events>
 */
void TextCtrl::OnText(wxCommandEvent &event)
{
	PrivCommandEvent::Fire<CommandEvent>(this, event, "onText");
}

void TextCtrl::OnTextEnter(wxCommandEvent &event)
{
	PrivCommandEvent::Fire<CommandEvent>(this, event, "onTextEnter");
}

BEGIN_EVENT_TABLE(TextCtrl, wxTextCtrl)
	EVT_TEXT(-1, TextCtrl::OnText)
	EVT_TEXT_ENTER(-1, TextCtrl::OnTextEnter)
END_EVENT_TABLE()
