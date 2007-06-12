#include "precompiled.h"

#ifndef WX_PRECOMP
    #include <wx/wx.h>
#endif

#include "../../common/main.h"
#include "../../common/jsutil.h"

#include "settings.h"

#include "colour.h"
#include "font.h"
#include "../control/window.h"

using namespace wxjs;
using namespace wxjs::gui;

WXJS_INIT_CLASS(SystemSettings, "wxSystemSettings", 0)

WXJS_BEGIN_STATIC_PROPERTY_MAP(SystemSettings)
    WXJS_STATIC_PROPERTY(P_SCREEN_TYPE, "screenType")
WXJS_END_PROPERTY_MAP()

bool SystemSettings::GetStaticProperty(JSContext *cx, int id, jsval *vp)
{
	switch ( id )
	{
	case P_SCREEN_TYPE:
		*vp = ToJS(cx, static_cast<int>(wxSystemSettings::GetScreenType()));
		break;
	}
	return true;
}

bool SystemSettings::SetStaticProperty(JSContext *cx, int id, jsval *vp)
{
	switch ( id )
	{
	case P_SCREEN_TYPE:
		int screenType;
		if (! FromJS(cx, *vp, screenType) )
			return false;
		wxSystemSettings::SetScreenType(static_cast<wxSystemScreenType>(screenType));
		break;
	}
	return true;
}

WXJS_BEGIN_STATIC_METHOD_MAP(SystemSettings)
	WXJS_METHOD("getColour", getColour, 1)
	WXJS_METHOD("getFont", getFont, 1)
	WXJS_METHOD("getMetric", getMetric, 1)
	WXJS_METHOD("hasFeature", hasFeature, 1)
WXJS_END_METHOD_MAP()

JSBool SystemSettings::getColour(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
	int index;
	if (! FromJS(cx, argv[0], index))
		return JS_FALSE;
	wxColour colour = wxSystemSettings::GetColour(static_cast<wxSystemColour>(index));
	*rval = Colour::CreateObject(cx, new wxColour(colour));
	return JS_TRUE;
}

JSBool SystemSettings::getFont(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
	int index;
	if (! FromJS(cx, argv[0], index))
		return JS_FALSE;
	wxFont font = wxSystemSettings::GetFont(static_cast<wxSystemFont>(index));
	*rval = Font::CreateObject(cx, new wxFont(font));
	return JS_TRUE;
}

JSBool SystemSettings::getMetric(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
	int index;
	wxWindow* window = NULL;
	if ( argc >= 2 && ! JSVAL_IS_NULL(argv[1]) )
	{
		window = Window::GetPrivate(cx, argv[1]);
		if ( window == NULL )
			return JS_FALSE;
	}
	if (! FromJS(cx, argv[0], index))
		return JS_FALSE;
	int metric = wxSystemSettings::GetMetric(static_cast<wxSystemMetric>(index), window);
	*rval = ToJS(cx, metric);
	return JS_TRUE;
}

JSBool SystemSettings::hasFeature(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
	int index;
	if (! FromJS(cx, argv[0], index))
		return JS_FALSE;
	bool present = wxSystemSettings::HasFeature(static_cast<wxSystemFeature>(index));
	*rval = ToJS(cx, present);
	return JS_TRUE;
}

