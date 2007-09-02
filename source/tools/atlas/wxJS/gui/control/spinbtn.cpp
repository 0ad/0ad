#include "precompiled.h"

#ifndef WX_PRECOMP
    #include <wx/wx.h>
#endif

#include "../../common/main.h"

#include "../event/jsevent.h"
#include "../event/command.h"
#include "../event/spinevt.h"

#include "spinbtn.h"
#include "window.h"

#include "../misc/point.h"
#include "../misc/size.h"
#include "../misc/validate.h"
#include "../errors.h"

using namespace wxjs;
using namespace wxjs::gui;

/***
 * <module>gui</module>
 * <file>spinbtn</file>
 * <class name="wxSplitButton" prototype="@wxControl">
 *  A wxSpinButton has two small up and down (or left and right) arrow buttons. 
 *  It is often used next to a text control for increment and decrementing a 
 *  value. Portable programs should try to use @wxSpinButton instead as 
 *  wxSpinButton is not implemented for all platforms but @wxSpinButton is as it 
 *  degenerates to a simple @wxTextCtrl on such platforms.
 * </class>
 */
WXJS_INIT_CLASS(SpinButton, "wxSpinButton", 2)
void SpinButton::InitClass(JSContext* WXUNUSED(cx),
                         JSObject* WXUNUSED(obj), 
                         JSObject* WXUNUSED(proto))
{
	SpinButtonEventHandler::InitConnectEventMap();
}

/***
 * <constants>
 *	<type name="styles">
 *   <constant name="ARROW_KEYS" />
 *   <constant name="WRAP" />
 *   <constant name="HORIZONTAL" />
 *   <constant name="VERTICAL" />
 *  </type>
 * </constants>
 */
WXJS_BEGIN_CONSTANT_MAP(SpinButton)
	WXJS_CONSTANT(wxSP_, ARROW_KEYS)
	WXJS_CONSTANT(wxSP_, WRAP)
    WXJS_CONSTANT(wxSP_, HORIZONTAL)
    WXJS_CONSTANT(wxSP_, VERTICAL)
WXJS_END_CONSTANT_MAP()

/***
 * <properties>
 *  <property name="value" type="String">
 *   The value of the spin control.
 *  </property>
 *  <property name="min" type="Integer" readonly="Y">
 *   Gets minimal allowable value.
 *  </property>
 *  <property name="max" type="Integer" readonly="Y">
 *   Gets maximal allowable value.
 *  </property>
 * </properties>
 */   
WXJS_BEGIN_PROPERTY_MAP(SpinButton)
	WXJS_PROPERTY(P_VALUE, "value")
	WXJS_READONLY_PROPERTY(P_MIN, "min")
	WXJS_READONLY_PROPERTY(P_MAX, "max")
WXJS_END_PROPERTY_MAP()

bool SpinButton::GetProperty(wxSpinButton *p, JSContext *cx, JSObject *obj, int id, jsval *vp)
{
	switch (id) 
	{
	case P_VALUE:
		*vp = ToJS(cx, p->GetValue());
		break;
	case P_MIN:
		*vp = ToJS(cx, p->GetMin());
		break;
	case P_MAX:
		*vp = ToJS(cx, p->GetMax());
		break;
	}
	return true;
}

bool SpinButton::SetProperty(wxSpinButton *p, JSContext *cx, JSObject *obj, int id, jsval *vp)
{
	switch (id) 
	{
	case P_VALUE:
		{
			int value;
			FromJS(cx, *vp, value);
			p->SetValue(value);
			break;
		}
	}
	return true;
}

bool SpinButton::AddProperty(wxSpinButton *p, 
                           JSContext* WXUNUSED(cx), 
                           JSObject* WXUNUSED(obj), 
                           const wxString &prop, 
                           jsval* WXUNUSED(vp))
{
	if ( WindowEventHandler::ConnectEvent(p, prop, true) )
		return true;

	SpinButtonEventHandler::ConnectEvent(p, prop, true);

	return true;
}

