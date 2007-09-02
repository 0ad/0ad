#include "precompiled.h"

/*
 * wxJavaScript - filedlg.cpp
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
 * $Id: filedlg.cpp 810 2007-07-13 20:07:05Z fbraem $
 */

#include <wx/wx.h>

#include "../../common/main.h"
#include "../../ext/wxjs_ext.h"

#include "filedlg.h"
#include "window.h"

#include "../errors.h"

using namespace wxjs;
using namespace wxjs::gui;

/***
 * <file>control/filedlg</file>
 * <module>gui</module>
 * <class name="wxFileDialog" prototype="@wxDialog">
 *	A dialog for saving or opening a file. The following shows a save dialog:
 *  <pre><code class="whjs">
 *   var dlg = new wxFileDialog(frame, "Save a file");
 *   dlg.style = wxFileDialog.SAVE;
 *   dlg.showModal();
 *  </code></pre>
 * </class>
 */
WXJS_INIT_CLASS(FileDialog, "wxFileDialog", 1)

/***
 * <properties>
 *	<property name="directory" type="String">
 *	 Get/Set the default directory
 *  </property>
 *  <property name="filename" type="String">
 *	 Get/Set the default filename
 *  </property>
 *	<property name="filenames" type="Array" readonly="Y">
 *	 Get an array of the selected file names
 *  </property>
 *	<property name="filterIndex" type="Integer">
 *	 Get/Set the filter index (wildcards)
 *  </property>
 *	<property name="message" type="String">
 *	 Get/Set the message of the dialog
 *  </property>
 *	<property name="path" type="String">
 *	 Get/Set the full path of the selected file
 *  </property>
 *	<property name="paths" type="Array" readonly="Y">
 *	 Gets the full path of all selected files
 *  </property>
 *	<property name="wildcard" type="String">
 *	 Gets/Sets the wildcard such as "*.*" or 
 *	 "BMP files (*.bmp)|*.bmp|GIF files (*.gif)|*.gif". 
 *  </property>
 * </properties>
 */
WXJS_BEGIN_PROPERTY_MAP(FileDialog)
  WXJS_PROPERTY(P_DIRECTORY, "directory")
  WXJS_PROPERTY(P_FILENAME, "filename")
  WXJS_READONLY_PROPERTY(P_FILENAMES, "filenames")
  WXJS_PROPERTY(P_FILTER_INDEX, "filterIndex")
  WXJS_PROPERTY(P_MESSAGE, "message")
  WXJS_PROPERTY(P_PATH, "path")
  WXJS_READONLY_PROPERTY(P_PATHS, "paths")
  WXJS_PROPERTY(P_WILDCARD, "wildcard")
WXJS_END_PROPERTY_MAP()

bool FileDialog::GetProperty(wxFileDialog *p,
                             JSContext *cx,
                             JSObject* WXUNUSED(obj),
                             int id,
                             jsval *vp)
{
	switch(id) 
	{
	case P_DIRECTORY:
		*vp = ToJS(cx, p->GetDirectory());
		break;
	case P_FILENAME:
		*vp = ToJS(cx, p->GetFilename());
		break;
	case P_FILENAMES:
        {
            wxArrayString filenames;
            p->GetFilenames(filenames);
            *vp = ToJS(cx, filenames);
			break;
		}
	case P_FILTER_INDEX:
		*vp = ToJS(cx, p->GetFilterIndex());
		break;
	case P_MESSAGE:
		*vp = ToJS(cx, p->GetMessage());
		break;
	case P_PATH:
		*vp = ToJS(cx, p->GetPath());
		break;
	case P_PATHS:
		{
			wxArrayString paths;
			p->GetPaths(paths);
            *vp = ToJS(cx, paths);
			break;
		}
	case P_WILDCARD:
		*vp = ToJS(cx, p->GetWildcard());
		break;
	}
	return true;
}