WXJS_BEGIN_CONSTANT_MAP(SystemSettings)
	// wxSystemFont
	WXJS_CONSTANT(wxSYS_, OEM_FIXED_FONT)
	WXJS_CONSTANT(wxSYS_, ANSI_FIXED_FONT)
	WXJS_CONSTANT(wxSYS_, ANSI_VAR_FONT)
	WXJS_CONSTANT(wxSYS_, SYSTEM_FONT)
	WXJS_CONSTANT(wxSYS_, DEVICE_DEFAULT_FONT)
	WXJS_CONSTANT(wxSYS_, DEFAULT_PALETTE)
	WXJS_CONSTANT(wxSYS_, SYSTEM_FIXED_FONT)
	WXJS_CONSTANT(wxSYS_, DEFAULT_GUI_FONT)
	// wxSystemColour
	WXJS_CONSTANT(wxSYS_, COLOUR_SCROLLBAR)
	WXJS_CONSTANT(wxSYS_, COLOUR_BACKGROUND)
	WXJS_CONSTANT(wxSYS_, COLOUR_DESKTOP)
	WXJS_CONSTANT(wxSYS_, COLOUR_ACTIVECAPTION)
	WXJS_CONSTANT(wxSYS_, COLOUR_INACTIVECAPTION)
	WXJS_CONSTANT(wxSYS_, COLOUR_MENU)
	WXJS_CONSTANT(wxSYS_, COLOUR_WINDOW)
	WXJS_CONSTANT(wxSYS_, COLOUR_WINDOWFRAME)
	WXJS_CONSTANT(wxSYS_, COLOUR_MENUTEXT)
	WXJS_CONSTANT(wxSYS_, COLOUR_WINDOWTEXT)
	WXJS_CONSTANT(wxSYS_, COLOUR_CAPTIONTEXT)
	WXJS_CONSTANT(wxSYS_, COLOUR_ACTIVEBORDER)
	WXJS_CONSTANT(wxSYS_, COLOUR_INACTIVEBORDER)
	WXJS_CONSTANT(wxSYS_, COLOUR_APPWORKSPACE)
	WXJS_CONSTANT(wxSYS_, COLOUR_HIGHLIGHT)
	WXJS_CONSTANT(wxSYS_, COLOUR_HIGHLIGHTTEXT)
	WXJS_CONSTANT(wxSYS_, COLOUR_BTNFACE)
	WXJS_CONSTANT(wxSYS_, COLOUR_3DFACE)
	WXJS_CONSTANT(wxSYS_, COLOUR_BTNSHADOW)
	WXJS_CONSTANT(wxSYS_, COLOUR_3DSHADOW)
	WXJS_CONSTANT(wxSYS_, COLOUR_GRAYTEXT)
	WXJS_CONSTANT(wxSYS_, COLOUR_BTNTEXT)
	WXJS_CONSTANT(wxSYS_, COLOUR_INACTIVECAPTIONTEXT)
	WXJS_CONSTANT(wxSYS_, COLOUR_BTNHIGHLIGHT)
	WXJS_CONSTANT(wxSYS_, COLOUR_BTNHILIGHT)
	WXJS_CONSTANT(wxSYS_, COLOUR_3DHIGHLIGHT)
	WXJS_CONSTANT(wxSYS_, COLOUR_3DHILIGHT)
	WXJS_CONSTANT(wxSYS_, COLOUR_3DDKSHADOW)
	WXJS_CONSTANT(wxSYS_, COLOUR_3DLIGHT)
	WXJS_CONSTANT(wxSYS_, COLOUR_INFOTEXT)
	WXJS_CONSTANT(wxSYS_, COLOUR_INFOBK)
	WXJS_CONSTANT(wxSYS_, COLOUR_LISTBOX)
	WXJS_CONSTANT(wxSYS_, COLOUR_HOTLIGHT)
	WXJS_CONSTANT(wxSYS_, COLOUR_GRADIENTACTIVECAPTION)
	WXJS_CONSTANT(wxSYS_, COLOUR_GRADIENTINACTIVECAPTION)
	WXJS_CONSTANT(wxSYS_, COLOUR_MENUHILIGHT)
	WXJS_CONSTANT(wxSYS_, COLOUR_MENUBAR)
	// wxSystemMetric
	WXJS_CONSTANT(wxSYS_, MOUSE_BUTTONS)
	WXJS_CONSTANT(wxSYS_, BORDER_X)
	WXJS_CONSTANT(wxSYS_, BORDER_Y)
	WXJS_CONSTANT(wxSYS_, CURSOR_X)
	WXJS_CONSTANT(wxSYS_, CURSOR_Y)
	WXJS_CONSTANT(wxSYS_, DCLICK_X)
	WXJS_CONSTANT(wxSYS_, DCLICK_Y)
	WXJS_CONSTANT(wxSYS_, DRAG_X)
	WXJS_CONSTANT(wxSYS_, DRAG_Y)
	WXJS_CONSTANT(wxSYS_, EDGE_X)
	WXJS_CONSTANT(wxSYS_, EDGE_Y)
	WXJS_CONSTANT(wxSYS_, HSCROLL_ARROW_X)
	WXJS_CONSTANT(wxSYS_, HSCROLL_ARROW_Y)
	WXJS_CONSTANT(wxSYS_, HTHUMB_X)
	WXJS_CONSTANT(wxSYS_, ICON_X)
	WXJS_CONSTANT(wxSYS_, ICON_Y)
	WXJS_CONSTANT(wxSYS_, ICONSPACING_X)
	WXJS_CONSTANT(wxSYS_, ICONSPACING_Y)
	WXJS_CONSTANT(wxSYS_, WINDOWMIN_X)
	WXJS_CONSTANT(wxSYS_, WINDOWMIN_Y)
	WXJS_CONSTANT(wxSYS_, SCREEN_X)
	WXJS_CONSTANT(wxSYS_, SCREEN_Y)
	WXJS_CONSTANT(wxSYS_, FRAMESIZE_X)
	WXJS_CONSTANT(wxSYS_, FRAMESIZE_Y)
	WXJS_CONSTANT(wxSYS_, SMALLICON_X)
	WXJS_CONSTANT(wxSYS_, SMALLICON_Y)
	WXJS_CONSTANT(wxSYS_, HSCROLL_Y)
	WXJS_CONSTANT(wxSYS_, VSCROLL_X)
	WXJS_CONSTANT(wxSYS_, VSCROLL_ARROW_X)
	WXJS_CONSTANT(wxSYS_, VSCROLL_ARROW_Y)
	WXJS_CONSTANT(wxSYS_, VTHUMB_Y)
	WXJS_CONSTANT(wxSYS_, CAPTION_Y)
	WXJS_CONSTANT(wxSYS_, MENU_Y)
	WXJS_CONSTANT(wxSYS_, NETWORK_PRESENT)
	WXJS_CONSTANT(wxSYS_, PENWINDOWS_PRESENT)
	WXJS_CONSTANT(wxSYS_, SHOW_SOUNDS)
	WXJS_CONSTANT(wxSYS_, SWAP_BUTTONS)
	// wxSystemFeature
	WXJS_CONSTANT(wxSYS_, CAN_DRAW_FRAME_DECORATIONS)
	WXJS_CONSTANT(wxSYS_, CAN_ICONIZE_FRAME)
	WXJS_CONSTANT(wxSYS_, TABLET_PRESENT)
	// wxSystemScreenType
	WXJS_CONSTANT(wxSYS_, SCREEN_NONE)
	WXJS_CONSTANT(wxSYS_, SCREEN_TINY)
	WXJS_CONSTANT(wxSYS_, SCREEN_PDA)
	WXJS_CONSTANT(wxSYS_, SCREEN_SMALL)
	WXJS_CONSTANT(wxSYS_, SCREEN_DESKTOP)
WXJS_END_CONSTANT_MAP()
