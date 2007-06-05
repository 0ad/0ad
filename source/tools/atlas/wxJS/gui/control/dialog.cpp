#include "precompiled.h"

/*
 * wxJavaScript - dialog.cpp
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
 * $Id: dialog.cpp 598 2007-03-07 20:13:28Z fbraem $
 */
// dialog.cpp

#ifndef WX_PRECOMP
	#include <wx/wx.h>
#endif

#include "../../common/main.h"

#include "../event/jsevent.h"
#include "../event/evthand.h"
#include "../event/jsevent.h"
#include "../event/close.h"

#include "dialog.h"
#include "window.h"

#include "../misc/point.h"
#include "../misc/size.h"

using namespace wxjs;
using namespace wxjs::gui;

Dialog::Dialog(JSContext *cx, JSObject *obj)
   :   wxDialog()
	 , Object(obj, cx)
{
	PushEventHandler(new EventHandler(this));
}

Dialog::~Dialog()
{
	PopEventHandler(true);
}

/***
 * <file>control/dialog</file>
 * <module>gui</module>
 * <class name="wxDialog" prototype="wxTopLevelWindow">
 *	A dialog box is a window with a title bar and sometimes a system menu, 
 *	which can be moved around the screen. It can contain controls and other windows.
 *  <br /><br />
 *  The following sample shows a simple dialog:
 *  <pre><code class="whjs">
 *  wxTheApp.onInit = function()
 *  {
 *    dlg = new wxDialog(null, -1, "test");
 *  
 *    dlg.button = new wxButton(dlg, 1, "Ok");
 *  
 *    dlg.button.onClicked = function()
 *    {
 *  	  endModal(1);
 *    }
 *    
 *    dlg.showModal();
 *    
 *    // Return false, will end the main loop
 *    return false;
 *  }
 *  
 *  wxTheApp.mainLoop();
 *  </code></pre>
 * </class>
 */
WXJS_INIT_CLASS(Dialog, "wxDialog", 3)

/***
 * <properties>
 *	<property name="returnCode" type="Integer" readonly="Y">
 *	 The returncode of the modal dialog
 *  </property>
 *  <property name="modal" type="Boolean" readonly="Y">
 *	 Returns true when the dialog is a modal dialog
 *  </property>
 * </properties>
 */
WXJS_BEGIN_PROPERTY_MAP(Dialog)
  WXJS_READONLY_PROPERTY(P_RETURN_CODE, "returnCode")
  WXJS_READONLY_PROPERTY(P_MODAL, "modal")
WXJS_END_PROPERTY_MAP()

bool Dialog::GetProperty(wxDialog *p, JSContext *cx, JSObject *obj, int id, jsval *vp)
{
	switch (id)
	{
	case P_RETURN_CODE:
		*vp = ToJS(cx, p->GetReturnCode());
		break;
	case P_MODAL:
		*vp = ToJS(cx, p->IsModal());
		break;
	}
	return true;
}

/***
 * <constants>
 *	<type name="Style">
 *	 <constant name="DIALOG_MODAL" />
 *	 <constant name="CAPTION" />
 *	 <constant name="DEFAULT_DIALOG_STYLE" />
 *	 <constant name="RESIZE_BORDER" />
 *	 <constant name="SYSTEM_MENU" />
 *	 <constant name="THICK_FRAME" />
 *	 <constant name="STAY_ON_TOP" />
 *	 <constant name="NO_3D" />
 *	 <constant name="DIALOG_EX_CONTEXTHELP" />
 *  </type>
 * </constants>
 */
WXJS_BEGIN_CONSTANT_MAP(Dialog)
  // Style constants
  WXJS_CONSTANT(wx, DIALOG_MODAL)
  WXJS_CONSTANT(wx, CAPTION)
  WXJS_CONSTANT(wx, DEFAULT_DIALOG_STYLE)
  WXJS_CONSTANT(wx, RESIZE_BORDER)
  WXJS_CONSTANT(wx, SYSTEM_MENU)
  WXJS_CONSTANT(wx, THICK_FRAME)
  WXJS_CONSTANT(wx, STAY_ON_TOP)
  WXJS_CONSTANT(wx, NO_3D)
  WXJS_CONSTANT(wx, DIALOG_EX_CONTEXTHELP)
