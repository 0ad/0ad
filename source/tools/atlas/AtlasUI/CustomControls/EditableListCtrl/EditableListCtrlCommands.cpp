#include "stdafx.h"

#include "EditableListCtrlCommands.h"

#include "EditableListCtrl.h"

//////////////////////////////////////////////////////////////////////////

IMPLEMENT_CLASS(ImportCommand, AtlasWindowCommand);

ImportCommand::ImportCommand(EditableListCtrl* ctrl, AtObj& in)
	: AtlasWindowCommand(true, _("Import")), m_Ctrl(ctrl), m_In(in)
{
}

bool ImportCommand::Do()
{
	m_Ctrl->CloneListData(m_OldData);

	m_Ctrl->DoImport(m_In);

	m_Ctrl->UpdateDisplay();

	return true;
}

bool ImportCommand::Undo()
{
	m_Ctrl->SetListData(m_OldData);

	m_Ctrl->UpdateDisplay();

	return true;
}

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
