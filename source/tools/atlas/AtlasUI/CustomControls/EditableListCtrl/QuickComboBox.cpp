#include "stdafx.h"

#include "QuickComboBox.h"

const int verticalPadding = 2;

QuickComboBox::QuickComboBox(wxWindow* parent,
							 wxRect& location,
							 const wxArrayString& choices,
							 const wxValidator& validator)

: wxComboBox(parent, wxID_ANY, wxEmptyString,
			 location.GetPosition()-wxPoint(0,verticalPadding),
			 location.GetSize()+wxSize(0,verticalPadding*2),
			 choices,
			 wxSUNKEN_BORDER | wxCB_DROPDOWN,
			 validator)
{
	GetValidator()->TransferToWindow();

	SetFocus();
}


void QuickComboBox::OnKillFocus(wxFocusEvent& event)
{
	// We need to test whether there's actually a window receiving focus;
	// otherwise it tries to destroy the control while wx is focusing
	// on the text-input box, and everything crashes.
	if (event.GetWindow())
	{
		GetValidator()->TransferFromWindow();
		Destroy();
	}
}

void QuickComboBox::OnChar(wxKeyEvent& event)
{
	if (event.GetKeyCode() == WXK_RETURN)
		GetParent()->SetFocus();
	else if (event.GetKeyCode() == WXK_ESCAPE)
		Destroy();
	else
		event.Skip();
}

BEGIN_EVENT_TABLE(QuickComboBox, wxComboBox)
	EVT_KILL_FOCUS(QuickComboBox::OnKillFocus)
	EVT_CHAR(QuickComboBox::OnChar)
END_EVENT_TABLE()
