#include "precompiled.h"

/*
 * wxJavaScript - slider.cpp
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
 * $Id: slider.cpp 810 2007-07-13 20:07:05Z fbraem $
 */
// slider.cpp

#ifndef WX_PRECOMP
    #include <wx/wx.h>
#endif

#include "../../common/main.h"
#include "../../ext/wxjs_ext.h"

#include "../event/jsevent.h"
#include "../event/command.h"
#include "../event/scroll.h"

#include "../misc/size.h"
#include "../misc/validate.h"

#include "slider.h"
#include "window.h"
#include "../errors.h"

using namespace wxjs;
using namespace wxjs::gui;

/***
 * <file>control/slider</file>
 * <module>gui</module>
 * <class name="wxSlider" prototype="@wxControl">
 *  A slider is a control with a handle which can be pulled back and forth
 *  to change the value.
 *  In Windows versions below Windows 95, a scrollbar is used to simulate the
 *  slider. In Windows 95, the track bar control is used.
 *  See also @wxScrollEvent.
 * </class>
 */
WXJS_INIT_CLASS(Slider, "wxSlider", 5)

void Slider::InitClass(JSContext* WXUNUSED(cx),
                       JSObject* WXUNUSED(obj), 
                       JSObject* WXUNUSED(proto))
{
  SliderEventHandler::InitConnectEventMap();
}

/***
 * <properties>
 *  <property name="lineSize" type="Integer">
 *   Get/Set the line size
 *  </property>
 *  <property name="max" type="Integer">
 *   Get/Set the maximum value
 *  </property>
 *  <property name="min" type="Integer">
 *   Get/Set the minimum value
 *  </property>
 *  <property name="pageSize" type="Integer">
 *   Get/Set the pagesize
 *  </property>
 *  <property name="selEnd" type="Integer">
 *   Get/Set the end selection point
 *  </property>
 *  <property name="selStart" type="Integer">
 *   Get/Set the start selection point
 *  </property>
 *  <property name="value" type="Integer">
 *   Get/Set the current value
 *  </property>
 * </properties>
 */
WXJS_BEGIN_PROPERTY_MAP(Slider)
	WXJS_PROPERTY(P_LINESIZE, "lineSize")
	WXJS_PROPERTY(P_MAX, "max")
	WXJS_PROPERTY(P_MIN, "min")
	WXJS_PROPERTY(P_PAGESIZE, "pageSize")
	WXJS_PROPERTY(P_SEL_END, "selEnd")
	WXJS_PROPERTY(P_SEL_START, "selStart")
	WXJS_PROPERTY(P_THUMB_LENGTH, "thumbLength")
	WXJS_PROPERTY(P_VALUE, "value")
WXJS_END_PROPERTY_MAP()

/***
 * <constants>
 *  <type name="Styles">
 *   <constant name="HORIZONTAL">
 *    Displays the slider horizontally.  
 *   </constant>
 *   <constant name="VERTICAL">
 *    Displays the slider vertically.  
 *   </constant>
 *   <constant name="AUTOTICKS">
 *    Displays tick marks.  
 *   </constant>
 *   <constant name="LABELS">
 *    Displays minimum, maximum and value labels. 
 *    (NB: only displays the current value label under wxGTK)  
 *   </constant>
 *   <constant name="LEFT">
 *    Displays ticks on the left, if a vertical slider.  
 *   </constant>
 *   <constant name="RIGHT">
 *    Displays ticks on the right, if a vertical slider.  
 *   </constant>
 *   <constant name="TOP">
 *    Displays ticks on the top, if a horizontal slider.  
 *   </constant>
 *   <constant name="SELRANGE">
 *    Allows the user to select a range on the slider. Windows 95 only.  
 *   </constant>
 *  </type>
 * </constants>
 */
