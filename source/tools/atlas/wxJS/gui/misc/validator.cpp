#include "precompiled.h"

/*
 * wxJavaScript - validator.cpp
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
 * $Id: validator.cpp 784 2007-06-25 18:34:22Z fbraem $
 */
// validator.cpp

#ifndef WX_PRECOMP
    #include <wx/wx.h>
#endif

#include "../../common/main.h"
#include "../../common/jsutil.h"

#include "../control/window.h"

#include "validate.h"

using namespace wxjs;
using namespace wxjs::gui;

Validator::Validator() : wxValidator()
{
}

Validator::Validator(const Validator &copy) : wxValidator()
{
  JavaScriptClientData *data 
    = dynamic_cast<JavaScriptClientData*>(copy.GetClientObject());
  if ( data != NULL )
  {
    SetClientObject(new JavaScriptClientData(*data));
    data->Protect(false);
  }
}

bool Validator::TransferToWindow()
{
  JavaScriptClientData *data
    = dynamic_cast<JavaScriptClientData*>(GetClientObject());
  if ( data == NULL )
    return true;

  JSContext *cx = data->GetContext();
  JSObject *obj = data->GetObject();

  jsval fval;
  if ( GetFunctionProperty(cx, obj, "transferToWindow", &fval) == JS_TRUE )
  {
    jsval rval;
    if ( JS_CallFunctionValue(cx, obj, fval, 0, NULL, &rval) == JS_FALSE )
    {
      if ( JS_IsExceptionPending(cx) )
      {
          JS_ReportPendingException(cx);
      }
      return false;
    }
    else
    {
      return JSVAL_IS_BOOLEAN(rval) ? JSVAL_TO_BOOLEAN(rval) == JS_TRUE : false;
    }
  }

  return true;
}

bool Validator::TransferFromWindow()
{
  JavaScriptClientData *data
    = dynamic_cast<JavaScriptClientData*>(GetClientObject());
  if ( data == NULL )
    return true;

  JSContext *cx = data->GetContext();
  JSObject *obj = data->GetObject();

  jsval fval;
  if ( GetFunctionProperty(cx, obj, "transferFromWindow", &fval) == JS_TRUE )
  {
    jsval rval;
    if ( JS_CallFunctionValue(cx, obj, fval, 0, NULL, &rval) == JS_FALSE )
    {
      if ( JS_IsExceptionPending(cx) )
      {
        JS_ReportPendingException(cx);
      }
      return false;
    }
    else
    {
      return JSVAL_IS_BOOLEAN(rval) ? JSVAL_TO_BOOLEAN(rval) == JS_TRUE : false;
    }
  }
  return true;
}

bool Validator::Validate(wxWindow *parent)
{
  JavaScriptClientData *data
    = dynamic_cast<JavaScriptClientData*>(GetClientObject());
  if ( data == NULL )
    return true;

  JSContext *cx = data->GetContext();
  JSObject *obj = data->GetObject();

  JavaScriptClientData *parentData = NULL;
  if ( parent != NULL )
  {
    parentData = dynamic_cast<JavaScriptClientData*>(parent->GetClientObject());
  }

  jsval fval;
  if ( GetFunctionProperty(cx, obj, "validate", &fval) == JS_TRUE )
  {
    jsval argv[] = 
    { 
      parentData == NULL ? JSVAL_VOID 
                         : OBJECT_TO_JSVAL(parentData->GetObject()) 
    };

	jsval rval;
	if ( JS_CallFunctionValue(cx, obj, fval, 1, argv, &rval) == JS_FALSE )
	{
      if ( JS_IsExceptionPending(cx) )
      {
          JS_ReportPendingException(cx);
      }
	  return false;
	}
	else
    {
	  return JSVAL_IS_BOOLEAN(rval) ? JSVAL_TO_BOOLEAN(rval) == JS_TRUE : false;
    }
  }

	return false;
}

/***
 * <file>misc/validator</file>
 * <module>gui</module>
 * <class name="wxValidator">
 *  Use wxValidator to create your own validators.  
 * </class>
 */

WXJS_INIT_CLASS(Validator, "wxValidator", 0)

/***
 * <properties>
 *  <property name="bellOnError" type="Boolean" />
 *  <property name="transferFromWindow" type="Function">
 *   Assign a function to this property that transfers the content of the window
 *   to a variable. You must return true on success, false on failure. 
 *  </property>
 *  <property name="transferToWindow" type="Function">
 *   Assign a function to this property that transfers the variable to the window.
 *   You must return true on success, false on failure. 
 *  </property>
 *  <property name="validate" type="Function">
 *   Assign a function to this property that checks the content of the associated window. The function
 *   can have one argument: the parent of the associated window. This function should return false
 *   when the content is invalid, true when it is valid.
 *  </property>
 *  <property name="window" type="@wxWindow">
 *   The window associated with this validator.
 *  </property>
 * </properties>
 */
WXJS_BEGIN_PROPERTY_MAP(Validator)
  WXJS_PROPERTY(P_WINDOW, "window")
  WXJS_PROPERTY(P_BELL_ON_ERROR, "bellOnError")
WXJS_END_PROPERTY_MAP()

bool Validator::GetProperty(wxValidator *p, JSContext *cx, JSObject *obj, int id, jsval *vp)
{
  switch (id) 
  {
  case P_BELL_ON_ERROR:
    *vp = ToJS(cx, p->IsSilent());
    break;
  case P_WINDOW:
    {
      wxWindow *win = p->GetWindow();
      if ( win != NULL )
      {
        JavaScriptClientData *data
          = dynamic_cast<JavaScriptClientData*>(win->GetClientObject());
        *vp = data == NULL ? JSVAL_VOID 
                           : OBJECT_TO_JSVAL(data->GetObject());
      }
      break;
    }
  }
  return true;
}

bool Validator::SetProperty(wxValidator *p, JSContext *cx, JSObject *obj, int id, jsval *vp)
{
    switch (id) 
	{
	case P_BELL_ON_ERROR:
		{
			bool bell;
			if ( FromJS(cx, *vp, bell) )
				p->SetBellOnError(bell);
			break;
		}
	case P_WINDOW:
        {
            wxWindow *win = Window::GetPrivate(cx, *vp);
            if ( win != NULL )
				p->SetWindow(win);
		}
		break;
	}
    
	return true;
}

/**
 * <ctor>
 *  <function />
 *  <desc>
 *   Constructs a new wxValidator object
 *  </desc>
 * </ctor>
 */
wxValidator *Validator::Construct(JSContext *cx,
                                  JSObject *obj,
                                  uintN argc, 
                                  jsval *argv,
                                  bool constructing)
{
  Validator *v = new Validator();
  v->SetClientObject(new JavaScriptClientData(cx, obj, false, true));
  return v;
}
