#include "stdafx.h"

#include "FieldEditCtrl.h"

#include "ListCtrlValidator.h"
#include "QuickTextCtrl.h"
#include "AtlasDialog.h"
#include "EditableListCtrl.h"
#include "AtlasObject/AtlasObject.h"
#include "AtlasObject/AtlasObjectText.h"

#include <string>

void FieldEditCtrl_Text::StartEdit(wxWindow* parent, wxRect rect, long row, int col)
{
	new QuickTextCtrl(parent, rect, ListCtrlValidator((EditableListCtrl*)parent, row, col));
}

//////////////////////////////////////////////////////////////////////////

class EditCommand_Dialog : public wxCommand
{
public:
	EditCommand_Dialog(EditableListCtrl* ctrl, long row, int col, AtObj& newData)
		: wxCommand(true, _("Edit")), m_Ctrl(ctrl), m_Row(row), m_Col(col), m_NewData(newData)
	{
	}

	bool Do()
	{
		m_Ctrl->CloneListData(m_OldData);

		m_Ctrl->MakeSizeAtLeast(m_Row+1);

		m_Ctrl->SetCellObject(m_Row, m_Col, m_NewData);

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
	AtObj m_NewData;
	std::vector<AtObj> m_OldData;

};

FieldEditCtrl_Dialog::FieldEditCtrl_Dialog(wxString dialogType)
	: m_DialogType(dialogType)
{
}

void FieldEditCtrl_Dialog::StartEdit(wxWindow* parent, wxRect WXUNUSED(rect), long row, int col)
{
	AtlasDialog* dialog = wxDynamicCast(wxCreateDynamicObject(m_DialogType), AtlasDialog);
	wxCHECK2(dialog, return);

	dialog->SetParent(parent);

	EditableListCtrl* editCtrl = (EditableListCtrl*)parent;

	AtObj in (editCtrl->GetCellObject(row, col));
	dialog->Import(in);

	int ret = dialog->ShowModal();

	if (ret == wxID_OK)
	{
		AtObj out (dialog->Export());

		AtlasWindowCommandProc::GetFromParentFrame(parent)->Submit(
			new EditCommand_Dialog(editCtrl, row, col, out)
		);
	}

	delete dialog;
}