WXJS_BEGIN_CONSTANT_MAP(Slider)
	WXJS_CONSTANT(wxSL_, HORIZONTAL)
	WXJS_CONSTANT(wxSL_, VERTICAL)
	WXJS_CONSTANT(wxSL_, TICKS)
	WXJS_CONSTANT(wxSL_, AUTOTICKS)
	WXJS_CONSTANT(wxSL_, LABELS)
	WXJS_CONSTANT(wxSL_, LEFT)
	WXJS_CONSTANT(wxSL_, TOP)
	WXJS_CONSTANT(wxSL_, RIGHT)
	WXJS_CONSTANT(wxSL_, BOTTOM)
	WXJS_CONSTANT(wxSL_, BOTH)
	WXJS_CONSTANT(wxSL_, SELRANGE)
WXJS_END_CONSTANT_MAP()

bool Slider::GetProperty(wxSlider *p,
                         JSContext *cx,
                         JSObject* WXUNUSED(obj),
                         int id,
                         jsval *vp)
{
    switch (id) 
	{
	case P_LINESIZE:
		*vp = ToJS(cx, p->GetLineSize());
		break;
	case P_MAX:
		*vp = ToJS(cx, p->GetMax());
		break;
	case P_MIN:
		*vp = ToJS(cx, p->GetMin());
		break;
	case P_PAGESIZE:
		*vp = ToJS(cx, p->GetPageSize());
		break;
	case P_SEL_END:
		*vp = ToJS(cx, p->GetSelEnd());
		break;
	case P_SEL_START:
		*vp = ToJS(cx, p->GetSelStart());
		break;
	case P_THUMB_LENGTH:
		*vp = ToJS(cx ,p->GetThumbLength());
		break;
	case P_VALUE:
		*vp = ToJS(cx, p->GetValue());
		break;
    }
    return true;
}

bool Slider::SetProperty(wxSlider *p,
                         JSContext *cx,
                         JSObject* WXUNUSED(obj),
                         int id,
                         jsval *vp)
{
    switch (id) 
	{
	case P_LINESIZE:
		{
			int value;
			if ( FromJS(cx, *vp, value) )
				p->SetLineSize(value);
			break;
		}
	case P_MAX:
		{
			int value;
			if ( FromJS(cx, *vp, value) )
				p->SetRange(p->GetMin(), value);
			break;
		}
	case P_MIN:
		{
			int value;
			if ( FromJS(cx, *vp, value) )
				p->SetRange(value, p->GetMax());
			break;
		}
	case P_PAGESIZE:
		{
			int value;
			if ( FromJS(cx, *vp, value) )
				p->SetPageSize(value);
			break;
		}
	case P_SEL_END:
		{
			int value;
			if ( FromJS(cx, *vp, value) )
				p->SetSelection(p->GetSelStart(), value);
			break;
		}
	case P_SEL_START:
		{
			int value;
			if ( FromJS(cx, *vp, value) )
				p->SetSelection(value, p->GetSelEnd());
			break;
		}
	case P_THUMB_LENGTH:
		{
			int value;
			if ( FromJS(cx, *vp, value) )
				p->SetThumbLength(value);
			break;
		}
	case P_TICK:
		{
			int value;
			if ( FromJS(cx, *vp, value) )
				p->SetTick(value);
			break;
		}
	case P_VALUE:
		{
			int value;
			if ( FromJS(cx, *vp, value) )
				p->SetValue(value);
			break;
		}
    }
    return true;
}

bool Slider::AddProperty(wxSlider *p, 
                         JSContext* WXUNUSED(cx), 
                         JSObject* WXUNUSED(obj), 
                         const wxString &prop, 
                         jsval* WXUNUSED(vp))
{
    if ( WindowEventHandler::ConnectEvent(p, prop, true) )
        return true;
    
    SliderEventHandler::ConnectEvent(p, prop, true);

    return true;
}


