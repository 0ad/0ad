#include "stdafx.h"

#include "ListCtrlValidator.h"

#include "AtlasWindowCommandProc.h"
#include "EditableListCtrl.h"

#include "AtlasObject/AtlasObject.h"
#include "AtlasObject/AtlasObjectText.h"

class EditCommand_Text : public wxCommand
{
public:
	EditCommand_Text(EditableListCtrl* ctrl, long row, int col, wxString newText)
		: wxCommand(true, _("Edit")), m_Ctrl(ctrl), m_Row(row), m_Col(col), m_NewText(newText)
	{
	}

	bool Do()
	{
		m_Ctrl->CloneListData(m_OldData);

		m_Ctrl->MakeSizeAtLeast(m_Row+1);

		m_Ctrl->SetCellString(m_Row, m_Col, m_NewText);

		m_Ctrl->UpdateDisplay();
		m_Ctrl->SetSelection(m_Row);

		return true;
	}

	bool Undo()
	{
		m_Ctrl->SetListData(m_OldData);

		m_Ctrl->UpdateDisplay();
		m_Ctrl->SetSelection(m_Row);

		return true;
	}

private:
	EditableListCtrl* m_Ctrl;
	long m_Row;
	int m_Col;
	wxString m_NewText;
	std::vector<AtObj> m_OldData;
};


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
