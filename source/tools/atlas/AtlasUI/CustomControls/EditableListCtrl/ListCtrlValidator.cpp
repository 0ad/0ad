#include "stdafx.h"

#include "ListCtrlValidator.h"

#include "AtlasWindowCommandProc.h"
#include "EditableListCtrl.h"
#include "EditableListCtrlCommands.h"

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
	wxTextCtrl* textCtrl = wxDynamicCast(GetWindow(), wxTextCtrl);

	wxString text (m_listCtrl->GetCellString(m_Row, m_Col));

	textCtrl->SetValue(text);

	return true;
}

bool ListCtrlValidator::TransferFromWindow()
{
	wxTextCtrl* textCtrl = wxDynamicCast(GetWindow(), wxTextCtrl);

	wxString newText = textCtrl->GetValue();
	
	AtlasWindowCommandProc::GetFromParentFrame(m_listCtrl)->Submit(
		new EditCommand_Text(m_listCtrl, m_Row, m_Col, newText)
	);

	return true;
}

bool ListCtrlValidator::Validate(wxWindow* WXUNUSED(parent))
{
	return true;
}