bool Slider::DeleteProperty(wxSlider *p, 
                            JSContext* WXUNUSED(cx), 
                            JSObject* WXUNUSED(obj), 
                            const wxString &prop)
{
  if ( WindowEventHandler::ConnectEvent(p, prop, false) )
    return true;
  
  SliderEventHandler::ConnectEvent(p, prop, false);
  return true;
}

/***
 * <ctor>
 *  <function />
 *  <function>
 *   <arg name="Parent" type="@wxWindow">
 *    The parent of wxSlider.
 *   </arg>
 *   <arg name="Id" type="Integer">
 *    An window identifier. Use -1 when you don't need it.
 *   </arg>
 *   <arg name="Value" type="Integer">
 *    Initial position of the slider
 *   </arg>
 *   <arg name="Min" type="Integer">
 *    Minimum slider position.
 *   </arg>
 *   <arg name="Max" type="Integer">
 *    Maximum slider position.
 *   </arg>
 *   <arg name="Position" type="@wxPoint" default="wxDefaultPosition">
 *    The position of the Slider control on the given parent.
 *   </arg>
 *   <arg name="Size" type="@wxSize" default="wxDefaultSize">
 *    The size of the Slider control.
 *   </arg>
 *   <arg name="Style" type="Integer" default="wxSlider.HORIZONTAL">
 *    The wxSlider style.
 *   </arg>
 *   <arg name="Validator" type="@wxValidator" default="null" />
 *  </function>
 *  <desc>
 *   Constructs a new wxSlider object.
 *  </desc>
 * </ctor>
 */
wxSlider* Slider::Construct(JSContext *cx,
                            JSObject *obj,
                            uintN argc,
                            jsval *argv,
                            bool WXUNUSED(constructing))
{
  wxSlider *p = new wxSlider();
  SetPrivate(cx, obj, p);

  if ( argc > 0 )
  {
    jsval rval;
    if ( ! create(cx, obj, argc, argv, &rval) )
      return NULL;
  }
  return p;
}

WXJS_BEGIN_METHOD_MAP(Slider)
  WXJS_METHOD("create", create, 5)
  WXJS_METHOD("clearSel", clearSel, 0)
  WXJS_METHOD("setRange", setRange, 2)
  WXJS_METHOD("setSelection", setSelection, 2)
  WXJS_METHOD("setTickFreq", setTickFreq, 2)
WXJS_END_METHOD_MAP()

/***
 * <method name="create">
 *  <function>
 *   <arg name="Parent" type="@wxWindow">
 *    The parent of wxSlider.
 *   </arg>
 *   <arg name="Id" type="Integer">
 *    An window identifier. Use -1 when you don't need it.
 *   </arg>
 *   <arg name="Value" type="Integer">
 *    Initial position of the slider
 *   </arg>
 *   <arg name="Min" type="Integer">
 *    Minimum slider position.
 *   </arg>
 *   <arg name="Max" type="Integer">
 *    Maximum slider position.
 *   </arg>
 *   <arg name="Position" type="@wxPoint" default="wxDefaultPosition">
 *    The position of the Slider control on the given parent.
 *   </arg>
 *   <arg name="Size" type="@wxSize" default="wxDefaultSize">
 *    The size of the Slider control.
 *   </arg>
 *   <arg name="Style" type="Integer" default="wxSlider.HORIZONTAL">
 *    The wxSlider style.
 *   </arg>
 *   <arg name="Validator" type="@wxValidator" default="null" />
 *  </function>
 *  <desc>
 *   Constructs a new wxSlider object.
 *  </desc>
 * </method>
 */
