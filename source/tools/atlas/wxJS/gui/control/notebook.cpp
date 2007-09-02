#include "precompiled.h"

#ifndef WX_PRECOMP
    #include <wx/wx.h>
#endif

#include "../../common/main.h"

#include "../event/jsevent.h"
#include "../event/notebookevt.h"

#include "notebook.h"
#include "bookctrl.h"
#include "window.h"

#include "../misc/point.h"
#include "../misc/size.h"
#include "../errors.h"

using namespace wxjs;
using namespace wxjs::gui;

WXJS_INIT_CLASS(Notebook, "wxNotebook", 2)

void Notebook::InitClass(JSContext* WXUNUSED(cx),
						 JSObject* WXUNUSED(obj),
						 JSObject* WXUNUSED(proto))
{
	NotebookEventHandler::InitConnectEventMap();
}

WXJS_BEGIN_CONSTANT_MAP(Notebook)
	WXJS_CONSTANT(wxNB_, HITTEST_NOWHERE)
	WXJS_CONSTANT(wxNB_, HITTEST_ONICON)
	WXJS_CONSTANT(wxNB_, HITTEST_ONLABEL)
	WXJS_CONSTANT(wxNB_, HITTEST_ONITEM)
	WXJS_CONSTANT(wxNB_, HITTEST_ONPAGE)
	WXJS_CONSTANT(wxNB_, DEFAULT)
	WXJS_CONSTANT(wxNB_, TOP)
	WXJS_CONSTANT(wxNB_, BOTTOM)
	WXJS_CONSTANT(wxNB_, LEFT)
	WXJS_CONSTANT(wxNB_, RIGHT)
	WXJS_CONSTANT(wxNB_, FIXEDWIDTH)
	WXJS_CONSTANT(wxNB_, MULTILINE)
	WXJS_CONSTANT(wxNB_, NOPAGETHEME)
	WXJS_CONSTANT(wxNB_, FLAT)
WXJS_END_CONSTANT_MAP()

WXJS_BEGIN_PROPERTY_MAP(Notebook)
	WXJS_READONLY_PROPERTY(P_ROW_COUNT, "rowCount")
WXJS_END_PROPERTY_MAP()

bool Notebook::GetProperty(wxNotebook *p, JSContext *cx, JSObject *obj, int id, jsval *vp)
{
	switch (id)
	{
	case P_ROW_COUNT:
		*vp = ToJS(cx, p->GetRowCount());
		break;
	}
	return true;
}

bool Notebook::SetProperty(wxNotebook *p, JSContext *cx, JSObject *obj, int id, jsval *vp)
{
	return true;
}

bool Notebook::AddProperty(wxNotebook *p, 
						   JSContext* WXUNUSED(cx), 
						   JSObject* WXUNUSED(obj), 
						   const wxString &prop, 
						   jsval* WXUNUSED(vp))
{
	if ( WindowEventHandler::ConnectEvent(p, prop, true) )
		return true;

	NotebookEventHandler::ConnectEvent(p, prop, true);

	return true;
}

bool Notebook::DeleteProperty(wxNotebook *p, 
							  JSContext* WXUNUSED(cx), 
							  JSObject* WXUNUSED(obj), 
							  const wxString &prop)
{
	if ( WindowEventHandler::ConnectEvent(p, prop, false) )
		return true;
	
	NotebookEventHandler::ConnectEvent(p, prop, false);
	return true;
}

// HACK: so I don't get PAGE_CHANGED events when ~wxNotebook is calling DeleteAllPages
struct Wrapper_wxNotebook : public wxNotebook
{
	~Wrapper_wxNotebook()
	{
		wxList* d = GetDynamicEventTable();
		if (d)
		{
			for (wxList::iterator it = d->begin(), end = d->end(); it != end; ++it)
			{
				wxDynamicEventTableEntry *entry = (wxDynamicEventTableEntry*)*it;
				delete entry->m_callbackUserData;
				delete entry;
			}
			d->Clear();
		}
	}
};

wxNotebook* Notebook::Construct(JSContext *cx,
								JSObject *obj,
								uintN argc,
								jsval *argv,
								bool WXUNUSED(constructing))
{
	wxNotebook *p = new Wrapper_wxNotebook();
	SetPrivate(cx, obj, p);
	
	if ( argc > 0 )
	{
		jsval rval;
		if ( ! create(cx, obj, argc, argv, &rval) )
			return NULL;
	}
	return p;
}

WXJS_BEGIN_METHOD_MAP(Notebook)
	WXJS_METHOD("create", create, 2)
WXJS_END_METHOD_MAP()

JSBool Notebook::create(JSContext *cx,
						JSObject *obj,
						uintN argc,
						jsval *argv,
						jsval *rval)
{
	wxNotebook *p = GetPrivate(cx, obj);
	*rval = JSVAL_FALSE;

	if ( argc > 5 )
		argc = 5;

	wxWindowID id = -1;
	const wxPoint *pt = &wxDefaultPosition;
	const wxSize *size = &wxDefaultSize;
	int style = 0;

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

WXJS_INIT_EVENT_MAP(wxNotebook)
const wxString WXJS_PAGE_CHANGED_EVENT = wxT("onPageChanged");
const wxString WXJS_PAGE_CHANGING_EVENT = wxT("onPageChanging");

void NotebookEventHandler::OnPageChanged(wxNotebookEvent &event)
{
	PrivNotebookEvent::Fire<NotebookEvent>(event, WXJS_PAGE_CHANGED_EVENT);
}

void NotebookEventHandler::OnPageChanging(wxNotebookEvent &event)
{
	PrivNotebookEvent::Fire<NotebookEvent>(event, WXJS_PAGE_CHANGING_EVENT);
}

void NotebookEventHandler::ConnectPageChanged(wxNotebook *p, bool connect)
{
	if ( connect )
	{
		p->Connect(wxEVT_COMMAND_NOTEBOOK_PAGE_CHANGED,
				   wxNotebookEventHandler(NotebookEventHandler::OnPageChanged));
	}
	else
	{
		p->Disconnect(wxEVT_COMMAND_NOTEBOOK_PAGE_CHANGED,
					  wxNotebookEventHandler(NotebookEventHandler::OnPageChanged));
	}
}

void NotebookEventHandler::ConnectPageChanging(wxNotebook *p, bool connect)
{
	if ( connect )
	{
		p->Connect(wxEVT_COMMAND_NOTEBOOK_PAGE_CHANGING,
				   wxNotebookEventHandler(NotebookEventHandler::OnPageChanging));
	}
	else
	{
		p->Disconnect(wxEVT_COMMAND_NOTEBOOK_PAGE_CHANGING,
					  wxNotebookEventHandler(NotebookEventHandler::OnPageChanging));
	}
}

void NotebookEventHandler::InitConnectEventMap()
{
	AddConnector(WXJS_PAGE_CHANGED_EVENT, ConnectPageChanged);
	AddConnector(WXJS_PAGE_CHANGING_EVENT, ConnectPageChanging);
}
