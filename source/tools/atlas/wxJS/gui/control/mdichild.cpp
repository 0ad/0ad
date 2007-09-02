#include "precompiled.h"

/*
 * wxJavaScript - mdichild.cpp
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
 * $Id$
 */

// 3rd party includes
#include <wx/wx.h>

// wxJS includes
#include "../../common/main.h"
#include "../../ext/wxjs_ext.h"

// wxJS_gui includes
#include "../errors.h"
#include "../misc/size.h"
#include "mdi.h"
#include "mdichild.h"
#include "frame.h"
#include "window.h"
#include "toolbar.h"

using namespace wxjs::gui;

wxToolBar* MDIChildFrame::OnCreateToolBar(long style, 
                                          wxWindowID id,
                                          const wxString& name)
{
  ToolBar *tbar = new ToolBar();
  tbar->Create(this, id, wxDefaultPosition, wxDefaultSize, style, name);
  return tbar;
}

/***
 * <module>gui</module>
 * <file>control/mdichild</file>
 * <class name="wxMDIChildFrame" prototype="@wxFrame">
 *  An MDI child frame is a frame that can only exist in a @wxMDIParentFrame 
 * </class>
 */
WXJS_INIT_CLASS(MDIChildFrame, "wxMDIChildFrame", 3)

bool MDIChildFrame::AddProperty(wxMDIChildFrame *p, 
                                JSContext* WXUNUSED(cx), 
                                JSObject* WXUNUSED(obj), 
                                const wxString &prop, 
                                jsval* WXUNUSED(vp))
{
    if ( WindowEventHandler::ConnectEvent(p, prop, true) )
        return true;
    
    FrameEventHandler::ConnectEvent(p, prop, true);

    return true;
}


bool MDIChildFrame::DeleteProperty(wxMDIChildFrame *p, 
                            JSContext* WXUNUSED(cx), 
                            JSObject* WXUNUSED(obj), 
                            const wxString &prop)
{
  if ( WindowEventHandler::ConnectEvent(p, prop, false) )
    return true;
  
  FrameEventHandler::ConnectEvent(p, prop, false);
  return true;
}

/***
 * <ctor>
 *  <function />
 *  <function>
 *   <arg name="Parent" type="@wxMDIParentFrame">
 *    The parent frame. This can't be null.
 *   </arg>
 *   <arg name="Id" type="Integer">The window ID</arg>
 *   <arg name="Title" type="String">The title of the window</arg>
 *   <arg name="Pos" type="@wxPoint" default="wxDefaultPosition" />
 *   <arg name="Size" type="@wxSize" default="wxDefaultSize" />
 *   <arg name="Style" type="Integer" 
 *        default="wxDEFAULT_FRAME_STYLE" />
 *  </function>
 *  <desc>
 *   Creates a new MDI child frame.
 *  </desc>
 * </ctor>
 */
wxMDIChildFrame* MDIChildFrame::Construct(JSContext* cx,
                                          JSObject* obj,
                                          uintN argc, 
                                          jsval *argv,
                                          bool WXUNUSED(constructing))
{
  MDIChildFrame *p = new MDIChildFrame();
  SetPrivate(cx, obj, p);
  
  if ( argc > 0 )
  {
    jsval rval;
    if ( ! create(cx, obj, argc, argv, &rval) )
      return NULL;
  }
  return p;
}

WXJS_BEGIN_METHOD_MAP(MDIChildFrame)
  WXJS_METHOD("create", create, 3)
  WXJS_METHOD("activate", activate, 0)
  WXJS_METHOD("maximize", maximize, 1)
  WXJS_METHOD("restore", restore, 0)
WXJS_END_METHOD_MAP()

/***
 * <method name="create">
 *  <function returns="Boolean">
 *   <arg name="Parent" type="@wxMDIParentFrame">
 *    The parent frame. This can't be null.
 *   </arg>
 *   <arg name="Id" type="Integer">The window ID</arg>
 *   <arg name="Title" type="String">The title of the window</arg>
 *   <arg name="Pos" type="@wxPoint" default="wxDefaultPosition" />
 *   <arg name="Size" type="@wxSize" default="wxDefaultSize" />
 *   <arg name="Style" type="Integer" 
 *        default="wxDEFAULT_FRAME_STYLE | wxVSCROLL | wxHSCROLL" />
 *  </function>
 *  <desc>
 *   Sets a pixel to a particular color.
 *  </desc>
 * </method>
 */