JSBool Slider::create(JSContext *cx,
                      JSObject *obj,
                      uintN argc,
                      jsval *argv,
                      jsval *rval)
{
  wxSlider* p = GetPrivate(cx, obj);
  *rval = JSVAL_FALSE;

  if ( argc > 9 )
    argc = 9;

  const wxPoint *pt = &wxDefaultPosition;
  const wxSize *size = &wxDefaultSize;
  int style = 0;
  const wxValidator *val = &wxDefaultValidator;

  switch(argc)
    {
    case 9:
        val = Validator::GetPrivate(cx, argv[8]);
        if ( val == NULL )
        {
          JS_ReportError(cx, WXJS_INVALID_ARG_TYPE, 9, "wxValidator");
          return JS_FALSE;
        }
    case 8:
        if ( ! FromJS(cx, argv[7], style) )
        {
          JS_ReportError(cx, WXJS_INVALID_ARG_TYPE, 8, "Integer");
          return JS_FALSE;
        }
        // Fall through
    case 7:
		size = Size::GetPrivate(cx, argv[6]);
		if ( size == NULL )
        {
          JS_ReportError(cx, WXJS_INVALID_ARG_TYPE, 7, "wxSize");
          return JS_FALSE;
        }
		// Fall through
	case 6:
      pt = wxjs::ext::GetPoint(cx, argv[5]);
		if ( pt == NULL )
        {
          JS_ReportError(cx, WXJS_INVALID_ARG_TYPE, 6, "wxPoint");
          return JS_FALSE;
        }
		// Fall through
    default:
        int max;
        if ( ! FromJS(cx, argv[4], max) )
        {
          JS_ReportError(cx, WXJS_INVALID_ARG_TYPE, 5, "Integer");
          return JS_FALSE;
        }

        int min;
        if ( ! FromJS(cx, argv[3], min) )
        {
          JS_ReportError(cx, WXJS_INVALID_ARG_TYPE, 4, "Integer");
          return JS_FALSE;
        }

        int value;
        if ( ! FromJS(cx, argv[2], value) )
        {
          JS_ReportError(cx, WXJS_INVALID_ARG_TYPE, 3, "Integer");
          return JS_FALSE;
        }

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

        if ( p->Create(parent, id, value, min, max, *pt, *size, style, *val) )
        {
          *rval = JSVAL_TRUE;
          p->SetClientObject(new JavaScriptClientData(cx, obj, true, false));
        }
    }
  return JS_TRUE;
}

/***
 * <method name="clearSel">
 *  <function />
 *  <desc>
 *   Clears the selection, for a slider with the SELRANGE style.
 *  </desc>
 * </method>
 */
JSBool Slider::clearSel(JSContext *cx,
                        JSObject *obj,
                        uintN WXUNUSED(argc),
                        jsval* WXUNUSED(argv),
                        jsval* WXUNUSED(rval))
{
    wxSlider *p = GetPrivate(cx, obj);
	if ( p == NULL )
		return JS_FALSE;

	p->ClearSel();

	return JS_TRUE;
}

/***
 * <method name="clearTicks">
 *  <function />
 *  <desc>
 *   Clears the ticks.
 *  </desc>
 * </method>
 */
JSBool Slider::clearTicks(JSContext *cx,
                          JSObject *obj,
                          uintN WXUNUSED(argc),
                          jsval* WXUNUSED(argv),
                          jsval* WXUNUSED(rval))
{
    wxSlider *p = GetPrivate(cx, obj);
	if ( p == NULL )
		return JS_FALSE;

	p->ClearTicks();

	return JS_TRUE;
}

/***
 * <method name="setRange">
 *  <function>
 *   <arg name="Min" type="Integer">
 *    The minimum value
 *   </arg>
 *   <arg name="Max" type="Integer">
 *    The maximum value
 *   </arg>
 *  </function>
 *  <desc>
 *   Sets the minimum and maximum slider values.
 *  </desc>
 * </method>
 */
JSBool Slider::setRange(JSContext *cx,
                        JSObject *obj,
                        uintN WXUNUSED(argc),
                        jsval* argv,
                        jsval* WXUNUSED(rval))
{
    wxSlider *p = GetPrivate(cx, obj);
	if ( p == NULL )
		return JS_FALSE;

	int mini;
	int maxi;
	
	if (    FromJS(cx, argv[0], mini) 
		 && FromJS(cx, argv[1], maxi) )
	{
		p->SetRange(mini, maxi);
	}
	else
	{
		return JS_FALSE;
	}

	return JS_TRUE;
}

