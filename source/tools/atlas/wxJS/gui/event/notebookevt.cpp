#include "precompiled.h"

#ifndef WX_PRECOMP
	#include <wx/wx.h>
#endif

#include "../../common/main.h"
#include "../../common/apiwrap.h"

#include "jsevent.h"
#include "../misc/size.h"
#include "notebookevt.h"

using namespace wxjs;
using namespace wxjs::gui;

WXJS_INIT_CLASS(NotebookEvent, "wxNotebookEvent", 0)

WXJS_BEGIN_PROPERTY_MAP(NotebookEvent)
	WXJS_PROPERTY(P_OLD_SELECTION, "oldSelection")
	WXJS_PROPERTY(P_SELECTION, "selection")
WXJS_END_PROPERTY_MAP()

bool NotebookEvent::GetProperty(PrivNotebookEvent *p,
								JSContext *cx,
								JSObject* WXUNUSED(obj),
								int id,
								jsval *vp)
{
	wxNotebookEvent *event = (wxNotebookEvent*) p->GetEvent();

	switch ( id )
	{
	case P_OLD_SELECTION:
		*vp = ToJS(cx, event->GetOldSelection());
		break;
	case P_SELECTION:
		*vp = ToJS(cx, event->GetSelection());
		break;
	}
	return true;
}

bool NotebookEvent::SetProperty(PrivNotebookEvent *p,
								JSContext *cx,
								JSObject* WXUNUSED(obj),
								int id,
								jsval *vp)
{
	wxNotebookEvent *event = (wxNotebookEvent*) p->GetEvent();

	switch ( id )
	{
	case P_OLD_SELECTION:
		{
			int sel;
			if ( FromJS(cx, *vp, sel) )
				event->SetOldSelection(sel);
			break;
		}
	case P_SELECTION:
		{
			int sel;
			if ( FromJS(cx, *vp, sel) )
				event->SetSelection(sel);
			break;
		}
	}
	return true;
}