bool SpinButton::DeleteProperty(wxSpinButton *p, 
                              JSContext* WXUNUSED(cx), 
                              JSObject* WXUNUSED(obj), 
                              const wxString &prop)
{
	if ( WindowEventHandler::ConnectEvent(p, prop, false) )
		return true;
	
	SpinButtonEventHandler::ConnectEvent(p, prop, false);
	return true;
}

/***
 * <ctor>
 *  <function />
 *  <function>
 *   <arg name="parent" type="@wxWindow">
 *    Parent window. Must not be NULL.
 *   </arg>
 *   <arg name="id" type="Integer">
 *    Window identifier. A value of -1 indicates a default value.
 *   </arg>
 *   <arg name="pos" type="@wxPoint" default="wxDefaultPosition">
 *    Window position.
 *   </arg>
 *   <arg name="size" type="@wxSize" default="wxDefaultSize">
 *    Window size.
 *   </arg>
 *   <arg name="style" type="Integer" default="wxSpinButton.ARROW_KEYS">
 *    Window style.
 *   </arg>
 *  </function>
 *  <desc>
 *   Create a wxSpinButton
 *  </desc>
 * </ctor>
 */
wxSpinButton* SpinButton::Construct(JSContext *cx,
                                JSObject *obj,
                                uintN argc,
                                jsval *argv,
                                bool WXUNUSED(constructing))
{
	wxSpinButton *p = new wxSpinButton();
	SetPrivate(cx, obj, p);
	
	if ( argc > 0 )
	{
		jsval rval;
		if ( ! create(cx, obj, argc, argv, &rval) )
			return NULL;
	}
	return p;
}

WXJS_BEGIN_METHOD_MAP(SpinButton)
  WXJS_METHOD("create", create, 2)
  WXJS_METHOD("setRange", setRange, 2)
WXJS_END_METHOD_MAP()

/***
 * <method name="create">
 *  <function returns="Boolean">
 *   <arg name="parent" type="@wxWindow">
 *    Parent window. Must not be NULL.
 *   </arg>
 *   <arg name="id" type="Integer">
 *    Window identifier. A value of -1 indicates a default value.
 *   </arg>
 *   <arg name="pos" type="@wxPoint" default="wxDefaultPosition">
 *    Window position.
 *   </arg>
 *   <arg name="size" type="@wxSize" default="wxDefaultSize">
 *    Window size.
 *   </arg>
 *   <arg name="style" type="Integer" default="wxSpinButton.HORIZONTAL">
 *    Window style.
 *   </arg>
 *  </function>
 *  <desc>
 *   Create a wxSpinButton
 *  </desc>
 * </method>
 */
JSBool SpinButton::create(JSContext *cx,
                        JSObject *obj,
                        uintN argc,
                        jsval *argv,
                        jsval *rval)
{
	wxSpinButton *p = GetPrivate(cx, obj);
	*rval = JSVAL_FALSE;

	if ( argc > 5 )
		argc = 5;

	wxWindowID id = -1;
	const wxPoint *pt = &wxDefaultPosition;
	const wxSize *size = &wxDefaultSize;
	int style = wxSP_ARROW_KEYS;

	switch(argc)
	{
	case 5:
		if ( ! FromJS(cx, argv[4], style) )
		{
			JS_ReportError(cx, WXJS_INVALID_ARG_TYPE, 4, "Integer");
			return JS_FALSE;
		}
		// Fall through
	case 4:
		size = Size::GetPrivate(cx, argv[3]);
		if ( size == NULL )
		{
			JS_ReportError(cx, WXJS_INVALID_ARG_TYPE, 3, "wxSize");
			return JS_FALSE;
		}
		// Fall through
	case 3:
		pt = Point::GetPrivate(cx, argv[2]);
		if ( pt == NULL )
		{
			JS_ReportError(cx, WXJS_INVALID_ARG_TYPE, 3, "wxPoint");
			return JS_FALSE;
		}
		// Fall through
	default:

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
		}
	}

	return JS_TRUE;
}

