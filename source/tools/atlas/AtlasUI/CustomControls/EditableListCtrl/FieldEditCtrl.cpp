#include "stdafx.h"

#include "FieldEditCtrl.h"

#include "EditableListCtrlCommands.h"
#include "ListCtrlValidator.h"

#include "QuickTextCtrl.h"
#include "QuickComboBox.h"
#include "QuickFileCtrl.h"

#include "AtlasDialog.h"
#include "EditableListCtrl.h"
#include "AtlasObject/AtlasObject.h"
#include "AtlasObject/AtlasObjectText.h"
#include "Datafile.h"

#include <string>

//////////////////////////////////////////////////////////////////////////

void FieldEditCtrl_Text::StartEdit(wxWindow* parent, wxRect rect, long row, int col)
{
	new QuickTextCtrl(parent, rect, ListCtrlValidator((EditableListCtrl*)parent, row, col));
}

//////////////////////////////////////////////////////////////////////////

FieldEditCtrl_List::FieldEditCtrl_List(const char* listType)
	: m_ListType(listType)
{
}

void FieldEditCtrl_List::StartEdit(wxWindow* parent, wxRect rect, long row, int col)
{
	wxArrayString choices;
	
	AtObj list (Datafile::ReadList(m_ListType));
	for (AtIter it = list["item"]; it.defined(); ++it)
		 choices.Add(it);

	new QuickComboBox(parent, rect, choices, ListCtrlValidator((EditableListCtrl*)parent, row, col));
}

//////////////////////////////////////////////////////////////////////////

FieldEditCtrl_Dialog::FieldEditCtrl_Dialog(const wxString& dialogType)
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
	dialog->ThawData(in);

	int ret = dialog->ShowModal();

	if (ret == wxID_OK)
	{
		AtObj out (dialog->FreezeData());

		AtlasWindowCommandProc::GetFromParentFrame(parent)->Submit(
			new EditCommand_Dialog(editCtrl, row, col, out)
		);
	}

	delete dialog;
}

//////////////////////////////////////////////////////////////////////////

FieldEditCtrl_File::FieldEditCtrl_File(const wxString& rootDir, const wxString& fileMask)
	: m_FileMask(fileMask)
{
	// Make the path correct, relative to binaries/system
	m_RootDir = _T("../data/mods/official/") + rootDir;
}

void FieldEditCtrl_File::StartEdit(wxWindow* parent, wxRect rect, long row, int col)
{
	new QuickFileCtrl(parent, rect, m_RootDir, m_FileMask, ListCtrlValidator((EditableListCtrl*)parent, row, col));
}