WXJS_END_CONSTANT_MAP()

/***
 * <ctor>
 *	<function>
 *	 <arg name="Parent" type="@wxWindow">
 *	  The parent of the dialog. null is Allowed.
 *   </arg>
 *	 <arg name="Id" type="Integer">
 *    The window identifier
 *   </arg>
 *   <arg name="title" type="String">
 *    The title of the dialog
 *   </arg>
 *   <arg name="Position" type="@wxPoint" default="wxDefaultPosition">
 *    The position of the dialog.
 *   </arg>
 *   <arg name="Size" type="@wxSize" default="wxDefaultSize">
 *    The size of the dialog
 *   </arg>
 *   <arg name="Style" type="Integer" default="wxDialog.DEFAULT_DIALOG_STYLE">
 *    The window style
 *   </arg>
 *  </function>
 *  <desc>
 *   Creates a dialog
 *  </desc>
 * </ctor>
 */
wxDialog* Dialog::Construct(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, bool constructing)
{
	if ( argc > 6 )
        argc = 6;

	int style = wxDEFAULT_DIALOG_STYLE;
    const wxPoint *pt = &wxDefaultPosition;
    const wxSize *size = &wxDefaultSize;

    switch(argc)
    {
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
    default:
        wxString title;
        FromJS(cx, argv[2], title);

        int id;
        if ( ! FromJS(cx, argv[1], id) )
            break;

        wxWindow *parent = Window::GetPrivate(cx, argv[0]);
		if ( parent != NULL )
        {
            Object *wxjsParent = dynamic_cast<Object *>(parent);
            JS_SetParent(cx, obj, wxjsParent->GetObject());
        }

	    Dialog *p = new Dialog(cx, obj);

	    if ( parent == NULL )
		    style |= wxDIALOG_NO_PARENT;

	    p->Create(parent, id, title, *pt, *size, style); 
        return p;
    }

    return NULL;
}

void Dialog::Destruct(JSContext *cx, wxDialog *p)
{
//    p->Destroy();
}

WXJS_BEGIN_METHOD_MAP(Dialog)
  WXJS_METHOD("endModal", end_modal, 1)
  WXJS_METHOD("showModal", show_modal, 0)
WXJS_END_METHOD_MAP()

/***
 * <method name="endModal">
 *	<function returns="Integer">
 *   <arg name="ReturnCode" type="Integer">
 *    The value to be returned from @wxDialog#showModal
 *   </arg>
 *  </function>
 *  <desc>
 *   Ends a modal dialog.
 *  </desc>
 * </method>
 */
JSBool Dialog::end_modal(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
    wxDialog *p = Dialog::GetPrivate(cx, obj);
	if ( p == NULL )
		return JS_FALSE;

    int code;
    if ( FromJS(cx, argv[0], code) )
    {
    	p->EndModal(code);
        return JS_TRUE;
    }

	return JS_FALSE;
}

/***
 * <method name="showModal">
 *	<function returns="Integer" />
 *	<desc>
 *   Shows a modal dialog. The value returned is the return code set by @wxDialog#endModal.
 *  </desc>
 * </method>
 */
JSBool Dialog::show_modal(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
    wxDialog *p = Dialog::GetPrivate(cx, obj);
	if ( p == NULL )
		return JS_FALSE;

	*rval = ToJS(cx, p->ShowModal());

	return JS_TRUE;
}

/***
 * <events>
 *	<event name="onClose">
 *	 Called when the dialog is closed. The type of the argument that your 
 *	 handler receives is @wxCloseEvent.
 *  </event>
 * </events>
 */
void Dialog::OnClose(wxCloseEvent &event)
{
	bool destroy = true;

	if ( PrivCloseEvent::Fire<CloseEvent>(this, event, "onClose") )
    {
		destroy = ! event.GetVeto();
	}

	// When the close event is not handled by JavaScript,
	// wxJS destroys the dialog.
	if ( destroy )
	{
		Destroy();
	}

}

BEGIN_EVENT_TABLE(Dialog, wxDialog)
	EVT_CLOSE(Dialog::OnClose)
END_EVENT_TABLE()
