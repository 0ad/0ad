#include "precompiled.h"

/*
 * wxJavaScript - init.cpp
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
 * $Id$
 */
// main.cpp
#include <wx/setup.h>

#ifndef WX_PRECOMP
    #include <wx/wx.h>
#endif
#include <wx/intl.h>

#if defined(__WXMSW__)
	#include <wx/msw/private.h>
#endif
#include <wx/calctrl.h>

#if defined(__WXMSW__)
    #include <wx/msw/ole/automtn.h>
#endif

#include <js/jsapi.h>
#include "../common/wxjs.h"
#include "../common/main.h"
#include "../common/jsutil.h"
#include "../common/index.h"
 
// All wxJS objects
#include "control/window.h"
#include "control/toplevel.h"
#include "control/frame.h"
#include "control/mdi.h"
#include "control/mdichild.h"
#include "control/dialog.h"
#include "control/menu.h"
#include "control/menuitem.h"
#include "control/menubar.h"

// Controls
#include "control/control.h"
#include "control/textctrl.h"
#include "control/button.h"
#include "control/bmpbtn.h"
#include "control/sttext.h"
#include "control/staticbx.h"
#include "control/checkbox.h"
#include "control/ctrlitem.h"
#include "control/item.h"
#include "control/listbox.h"
#include "control/chklstbx.h"
#include "control/chklstbxchk.h"
#include "control/choice.h"
#include "control/combobox.h"
#include "control/calendar.h"
#include "control/caldate.h"
#include "control/gauge.h"
#include "control/radiobox.h"
#include "control/radiobtn.h"
#include "control/slider.h"
#include "control/helpbtn.h"
#include "control/splitwin.h"
#include "control/statbar.h"
#include "control/toolbar.h"
#include "control/txtdlg.h"
#include "control/pwdlg.h"
#include "control/scrollwnd.h"
#include "control/htmlwin.h"
#include "control/spinctrl.h"
#include "control/spinbtn.h"
#include "control/bookctrl.h"
#include "control/notebook.h"

// Validators
#include "misc/validate.h"
#include "misc/genval.h"
#include "misc/textval.h"

// Sizers
#include "misc/sizer.h"
#include "misc/gridszr.h"
#include "misc/flexgrid.h"
#include "misc/boxsizer.h"
#include "misc/stsizer.h"

// Dialogs
#include "control/panel.h"
#include "control/filedlg.h"
#include "control/dirdlg.h"
#include "control/coldata.h"
#include "control/coldlg.h"
#include "control/fontdata.h"
#include "control/fontdlg.h"
#include "control/findrdlg.h"
#include "control/finddata.h"

// Miscellaneous wxWindow classes
#include "misc/size.h"
#include "misc/rect.h"
#include "misc/accentry.h"
#include "misc/acctable.h"
#include "misc/colour.h"
#include "misc/font.h"
#include "misc/fontenum.h"
#include "misc/fontlist.h"
#include "misc/bitmap.h"
#include "misc/image.h"
#include "misc/imghand.h"
#include "misc/icon.h"
#include "misc/colourdb.h"
#include "misc/cshelp.h"
#include "misc/constant.h"
#if defined(__WXMSW__)
    #include "misc/autoobj.h"
#endif
#include "misc/settings.h"
#include "misc/timer.h"

// Events
#include "event/jsevent.h"

// Common Controls
#include "misc/cmnconst.h"
#include "misc/imagelst.h"
#include "control/listctrl.h"
#include "control/listhit.h"
#include "control/listitem.h"
#include "control/listitattr.h"
#include "control/treectrl.h"
#include "control/treeitem.h"
#include "control/treeid.h"
#include "control/treehit.h"

#include "misc/globfun.h"
#include "init.h"

using namespace wxjs;
using namespace wxjs::gui;

