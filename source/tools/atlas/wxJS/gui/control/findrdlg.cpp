#include "precompiled.h"

/*
 * wxJavaScript - findrdlg.cpp
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
 * $Id: findrdlg.cpp 598 2007-03-07 20:13:28Z fbraem $
 */
// findrdlg.cpp

#ifndef WX_PRECOMP
	#include <wx/wx.h>
#endif

#include "../../common/main.h"

#include "../event/jsevent.h"
#include "../event/findr.h"

#include "findrdlg.h"
#include "finddata.h"
#include "window.h"

using namespace wxjs;
using namespace wxjs::gui;

FindReplaceDialog::FindReplaceDialog(JSContext *cx, JSObject *obj)
	: wxFindReplaceDialog()
	, Object(obj, cx)
{
}

/***
 * <file>control/findrdlg</file>
 * <module>gui</module>
 * <class name="wxFindReplaceDialog" prototype="@wxDialog">
 *  wxFindReplaceDialog is a standard modeless dialog which is used to allow the user to search 
 *  for some text (and possible replace it with something else). The actual searching is supposed
 *  to be done in the owner window which is the parent of this dialog. Note that it means that 
 *  unlike for the other standard dialogs this one must have a parent window. 
 *  Also note that there is no way to use this dialog in a modal way, it is always, 
 *  by design and implementation, modeless.
 * </class>
 */
//TODO: add a sample!
WXJS_INIT_CLASS(FindReplaceDialog, "wxFindReplaceDialog", 0)

/***
 * <properties>
 *	<property name="data" type="@wxFindReplaceData">
 *	 Get/Set the data
 *  </property>
 * </properties>
 */
WXJS_BEGIN_PROPERTY_MAP(FindReplaceDialog)
  WXJS_PROPERTY(P_DATA, "data")
WXJS_END_PROPERTY_MAP()

bool FindReplaceDialog::GetProperty(wxFindReplaceDialog *p, JSContext *cx, JSObject *obj, int id, jsval *vp)
{
	if (id == P_DATA )
        *vp = FindReplaceData::CreateObject(cx, new wxFindReplaceData(*p->GetData()));
    return true;
}

/***
 * <constants>
 *  <type name="Styles">
 *   <constant name="REPLACEDIALOG" />
 *   <constant name="NOUPDOWN" />
 *   <constant name="NOMACTHCASE" />
 *   <constant name="NOWHOLEWORD" />
 *  </type>
 * </constants>
 */
WXJS_BEGIN_CONSTANT_MAP(FindReplaceDialog)
  WXJS_CONSTANT(wxFR_, REPLACEDIALOG)
  WXJS_CONSTANT(wxFR_, NOUPDOWN)
  WXJS_CONSTANT(wxFR_, NOMATCHCASE)
  WXJS_CONSTANT(wxFR_, NOWHOLEWORD)
WXJS_END_CONSTANT_MAP()

/***
 * <ctor>
 *	<function>
 *	 <arg name="Parent" type="@wxWindow">
 *	  The parent of wxFindReplaceDialog. Can't be null.
 *   </arg>
 *   <arg name="Data" type="@wxFindReplaceData" />
 *   <arg name="Title" type="String">
 *	  The title of the dialog
 *   </arg>
 *	 <arg name="Style" type="Integer" default="0" />
 *  </function>
 *  <function />
 *  <desc>
 *	 Constructs a new wxFindReplaceDialog object
 *  </desc>
 * </ctor>
 */
wxFindReplaceDialog *FindReplaceDialog::Construct(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, bool constructing)
{
    if ( argc > 4 )
        argc = 4;

    if ( argc == 0 )
        return new FindReplaceDialog(cx, obj);

    if ( argc < 3 )
        return NULL;

    int style = 0;
    if (     argc == 4
        && ! FromJS(cx, argv[3], style) )
        return NULL;

    wxString message;
    FromJS(cx, argv[2], message);

    wxFindReplaceData *data = FindReplaceData::GetPrivate(cx, argv[1]);
    if ( data == NULL )
        return NULL;

    wxWindow *parent = Window::GetPrivate(cx, argv[0]);
    if ( parent == NULL )
        return NULL;

    Object *wxjsParent = dynamic_cast<Object *>(parent);
    JS_SetParent(cx, obj, wxjsParent->GetObject());

    FindReplaceDialog *p = new FindReplaceDialog(cx, obj);

    // Copy the data.
    p->m_data.SetFlags(data->GetFlags());
    p->m_data.SetFindString(data->GetFindString());
    p->m_data.SetReplaceString(data->GetReplaceString());

    p->Create(parent, &p->m_data, message, style);

    return p;
}