/***
 * <method name="setSelection">
 *  <function>
 *   <arg name="Start" type="Integer">
 *    The selection start position
 *   </arg>
 *   <arg name="End" type="Integer">
 *    The selection end position
 *   </arg>
 *  </function>
 *  <desc>
 *   Sets the selection
 *  </desc>
 * </method>
 */
JSBool Slider::setSelection(JSContext *cx,
                            JSObject *obj,
                            uintN WXUNUSED(argc),
                            jsval *argv,
                            jsval* WXUNUSED(rval))
{
    wxSlider *p = GetPrivate(cx, obj);
	if ( p == NULL )
		return JS_FALSE;

	int start;
	int end;
		
	if (    FromJS(cx, argv[0], start)
		 && FromJS(cx, argv[1], end)   )
	{
		p->SetSelection(start, end);
	}
	else
	{
		return JS_FALSE;
	}

	return JS_TRUE;
}

/***
 * <method name="setTickFreq">
 *  <function>
 *   <arg name="Freq" type="Integer">
 *    Frequency
 *   </arg>
 *   <arg name="Pos" type="Integer">
 *    Position
 *   </arg>
 *  </function>
 *  <desc>
 *   Sets the tick mark frequency and position.
 *  </desc>
 * </method>
 */
JSBool Slider::setTickFreq(JSContext *cx,
                           JSObject *obj,
                           uintN WXUNUSED(argc),
                           jsval *argv,
                           jsval* WXUNUSED(rval))
{
    wxSlider *p = GetPrivate(cx, obj);
	if ( p == NULL )
		return JS_FALSE;

	int n;
	int pos;

	if (    FromJS(cx, argv[0], n)
		 && FromJS(cx, argv[1], pos) )
	{
		p->SetTickFreq(n, pos);
	}
	else
	{
		return JS_FALSE;
	}

	return JS_TRUE;
}

/***
 * <events>
 *  <event name="onScrollChanged">
 *   Event is triggered when scrolling/ moving has finished independently of the
 *   way it had started. The argument of the function is a @wxScrollEvent.
 *  </event>
 *  <event name="onScrollTop">
 *   Catch a command to put the scroll thumb at the maximum position. 
 *   The argument of the function is a @wxScrollEvent.
 *  </event>
 *  <event name="onScrollBottom">
 *   Catch a command to put the scroll thumb at the maximum position.
 *   The argument of the function is a @wxScrollEvent.
 *  </event>
 *  <event name="onScrollLineUp">
 *   Catch a line up command.
 *   The argument of the function is a @wxScrollEvent.
 *  </event>
 *  <event name="onScrollLineDown">
 *   Catch a line down command.
 *   The argument of the function is a @wxScrollEvent.
 *  </event>
 *  <event name="onScrollPageUp">
 *   Catch a page up command.
 *   The argument of the function is a @wxScrollEvent.
 *  </event>
 *  <event name="onScrollPageDown">
 *   Catch a page down command. 
 *   The argument of the function is a @wxScrollEvent.
 *  </event>
 *  <event name="onScrollThumbTrack">
 *   Catch a thumbtrack command (continuous movement of the scroll thumb). 
 *   The argument of the function is a @wxScrollEvent.
 *  </event>
 *  <event name="onScrollThumbRelease">
 *   Catch a thumbtrack release command.
 *   The argument of the function is a @wxScrollEvent.
 *  </event>
 * </events>
 */
