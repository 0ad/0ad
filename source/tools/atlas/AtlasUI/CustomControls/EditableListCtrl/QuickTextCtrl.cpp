/* Copyright (C) 2009 Wildfire Games.
 * This file is part of 0 A.D.
 *
 * 0 A.D. is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * 0 A.D. is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with 0 A.D.  If not, see <http://www.gnu.org/licenses/>.
 */

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
