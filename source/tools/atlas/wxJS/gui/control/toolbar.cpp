#include "precompiled.h"

/*
 * wxJavaScript - toolbar.cpp
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
 * $Id: toolbar.cpp 810 2007-07-13 20:07:05Z fbraem $
 */

#include <wx/wxprec.h>
#ifndef WX_PRECOMP
	#include <wx/wx.h>
#endif

#include <wx/toolbar.h>

#include "../../common/main.h"
#include "../../ext/wxjs_ext.h"

#include "../event/jsevent.h"
#include "../event/command.h"

#include "window.h"
#include "control.h"
#include "toolbar.h"
#include "tbartool.h"

#include "../misc/bitmap.h"
#include "../misc/size.h"
#include "../errors.h"

using namespace wxjs;
using namespace wxjs::gui;

bool ToolBar::DoDeleteTool(size_t pos, wxToolBarToolBase *tool)
{
  if ( ! tool->IsControl() )
  {
    ToolBarToolData *data 
      = dynamic_cast<ToolBarToolData*>(tool->GetClientData());
    if ( data )
    {
      delete data;
    }
  }
  return wxToolBar::DoDeleteTool(pos, tool);
}

/***
 * <file>control/toolbar</file>
 * <module>gui</module>
 * <class name="wxToolBar" prototype="@wxControl">
 *  A toolbar. <br /><br />
 *  Event handling is done as follows:
 *   When the tool has a function set as @wxToolBarTool#onTool, then that
 *   function will be executed.
 *   When a menu exists with the same id of the selected tool, the menu action is executed.
 *   When there is no menu, the action associated with the toolbar is executed.
 *   See @wxToolBar#actions and @wxMenu#actions
 * <br /><br />
 *   See also @wxFrame#createToolBar
 * </class>
 */
WXJS_INIT_CLASS(ToolBar, "wxToolBar", 2)

/***
 * <properties>
 *  <property name="actions" type="Array">
 *   An array containing the function callbacks. A function will be called when a tool is selected.
 *   Use the id as index of the array.
 *  </property>
 *  <property name="margins" type="@wxSize">
 *   Gets/Sets the left/right and top/bottom margins, which are also used for inter-toolspacing.
 *  </property>
 *  <property name="toolBitmapSize" type="@wxSize">
 *   Gets/Sets the size of bitmap that the toolbar expects to have.
 *   The default bitmap size is 16 by 15 pixels.
 *  </property>
 *  <property name="toolPacking" type="Integer">
 *   Gets/Sets the value used for spacing tools. The default value is 1.
 *   The packing is used for spacing in the vertical direction if the toolbar is horizontal, 
 *   and for spacing in the horizontal direction if the toolbar is vertical.
 *  </property>
 *  <property name="toolSeparation" type="Integer">
 *   Gets/Sets the separator size. The default value is 5.
 *  </property>
 *  <property name="toolSize" type="@wxSize" readonly="Y">
 *   Gets the size of a whole button, which is usually larger than a tool bitmap 
 *   because of added 3D effects.
 *  </property>
 * </properties>
 */
WXJS_BEGIN_PROPERTY_MAP(ToolBar)
    WXJS_PROPERTY(P_TOOL_SIZE, "margins")
    WXJS_READONLY_PROPERTY(P_TOOL_SIZE, "toolSize")
    WXJS_PROPERTY(P_TOOL_BITMAP_SIZE, "toolBitmapSize")
    WXJS_PROPERTY(P_TOOL_PACKING, "toolPacking")
    WXJS_PROPERTY(P_TOOL_SEPARATION, "toolSeparation")
WXJS_END_PROPERTY_MAP()

bool ToolBar::GetProperty(wxToolBar* p, 
                          JSContext* cx, 
                          JSObject* WXUNUSED(obj),
                          int id,
                          jsval* vp)
{
    switch (id)
    {
    case P_TOOL_SIZE:
        {
			const wxSize &size = p->GetToolSize();
            *vp = Size::CreateObject(cx, new wxSize(size));
            break;
        }
    case P_TOOL_BITMAP_SIZE:
		{
			const wxSize &size = p->GetToolBitmapSize();
			*vp = Size::CreateObject(cx, new wxSize(size));
			break;
		}
    case P_MARGINS:
        *vp = Size::CreateObject(cx, new wxSize(p->GetMargins()));
        break;
    case P_TOOL_PACKING:
        *vp = ToJS(cx, p->GetToolPacking());
        break;
    case P_TOOL_SEPARATION:
        *vp = ToJS(cx, p->GetToolSeparation());
        break;
    }
    return true;
}

bool ToolBar::SetProperty(wxToolBar* p,
                          JSContext* cx,
                          JSObject* WXUNUSED(obj),
                          int id,
                          jsval *vp)
{
    switch (id)
    {
    case P_TOOL_BITMAP_SIZE:
        {
            wxSize *size = Size::GetPrivate(cx, *vp);
            if ( size != NULL )
                p->SetToolBitmapSize(*size);
            break;
        }
    case P_MARGINS:
        {
            wxSize *size = Size::GetPrivate(cx, *vp);
            if ( size != NULL )
                p->SetMargins(size->GetWidth(), size->GetHeight());
            break;
        }
    case P_TOOL_PACKING:
        {
            int pack;
            if ( FromJS(cx, *vp, pack) )
                p->SetToolPacking(pack);
            break;
        }
    case P_TOOL_SEPARATION:
        {
            int size;
            if ( FromJS(cx, *vp, size) )
                p->SetToolSeparation(size);
            break;
        }
    }
    return true;
}