/***
 * <method name="setRange">
 *  <function>
 *   <arg name="Min" type="Integer" />
 *   <arg name="Max" type="Integer" />
 *  </function>
 *  <desc>
 *   Sets range of allowable values.
 *  </desc>
 * </method>
 */
JSBool SpinButton::setRange(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
	wxSpinButton *p = GetPrivate(cx, obj);
	if ( p == NULL )
		return JS_FALSE;

	int minVal, maxVal;

	if (! FromJS(cx, argv[0], minVal))
		return JS_FALSE;
		
	if (! FromJS(cx, argv[1], maxVal))
		return JS_FALSE;
	
	p->SetRange(minVal, maxVal);

	return JS_TRUE;
}

/***
 * <events>
 *  <event name="onSpin">
 *   Generated whenever an arrow is pressed. A @wxSpinEvent is passed
 *   as argument.
 *  </event>
 *  <event name="onSpinUp">
 *   Generated when left/up arrow is pressed. A @wxSpinEvent is passed
 *   as argument.
 *  </event>
 *  <event name="onSpinDown">
 *   Generated when right/down arrow is pressed. A @wxSpinEvent is passed
 *   as argument.
 *  </event>
 * </events>
 */
WXJS_INIT_EVENT_MAP(wxSpinButton)
const wxString WXJS_SPIN_EVENT = wxT("onSpin");
const wxString WXJS_SPIN_UP_EVENT = wxT("onSpinUp");
const wxString WXJS_SPIN_DOWN_EVENT = wxT("onSpinDown");

void SpinButtonEventHandler::OnSpin(wxSpinEvent &event)
{
	PrivSpinEvent::Fire<SpinEvent>(event, WXJS_SPIN_EVENT);
}

void SpinButtonEventHandler::OnSpinUp(wxSpinEvent &event)
{
	PrivSpinEvent::Fire<SpinEvent>(event, WXJS_SPIN_UP_EVENT);
}

void SpinButtonEventHandler::OnSpinDown(wxSpinEvent &event)
{
	PrivSpinEvent::Fire<SpinEvent>(event, WXJS_SPIN_DOWN_EVENT);
}

void SpinButtonEventHandler::ConnectSpin(wxSpinButton *p, bool connect)
{
	if ( connect )
	{
		p->Connect(wxEVT_SCROLL_THUMBTRACK,
				wxSpinEventHandler(SpinButtonEventHandler::OnSpin));
	}
	else
	{
		p->Disconnect(wxEVT_SCROLL_THUMBTRACK,
				wxSpinEventHandler(SpinButtonEventHandler::OnSpin));
	}
}

void SpinButtonEventHandler::ConnectSpinUp(wxSpinButton *p, bool connect)
{
	if ( connect )
	{
		p->Connect(wxEVT_SCROLL_LINEUP,
				wxSpinEventHandler(SpinButtonEventHandler::OnSpinUp));
	}
	else
	{
		p->Disconnect(wxEVT_SCROLL_LINEUP,
				wxSpinEventHandler(SpinButtonEventHandler::OnSpinUp));
	}
}

void SpinButtonEventHandler::ConnectSpinDown(wxSpinButton *p, bool connect)
{
	if ( connect )
	{
		p->Connect(wxEVT_SCROLL_LINEDOWN,
				wxSpinEventHandler(SpinButtonEventHandler::OnSpinDown));
	}
	else
	{
		p->Disconnect(wxEVT_SCROLL_LINEDOWN,
				wxSpinEventHandler(SpinButtonEventHandler::OnSpinDown));
	}
}

void SpinButtonEventHandler::InitConnectEventMap()
{
	AddConnector(WXJS_SPIN_EVENT, ConnectSpin);
	AddConnector(WXJS_SPIN_UP_EVENT, ConnectSpinUp);
	AddConnector(WXJS_SPIN_DOWN_EVENT, ConnectSpinDown);
}
