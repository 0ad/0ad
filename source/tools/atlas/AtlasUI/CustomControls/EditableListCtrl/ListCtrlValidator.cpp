#include "stdafx.h"

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