/***
 * <constants>
 *  <type name="Styles">
 *   <constant name="FLAT">
 *    Gives the toolbar a flat look ('coolbar' or 'flatbar' style). 
 *    Windows 95 and GTK 1.2 only.
 *   </constant>
 *   <constant name="DOCKABLE">
 *    Makes the toolbar floatable and dockable. GTK only.
 *   </constant>
 *   <constant name="HORIZONTAL">Specifies horizontal layout.</constant>
 *   <constant name="VERTICAL">
 *    Specifies vertical layout (not available for the GTK and Windows 
 *    95 toolbar).  
 *   </constant>
 *   <constant name="3DBUTTONS">
 *    Gives wxToolBarSimple a mild 3D look to its buttons.  
 *   </constant>
 *   <constant name="TEXT">
 *    Show the text in the toolbar buttons; by default only icons are shown.
 *   </constant>  
 *   <constant name="NOICONS">
 *    Specifies no icons in the toolbar buttons; by default they are shown.
 *   </constant>
 *   <constant name="NODIVIDER">
 *    Specifies no divider above the toolbar; by default it is shown. 
 *    Windows only.
 *   </constant>
 *   <constant name="NOALIGN">
 *    Specifies no alignment with the parent window. Windows only.
 *   </constant>
 *  </type>
 * </constants>
 */
WXJS_BEGIN_CONSTANT_MAP(ToolBar)
    WXJS_CONSTANT(wxTB_, FLAT)
    WXJS_CONSTANT(wxTB_, DOCKABLE)
    WXJS_CONSTANT(wxTB_, HORIZONTAL)
    WXJS_CONSTANT(wxTB_, VERTICAL)
    WXJS_CONSTANT(wxTB_, 3DBUTTONS)
    WXJS_CONSTANT(wxTB_, TEXT)
    WXJS_CONSTANT(wxTB_, NOICONS)
    WXJS_CONSTANT(wxTB_, NODIVIDER)
    WXJS_CONSTANT(wxTB_, NOALIGN)
WXJS_END_CONSTANT_MAP()

/***
 * <ctor>
 *  <function />
 *  <function>
 *   <arg name="Parent" type="@wxWindow">
 *    The parent of wxToolBar.
 *   </arg>
 *   <arg name="Id" type="Integer">
 *    An window identifier. Use -1 when you don't need it.
 *   </arg>
 *   <arg name="Position" type="@wxPoint" default="wxDefaultPosition">
 *    The position of the ToolBar control on the given parent.
 *   </arg>
 *   <arg name="Size" type="@wxSize">
 *    The size of the ToolBar control.
 *   </arg>
 *   <arg name="Style" type="Integer" 
 *        default="wxToolBar.HORIZONTAL | wxToolBar.NO_BORDER">
 *    The wxToolBar style.
 *   </arg>
 *  </function>
 *  <desc>
 *   Constructs a new wxToolBar object.
 *  </desc>
 * </ctor>
 */
wxToolBar* ToolBar::Construct(JSContext *cx,
                              JSObject *obj,
                              uintN argc,
                              jsval *argv,
                              bool WXUNUSED(constructing))
{
  ToolBar *p = new ToolBar();
  SetPrivate(cx, obj, p);

  if ( argc > 0 )
  {
    jsval rval;
    if ( ! create(cx, obj, argc, argv, &rval) )
      return NULL;
  }
  return p;
}

WXJS_BEGIN_METHOD_MAP(ToolBar)
  WXJS_METHOD("create", create, 2)
  WXJS_METHOD("addControl", addControl, 1)
  WXJS_METHOD("addSeparator", addSeparator, 0)
  WXJS_METHOD("addTool", addTool, 3)
  WXJS_METHOD("addCheckTool", addCheckTool, 4)
  WXJS_METHOD("addRadioTool", addRadioTool, 4)
  WXJS_METHOD("deleteTool", deleteTool, 1)
  WXJS_METHOD("deleteToolByPos", deleteToolByPos, 1)
  WXJS_METHOD("enableTool", enableTool, 2)
  WXJS_METHOD("findControl", findControl, 1)
  WXJS_METHOD("getToolEnabled", getToolEnabled, 1)
  WXJS_METHOD("getToolLongHelp", getToolLongHelp, 1)
  WXJS_METHOD("getToolShortHelp", getToolShortHelp, 1)
  WXJS_METHOD("getToolState", getToolState, 1)
  WXJS_METHOD("insertControl", insertControl, 2)
  WXJS_METHOD("insertSeparator", insertSeparator, 1)
  WXJS_METHOD("insertTool", insertTool, 3)
  WXJS_METHOD("realize", realize, 0)
  WXJS_METHOD("setToolLongHelp", setToolLongHelp, 2)
  WXJS_METHOD("setToolShortHelp", setToolShortHelp, 2)
  WXJS_METHOD("toggleTool", toggleTool, 2)