WXJS_INIT_EVENT_MAP(wxSlider)
const wxString WXJS_SCROLL_CHANGED_EVENT = wxT("onScrollChanged");
const wxString WXJS_SCROLL_TOP_EVENT = wxT("onScrollTop");
const wxString WXJS_SCROLL_BOTTOM_EVENT = wxT("onScrollBottom");
const wxString WXJS_SCROLL_LINEUP_EVENT = wxT("onScrollLineUp");
const wxString WXJS_SCROLL_LINEDOWN_EVENT = wxT("onScrollLineDown");
const wxString WXJS_SCROLL_PAGEUP_EVENT = wxT("onScrollPageUp");
const wxString WXJS_SCROLL_PAGEDOWN_EVENT = wxT("onScrollPageDown");
const wxString WXJS_SCROLL_THUMBTRACK_EVENT = wxT("onScrollThumbTrack");
const wxString WXJS_SCROLL_THUMBRELEASE_EVENT = wxT("onScrollThumbRelease");

void SliderEventHandler::OnScrollChanged(wxScrollEvent& event)
{
  PrivScrollEvent::Fire<ScrollEvent>(event, WXJS_SCROLL_CHANGED_EVENT);
}

void SliderEventHandler::OnScrollTop(wxScrollEvent& event)
{
  PrivScrollEvent::Fire<ScrollEvent>(event, WXJS_SCROLL_TOP_EVENT);
}

void SliderEventHandler::OnScrollBottom(wxScrollEvent& event)
{
  PrivScrollEvent::Fire<ScrollEvent>(event, WXJS_SCROLL_BOTTOM_EVENT);
}

void SliderEventHandler::OnScrollLineUp(wxScrollEvent& event)
{
  PrivScrollEvent::Fire<ScrollEvent>(event, WXJS_SCROLL_LINEUP_EVENT);
}

void SliderEventHandler::OnScrollLineDown(wxScrollEvent& event)
{
  PrivScrollEvent::Fire<ScrollEvent>(event, WXJS_SCROLL_LINEDOWN_EVENT);
}

void SliderEventHandler::OnScrollPageUp(wxScrollEvent& event)
{
  PrivScrollEvent::Fire<ScrollEvent>(event, WXJS_SCROLL_PAGEUP_EVENT);
}

void SliderEventHandler::OnScrollPageDown(wxScrollEvent& event)
{
  PrivScrollEvent::Fire<ScrollEvent>(event, WXJS_SCROLL_PAGEDOWN_EVENT);
}

void SliderEventHandler::OnScrollThumbTrack(wxScrollEvent& event)
{
  PrivScrollEvent::Fire<ScrollEvent>(event, WXJS_SCROLL_THUMBTRACK_EVENT);
}

void SliderEventHandler::OnScrollThumbRelease(wxScrollEvent& event)
{
  PrivScrollEvent::Fire<ScrollEvent>(event, WXJS_SCROLL_THUMBRELEASE_EVENT);
}

void SliderEventHandler::ConnectScrollChanged(wxSlider *p, bool connect)
{
  if ( connect )
  {
    p->Connect(wxEVT_SCROLL_CHANGED, 
               wxScrollEventHandler(SliderEventHandler::OnScrollChanged));
  }
  else
  {
    p->Disconnect(wxEVT_SCROLL_CHANGED, 
                  wxScrollEventHandler(SliderEventHandler::OnScrollChanged));
  }
}

void SliderEventHandler::ConnectScrollTop(wxSlider *p, bool connect)
{
  if ( connect )
  {
    p->Connect(wxEVT_SCROLL_TOP, 
               wxScrollEventHandler(SliderEventHandler::OnScrollTop));
  }
  else
  {
    p->Disconnect(wxEVT_SCROLL_TOP, 
                  wxScrollEventHandler(SliderEventHandler::OnScrollTop));
  }
}

void SliderEventHandler::ConnectScrollBottom(wxSlider *p, bool connect)
{
  if ( connect )
  {
    p->Connect(wxEVT_SCROLL_BOTTOM, 
               wxScrollEventHandler(SliderEventHandler::OnScrollBottom));
  }
  else
  {
    p->Disconnect(wxEVT_SCROLL_BOTTOM, 
                  wxScrollEventHandler(SliderEventHandler::OnScrollBottom));
  }
}