JSBool MDIChildFrame::create(JSContext* cx, 
                             JSObject* obj, 
                             uintN argc,
                             jsval* argv, 
                             jsval* rval)
{
  wxMDIChildFrame *p = GetPrivate(cx, obj);
  *rval = JSVAL_FALSE;
  
  int style = wxDEFAULT_FRAME_STYLE;
  const wxPoint* pos = &wxDefaultPosition;
  const wxSize* size = &wxDefaultSize;
  
  if ( argc > 6 )
  {
    argc = 6;
  }
  
  switch(argc)
  {
    case 6:
      if ( ! FromJS(cx, argv[5], style) )
      {
        JS_ReportError(cx, WXJS_INVALID_ARG_TYPE, 6, "Integer");
        return JS_FALSE;
      }
    case 5:
      size = Size::GetPrivate(cx, argv[4]);
      if ( size == NULL )
      {
        JS_ReportError(cx, WXJS_INVALID_ARG_TYPE, 5, "wxSize");
        return JS_FALSE;
      }
    case 4:
      pos = wxjs::ext::GetPoint(cx, argv[3]);
      if ( pos == NULL )
      {
        JS_ReportError(cx, WXJS_INVALID_ARG_TYPE, 4, "wxPoint");
        return JS_FALSE;
      }
    default:
      {
        wxString title;
        wxMDIParentFrame *parent = NULL;
        int id = -1;
        
        FromJS(cx, argv[2], title);
        
        if ( ! FromJS(cx, argv[1], id) )
        {
          JS_ReportError(cx, WXJS_INVALID_ARG_TYPE, 2, "Integer");
          return JS_FALSE;
        }
        
        parent = MDIParentFrame::GetPrivate(cx, argv[0]);
        if (    parent == NULL 
             || parent->GetHandle() == NULL )
        {
          JS_ReportError(cx, WXJS_INVALID_ARG_TYPE, 1, "wxMDIParentFrame");
          return JS_FALSE;
        }
        
        if ( p->Create(parent, id, title, *pos, *size, style) )
        {
          *rval = JSVAL_TRUE;
          p->SetClientObject(new JavaScriptClientData(cx, obj, true, false));
          p->Connect(wxID_ANY, wxID_ANY, wxEVT_COMMAND_MENU_SELECTED, 
                     wxCommandEventHandler(FrameEventHandler::OnMenu));
        }
      }
  }
  return JS_TRUE;
}    

/***
 * <method name="activate">
 *  <function />
 *  <desc>
 *   Activates this MDI child frame
 *  </desc>
 * </method>
 */
JSBool MDIChildFrame::activate(JSContext* cx, 
                               JSObject* obj, 
                               uintN WXUNUSED(argc),
                               jsval* WXUNUSED(argv), 
                               jsval* WXUNUSED(rval))
{
  wxMDIChildFrame *p = GetPrivate(cx, obj);
  
  p->Activate();
  return JS_TRUE;
}    

/***
 * <method name="maximize">
 *  <function>
 *   <arg name="Maximize" type="Boolean" />
 *  </function>
 *  <desc>
 *   Maximizes this MDI child frame
 *  </desc>
 * </method>
 */
JSBool MDIChildFrame::maximize(JSContext* cx, 
                               JSObject* obj, 
                               uintN WXUNUSED(argc),
                               jsval* argv, 
                               jsval* WXUNUSED(rval))
{
  wxMDIChildFrame *p = GetPrivate(cx, obj);
  
  bool sw = true;
  FromJS(cx, argv[0], sw);
  p->Maximize(sw);
  
  return JS_TRUE;
}    

/***
 * <method name="restore">
 *  <function />
 *  <desc>
 *   Restores this MDI child frame
 *  </desc>
 * </method>
 */
JSBool MDIChildFrame::restore(JSContext* cx, 
                              JSObject* obj, 
                              uintN WXUNUSED(argc),
                              jsval* WXUNUSED(argv), 
                              jsval* WXUNUSED(rval))
{
  wxMDIChildFrame *p = GetPrivate(cx, obj);
  
  p->Restore();
  return JS_TRUE;
}    