WXJS_END_METHOD_MAP()

/***
 * <method name="create">
 *  <function returns="Boolean">
 *   <arg name="Parent" type="@wxWindow">
 *    The parent of wxToolBar.
 *   </arg>
 *   <arg name="Id" type="Integer">
 *    An window identifier. Use -1 when you don't need it.
 *   </arg>
 *   <arg name="Position" type="@wxPoint" default="wxDefaultPosition">
 *    The position of the ToolBar control on the given parent.
 *   </arg>
 *   <arg name="Size" type="@wxSize">
 *    The size of the ToolBar control.
 *   </arg>
 *   <arg name="Style" type="Integer" 
 *        default="wxToolBar.HORIZONTAL | wxToolBar.NO_BORDER">
 *    The wxToolBar style.
 *   </arg>
 *  </function>
 *  <desc>
 *   Constructs a new wxToolBar object.
 *  </desc>
 * </method>
 */
JSBool ToolBar::create(JSContext *cx,
                       JSObject *obj,
                       uintN argc,
                       jsval *argv,
                       jsval *rval)
{
  wxToolBar *p = GetPrivate(cx, obj);
  *rval = JSVAL_FALSE;

  if ( argc > 5 )
      argc = 5;

  int style = wxTB_HORIZONTAL | wxNO_BORDER;
  const wxPoint *pt = &wxDefaultPosition;
  const wxSize *size = &wxDefaultSize;

  switch(argc)
  {
  case 5:
      if ( ! FromJS(cx, argv[4], style) )
      {
        JS_ReportError(cx, WXJS_INVALID_ARG_TYPE, 5, "Integer");
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
      JS_ReportError(cx, WXJS_INVALID_ARG_TYPE, 4, "wxPoint");
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

    if ( p->Create(parent, id, *pt, *size, style) )
    {
      *rval = JSVAL_TRUE;
      p->SetClientObject(new JavaScriptClientData(cx, obj, true, false));
      JSObject *objActionArray = JS_NewArrayObject(cx, 0, NULL);
      JS_DefineProperty(cx, obj, "actions", OBJECT_TO_JSVAL(objActionArray), 
                        NULL, NULL, JSPROP_ENUMERATE |JSPROP_PERMANENT);

      p->Connect(wxID_ANY, wxID_ANY, wxEVT_COMMAND_TOOL_CLICKED, 
                 wxCommandEventHandler(ToolEventHandler::OnTool));
    }
  }

  return JS_TRUE;
}

/***
 * <method name="addControl">
 *  <function>
 *   <arg name="Control" type="@wxControl" />
 *  </function> 
 *  <desc>
 *   Adds a control to the toolbar.
 *  </desc>
 * </method>
 */
JSBool ToolBar::addControl(JSContext *cx,
                           JSObject *obj,
                           uintN WXUNUSED(argc),
                           jsval *argv,
                           jsval* WXUNUSED(rval))
{
    wxToolBar *p = ToolBar::GetPrivate(cx, obj);
    if ( p == NULL )
        return JS_FALSE;

    wxControl *control = Control::GetPrivate(cx, argv[0]);
    if ( control != NULL )
    {
        p->AddControl(control);
        return JS_TRUE;
    }

    return JS_FALSE;
}

/***
 * <method name="addSeparator">
 *  <function />
 *  <desc>
 *   Adds a separator for spacing groups of tools.
 *  </desc>
 * </method>
 */
JSBool ToolBar::addSeparator(JSContext *cx,
                             JSObject *obj,
                             uintN WXUNUSED(argc),
                             jsval* WXUNUSED(argv),
                             jsval* WXUNUSED(rval))
{
    wxToolBar *p = GetPrivate(cx, obj);
    if ( p == NULL )
        return JS_FALSE;

    p->AddSeparator();
    return JS_TRUE;
}

/***
 * <method name="addTool">
 *  <function returns="@wxToolBarTool">
 *   <arg name="Id" type="Integer">
 *    An integer that may be used to identify this tool
 *   </arg>
 *   <arg name="Label" type="String" />
 *   <arg name="Bitmap1" type="wxBitmap">
 *    The primary tool bitmap for toggle and button tools.
 *   </arg>
 *   <arg name="ShortHelp" type="String" default="">
 *    Used for tooltips
 *   </arg>
 *   <arg name="Kind" type="Integer" default="wxItemKind.NORMAL">
 *    May be wxItemKind.NORMAL for a normal button (default),
 *    wxItemKind.CHECK for a checkable tool (such tool stays pressed after it had been toggled)
 *    or wxItemKind.RADIO for a checkable tool which makes part of a radio group of tools each 
 *    of which is automatically unchecked whenever another button in the group is checked.
 *   </arg>
 *  </function>
 *  <function>
 *   <arg name="Id" type="Integer">
 *    An integer that may be used to identify this tool
 *   </arg>
 *   <arg name="Label" type="String" />
 *   <arg name="Bitmap1" type="wxBitmap">
 *    The primary tool bitmap for toggle and button tools.
 *   </arg>
 *   <arg name="Bitmap2" type="wxBitmap">
 *    The second bitmap specifies the on-state bitmap for a toggle tool
 *   </arg>
 *   <arg name="Kind" type="Integer" default="wxItemKind.NORMAL">
 *    May be wxItemKind.NORMAL for a normal button (default),
 *    wxItemKind.CHECK for a checkable tool (such tool stays pressed after it had been toggled)
 *    or wxItemKind.RADIO for a checkable tool which makes part of a radio group of tools each 
 *    of which is automatically unchecked whenever another button in the group is checked.
 *   </arg>
 *   <arg name="ShortHelp" type="String" default="">
 *    Used for tooltips
 *   </arg>
 *   <arg name="LongHelp" type="String" default="">
 *    String shown in the statusbar.
 *   </arg>
 *  </function>
 *  <desc>
 *   Adds a tool to the toolbar.
 *  </desc>
 * </method>
 */
JSBool ToolBar::addTool(JSContext* cx,
                        JSObject* obj,
                        uintN argc,
                        jsval* argv,
                        jsval* rval)
{
    wxToolBar *p = GetPrivate(cx, obj);
    if ( p == NULL )
        return JS_FALSE;

    int id;
    if ( ! FromJS(cx, argv[0], id) )
        return JS_FALSE;

    wxString label;
    FromJS(cx, argv[1], label);
    wxBitmap *bmp1 = Bitmap::GetPrivate(cx, argv[2]);
    if ( bmp1 == NULL )
    {
      JS_ReportError(cx, WXJS_INVALID_ARG_TYPE, 2, "wxBitmap");
      return JS_FALSE;
    }

    wxToolBarToolBase *tool = NULL;
    if ( argc > 3 )
    {
        wxString shortHelp = wxEmptyString;
        int kind = wxITEM_NORMAL;
        wxBitmap *bmp2 = Bitmap::GetPrivate(cx, argv[3], false);
        if ( bmp2 == NULL )
        {
            if ( argc > 5 )
              argc = 5;
            switch(argc)
            {
            case 5:
                if ( ! FromJS(cx, argv[4], kind) )
                {
                  JS_ReportError(cx, WXJS_INVALID_ARG_TYPE, 2, "Integer");
                  return JS_FALSE;
                }
            case 4:
                FromJS(cx, argv[3], shortHelp);
            }
            tool = p->AddTool(id, label, *bmp1, shortHelp, (wxItemKind) kind);
        }
        else
        {
            wxString longHelp = wxEmptyString;
          
            if ( argc > 7 )
              argc = 7;
            switch(argc)
            {
            case 7:
                FromJS(cx, argv[6], longHelp);
            case 6:
                FromJS(cx, argv[5], shortHelp);
            case 5:
                if ( ! FromJS(cx, argv[4], kind) )
                {
                  JS_ReportError(cx, WXJS_INVALID_ARG_TYPE, 5, "Integer");
                  return JS_FALSE;
                }
            }
            tool = p->AddTool(id, label, *bmp1, *bmp2, (wxItemKind) kind,
                              shortHelp, longHelp);
        }
    }
    else
        tool = p->AddTool(id, label, *bmp1);

    if ( tool != NULL )
    {
      *rval = ToolBarToolBase::CreateObject(cx, tool);
      tool->SetClientData(new ToolBarToolData(cx, JSVAL_TO_OBJECT(*rval)));
    }

    return JS_TRUE;
}

/***
 * <method name="addCheckTool">
 *  <function returns="@wxToolBarTool">
 *   <arg name="Id" type="Integer">
 *    An integer that may be used to identify this tool
 *   </arg>
 *   <arg name="Label" type="String" />
 *   <arg name="Bitmap1" type="wxBitmap">
 *    The primary tool bitmap for toggle and button tools.
 *   </arg>
 *   <arg name="Bitmap2" type="wxBitmap">
 *    The second bitmap specifies the on-state bitmap for a toggle tool
 *   </arg>
 *   <arg name="ShortHelp" type="String" default="">
 *    Used for tooltips
 *   </arg>
 *   <arg name="LongHelp" type="String" default="">
 *    String shown in the statusbar.
 *   </arg>
 *  </function>
 *  <desc>
 *   Adds a new check (or toggle) tool to the toolbar.
 *  </desc>
 * </method>
 */
JSBool ToolBar::addCheckTool(JSContext *cx,
                             JSObject *obj,
                             uintN argc,
                             jsval *argv,
                             jsval *rval)
{
    wxToolBar *p = GetPrivate(cx, obj);
    if ( p == NULL )
        return JS_FALSE;

    wxString longHelp = wxEmptyString;
    wxString shortHelp = wxEmptyString;

    if ( argc > 6 )
      argc = 6;

    switch(argc)
    {
    case 6:
        FromJS(cx, argv[5], longHelp);
    case 5:
        FromJS(cx, argv[4], shortHelp);
    default:
        {
            int id;
            if ( ! FromJS(cx, argv[0], id) )
            {
              JS_ReportError(cx, WXJS_INVALID_ARG_TYPE, 1, "Integer");
              return JS_FALSE;
            }

            wxString label;
            FromJS(cx, argv[1], label);
            wxBitmap *bmp1 = Bitmap::GetPrivate(cx, argv[2]);
            if ( bmp1 == NULL )
            {
              JS_ReportError(cx, WXJS_INVALID_ARG_TYPE, 3, "wxBitmap");
              return JS_FALSE;
            }
            wxBitmap *bmp2 = Bitmap::GetPrivate(cx, argv[3]);
            if ( bmp2 == NULL )
            {
              JS_ReportError(cx, WXJS_INVALID_ARG_TYPE, 4, "wxBitmap");
              return JS_FALSE;
            }
            wxToolBarToolBase *tool = p->AddCheckTool(id, label, 
                                                      *bmp1, *bmp2, 
                                                      shortHelp, longHelp);
            if ( tool != NULL )
            {
              *rval = ToolBarToolBase::CreateObject(cx, tool);
              tool->SetClientData(new ToolBarToolData(cx, JSVAL_TO_OBJECT(*rval)));
            }
        }
    }
    return JS_TRUE;
}

/***
 * <method name="addRadioTool">
 *  <function>
 *   <arg name="Id" type="Integer">
 *    An integer that may be used to identify this tool
 *   </arg>
 *   <arg name="Label" type="String" />
 *   <arg name="Bitmap1" type="wxBitmap">
 *    The primary tool bitmap for toggle and button tools.
 *   </arg>
 *   <arg name="Bitmap2" type="wxBitmap">
 *    The second bitmap specifies the on-state bitmap for a toggle tool
 *   </arg>
 *   <arg name="ShortHelp" type="String" default="">
 *    Used for tooltips
 *   </arg>
 *   <arg name="LongHelp" type="String" default="">
 *    String shown in the statusbar.
 *   </arg>
 *  </function>
 *  <desc>
 *   Adds a new radio tool to the toolbar. Consecutive radio tools form a radio group 
 *   such that exactly one button in the group is pressed at any moment, in other words 
 *   whenever a button in the group is pressed the previously pressed button is 
 *   automatically released. You should avoid having the radio groups of only one 
 *   element as it would be impossible for the user to use such button.
 *   <br /><br />
 *   By default, the first button in the radio group is initially pressed, the others are not.
 *  </desc>
 * </method>
 */
JSBool ToolBar::addRadioTool(JSContext *cx,
                             JSObject *obj,
                             uintN argc,
                             jsval *argv,
                             jsval *rval)
{
    wxToolBar *p = GetPrivate(cx, obj);
    if ( p == NULL )
        return JS_FALSE;

    wxString longHelp = wxEmptyString;
    wxString shortHelp = wxEmptyString;

    if ( argc > 6 )
      argc = 6;
    switch(argc)
    {
    case 6:
        FromJS(cx, argv[5], longHelp);
    case 5:
        FromJS(cx, argv[4], shortHelp);
    default:
        {
            int id;
            if ( ! FromJS(cx, argv[0], id) )
            {
              JS_ReportError(cx, WXJS_INVALID_ARG_TYPE, 1, "Integer");
              return JS_FALSE;
            }

            wxString label;
            FromJS(cx, argv[1], label);
            wxBitmap *bmp1 = Bitmap::GetPrivate(cx, argv[2]);
            if ( bmp1 == NULL )
            {
              JS_ReportError(cx, WXJS_INVALID_ARG_TYPE, 3, "wxBitmap");
              return JS_FALSE;
            }
            wxBitmap *bmp2 = Bitmap::GetPrivate(cx, argv[3]);
            if ( bmp2 == NULL )
            {
              JS_ReportError(cx, WXJS_INVALID_ARG_TYPE, 4, "wxBitmap");
              return JS_FALSE;
            }
            wxToolBarToolBase *tool = p->AddRadioTool(id, label, 
                                                      *bmp1, *bmp2, 
                                                      shortHelp, longHelp);

            if ( tool != NULL )
            {
              *rval = ToolBarToolBase::CreateObject(cx, tool);
              tool->SetClientData(new ToolBarToolData(cx, JSVAL_TO_OBJECT(*rval)));
            }
        }
    }
    return JS_TRUE;
}

/***
 * <method name="deleteTool">
 *  <function returns="Boolean">
 *   <arg name="Id" type="Integer" />
 *  </function>
 *  <desc>
 *   Deletes the tool with the given id.
 *  </desc>
 * </method>
 */
JSBool ToolBar::deleteTool(JSContext *cx,
                           JSObject *obj,
                           uintN WXUNUSED(argc),
                           jsval *argv,
                           jsval *rval)
{
    wxToolBar *p = GetPrivate(cx, obj);
    if ( p == NULL )
        return JS_FALSE;

    int id;
    if ( FromJS(cx, argv[0], id) )
    {
        *rval = ToJS(cx, p->DeleteTool(id));
        return JS_TRUE;
    }

    return JS_FALSE;
}

/***
 * <method name="deleteToolByPos">
 *  <function returns="Boolean">
 *   <arg name="Pos" type="Integer" />
 *  </function>
 *  <desc>
 *   Deletes the tool on the given position.
 *  </desc>
 * </method>
 */
JSBool ToolBar::deleteToolByPos(JSContext *cx,
                                JSObject *obj,
                                uintN WXUNUSED(argc),
                                jsval *argv,
                                jsval *rval)
{
    wxToolBar *p = GetPrivate(cx, obj);
    if ( p == NULL )
        return JS_FALSE;

    int id;
    if ( FromJS(cx, argv[0], id) )
    {
        *rval = ToJS(cx, p->DeleteToolByPos(id));
        return JS_TRUE;
    }

    return JS_FALSE;
}

/***
 * <method name="enableTool">
 *  <function returns="Boolean">
 *   <arg name="Id" type="Integer">
 *    The tool identifier
 *   </arg>
 *   <arg name="Enable" type="Boolean">
 *    Enable/disable the tool
 *   </arg>
 *  </function>
 *  <desc>
 *   Enables/disables the tool with the given id.
 *  </desc>
 * </method>
 */
JSBool ToolBar::enableTool(JSContext *cx,
                           JSObject *obj,
                           uintN WXUNUSED(argc),
                           jsval *argv,
                           jsval *WXUNUSED(rval))
{
    wxToolBar *p = GetPrivate(cx, obj);
    if ( p == NULL )
        return JS_FALSE;

    int id;
    bool enable;
    if (    FromJS(cx, argv[0], id)
         && FromJS(cx, argv[1], enable) )
    {
        p->EnableTool(id, enable);
        return JS_TRUE;
    }

    return JS_FALSE;
}

/***
 * <method name="findControl">
 *  <function returns="@wxControl">
 *   <arg name="Id" type="Integer">
 *    The tool identifier
 *   </arg>
 *  </function>
 *  <desc>
 *   Returns the control identified by Id or null when not found.
 *  </desc>
 * </method>
 */
JSBool ToolBar::findControl(JSContext *cx,
                            JSObject *obj,
                            uintN WXUNUSED(argc),
                            jsval *argv,
                            jsval *rval)
{
    wxToolBar *p = GetPrivate(cx, obj);
    if ( p == NULL )
        return JS_FALSE;

    int id;
    if ( FromJS(cx, argv[0], id) )
    {
        wxControl *ctrl = p->FindControl(id);
        if ( ctrl == NULL )
        {
            *rval = JSVAL_VOID;
        }
        else
        {
          JavaScriptClientData *data 
            = dynamic_cast<JavaScriptClientData*>(ctrl->GetClientObject());
          if ( data )
          {
            *rval = OBJECT_TO_JSVAL(data->GetObject());
          }
        }
        return JS_TRUE;
    }

    return JS_FALSE;
}

/***
 * <method name="getToolEnabled">
 *  <function returns="Boolean">
 *   <arg name="Id" type="Integer">
 *    The tool identifier
 *   </arg>
 *  </function>
 *  <desc>
 *   Returns true when the tool with the given id is enabled.
 *   See @wxToolBar#enableTool
 *  </desc>
 * </method>
 */
JSBool ToolBar::getToolEnabled(JSContext *cx,
                               JSObject *obj,
                               uintN WXUNUSED(argc),
                               jsval *argv,
                               jsval *rval)
{
    wxToolBar *p = GetPrivate(cx, obj);
    if ( p == NULL )
        return JS_FALSE;

    int id;
    if ( FromJS(cx, argv[0], id) )
    {
        *rval = ToJS(cx, p->GetToolEnabled(id));
        return JS_TRUE;
    }

    return JS_FALSE;
}

/***
 * <method name="getToolLongHelp">
 *  <function returns="String">
 *   <arg name="Id" type="Integer">
 *    The tool identifier
 *   </arg>
 *  </function>
 *  <desc>
 *   Returns the long help from the tool with the given id.
 *  </desc>
 * </method>
 */
JSBool ToolBar::getToolLongHelp(JSContext *cx,
                                JSObject *obj,
                                uintN WXUNUSED(argc),
                                jsval *argv,
                                jsval *rval)
{
    wxToolBar *p = GetPrivate(cx, obj);
    if ( p == NULL )
        return JS_FALSE;

    int id;
    if ( FromJS(cx, argv[0], id) )
    {
        *rval = ToJS(cx, p->GetToolLongHelp(id));
        return JS_TRUE;
    }

    return JS_FALSE;
}

/***
 * <method name="getToolShortHelp">
 *  <function returns="String">
 *   <arg name="Id" type="Integer">
 *    The tool identifier
 *   </arg>
 *  </function>
 *  <desc>
 *   Returns the short help from the tool with the given id.
 *  </desc>
 * </method>
 */
JSBool ToolBar::getToolShortHelp(JSContext *cx,
                                 JSObject *obj,
                                 uintN WXUNUSED(argc),
                                 jsval *argv,
                                 jsval *rval)
{
    wxToolBar *p = GetPrivate(cx, obj);
    if ( p == NULL )
        return JS_FALSE;

    int id;
    if ( FromJS(cx, argv[0], id) )
    {
        *rval = ToJS(cx, p->GetToolShortHelp(id));
        return JS_TRUE;
    }

    return JS_FALSE;
}

/***
 * <method name="getToolState">
 *  <function>
 *   <arg name="Id" type="Integer">
 *    The tool identifier
 *   </arg>
 *  </function>
 *  <desc>
 *   Returns true when the tool with the given id is toggled on.
 *  </desc>
 * </method>
 */
JSBool ToolBar::getToolState(JSContext *cx,
                             JSObject *obj,
                             uintN WXUNUSED(argc),
                             jsval *argv,
                             jsval *rval)
{
    wxToolBar *p = GetPrivate(cx, obj);
    if ( p == NULL )
        return JS_FALSE;

    int id;
    if ( FromJS(cx, argv[0], id) )
    {
        *rval = ToJS(cx, p->GetToolState(id));
        return JS_TRUE;
    }

    return JS_FALSE;
}

/***
 * <method name="insertControl">
 *  <function>
 *   <arg name="Pos" type="Integer" />
 *   <arg name="Control" type="@wxControl" />
 *  </function>
 *  <desc>
 *   Inserts the control into the toolbar at the given position.
 *   You must call @wxToolBar#realize for the change to take place.
 *  </desc>
 * </method>
 */
JSBool ToolBar::insertControl(JSContext *cx,
                              JSObject *obj,
                              uintN WXUNUSED(argc),
                              jsval *argv,
                              jsval* WXUNUSED(rval))
{
    wxToolBar *p = GetPrivate(cx, obj);
    if ( p == NULL )
        return JS_FALSE;

    int pos;
    if ( FromJS(cx, argv[0], pos ) )
    {
        wxControl *control = Control::GetPrivate(cx, argv[1]);
        if ( control != NULL )
        {
            p->InsertControl(pos, control);
            return JS_TRUE;
        }
    }
    return JS_FALSE;
}

/***
 * <method name="insertSeparator">
 *  <function>
 *   <arg name="Pos" type="Integer" />
 *  </function>
 *  <desc>
 *   Inserts a separator into the toolbar at the given position.
 *   You must call @wxToolBar#realize for the change to take place.
 *  </desc>
 * </method>
 */
JSBool ToolBar::insertSeparator(JSContext *cx,
                                JSObject *obj,
                                uintN WXUNUSED(argc), 
                                jsval *argv,
                                jsval* WXUNUSED(rval))
{
    wxToolBar *p = GetPrivate(cx, obj);
    if ( p == NULL )
        return JS_FALSE;

    int pos;
    if ( FromJS(cx, argv[0], pos ) )
    {
        p->InsertSeparator(pos);
        return JS_TRUE;
    }
    return JS_FALSE;
}

/***
 * <method name="insertTool">
 *  <function returns="@wxToolBarTool">
 *   <arg name="Pos" type="Integer" />
 *   <arg name="Id" type="Integer">
 *    An integer that may be used to identify this tool
 *   </arg>
 *   <arg name="Label" type="String" />
 *   <arg name="Bitmap1" type="wxBitmap">
 *    The primary tool bitmap for toggle and button tools.
 *   </arg>
 *   <arg name="Bitmap2" type="wxBitmap">
 *    The second bitmap specifies the on-state bitmap for a toggle tool
 *   </arg>
 *   <arg name="Toogle" type="Boolean" default="false">
 *    Is the tool a toggle tool?
 *   </arg>
 *   <arg name="ShortHelp" type="String" default="">
 *    Used for tooltips
 *   </arg>
 *   <arg name="LongHelp" type="String" default="">
 *    String shown in the statusbar.
 *   </arg>
 *  </function>
 *  <desc>
 *  Inserts the tool to the toolbar at the given position.
 *  </desc>
 * </method>
 */
JSBool ToolBar::insertTool(JSContext *cx,
                           JSObject *obj,
                           uintN argc,
                           jsval *argv,
                           jsval* rval)
{
    wxToolBar *p = GetPrivate(cx, obj);
    if ( p == NULL )
        return JS_FALSE;

    wxString longHelp = wxEmptyString;
    wxString shortHelp = wxEmptyString;
    bool toggle = false;
    wxBitmap *bmp2 = NULL;

    switch(argc)
    {
    case 7:
        FromJS(cx, argv[7], longHelp);
        // Fall through
    case 6:
        FromJS(cx, argv[6], shortHelp);
        // Fall through
    case 5:
        if ( ! FromJS(cx, argv[4], toggle) )
        {
          JS_ReportError(cx, WXJS_INVALID_ARG_TYPE, 5, "Boolean");
          return JS_FALSE;
        }
        // Fall through
    case 4:
        bmp2 = Bitmap::GetPrivate(cx, argv[3]);
        if ( bmp2 == NULL )
        {
          JS_ReportError(cx, WXJS_INVALID_ARG_TYPE, 4, "wxBitmap");
          return JS_FALSE;
        }
        // Fall through
    default:
        wxBitmap *bmp1 = Bitmap::GetPrivate(cx, argv[2]);
        if ( bmp1 == NULL )
        {
          JS_ReportError(cx, WXJS_INVALID_ARG_TYPE, 3, "wxBitmap");
          return JS_FALSE;
        }
        int id;
        int pos;
        if ( ! FromJS(cx, argv[1], id) )
        {
          JS_ReportError(cx, WXJS_INVALID_ARG_TYPE, 2, "Integer");
          return JS_FALSE;
        }
        
        if ( ! FromJS(cx, argv[0], pos) )
        {
          JS_ReportError(cx, WXJS_INVALID_ARG_TYPE, 1, "Integer");
          return JS_FALSE;
        }

        wxToolBarToolBase *tool = p->InsertTool(pos, id, *bmp1, 
                                                bmp2 == NULL ? wxNullBitmap : *bmp2,
                                                toggle, NULL, shortHelp, longHelp);
        if ( tool != NULL )
        {
          *rval = ToolBarToolBase::CreateObject(cx, tool);
          tool->SetClientData(new ToolBarToolData(cx, JSVAL_TO_OBJECT(*rval)));
        }
    }
    return JS_FALSE;
}

/***
 * <method name="realize">
 *  <function returns="Boolean" />
 *  <desc>
 *   Call this method after you have added tools.
 *  </desc>
 * </method>
 */
JSBool ToolBar::realize(JSContext *cx,
                        JSObject *obj,
                        uintN WXUNUSED(argc),
                        jsval* WXUNUSED(argv),
                        jsval *rval)
{
    wxToolBar *p = GetPrivate(cx, obj);
    if ( p == NULL )
        return JS_FALSE;

    *rval = ToJS(cx, p->Realize());
    return JS_TRUE;
}

/***
 * <method name="setToolLongHelp">
 *  <function>
 *   <arg name="Id" type="Integer">
 *    The tool identifier
 *   </arg>
 *   <arg name="Help" type="String" />
 *  </function>
 *  <desc>
 *   Sets the long help from the tool with the given id.
 *  </desc>
 * </method>
 */
JSBool ToolBar::setToolLongHelp(JSContext *cx,
                                JSObject *obj,
                                uintN WXUNUSED(argc),
                                jsval* argv,
                                jsval* WXUNUSED(rval))
{
    wxToolBar *p = GetPrivate(cx, obj);
    if ( p == NULL )
        return JS_FALSE;

    int id;
    if ( FromJS(cx, argv[0], id) )
    {
        wxString longHelp;
        FromJS(cx, argv[1], longHelp);
        p->SetToolLongHelp(id, longHelp);
        return JS_TRUE;
    }

    return JS_FALSE;
}

/***
 * <method name="setToolShortHelp">
 *  <function>
 *   <arg name="Id" type="Integer">
 *    The tool identifier
 *   </arg>
 *   <arg name="Help" type="String" />
 *  </function>
 *  <desc>
 *   Sets the short help for the tool with the given id. The short help will be used
 *   to show a tooltip.
 *  </desc>
 * </method>
 */
JSBool ToolBar::setToolShortHelp(JSContext *cx,
                                 JSObject *obj,
                                 uintN WXUNUSED(argc),
                                 jsval *argv,
                                 jsval* WXUNUSED(rval))
{
    wxToolBar *p = GetPrivate(cx, obj);
    if ( p == NULL )
        return JS_FALSE;

    int id;
    if ( FromJS(cx, argv[0], id) )
    {
        wxString longHelp;
        FromJS(cx, argv[1], longHelp);
        p->SetToolShortHelp(id, longHelp);
        return JS_TRUE;
    }

    return JS_FALSE;
}

/***
 * <method name="toggleTool">
 *  <function>
 *   <arg name="Id" type="Integer">
 *    The tool identifier
 *   </arg>
 *   <arg name="Toggle" type="Boolean" />
 *  </function>
 *  <desc>
 *   Toggles a tool on or off.
 *   See @wxToolBar#getToolState.
 *  </desc>
 * </method>
 */
JSBool ToolBar::toggleTool(JSContext *cx,
                           JSObject *obj,
                           uintN WXUNUSED(argc),
                           jsval *argv,
                           jsval* WXUNUSED(rval))
{
    wxToolBar *p = GetPrivate(cx, obj);
    if ( p == NULL )
        return JS_FALSE;

    int id;
    bool toggle;
    if (    FromJS(cx, argv[0], id) 
         && FromJS(cx, argv[1], toggle) )
    {
        p->ToggleTool(id, toggle);
        return JS_TRUE;
    }

    return JS_FALSE;
}

void ToolEventHandler::OnTool(wxCommandEvent &event)
{
  ToolBar *p = dynamic_cast<ToolBar *>(event.GetEventObject());
  wxToolBarToolBase *tool = p->FindById(event.GetId());
  if ( tool == NULL )
    return;

  if ( tool->IsControl() )
    return;

  ToolBarToolData *data 
    = dynamic_cast<ToolBarToolData*>(tool->GetClientData());

  JSContext *cx = data->GetContext();

  jsval jsAction;
  if ( JS_GetProperty(cx, data->GetObject(), "onTool", &jsAction) == JS_TRUE )
  {
    if ( jsAction == JSVAL_VOID )
    {
      // No action found, skip the event and when a menu item with the same
      // id exists, it will try to run the associated action of that item
      event.Skip();
    }
    else
    {
      JSFunction *fnAction = JS_ValueToFunction(cx, jsAction);
      if ( fnAction != NULL )
      {
        PrivCommandEvent *wxjsEvent = new PrivCommandEvent(event);
        jsval argv[] = { CommandEvent::CreateObject(cx, wxjsEvent) };

        jsval rval;
        JSBool result = JS_CallFunction(cx, data->GetObject(), 
                                        fnAction, 1, argv, &rval);
        if ( result == JS_FALSE )
        {
          JS_ReportPendingException(cx); 
        }                
      }
    }
  }
}