void SliderEventHandler::ConnectScrollLineUp(wxSlider *p, bool connect)
{
  if ( connect )
  {
    p->Connect(wxEVT_SCROLL_LINEUP, 
               wxScrollEventHandler(SliderEventHandler::OnScrollTop));
  }
  else
  {
    p->Disconnect(wxEVT_SCROLL_LINEUP, 
                  wxScrollEventHandler(SliderEventHandler::OnScrollTop));
  }
}

void SliderEventHandler::ConnectScrollLineDown(wxSlider *p, bool connect)
{
  if ( connect )
  {
    p->Connect(wxEVT_SCROLL_LINEDOWN, 
               wxScrollEventHandler(SliderEventHandler::OnScrollLineDown));
  }
  else
  {
    p->Disconnect(wxEVT_SCROLL_LINEDOWN, 
                  wxScrollEventHandler(SliderEventHandler::OnScrollLineDown));
  }
}

void SliderEventHandler::ConnectScrollPageUp(wxSlider *p, bool connect)
{
  if ( connect )
  {
    p->Connect(wxEVT_SCROLL_PAGEUP, 
               wxScrollEventHandler(SliderEventHandler::OnScrollPageUp));
  }
  else
  {
    p->Disconnect(wxEVT_SCROLL_PAGEUP, 
               wxScrollEventHandler(SliderEventHandler::OnScrollPageUp));
  }
}

void SliderEventHandler::ConnectScrollPageDown(wxSlider *p, bool connect)
{
  if ( connect )
  {
    p->Connect(wxEVT_SCROLL_PAGEDOWN, 
               wxScrollEventHandler(SliderEventHandler::OnScrollPageDown));
  }
  else
  {
    p->Disconnect(wxEVT_SCROLL_PAGEDOWN, 
               wxScrollEventHandler(SliderEventHandler::OnScrollPageDown));
  }
}

void SliderEventHandler::ConnectScrollThumbTrack(wxSlider *p, bool connect)
{
  if ( connect )
  {
    p->Connect(wxEVT_SCROLL_THUMBTRACK, 
               wxScrollEventHandler(SliderEventHandler::OnScrollThumbTrack));
  }
  else
  {
    p->Disconnect(wxEVT_SCROLL_THUMBTRACK, 
                  wxScrollEventHandler(SliderEventHandler::OnScrollThumbTrack));
  }
}

void SliderEventHandler::ConnectScrollThumbRelease(wxSlider *p, bool connect)
{
  if ( connect )
  {
    p->Connect(wxEVT_SCROLL_THUMBRELEASE, 
               wxScrollEventHandler(SliderEventHandler::OnScrollThumbRelease));
  }
  else
  {
    p->Disconnect(wxEVT_SCROLL_THUMBRELEASE, 
                  wxScrollEventHandler(SliderEventHandler::OnScrollThumbRelease));
  }
}

void SliderEventHandler::InitConnectEventMap()
{
  AddConnector(WXJS_SCROLL_CHANGED_EVENT, ConnectScrollChanged);
  AddConnector(WXJS_SCROLL_TOP_EVENT, ConnectScrollTop);
  AddConnector(WXJS_SCROLL_BOTTOM_EVENT, ConnectScrollBottom);
  AddConnector(WXJS_SCROLL_LINEUP_EVENT, ConnectScrollLineUp);
  AddConnector(WXJS_SCROLL_LINEDOWN_EVENT, ConnectScrollLineDown);
  AddConnector(WXJS_SCROLL_PAGEUP_EVENT, ConnectScrollPageUp);
  AddConnector(WXJS_SCROLL_PAGEDOWN_EVENT, ConnectScrollPageDown);
  AddConnector(WXJS_SCROLL_THUMBTRACK_EVENT, ConnectScrollThumbTrack);
  AddConnector(WXJS_SCROLL_THUMBRELEASE_EVENT, ConnectScrollThumbRelease);
}