bool wxjs::gui::InitClass(JSContext *cx, JSObject *global)
{
    InitGuiConstants(cx, global);

    JSObject *obj;

    // Initialize wxJS JavaScript objects
	obj = Window::JSInit(cx, global);
	wxASSERT_MSG(obj != NULL, wxT("wxWindow prototype creation failed"));
	if (! obj)
		return false;

	obj = Control::JSInit(cx, global, Window::GetClassPrototype());
	wxASSERT_MSG(obj != NULL, wxT("wxControl prototype creation failed"));
	if (! obj)
		return false;

	obj = TopLevelWindow::JSInit(cx, global, Window::GetClassPrototype());
	wxASSERT_MSG(obj != NULL, wxT("wxTopLevelWindow prototype creation failed"));
	if (! obj )
		return false;

	obj = Frame::JSInit(cx, global, TopLevelWindow::GetClassPrototype());
	wxASSERT_MSG(obj != NULL, wxT("wxFrame prototype creation failed"));
	if (! obj )
		return false;

    obj = MDIParentFrame::JSInit(cx, global, Frame::GetClassPrototype());
	wxASSERT_MSG(obj != NULL, wxT("wxMDIParentFrame prototype creation failed"));
	if (! obj )
		return false;

    obj = MDIChildFrame::JSInit(cx, global, Frame::GetClassPrototype());
	wxASSERT_MSG(obj != NULL, wxT("wxMDIChildFrame prototype creation failed"));
	if (! obj )
		return false;

	obj = Menu::JSInit(cx, global);
	wxASSERT_MSG(obj != NULL, wxT("wxMenu prototype creation failed"));
	if (! obj )
		return false;

	obj = MenuItem::JSInit(cx, global);
	wxASSERT_MSG(obj != NULL, wxT("wxMenuItem prototype creation failed"));
	if (! obj )
		return false;

	obj = MenuBar::JSInit(cx, global);
	wxASSERT_MSG(obj != NULL, wxT("wxMenuBar prototype creation failed"));
	if (! obj )
		return false;

	obj = TextCtrl::JSInit(cx, global, Control::GetClassPrototype());
	wxASSERT_MSG(obj != NULL, wxT("wxTextCtrl prototype creation failed"));
	if (! obj )
		return false;

	obj = Button::JSInit(cx, global, Control::GetClassPrototype());
	wxASSERT_MSG(obj != NULL, wxT("wxButton prototype creation failed"));
	if (! obj )
		return false;

	obj = BitmapButton::JSInit(cx, global, Button::GetClassPrototype());
	wxASSERT_MSG(obj != NULL, wxT("wxBitmapButton prototype creation failed"));
	if (! obj )
		return false;

	obj = ContextHelpButton::JSInit(cx, global, BitmapButton::GetClassPrototype());
	wxASSERT_MSG(obj != NULL, wxT("wxContextHelptButton prototype creation failed"));
	if (! obj )
		return false;

    obj = StaticText::JSInit(cx, global, Control::GetClassPrototype());
	wxASSERT_MSG(obj != NULL, wxT("wxStaticText prototype creation failed"));
	if (! obj )
		return false;

	obj = StaticBox::JSInit(cx, global, Control::GetClassPrototype());
	wxASSERT_MSG(obj != NULL, wxT("wxStaticBox prototype creation failed"));
	if (! obj )
		return false;

	obj = CheckBox::JSInit(cx, global, Control::GetClassPrototype());
	wxASSERT_MSG(obj != NULL, wxT("wxCheckBox prototype creation failed"));
	if (! obj )
		return false;

	obj = ControlWithItems::JSInit(cx, global, Control::GetClassPrototype());
	wxASSERT_MSG(obj != NULL, wxT("wxControlWithItems prototype creation failed"));
	if (! obj )
		return false;

	obj = ListBox::JSInit(cx, global, ControlWithItems::GetClassPrototype());
	wxASSERT_MSG(obj != NULL, wxT("wxListBox prototype creation failed"));
	if (! obj )
		return false;

	obj = CheckListBox::JSInit(cx, global, ListBox::GetClassPrototype());
	wxASSERT_MSG(obj != NULL, wxT("wxCheckListBox prototype creation failed"));
	if (! obj )
		return false;
	
	obj = CheckListBoxItem::JSInit(cx, global);
	wxASSERT_MSG(obj != NULL, wxT("wxCheckListBoxItem prototype creation failed"));
	if (! obj )
		return false;

	obj = Choice::JSInit(cx, global, ControlWithItems::GetClassPrototype());
	wxASSERT_MSG(obj != NULL, wxT("wxChoice prototype creation failed"));
	if (! obj )
		return false;

	obj = CalendarCtrl::JSInit(cx, global, Control::GetClassPrototype());
	wxASSERT_MSG(obj != NULL, wxT("wxCalendarCtrl prototype creation failed"));
	if (! obj )
		return false;

	obj = Gauge::JSInit(cx, global, Control::GetClassPrototype());
	wxASSERT_MSG(obj != NULL, wxT("wxGauge prototype creation failed"));
	if (! obj )
		return false;

	obj = RadioBox::JSInit(cx, global, Control::GetClassPrototype());
	wxASSERT_MSG(obj != NULL, wxT("wxRadioBox prototype creation failed"));
	if (! obj )
		return false;

	obj = RadioButton::JSInit(cx, global, Control::GetClassPrototype());
	wxASSERT_MSG(obj != NULL, wxT("wxRadioButton prototype creation failed"));
	if (! obj )
		return false;

	obj = Panel::JSInit(cx, global, Window::GetClassPrototype());
	wxASSERT_MSG(obj != NULL, wxT("wxPanel prototype creation failed"));
	if (! obj )
		return false;

	obj = Dialog::JSInit(cx, global, TopLevelWindow::GetClassPrototype());
	wxASSERT_MSG(obj != NULL, wxT("wxDialog prototype creation failed"));
	if (! obj )
		return false;

	obj = CalendarDateAttr::JSInit(cx, global);
	wxASSERT_MSG(obj != NULL, wxT("wxCalendarDateAttr prototype creation failed"));
	if (! obj )
		return false;

	obj = ComboBox::JSInit(cx, global, ControlWithItems::GetClassPrototype());
	wxASSERT_MSG(obj != NULL, wxT("wxComboBox prototype creation failed"));
	if (! obj )
		return false;

	obj = Slider::JSInit(cx, global, Control::GetClassPrototype());
	wxASSERT_MSG(obj != NULL, wxT("wxSlider prototype creation failed"));
	if (! obj )
		return false;

	obj = Size::JSInit(cx, global);
	wxASSERT_MSG(obj != NULL, wxT("wxSize prototype creation failed"));
	if (! obj )
		return false;

	obj = Rect::JSInit(cx, global);
	wxASSERT_MSG(obj != NULL, wxT("wxRect prototype creation failed"));
	if (! obj )
		return false;

	obj = FileDialog::JSInit(cx, global, Dialog::GetClassPrototype());
	wxASSERT_MSG(obj != NULL, wxT("wxFileDialog prototype creation failed"));
	if (! obj )
		return false;

	if ( ! InitEventClasses(cx, global) )
		return false;

	obj = Sizer::JSInit(cx, global);
	wxASSERT_MSG(obj != NULL, wxT("wxSizer prototype creation failed"));
	if (! obj )
		return false;

	obj = GridSizer::JSInit(cx, global, Sizer::GetClassPrototype());
	wxASSERT_MSG(obj != NULL, wxT("wxGridSizer prototype creation failed"));
	if (! obj )
		return false;

	obj = FlexGridSizer::JSInit(cx, global, GridSizer::GetClassPrototype());
	wxASSERT_MSG(obj != NULL, wxT("wxFlexGridSizer prototype creation failed"));
	if (! obj )
		return false;

	obj = BoxSizer::JSInit(cx, global, Sizer::GetClassPrototype());
	wxASSERT_MSG(obj != NULL, wxT("wxBoxSizer prototype creation failed"));
	if (! obj )
		return false;

	obj = StaticBoxSizer::JSInit(cx, global, BoxSizer::GetClassPrototype());
	wxASSERT_MSG(obj != NULL, wxT("wxStaticBoxSizer prototype creation failed"));
	if (! obj )
		return false;

	obj = Validator::JSInit(cx, global);
	wxASSERT_MSG(obj != NULL, wxT("wxValidator prototype creation failed"));
	if (! obj )
		return false;

	obj = GenericValidator::JSInit(cx, global, Validator::GetClassPrototype());
	wxASSERT_MSG(obj != NULL, wxT("wxGenericValidator prototype creation failed"));
	if (! obj )
		return false;

	obj = TextValidator::JSInit(cx, global, Validator::GetClassPrototype());
	wxASSERT_MSG(obj != NULL, wxT("wxTextValidator prototype creation failed"));
	if (! obj )
		return false;

    obj = AcceleratorEntry::JSInit(cx, global);
	wxASSERT_MSG(obj != NULL, wxT("wxAcceleratorEntry prototype creation failed"));
	if (! obj )
		return false;

	obj = AcceleratorTable::JSInit(cx, global);
	wxASSERT_MSG(obj != NULL, wxT("wxAcceleratorTable prototype creation failed"));
	if (! obj )
		return false;

	obj = Colour::JSInit(cx, global);
	wxASSERT_MSG(obj != NULL, wxT("wxColour prototype creation failed"));
	if (! obj )
		return false;

	obj = ColourDatabase::JSInit(cx, global);
	wxASSERT_MSG(obj != NULL, wxT("wxColourDatabase prototype creation failed"));
	if (! obj )
		return false;

    obj = Font::JSInit(cx, global);
	wxASSERT_MSG(obj != NULL, wxT("wxFont prototype creation failed"));
	if (! obj )
		return false;

	obj = ColourData::JSInit(cx, global);
	wxASSERT_MSG(obj != NULL, wxT("wxColourData prototype creation failed"));
	if (! obj )
		return false;

	obj = ColourDialog::JSInit(cx, global, Dialog::GetClassPrototype());
	wxASSERT_MSG(obj != NULL, wxT("wxColourDialog prototype creation failed"));
	if (! obj )
		return false;

	obj = FontData::JSInit(cx, global);
	wxASSERT_MSG(obj != NULL, wxT("wxFontData prototype creation failed"));
	if (! obj )
		return false;

	obj = FontDialog::JSInit(cx, global, Dialog::GetClassPrototype());
	wxASSERT_MSG(obj != NULL, wxT("wxFontDialog prototype creation failed"));
	if (! obj )
		return false;

	obj = FontList::JSInit(cx, global);
	wxASSERT_MSG(obj != NULL, wxT("wxFontList prototype creation failed"));
	if (! obj )
		return false;

	obj = DirDialog::JSInit(cx, global, Dialog::GetClassPrototype());
	wxASSERT_MSG(obj != NULL, wxT("wxDirDialog prototype creation failed"));
	if (! obj )
		return false;

	obj = FindReplaceData::JSInit(cx, global);
	wxASSERT_MSG(obj != NULL, wxT("wxFindReplaceData prototype creation failed"));
	if (! obj )
		return false;

	obj = FindReplaceDialog::JSInit(cx, global, Dialog::GetClassPrototype());
	wxASSERT_MSG(obj != NULL, wxT("wxFindReplaceDialog prototype creation failed"));
	if (! obj )
		return false;

	obj = FontEnumerator::JSInit(cx, global);
	wxASSERT_MSG(obj != NULL, wxT("wxFontEnumerator prototype creation failed"));
	if (! obj )
		return false;

	obj = Bitmap::JSInit(cx, global);
	wxASSERT_MSG(obj != NULL, wxT("wxBitmap prototype creation failed"));
	if (! obj )
		return false;

	obj = Icon::JSInit(cx, global, Bitmap::GetClassPrototype());
	wxASSERT_MSG(obj != NULL, wxT("wxIcon prototype creation failed"));
	if (! obj )
		return false;

    obj = ContextHelp::JSInit(cx, global);
	wxASSERT_MSG(obj != NULL, wxT("wxFile prototype creation failed"));
	if (! obj )
		return false;

    InitCommonConst(cx, global);

    obj = ImageList::JSInit(cx, global);
	wxASSERT_MSG(obj != NULL, wxT("wxImageList prototype creation failed"));
	if (! obj )
		return false;

    obj = ListCtrl::JSInit(cx, global, Control::GetClassPrototype());
	wxASSERT_MSG(obj != NULL, wxT("wxListCtrl prototype creation failed"));
	if (! obj )
		return false;

    obj = ListItem::JSInit(cx, global);
	wxASSERT_MSG(obj != NULL, wxT("wxListItem prototype creation failed"));
	if (! obj )
		return false;
    
    obj = ListItemAttr::JSInit(cx, global);
	wxASSERT_MSG(obj != NULL, wxT("wxListItemAttr prototype creation failed"));
	if (! obj )
		return false;

    obj = ListHitTest::JSInit(cx, global);
	wxASSERT_MSG(obj != NULL, wxT("wxListHitTest prototype creation failed"));
	if (! obj )
		return false;

    obj = TreeCtrl::JSInit(cx, global, Control::GetClassPrototype());
	wxASSERT_MSG(obj != NULL, wxT("wxTreeCtrl prototype creation failed"));
	if (! obj )
		return false;

    obj = TreeItem::JSInit(cx, global);
	wxASSERT_MSG(obj != NULL, wxT("wxTreeItem prototype creation failed"));
	if (! obj )
		return false;
    
    obj = TreeItemId::JSInit(cx, global);
	wxASSERT_MSG(obj != NULL, wxT("wxTreeItemId prototype creation failed"));
	if (! obj )
		return false;

    obj = TreeHitTest::JSInit(cx, global);
	wxASSERT_MSG(obj != NULL, wxT("wxTreeHitTest prototype creation failed"));
	if (! obj )
		return false;

    obj = SplitterWindow::JSInit(cx, global, Window::GetClassPrototype());
	wxASSERT_MSG(obj != NULL, wxT("wxSplitterWindow prototype creation failed"));
	if (! obj )
		return false;

    obj = StatusBar::JSInit(cx, global, Window::GetClassPrototype());
	wxASSERT_MSG(obj != NULL, wxT("wxStatusBar prototype creation failed"));
	if (! obj )
		return false;

    obj = ToolBar::JSInit(cx, global, Control::GetClassPrototype());
	wxASSERT_MSG(obj != NULL, wxT("wxToolBar prototype creation failed"));
	if (! obj )
		return false;

#ifdef __WXMSW__
    obj = AutomationObject::JSInit(cx, global);
	wxASSERT_MSG(obj != NULL, wxT("wxAutomationObject prototype creation failed"));
	if (! obj )
		return false;
#endif

    obj = Image::JSInit(cx, global);
    wxASSERT_MSG(obj != NULL, wxT("wxImage prototype creation failed"));
    if (! obj )
        return false;

    obj = ImageHandler::JSInit(cx, global);
    wxASSERT_MSG(obj != NULL, wxT("wxImageHandler prototype creation failed"));
    if (! obj )
        return false;

	obj = BMPHandler::JSInit(cx, global, ImageHandler::GetClassPrototype());
    wxASSERT_MSG(obj != NULL, wxT("wxBMPHandler prototype creation failed"));
    if (! obj )
        return false;

    obj = GIFHandler::JSInit(cx, global, ImageHandler::GetClassPrototype());
    wxASSERT_MSG(obj != NULL, wxT("wxGIFHandler prototype creation failed"));
    if (! obj )
        return false;

    obj = ICOHandler::JSInit(cx, global, ImageHandler::GetClassPrototype());
    wxASSERT_MSG(obj != NULL, wxT("wxICOHandler prototype creation failed"));
    if (! obj )
        return false;

    obj = JPEGHandler::JSInit(cx, global, ImageHandler::GetClassPrototype());
    wxASSERT_MSG(obj != NULL, wxT("wxJPEGHandler prototype creation failed"));
    if (! obj )
        return false;

    obj = PCXHandler::JSInit(cx, global, ImageHandler::GetClassPrototype());
    wxASSERT_MSG(obj != NULL, wxT("wxPCXHandler prototype creation failed"));
    if (! obj )
        return false;

    obj = PNGHandler::JSInit(cx, global, ImageHandler::GetClassPrototype());
    wxASSERT_MSG(obj != NULL, wxT("wxPNGHandler prototype creation failed"));
    if (! obj )
        return false;

    obj = PNMHandler::JSInit(cx, global, ImageHandler::GetClassPrototype());
    wxASSERT_MSG(obj != NULL, wxT("wxPNMHandler prototype creation failed"));
    if (! obj )
        return false;

#if wxUSE_LIBTIFF
    obj = TIFFHandler::JSInit(cx, global, ImageHandler::GetClassPrototype());
    wxASSERT_MSG(obj != NULL, wxT("wxTIFFHandler prototype creation failed"));
    if (! obj )
        return false;
#endif

    obj = XPMHandler::JSInit(cx, global, ImageHandler::GetClassPrototype());
    wxASSERT_MSG(obj != NULL, wxT("wxXPMHandler prototype creation failed"));
    if (! obj )
        return false;

    obj = TextEntryDialog::JSInit(cx, global, Dialog::GetClassPrototype());
    wxASSERT_MSG(obj != NULL, wxT("wxTextEntryDialog prototype creation failed"));
    if (! obj )
        return false;

    obj = PasswordEntryDialog::JSInit(cx, global, TextEntryDialog::GetClassPrototype());
    wxASSERT_MSG(obj != NULL, wxT("wxPasswordEntryDialog prototype creation failed"));
    if (! obj )
        return false;

    obj = ScrolledWindow::JSInit(cx, global, Panel::GetClassPrototype());
    wxASSERT_MSG(obj != NULL, wxT("wxScrollWindow prototype creation failed"));
    if (! obj )
        return false;

    obj = HtmlWindow::JSInit(cx, global, ScrolledWindow::GetClassPrototype());
    wxASSERT_MSG(obj != NULL, wxT("wxHtmlWindow prototype creation failed"));
    if (! obj )
        return false;

	obj = SpinCtrl::JSInit(cx, global, Control::GetClassPrototype());
	wxASSERT_MSG(obj != NULL, wxT("wxSpinCtrl prototype creation failed"));
	if (! obj )
		return false;

    obj = SpinButton::JSInit(cx, global, Control::GetClassPrototype());
	wxASSERT_MSG(obj != NULL, wxT("wxSpinButton prototype creation failed"));
	if (! obj )
		return false;

    obj = SystemSettings::JSInit(cx, global, NULL);
	wxASSERT_MSG(obj != NULL, wxT("wxSystemSettings prototype creation failed"));
	if (! obj )
		return false;

    obj = BookCtrlBase::JSInit(cx, global, Control::GetClassPrototype());
	wxASSERT_MSG(obj != NULL, wxT("wxBookCtrlBase prototype creation failed"));
	if (! obj )
		return false;

    obj = Notebook::JSInit(cx, global, BookCtrlBase::GetClassPrototype());
	wxASSERT_MSG(obj != NULL, wxT("wxNotebook prototype creation failed"));
	if (! obj )
		return false;

    obj = Timer::JSInit(cx, global);
	wxASSERT_MSG(obj != NULL, wxT("wxTimer prototype creation failed"));
	if (! obj )
		return false;

    // Define the global functions
	InitFunctions(cx, global);

	DefineGlobals(cx, global);
	return true;
}

bool wxjs::gui::InitObject(JSContext *cx, JSObject *obj)
{
  return true;
}

void wxjs::gui::Destroy()
{
}
