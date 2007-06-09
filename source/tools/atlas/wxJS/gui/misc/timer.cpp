#include "precompiled.h"

#ifndef WX_PRECOMP
	#include <wx/wx.h>
#endif

#include "../../common/main.h"

#include "../event/jsevent.h"
#include "../control/window.h"

#include "timer.h"

using namespace wxjs;
using namespace wxjs::gui;

WXJS_INIT_CLASS(Timer, "wxTimer", 0)

WXJS_BEGIN_PROPERTY_MAP(Timer)
	WXJS_READONLY_PROPERTY(P_INTERVAL, "interval")
	WXJS_READONLY_PROPERTY(P_ONESHOT, "oneShot")
	WXJS_READONLY_PROPERTY(P_RUNNING, "running")
WXJS_END_PROPERTY_MAP()

bool Timer::GetProperty(wxTimer *p, JSContext *cx, JSObject *obj, int id, jsval *vp)
{
	switch (id)
	{
	case P_INTERVAL:
		*vp = ToJS(cx, p->GetInterval());
		break;
	case P_ONESHOT:
		*vp = ToJS(cx, p->IsOneShot());
		break;
	case P_RUNNING:
		*vp = ToJS(cx, p->IsRunning());
		break;
	}
	return true;
}

bool Timer::SetProperty(wxTimer *p, JSContext *cx, JSObject *obj, int id, jsval *vp)
{
	return true;
}

WXJS_BEGIN_METHOD_MAP(Timer)
	WXJS_METHOD("start", start, 1)
	WXJS_METHOD("stop", stop, 0)
WXJS_END_METHOD_MAP()

wxTimer *Timer::Construct(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, bool constructing)
{
	Timer *p = new Timer();
	p->SetClientObject(new JavaScriptClientData(cx, obj, false, true));
	
	/*
	Allow an optional argument, identifying a window to link this timer to.
	When that window is destroyed, the timer will be stopped. (This is useful
	since timers typically depend on a certain window existing, when normally
	you'd store it as a C++ class member so it'll be stopped in the destructor.)
	If the argument is skipped, the timer will continue running for as long as
	possible.
	*/
	if ( argc >= 1 )
	{
		if ( Window::HasPrototype(cx, argv[0]) )
		{
			wxWindow *win = Window::GetPrivate(cx, argv[0], false);
			// Attach using the window's ID to avoid seeing propagated events
			win->Connect(win->GetId(), wxEVT_DESTROY, wxWindowDestroyEventHandler(Timer::OnWindowDestroy), NULL, p);
		}	
	}
	
	return p;
}

void Timer::OnWindowDestroy(wxWindowDestroyEvent &event)
{
	Stop();

	JavaScriptClientData *data 
			= dynamic_cast<JavaScriptClientData*>(GetClientObject());
	if ( data != NULL )
	{
		data->Protect(false);
	}
	
	event.Skip();
}

JSBool Timer::start(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
	wxTimer *timer = Timer::GetPrivate(cx, obj);
	
	int interval;
	bool oneShot = false;
	
	if ( ! FromJS(cx, argv[0], interval) )
		return JS_FALSE;

	if ( argc > 1 )
		if ( ! FromJS(cx, argv[1], oneShot) )
			return JS_FALSE;

	*rval = ToJS(cx, timer->Start(interval, oneShot));

	// Don't allow a running timer to be GCed
	JavaScriptClientData *data 
			= dynamic_cast<JavaScriptClientData*>(timer->GetClientObject());
	if ( data != NULL )
	{
		data->Protect(true);
	}

	return JS_TRUE;
}

JSBool Timer::stop(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
	wxTimer *timer = Timer::GetPrivate(cx, obj);
	
	timer->Stop();

	// Allow the timer to be GCed when it's not running
	JavaScriptClientData *data 
			= dynamic_cast<JavaScriptClientData*>(timer->GetClientObject());
	if ( data != NULL )
	{
		data->Protect(false);
	}

	return JS_TRUE;
}

void Timer::Notify()
{
	JavaScriptClientData *data 
			= dynamic_cast<JavaScriptClientData*>(GetClientObject());
	if ( data == NULL )
		return;
	
	// If this was a one-shot timer, it's not running any more, so allow it to be GCed
	if ( IsOneShot() )
	{
		data->Protect(false);
	}
	
	jsval fval;
	if ( GetFunctionProperty(data->GetContext(), data->GetObject(), "onNotify", &fval) == JS_TRUE )
	{
		jsval rval;
		if ( JS_CallFunctionValue(data->GetContext(), data->GetObject(), fval, 0, NULL, &rval) == JS_FALSE )
		{
			if ( JS_IsExceptionPending(data->GetContext()) )
			{
				JS_ReportPendingException(data->GetContext());
			}
		}
	}
}
