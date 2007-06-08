#include "precompiled.h"

/*
 * wxJavaScript - helpbtn.cpp
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
 * $Id: helpbtn.cpp 708 2007-05-14 15:30:45Z fbraem $
 */
#ifndef WX_PRECOMP
    #include <wx/wx.h>
#endif

#include "../../common/main.h"


#include "../event/jsevent.h"
#include "../event/command.h"

#include "../misc/size.h"
#include "../misc/point.h"
#include "helpbtn.h"
#include "button.h"
#include "window.h"
#include "../errors.h"

using namespace wxjs;
using namespace wxjs::gui;

/***
 * <file>control/helpbtn</file>
 * <module>gui</module>
 * <class name="wxContextHelpButton" prototype="@wxBitmapButton">
 *  Instances of this class may be used to add a question mark button that
 *  when pressed, puts the application into context-help mode. It does this by 
 *  creating a @wxContextHelp object which itself generates a @wxHelpEvent event
 * when the user clicks on a window.
 * </class>
 */
WXJS_INIT_CLASS(ContextHelpButton, "wxContextHelpButton", 1)

bool ContextHelpButton::AddProperty(wxContextHelpButton *p, 
                                    JSContext* WXUNUSED(cx), 
                                    JSObject* WXUNUSED(obj), 
                                    const wxString &prop, 
                                    jsval* WXUNUSED(vp))
{
  return ButtonEventHandler::ConnectEvent(p, prop, true);
}

bool ContextHelpButton::DeleteProperty(wxContextHelpButton *p, 
                                       JSContext* WXUNUSED(cx), 
                                       JSObject* WXUNUSED(obj), 
                                       const wxString &prop)
{
  return ButtonEventHandler::ConnectEvent(p, prop, false);
}

/***
 * <ctor>
 *  <function>
 *   <arg name="Parent" type="@wxWindow">
 *    The parent of wxContextHelpButton.
 *   </arg>
 *  </function>
 *  <desc>
 *   Constructs a new wxContextHelpButton object.
 *  </desc>
 * </ctor>
 */
wxContextHelpButton* ContextHelpButton::Construct(JSContext *cx,
                                                  JSObject *obj,
                                                  uintN argc,
                                                  jsval *argv,
                                                  bool WXUNUSED(constructing))
{
  if ( argc > 0 )
  {
    wxWindow *parent = Window::GetPrivate(cx, argv[0]);
    if ( parent == NULL )
    {
        JS_ReportError(cx, WXJS_NO_PARENT_ERROR, GetClass()->name);
        return NULL;
    }
    JavaScriptClientData *clntParent =
          dynamic_cast<JavaScriptClientData *>(parent->GetClientObject());
    if ( clntParent == NULL )
    {
        JS_ReportError(cx, WXJS_NO_PARENT_ERROR, GetClass()->name);
        return JS_FALSE;
    }
    JS_SetParent(cx, obj, clntParent->GetObject());

    wxContextHelpButton *p = new wxContextHelpButton(parent);
    p->SetClientObject(new JavaScriptClientData(cx, obj, true, false));
    return p;
  }
  return NULL;
}
