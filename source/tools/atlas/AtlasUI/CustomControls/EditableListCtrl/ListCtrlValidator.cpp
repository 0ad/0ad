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

#include "ListCtrlValidator.h"

#include "General/AtlasWindowCommandProc.h"
#include "EditableListCtrl/EditableListCtrl.h"
#include "EditableListCtrl/EditableListCtrlCommands.h"

#include "AtlasObject/AtlasObject.h"
#include "AtlasObject/AtlasObjectText.h"


ListCtrlValidator::ListCtrlValidator()
: m_listCtrl(NULL)
{
}

ListCtrlValidator::ListCtrlValidator(EditableListCtrl* listCtrl, long row, int col)
	: m_listCtrl(listCtrl), m_Row(row), m_Col(col)
{
}

wxObject* ListCtrlValidator::Clone() const
{
	return new ListCtrlValidator(m_listCtrl, m_Row, m_Col);
}

bool ListCtrlValidator::TransferToWindow()
{

	wxString text (m_listCtrl->GetCellString(m_Row, m_Col));

	wxTextCtrl* textCtrl; wxComboBox* comboBox; // one of these will be the right object

	if (NULL != (textCtrl = wxDynamicCast(GetWindow(), wxTextCtrl)))
		textCtrl->SetValue(text);
	else if (NULL != (comboBox = wxDynamicCast(GetWindow(), wxComboBox)))
		comboBox->SetValue(text);
	else
	{
		wxLogError(L"Internal error: ListCtrlValidator::TransferToWindow: invalid window");
		return false;
	}

	return true;
}

bool ListCtrlValidator::TransferFromWindow()
{
	wxString newText;

	wxTextCtrl* textCtrl; wxComboBox* comboBox; // one of these will be the right object

	if (NULL != (textCtrl = wxDynamicCast(GetWindow(), wxTextCtrl)))
		newText = textCtrl->GetValue();
	else if (NULL != (comboBox = wxDynamicCast(GetWindow(), wxComboBox)))
		newText = comboBox->GetValue();
	else
	{
		wxLogError(L"Internal error: ListCtrlValidator::TransferFromWindow: invalid window");
		return false;
	}

	AtlasWindowCommandProc::GetFromParentFrame(m_listCtrl)->Submit(
		new EditCommand_Text(m_listCtrl, m_Row, m_Col, newText)
	);

	return true;
}

bool ListCtrlValidator::Validate(wxWindow* WXUNUSED(parent))
{
	return true;
}
