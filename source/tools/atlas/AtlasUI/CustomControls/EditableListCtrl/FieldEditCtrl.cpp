#include "stdafx.h"

#include "FieldEditCtrl.h"

#include "EditableListCtrlCommands.h"
#include "ListCtrlValidator.h"
#include "QuickTextCtrl.h"
#include "AtlasDialog.h"
#include "EditableListCtrl.h"
#include "AtlasObject/AtlasObject.h"
#include "AtlasObject/AtlasObjectText.h"

#include <string>

//////////////////////////////////////////////////////////////////////////

void FieldEditCtrl_Text::StartEdit(wxWindow* parent, wxRect rect, long row, int col)
{
	new QuickTextCtrl(parent, rect, ListCtrlValidator((EditableListCtrl*)parent, row, col));
}

//////////////////////////////////////////////////////////////////////////

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
