#include "precompiled.h"

#ifndef WX_PRECOMP
    #include <wx/wx.h>
#endif
#include <wx/file.h>

#include "../../common/main.h"

#include "../event/jsevent.h"
#include "../event/command.h"
#include "../event/spinevt.h"

#include "spinctrl.h"
#include "window.h"

#include "../misc/point.h"
#include "../misc/size.h"
#include "../misc/validate.h"
#include "../errors.h"

using namespace wxjs;
using namespace wxjs::gui;

WXJS_INIT_CLASS(SpinCtrl, "wxSpinCtrl", 1)
void SpinCtrl::InitClass(JSContext* WXUNUSED(cx),
                         JSObject* WXUNUSED(obj), 
                         JSObject* WXUNUSED(proto))
{
	SpinCtrlEventHandler::InitConnectEventMap();
}

WXJS_BEGIN_PROPERTY_MAP(SpinCtrl)
	WXJS_PROPERTY(P_VALUE, "value")
	WXJS_READONLY_PROPERTY(P_MIN, "min")
	WXJS_READONLY_PROPERTY(P_MAX, "max")
WXJS_END_PROPERTY_MAP()

WXJS_BEGIN_CONSTANT_MAP(SpinCtrl)
	WXJS_CONSTANT(wxSP_, ARROW_KEYS)
	WXJS_CONSTANT(wxSP_, WRAP)
WXJS_END_CONSTANT_MAP()

bool SpinCtrl::GetProperty(wxSpinCtrl *p, JSContext *cx, JSObject *obj, int id, jsval *vp)
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

bool SpinCtrl::SetProperty(wxSpinCtrl *p, JSContext *cx, JSObject *obj, int id, jsval *vp)
{
	switch (id) 
	{
	case P_VALUE:
		{
			wxString value;
			FromJS(cx, *vp, value);
			p->SetValue(value);
			break;
		}
	}
	return true;
}

bool SpinCtrl::AddProperty(wxSpinCtrl *p, 
                           JSContext* WXUNUSED(cx), 
                           JSObject* WXUNUSED(obj), 
                           const wxString &prop, 
                           jsval* WXUNUSED(vp))
{
	if ( WindowEventHandler::ConnectEvent(p, prop, true) )
		return true;

	SpinCtrlEventHandler::ConnectEvent(p, prop, true);

	return true;
}

bool SpinCtrl::DeleteProperty(wxSpinCtrl *p, 
                              JSContext* WXUNUSED(cx), 
                              JSObject* WXUNUSED(obj), 
                              const wxString &prop)
{
	if ( WindowEventHandler::ConnectEvent(p, prop, false) )
		return true;
	
	SpinCtrlEventHandler::ConnectEvent(p, prop, false);
	return true;
}

wxSpinCtrl* SpinCtrl::Construct(JSContext *cx,
                                JSObject *obj,
                                uintN argc,
                                jsval *argv,
                                bool WXUNUSED(constructing))
{
	wxSpinCtrl *p = new wxSpinCtrl();
	SetPrivate(cx, obj, p);
	
	if ( argc > 0 )
	{
		jsval rval;
		if ( ! create(cx, obj, argc, argv, &rval) )
			return NULL;
	}
	return p;
}

WXJS_BEGIN_METHOD_MAP(SpinCtrl)
	WXJS_METHOD("setRange", setRange, 2)
	WXJS_METHOD("setSelection", setSelection, 2)
WXJS_END_METHOD_MAP()

JSBool SpinCtrl::create(JSContext *cx,
                        JSObject *obj,
                        uintN argc,
                        jsval *argv,
                        jsval *rval)
{
	wxSpinCtrl *p = GetPrivate(cx, obj);
	*rval = JSVAL_FALSE;

	if ( argc > 9 )
		argc = 9;

	wxWindowID id = -1;
	wxString value = wxEmptyString;
	const wxPoint *pt = &wxDefaultPosition;
	const wxSize *size = &wxDefaultSize;
	int style = wxSP_ARROW_KEYS;
	int min = 0;
	int max = 100;
	int initial = 0;

	switch(argc)
	{
	case 9:
		if ( ! FromJS(cx, argv[8], initial) )
		{
			JS_ReportError(cx, WXJS_INVALID_ARG_TYPE, 9, "Integer");
			return JS_FALSE;
		}
		// Fall through
	case 8:
		if ( ! FromJS(cx, argv[7], max) )
		{
			JS_ReportError(cx, WXJS_INVALID_ARG_TYPE, 8, "Integer");
			return JS_FALSE;
		}
		// Fall through
	case 7:
		if ( ! FromJS(cx, argv[6], min) )
		{
			JS_ReportError(cx, WXJS_INVALID_ARG_TYPE, 7, "Integer");
			return JS_FALSE;
		}
		// Fall through
	case 6:
		if ( ! FromJS(cx, argv[5], style) )
		{
			JS_ReportError(cx, WXJS_INVALID_ARG_TYPE, 6, "Integer");
			return JS_FALSE;
		}
		// Fall through
	case 5:
		size = Size::GetPrivate(cx, argv[4]);
		if ( size == NULL )
		{
			JS_ReportError(cx, WXJS_INVALID_ARG_TYPE, 5, "wxSize");
			return JS_FALSE;
		}
		// Fall through
	case 4:
		pt = Point::GetPrivate(cx, argv[3]);
		if ( pt == NULL )
		{
			JS_ReportError(cx, WXJS_INVALID_ARG_TYPE, 4, "wxPoint");
			return JS_FALSE;
		}
		// Fall through
	case 3:
		FromJS(cx, argv[2], value);
		// Fall through
	case 2:
		if ( ! FromJS(cx, argv[1], id) )
		{
			JS_ReportError(cx, WXJS_INVALID_ARG_TYPE, 2, "Integer");
			return JS_FALSE;
		}
		// Fall through
	default:

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

		if ( p->Create(parent, id, value, *pt, *size, style, min, max, initial) )
		{
			*rval = JSVAL_TRUE;
			p->SetClientObject(new JavaScriptClientData(cx, obj, true, false));
		}
	}

	return JS_TRUE;
}