bool FileDialog::SetProperty(wxFileDialog *p,
                             JSContext *cx,
                             JSObject* WXUNUSED(obj),
                             int id,
                             jsval *vp)
{
	switch (id) 
	{
	case P_DIRECTORY:
		{
			wxString dir;
			FromJS(cx, *vp, dir);
			p->SetDirectory(dir);
			break;
		}
	case P_FILENAME:
		{
			wxString f;
			FromJS(cx, *vp, f);
			p->SetFilename(f);
			break;
		}
	case P_FILTER_INDEX:
		{
			int idx;
			if ( FromJS(cx, *vp, idx) )
				p->SetFilterIndex(idx);
			break;
		}
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
	case P_WILDCARD:
		{
			wxString wildcard;
			FromJS(cx, *vp, wildcard);
			p->SetWildcard(wildcard);
			break;
		}
	}
	return true;
}

/***
 * <constants>
 *	<type name="Style">
 *	 <constant name="OPEN" />
 *	 <constant name="SAVE" />
 *	 <constant name="OVERWRITE_PROMPT" />
 *	 <constant name="MULTIPLE" />
 *  </type>
 * </constants>
 */
WXJS_BEGIN_CONSTANT_MAP(FileDialog)
  WXJS_CONSTANT(wx, OPEN)
  WXJS_CONSTANT(wx, SAVE)
  WXJS_CONSTANT(wx, OVERWRITE_PROMPT)
  WXJS_CONSTANT(wx, MULTIPLE)
WXJS_END_CONSTANT_MAP()

/***
 * <ctor>
 *	<function>
 *	 <arg name="Parent" type="@wxWindow">
 *	  The parent of wxFileDialog.
 *   </arg>
 *	 <arg name="Message" type="String" default="'Choose a file'">
 *	  The title of the dialog
 *   </arg>
 *	 <arg name="DefaultDir" type="String" default="''">
 *	  The default directory
 *   </arg>
 * 	 <arg name="DefaultFile" type="String" default="''">
 *	  The default file
 *   </arg>
 *	 <arg name="WildCard" type="String" default="'*.*'">
 *	  A wildcard, such as "*.*"
 *    or "BMP files (*.bmp)|*.bmp|GIF files (*.gif)|*.gif".
 *   </arg>
 *	 <arg name="Style" type="Integer" default="0">
 *	  The style
 *   </arg>
 *	 <arg name="Position" type="@wxPoint" default="wxDefaultPosition"> 
 *	  The position of the dialog.
 *   </arg>
 *  </function>
 *	<desc>
 *   Constructs a new wxFileDialog object
 *  </desc>
 * </ctor>
 */
wxFileDialog* FileDialog::Construct(JSContext *cx,
                                    JSObject *obj,
                                    uintN argc, 
                                    jsval *argv,
                                    bool WXUNUSED(constructing))
{
	if ( argc > 7 )
        argc = 7;

    const wxPoint *pt = &wxDefaultPosition;
    int style = 0;
    wxString message = wxFileSelectorPromptStr;
    wxString wildcard = wxFileSelectorDefaultWildcardStr;
    wxString defaultFile = wxEmptyString;
    wxString defaultDir = wxEmptyString;

    switch(argc)
    {
    case 7:
      pt = wxjs::ext::GetPoint(cx, argv[6]);
		if ( pt == NULL )
        {
          JS_ReportError(cx, WXJS_INVALID_ARG_TYPE, 7, "wxPoint");
          return JS_FALSE;
        }
        // Fall through
    case 6:
        if ( ! FromJS(cx, argv[5], style) )
        {
          JS_ReportError(cx, WXJS_INVALID_ARG_TYPE, 6, "Integer");
          return JS_FALSE;
        }
    case 5:
        FromJS(cx, argv[4], wildcard);
        // Fall through
    case 4:
        FromJS(cx, argv[3], defaultFile);
        // Fall through
    case 3:
        FromJS(cx, argv[2], defaultDir);
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
        return new wxFileDialog(parent, message, defaultDir, defaultFile,
                                wildcard, style, *pt);
    }
	return NULL;
}

void FileDialog::Destruct(JSContext* WXUNUSED(cx), wxFileDialog *p)
{
	p->Destroy();
}
