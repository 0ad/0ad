#include "precompiled.h"

/*
 * wxJavaScript - dirdlg.cpp
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
 * $Id: dirdlg.cpp 708 2007-05-14 15:30:45Z fbraem $
 */

#ifndef WX_PRECOMP
	#include <wx/wx.h>
#endif

#include "../../common/main.h"

#include "dirdlg.h"
#include "window.h"
#include "../misc/point.h"

#include "../errors.h"

using namespace wxjs;
using namespace wxjs::gui;

/***
 * <file>control/dirdlg</file>
 * <module>gui</module>
 * <class name="wxDirDialog" prototype="wxDialog">
 *	This class represents the directory chooser dialog.
 * </class>
 */
WXJS_INIT_CLASS(DirDialog, "wxDirDialog", 1)

/***
 * <properties>
 *	<property name="message" type="String">
 *	 Get/Set the message of the dialog
 *  </property>
 *  <property name="path" type="String">
 *	 Get/Set the full path of the selected file
 *  </property>
 * </properties>
 */
WXJS_BEGIN_PROPERTY_MAP(DirDialog)
  WXJS_PROPERTY(P_MESSAGE, "message")
  WXJS_PROPERTY(P_PATH, "path")
WXJS_END_PROPERTY_MAP()

bool DirDialog::GetProperty(wxDirDialog *p,
                            JSContext *cx,
                            JSObject* WXUNUSED(obj),
                            int id,
                            jsval *vp)
{
	switch (id) 
	{
	case P_MESSAGE:
		*vp = ToJS(cx, p->GetMessage());
		break;
	case P_PATH:
		*vp = ToJS(cx, p->GetPath());
		break;
	}
	return true;
}

bool DirDialog::SetProperty(wxDirDialog *p,
                            JSContext *cx,
                            JSObject* WXUNUSED(obj),
                            int id,
                            jsval *vp)
{
	switch (id) 
	{
	case P_MESSAGE:
		{
			wxString msg;
			FromJS(cx, *vp, msg);
			p->SetMessage(msg);
			break;
		}
	case P_PATH:
		{
			wxString path;
			FromJS(cx, *vp, path);
			p->SetPath(path);
			break;
		}
	}
	return true;
}

/***
 * <ctor>
 *	<function>
 *   <arg name="Parent" type="@wxWindow">
 *    The parent of wxDirDialog
 *   </arg>
 *   <arg name="Message" type="String" default="'Choose a directory'">
 *    The title of the dialog
 *   </arg>
 *   <arg name="DefaultPath" type="String" default="">
 *    The default directory
 *   </arg>
 *   <arg name="Style" type="Integer" default="0">
 *    Unused
 *   </arg>
 *   <arg name="Position" type="@wxPoint" default="wxDefaultPosition">
 *	  The position of the dialog.
 *   </arg>
 *  </function>
 *	<desc>
 *   Constructs a new wxDirDialog object
 *  </desc>
 * </ctor>
 */
wxDirDialog* DirDialog::Construct(JSContext *cx,
                                  JSObject *obj,
                                  uintN argc,
                                  jsval *argv,
                                  bool WXUNUSED(constructing))
{
    if ( argc > 5 )
        argc = 5;

    const wxPoint *pt = &wxDefaultPosition;
    int style = 0;
    wxString defaultPath = wxEmptyString;
    wxString message = wxDirSelectorPromptStr;

    switch(argc)
    {
    case 5:
		pt = Point::GetPrivate(cx, argv[4]);
		if ( pt == NULL )
        {
          JS_ReportError(cx, WXJS_INVALID_ARG_TYPE, 5, "wxPoint");
          return JS_FALSE;
        }
        // Fall through
    case 4:
        if ( ! FromJS(cx, argv[3], style) )
        {
          JS_ReportError(cx, WXJS_INVALID_ARG_TYPE, 4, "Integer");
          return JS_FALSE;
        }
        // Fall through
    case 3:
        FromJS(cx, argv[2], defaultPath);
        // Fall through
    case 2:
        FromJS(cx, argv[1], message);
        // Fall through
    default:
        wxWindow *parent = Window::GetPrivate(cx, argv[0]);
		if ( parent != NULL )
        {
          JavaScriptClientData *clntParent =
                dynamic_cast<JavaScriptClientData *>(parent->GetClientObject());
          if ( clntParent == NULL )
          {
              JS_ReportError(cx, WXJS_NO_PARENT_ERROR, GetClass()->name);
              return JS_FALSE;
          }
          JS_SetParent(cx, obj, clntParent->GetObject());
        }
    
        return new wxDirDialog(parent, message, defaultPath, style, *pt);
    }

    return NULL;
}

void DirDialog::Destruct(JSContext* WXUNUSED(cx), wxDirDialog *p)
{
    p->Destroy();
}