JSBool SpinCtrl::setSelection(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
	wxSpinCtrl *p = GetPrivate(cx, obj);
	if ( p == NULL )
		return JS_FALSE;

	long from, to;

	if (! FromJS(cx, argv[0], from))
		return JS_FALSE;
		
	if (! FromJS(cx, argv[1], to))
		return JS_FALSE;
	
	p->SetSelection(from, to);

	return JS_TRUE;
}

JSBool SpinCtrl::setRange(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
	wxSpinCtrl *p = GetPrivate(cx, obj);
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

WXJS_INIT_EVENT_MAP(wxSpinCtrl)
const wxString WXJS_TEXT_EVENT = wxT("onText");
const wxString WXJS_SPIN_CTRL_EVENT = wxT("onSpinCtrl");
const wxString WXJS_SPIN_EVENT = wxT("onSpin");
const wxString WXJS_SPIN_UP_EVENT = wxT("onSpinUp");
const wxString WXJS_SPIN_DOWN_EVENT = wxT("onSpinDown");

void SpinCtrlEventHandler::OnText(wxCommandEvent &event)
{
	PrivCommandEvent::Fire<CommandEvent>(event, WXJS_TEXT_EVENT);
}

void SpinCtrlEventHandler::OnSpinCtrl(wxSpinEvent &event)
{
	PrivSpinEvent::Fire<SpinEvent>(event, WXJS_SPIN_CTRL_EVENT);
}

void SpinCtrlEventHandler::OnSpin(wxSpinEvent &event)
{
	PrivSpinEvent::Fire<SpinEvent>(event, WXJS_SPIN_EVENT);
}

void SpinCtrlEventHandler::OnSpinUp(wxSpinEvent &event)
{
	PrivSpinEvent::Fire<SpinEvent>(event, WXJS_SPIN_UP_EVENT);
}

void SpinCtrlEventHandler::OnSpinDown(wxSpinEvent &event)
{
	PrivSpinEvent::Fire<SpinEvent>(event, WXJS_SPIN_DOWN_EVENT);
}

void SpinCtrlEventHandler::ConnectText(wxSpinCtrl *p, bool connect)
{
	if ( connect )
	{
		p->Connect(wxEVT_COMMAND_TEXT_UPDATED,
				wxCommandEventHandler(OnText));
	}
	else
	{
		p->Disconnect(wxEVT_COMMAND_TEXT_UPDATED,
				wxCommandEventHandler(OnText));
	}
}

void SpinCtrlEventHandler::ConnectSpinCtrl(wxSpinCtrl *p, bool connect)
{
	if ( connect )
	{
		p->Connect(wxEVT_COMMAND_SPINCTRL_UPDATED,
				wxSpinEventHandler(OnSpinCtrl));
	}
	else
	{
		p->Disconnect(wxEVT_COMMAND_SPINCTRL_UPDATED,
				wxSpinEventHandler(OnSpinCtrl));
	}
}

void SpinCtrlEventHandler::ConnectSpin(wxSpinCtrl *p, bool connect)
{
	if ( connect )
	{
		p->Connect(wxEVT_SCROLL_THUMBTRACK,
				wxSpinEventHandler(OnSpin));
	}
	else
	{
		p->Disconnect(wxEVT_SCROLL_THUMBTRACK,
				wxSpinEventHandler(OnSpin));
	}
}

void SpinCtrlEventHandler::ConnectSpinUp(wxSpinCtrl *p, bool connect)
{
	if ( connect )
	{
		p->Connect(wxEVT_SCROLL_LINEUP,
				wxSpinEventHandler(OnSpinUp));
	}
	else
	{
		p->Disconnect(wxEVT_SCROLL_LINEUP,
				wxSpinEventHandler(OnSpinUp));
	}
}

void SpinCtrlEventHandler::ConnectSpinDown(wxSpinCtrl *p, bool connect)
{
	if ( connect )
	{
		p->Connect(wxEVT_SCROLL_LINEDOWN,
				wxSpinEventHandler(OnSpinDown));
	}
	else
	{
		p->Disconnect(wxEVT_SCROLL_LINEDOWN,
				wxSpinEventHandler(OnSpinDown));
	}
}


void SpinCtrlEventHandler::InitConnectEventMap()
{
	AddConnector(WXJS_TEXT_EVENT, ConnectText);
	AddConnector(WXJS_SPIN_CTRL_EVENT, ConnectSpinCtrl);
	AddConnector(WXJS_SPIN_EVENT, ConnectSpin);
	AddConnector(WXJS_SPIN_UP_EVENT, ConnectSpinUp);
	AddConnector(WXJS_SPIN_DOWN_EVENT, ConnectSpinDown);
}
