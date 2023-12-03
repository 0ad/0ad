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

#include "EditableListCtrlCommands.h"

#include "EditableListCtrl.h"

//////////////////////////////////////////////////////////////////////////

IMPLEMENT_CLASS(EditCommand_Dialog, AtlasWindowCommand);

EditCommand_Dialog::EditCommand_Dialog(EditableListCtrl* ctrl, long row, int col, AtObj& newData)
	: AtlasWindowCommand(true, _("Edit")), m_Ctrl(ctrl), m_Row(row), m_Col(col), m_NewData(newData)
{
}

bool EditCommand_Dialog::Do()
{
	m_Ctrl->CloneListData(m_OldData);

	m_Ctrl->MakeSizeAtLeast(m_Row+1);

	m_Ctrl->SetCellObject(m_Row, m_Col, m_NewData);

	m_Ctrl->UpdateDisplay();
	m_Ctrl->SetSelection(m_Row);

	return true;
}

bool EditCommand_Dialog::Undo()
{
	m_Ctrl->SetListData(m_OldData);

	m_Ctrl->UpdateDisplay();
	m_Ctrl->SetSelection(m_Row);

	return true;
}

//////////////////////////////////////////////////////////////////////////

IMPLEMENT_CLASS(EditCommand_Text, AtlasWindowCommand);

EditCommand_Text::EditCommand_Text(EditableListCtrl* ctrl, long row, int col, wxString newText)
	: AtlasWindowCommand(true, _("Edit")), m_Ctrl(ctrl), m_Row(row), m_Col(col), m_NewText(newText)
{
}

bool EditCommand_Text::Do()
{
	m_Ctrl->CloneListData(m_OldData);

	m_Ctrl->MakeSizeAtLeast(m_Row+1);

	m_Ctrl->SetCellString(m_Row, m_Col, m_NewText);

	m_Ctrl->UpdateDisplay();
	m_Ctrl->SetSelection(m_Row);

	return true;
}

bool EditCommand_Text::Undo()
{
	m_Ctrl->SetListData(m_OldData);

	m_Ctrl->UpdateDisplay();
	m_Ctrl->SetSelection(m_Row);

	return true;
}

//////////////////////////////////////////////////////////////////////////

IMPLEMENT_CLASS(PasteCommand, AtlasWindowCommand);

PasteCommand::PasteCommand(EditableListCtrl* ctrl, long row, AtObj& newData)
	: AtlasWindowCommand(true, _("Paste")), m_Ctrl(ctrl), m_Row(row), m_NewData(newData)
{
}

bool PasteCommand::Do()
{
	m_Ctrl->CloneListData(m_OldData);

	m_Ctrl->MakeSizeAtLeast(m_Row);

	m_Ctrl->m_ListData.insert(m_Ctrl->m_ListData.begin()+m_Row, m_NewData);

	m_Ctrl->UpdateDisplay();
	m_Ctrl->SetSelection(m_Row);

	return true;
}

bool PasteCommand::Undo()
{
	m_Ctrl->SetListData(m_OldData);

	m_Ctrl->UpdateDisplay();
	m_Ctrl->SetSelection(m_Row);

	return true;
}
