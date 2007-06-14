#include "precompiled.h"

#ifndef WX_PRECOMP
    #include <wx/wx.h>
#endif

#include "../../common/main.h"

#include "bookctrl.h"
#include "window.h"

using namespace wxjs;
using namespace wxjs::gui;

WXJS_INIT_CLASS(BookCtrlBase, "wxBookCtrlBase", 0)

WXJS_BEGIN_CONSTANT_MAP(BookCtrlBase)
	WXJS_CONSTANT(wxBK_, HITTEST_NOWHERE)
	WXJS_CONSTANT(wxBK_, HITTEST_ONICON)
	WXJS_CONSTANT(wxBK_, HITTEST_ONLABEL)
	WXJS_CONSTANT(wxBK_, HITTEST_ONITEM)
	WXJS_CONSTANT(wxBK_, HITTEST_ONPAGE)
	WXJS_CONSTANT(wxBK_, DEFAULT)
	WXJS_CONSTANT(wxBK_, TOP)
	WXJS_CONSTANT(wxBK_, BOTTOM)
	WXJS_CONSTANT(wxBK_, LEFT)
	WXJS_CONSTANT(wxBK_, RIGHT)
WXJS_END_CONSTANT_MAP()

WXJS_BEGIN_PROPERTY_MAP(BookCtrlBase)
	WXJS_READONLY_PROPERTY(P_PAGE_COUNT, "pageCount")
WXJS_END_PROPERTY_MAP()

bool BookCtrlBase::GetProperty(wxBookCtrlBase *p, JSContext *cx, JSObject *obj, int id, jsval *vp)
{
	switch (id)
	{
	case P_PAGE_COUNT:
		*vp = ToJS(cx, p->GetPageCount());
		break;
	}
	return true;
}

bool BookCtrlBase::SetProperty(wxBookCtrlBase *p, JSContext *cx, JSObject *obj, int id, jsval *vp)
{
	return true;
}

WXJS_BEGIN_METHOD_MAP(BookCtrlBase)
  WXJS_METHOD("addPage", addPage, 2)
  WXJS_METHOD("deleteAllPages", deleteAllPages, 0)
  WXJS_METHOD("getPage", getPage, 1)
WXJS_END_METHOD_MAP()

JSBool BookCtrlBase::addPage(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
	wxBookCtrlBase *p = GetPrivate(cx, obj);
	if ( p == NULL )
		return JS_FALSE;

	wxWindow *win;
	wxString text;
	bool select = false;
	int imageId = -1;
	
	if ( argc >= 4 )
	{
		if ( ! FromJS(cx, argv[3], imageId) )
			return JS_FALSE;
	}
	if ( argc >= 3 )
	{
		if ( ! FromJS(cx, argv[2], select) )
			return JS_FALSE;
	}
	
	FromJS(cx, argv[1], text);
	win = Window::GetPrivate(cx, argv[0]);
	if ( win == NULL )
		return JS_FALSE;
	
	p->AddPage(win, text, select, imageId);

	return JS_TRUE;
}

JSBool BookCtrlBase::deleteAllPages(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
	wxBookCtrlBase *p = GetPrivate(cx, obj);
	if ( p == NULL )
		return JS_FALSE;

	p->DeleteAllPages();
	
	return JS_TRUE;
}

JSBool BookCtrlBase::getPage(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
	wxBookCtrlBase *p = GetPrivate(cx, obj);
	if ( p == NULL )
		return JS_FALSE;
	
	size_t page;
	if ( ! FromJS(cx, argv[0], page) )
		return JS_FALSE;
	
	wxWindow *win = p->GetPage(page);
	
	JavaScriptClientData *data
		= dynamic_cast<JavaScriptClientData*>(win->GetClientObject());
	if ( data == NULL )
		return JS_FALSE;
	
	*rval = OBJECT_TO_JSVAL(data->GetObject());

	return JS_TRUE;
}
