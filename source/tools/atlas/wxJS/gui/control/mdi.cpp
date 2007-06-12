#include "precompiled.h"

#ifndef WX_PRECOMP
    #include <wx/wx.h>
#endif

#include "../../common/main.h"
#include "../errors.h"

#include "../misc/size.h"
#include "../misc/point.h"
#include "window.h"
#include "mdi.h"
#include "frame.h"
#include "toolbar.h"

using namespace wxjs::gui;

wxToolBar* MDIParentFrame::OnCreateToolBar(long style, 
                                           wxWindowID id,
                                           const wxString& name)
{
  ToolBar *tbar = new ToolBar();
  tbar->Create(this, id, wxDefaultPosition, wxDefaultSize, style, name);
  return tbar;
}

/***
 * <module>gui</module>
 * <file>control/mdiparent</file>
 * <class name="wxMDIParentFrame" prototype="@wxFrame">
 *  An MDI (Multiple Document Interface) parent frame is a window which can 
 *  contain MDI child frames in its own 'desktop'. It is a convenient way to 
 *  avoid window clutter, and is used in many popular Windows applications, 
 *  such as Microsoft Word(TM).
 * </class>
 */
WXJS_INIT_CLASS(MDIParentFrame, "wxMDIParentFrame", 3)

bool MDIParentFrame::AddProperty(wxMDIParentFrame *p, 
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

bool MDIParentFrame::DeleteProperty(wxMDIParentFrame *p, 
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
 *   <arg name="Parent" type="@wxWindow">The parent window</arg>
 *   <arg name="Id" type="Integer">The window ID</arg>
 *   <arg name="Title" type="String">The title of the window</arg>
 *   <arg name="Pos" type="@wxPoint" default="wxDefaultPosition" />
 *   <arg name="Size" type="@wxSize" default="wxDefaultSize" />
 *   <arg name="Style" type="Integer" 
 *        default="wxDEFAULT_FRAME_STYLE | wxVSCROLL | wxHSCROLL" />
 *  </function>
 *  <desc>
 *   Creates a new MDI parent frame.
 *  </desc>
 * </ctor>
 */
wxMDIParentFrame* MDIParentFrame::Construct(JSContext* cx,
                                            JSObject* obj,
                                            uintN argc, 
                                            jsval *argv,
                                            bool WXUNUSED(constructing))
{
  MDIParentFrame *p = new MDIParentFrame();
  SetPrivate(cx, obj, p);
  
  if ( argc > 0 )
  {
    jsval rval;
    if ( ! create(cx, obj, argc, argv, &rval) )
      return NULL;
  }
  return p;
}

/***
 * <properties>
 *  <property name="activeChild" type="@wxMDIChildFrame" readonly="Y">
 *   The active MDI child, if there is one.
 *  </property>
 * </properties>
 */
WXJS_BEGIN_PROPERTY_MAP(MDIParentFrame)
  WXJS_READONLY_PROPERTY(P_ACTIVE_CHILD, "activeChild")
WXJS_END_PROPERTY_MAP()

bool MDIParentFrame::GetProperty(wxMDIParentFrame *p, 
                                 JSContext *cx, 
                                 JSObject *obj, 
                                 int id, 
                                 jsval *vp)
{
  switch(id)
  {
  case P_ACTIVE_CHILD:
    {
      wxMDIChildFrame *child = p->GetActiveChild();
      if ( child != NULL )
      {
        JavaScriptClientData *data 
             = dynamic_cast<JavaScriptClientData*>(child->GetClientObject());
        if ( data )
        {
          *vp = OBJECT_TO_JSVAL(data->GetObject());
        }
      }
    }
  }
  return true;
}

WXJS_BEGIN_METHOD_MAP(MDIParentFrame)
  WXJS_METHOD("create", create, 3)
  WXJS_METHOD("activateNext", activateNext, 0)
  WXJS_METHOD("activatePrevious", activatePrevious, 0)
  WXJS_METHOD("arrangeIcons", arrangeIcons, 0)
  WXJS_METHOD("cascade", cascade, 0)
WXJS_END_METHOD_MAP()

/***
 * <method name="create">
 *  <function returns="Boolean">
 *   <arg name="Parent" type="@wxWindow">The parent window</arg>
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
JSBool MDIParentFrame::create(JSContext* cx, 
                              JSObject* obj, 
                              uintN argc,
                              jsval* argv, 
                              jsval* rval)
{
  wxMDIParentFrame *p = GetPrivate(cx, obj);
  *rval = JSVAL_FALSE;
  
  int style = wxDEFAULT_FRAME_STYLE | wxVSCROLL | wxHSCROLL;
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
      pos = Point::GetPrivate(cx, argv[3]);
      if ( pos == NULL )
      {
        JS_ReportError(cx, WXJS_INVALID_ARG_TYPE, 4, "wxPoint");
        return JS_FALSE;
      }
    default:
      {
        wxString title;
        wxWindow *parent = NULL;
        int id = -1;
        
        FromJS(cx, argv[2], title);
        
        if ( ! FromJS(cx, argv[1], id) )
        {
          JS_ReportError(cx, WXJS_INVALID_ARG_TYPE, 2, "wxPoint");
          return JS_FALSE;
        }
        
        if ( ! JSVAL_IS_NULL(argv[0]) )
        {
          parent = Window::GetPrivate(cx, argv[0]);
          if ( parent == NULL )
          {
            JS_ReportError(cx, WXJS_INVALID_ARG_TYPE, 1, "wxWindow");
            return JS_FALSE;
          }
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
 * <method name="activeNext">
 *  <function />
 *  <desc>
 *   Activates the MDI child following the currently active one.
 *  </desc>
 * </method>
 */
JSBool MDIParentFrame::activateNext(JSContext* cx, 
                                    JSObject* obj, 
                                    uintN WXUNUSED(argc),
                                    jsval* WXUNUSED(argv), 
                                    jsval* WXUNUSED(rval))
{
  wxMDIParentFrame *p = GetPrivate(cx, obj);
  
  p->ActivateNext();
  return JS_TRUE;
}    

/***
 * <method name="activePrevious">
 *  <function />
 *  <desc>
 *   Activates the MDI child preceding the currently active one.
 *  </desc>
 * </method>
 */
JSBool MDIParentFrame::activatePrevious(JSContext* cx, 
                                        JSObject* obj, 
                                        uintN WXUNUSED(argc),
                                        jsval* WXUNUSED(argv), 
                                        jsval* WXUNUSED(rval))
{
  wxMDIParentFrame *p = GetPrivate(cx, obj);
  
  p->ActivatePrevious();
  return JS_TRUE;
}    

/***
 * <method name="arrangeIcons">
 *  <function />
 *  <desc>
 *   Arranges any iconized (minimized) MDI child windows.
 *  </desc>
 * </method>
 */
JSBool MDIParentFrame::arrangeIcons(JSContext* cx, 
                                    JSObject* obj, 
                                    uintN WXUNUSED(argc),
                                    jsval* WXUNUSED(argv), 
                                    jsval* WXUNUSED(rval))
{
  wxMDIParentFrame *p = GetPrivate(cx, obj);
  
  p->ArrangeIcons();
  return JS_TRUE;
}    

/***
 * <method name="cascade">
 *  <function />
 *  <desc>
 *   Arranges the MDI child windows in a cascade.
 *  </desc>
 * </method>
 */
JSBool MDIParentFrame::cascade(JSContext* cx, 
                               JSObject* obj, 
                               uintN WXUNUSED(argc),
                               jsval* WXUNUSED(argv), 
                               jsval* WXUNUSED(rval))
{
  wxMDIParentFrame *p = GetPrivate(cx, obj);
  
  p->Cascade();
  return JS_TRUE;
}    

/***
 * <method name="cascade">
 *  <function>
 *   <arg name="Orientation" type="@wxOrientation" 
 *    default="wxOrientation.HORIZONTAL" />
 *  </function>
 *  <desc>
 *   Tiles the MDI child windows either horizontally or vertically.
 *   Currently only implemented for MSW, does nothing under the other platforms.
 *  </desc>
 * </method>
 */
JSBool MDIParentFrame::tile(JSContext* cx, 
                            JSObject* obj, 
                            uintN argc,
                            jsval* argv, 
                            jsval* WXUNUSED(rval))
{
  wxMDIParentFrame *p = GetPrivate(cx, obj);
  
  int orient = wxHORIZONTAL;
  if ( argc > 0 )
  {
    if ( ! FromJS(cx, argv[0], orient) )
    {
      return JS_TRUE;
    }
  }
  
  p->Tile((wxOrientation) orient);
  return JS_TRUE;
}    
