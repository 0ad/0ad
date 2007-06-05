#include "precompiled.h"

#include "QuickTextCtrl.h"

const int verticalPadding = 2;

QuickTextCtrl::QuickTextCtrl(wxWindow* parent,
							 wxRect& location,
							 const wxValidator& validator)

: wxTextCtrl(parent, wxID_ANY, wxEmptyString,
			 location.GetPosition()-wxPoint(0,verticalPadding),
			 location.GetSize()+wxSize(0,verticalPadding*2),
			 wxSUNKEN_BORDER | wxTE_PROCESS_TAB | wxTE_PROCESS_ENTER,
			 validator)
{
	GetValidator()->TransferToWindow();

	SetFocus();
	SetSelection(-1, -1);
}


void QuickTextCtrl::OnKillFocus(wxFocusEvent& WXUNUSED(event))
{
	GetValidator()->TransferFromWindow();
	Destroy();
}

void QuickTextCtrl::OnChar(wxKeyEvent& event)
{
	if (event.GetKeyCode() == WXK_RETURN)
		GetParent()->SetFocus();
	else if (event.GetKeyCode() == WXK_ESCAPE)
		Destroy();
	else
		event.Skip();
}

BEGIN_EVENT_TABLE(QuickTextCtrl, wxTextCtrl)
	EVT_KILL_FOCUS(QuickTextCtrl::OnKillFocus)
	EVT_CHAR(QuickTextCtrl::OnChar)
END_EVENT_TABLE()