WXJS_BEGIN_METHOD_MAP(FindReplaceDialog)
  WXJS_METHOD("create", create, 4)
WXJS_END_METHOD_MAP()

/***
 * <method name="create">
 *  <function>
 *	 <arg name="Parent" type="@wxWindow">
 *	  The parent of wxFindReplaceDialog. Can't be null.
 *   </arg>
 *   <arg name="Data" type="@wxFindReplaceData" />
 *   <arg name="Title" type="String">
 *	  The title of the dialog
 *   </arg>
 *	 <arg name="Style" type="Integer" default="0" />
 *  </function>
 *  <desc>
 *   Creates a wxFindReplaceDialog.
 *  </desc>
 * </method>
 */
JSBool FindReplaceDialog::create(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
    wxFindReplaceDialog *p = FindReplaceDialog::GetPrivate(cx, obj);
	if ( p == NULL )
		return JS_FALSE;

	int style = 0;
    if ( ! FromJS(cx, argv[3], style) )
        return JS_FALSE;

	wxString title;
    FromJS(cx, argv[2], title);

    wxFindReplaceData *data = FindReplaceData::GetPrivate(cx, argv[1]);
    if ( data == NULL )
        return JS_FALSE;

    wxWindow *parent = Window::GetPrivate(cx, argv[0]);
	if ( parent != NULL )
    {
        Object *wxjsParent = dynamic_cast<Object *>(parent);
        JS_SetParent(cx, obj, wxjsParent->GetObject());
    }

	p->Create(parent, data, title, style);
    return JS_TRUE;
}

/***
 * <events>
 *  <event name="onFind">The find button was pressed. The argument passed to the function is a @wxFindDialogEvent</event>
 *  <event name="onFindNext">The find next button was pressed. The argument passed to the function is a @wxFindDialogEvent</event>
 *  <event name="onFindReplace">The replace button was pressed. The argument passed to the function is a @wxFindDialogEvent</event>
 *  <event name="onFindReplaceAll">The replace all button was pressed. The argument passed to the function is a @wxFindDialogEvent</event>
 *  <event name="onFindClose">The dialog is being destroyed. The argument passed to the function is a @wxFindDialogEvent</event>
 * </events>
 */
void FindReplaceDialog::OnFind(wxFindDialogEvent& event)
{
	PrivFindDialogEvent::Fire<FindDialogEvent>(this, event, "onFind");
}

void FindReplaceDialog::OnFindNext(wxFindDialogEvent& event)
{
	PrivFindDialogEvent::Fire<FindDialogEvent>(this, event, "onFindNext");
}

void FindReplaceDialog::OnReplace(wxFindDialogEvent& event)
{
	PrivFindDialogEvent::Fire<FindDialogEvent>(this, event, "onFindReplace");
}

void FindReplaceDialog::OnReplaceAll(wxFindDialogEvent& event)
{
	PrivFindDialogEvent::Fire<FindDialogEvent>(this, event, "onFindReplaceAll");
}

void FindReplaceDialog::OnFindClose(wxFindDialogEvent& event)
{
	PrivFindDialogEvent::Fire<FindDialogEvent>(this, event, "onFindClose");
	Destroy();
}

BEGIN_EVENT_TABLE(FindReplaceDialog, wxFindReplaceDialog)
  EVT_FIND(-1, FindReplaceDialog::OnFind)
  EVT_FIND_NEXT(-1, FindReplaceDialog::OnFindNext)
  EVT_FIND_REPLACE(-1, FindReplaceDialog::OnReplace)
  EVT_FIND_REPLACE_ALL(-1, FindReplaceDialog::OnReplaceAll)
  EVT_FIND_CLOSE(-1, FindReplaceDialog::OnFindClose)
END_EVENT_TABLE()
