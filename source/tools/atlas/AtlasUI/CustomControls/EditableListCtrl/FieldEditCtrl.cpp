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

#include "FieldEditCtrl.h"

#include "EditableListCtrlCommands.h"
#include "ListCtrlValidator.h"

#include "QuickTextCtrl.h"
#include "QuickComboBox.h"
#include "QuickFileCtrl.h"

#include "Windows/AtlasDialog.h"
#include "EditableListCtrl/EditableListCtrl.h"
#include "AtlasObject/AtlasObject.h"
#include "AtlasObject/AtlasObjectText.h"
#include "General/Datafile.h"

#include "wx/colour.h"
#include "wx/colordlg.h"
#include "wx/regex.h"
#include "wx/filename.h"

#include <string>

//////////////////////////////////////////////////////////////////////////

void FieldEditCtrl_Text::StartEdit(wxWindow* parent, wxRect rect, long row, int col)
{
	ListCtrlValidator validator((EditableListCtrl*)parent, row, col);
	new QuickTextCtrl(parent, rect, validator);
}

//////////////////////////////////////////////////////////////////////////

void FieldEditCtrl_Color::StartEdit(wxWindow* parent, wxRect WXUNUSED(rect), long row, int col)
{
	EditableListCtrl* editCtrl = (EditableListCtrl*)parent;
	wxColor oldColor;
	wxString oldColorStr (editCtrl->GetCellObject(row, col));

	// Parse the "r g b" color string (and ignore leading/trailing junk)
	wxRegEx re (_T("([0-9]+) ([0-9]+) ([0-9]+)")); // don't use \d, since that requires wxRE_ADVANCED
	wxASSERT(re.IsValid());
	if (re.Matches(oldColorStr))
	{
		wxASSERT(re.GetMatchCount() == 4); // 1 for matched string, +3 for captured groups
		long r, g, b;
		re.GetMatch(oldColorStr, 1).ToLong(&r);
		re.GetMatch(oldColorStr, 2).ToLong(&g);
		re.GetMatch(oldColorStr, 3).ToLong(&b);
		oldColor = wxColor(r, g, b);
	}

	wxColor newColor = wxGetColourFromUser(parent, oldColor);

	if (newColor.IsOk()) // test whether the user cancelled the selection
	{
		wxString newColorStr = wxString::Format(_T("%d %d %d"), newColor.Red(), newColor.Green(), newColor.Blue());
		AtlasWindowCommandProc::GetFromParentFrame(editCtrl)->Submit(
			new EditCommand_Text(editCtrl, row, col, newColorStr)
		);
	}
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
		 choices.Add((wxString)it);

	ListCtrlValidator validator((EditableListCtrl*)parent, row, col);
	new QuickComboBox(parent, rect, choices, validator);
}

//////////////////////////////////////////////////////////////////////////

FieldEditCtrl_Dialog::FieldEditCtrl_Dialog(AtlasDialog* (*dialogCtor)(wxWindow*))
	: m_DialogCtor(dialogCtor)
{
}

void FieldEditCtrl_Dialog::StartEdit(wxWindow* parent, wxRect WXUNUSED(rect), long row, int col)
{
	AtlasDialog* dialog = m_DialogCtor(parent);
	wxCHECK2(dialog, return);

	dialog->SetParent(parent);

	EditableListCtrl* editCtrl = (EditableListCtrl*)parent;

	AtObj in (editCtrl->GetCellObject(row, col));
	dialog->ImportData(in);

	int ret = dialog->ShowModal();

	if (ret == wxID_OK)
	{
		AtObj out (dialog->ExportData());

		AtlasWindowCommandProc::GetFromParentFrame(parent)->Submit(
			new EditCommand_Dialog(editCtrl, row, col, out)
		);
	}

	delete dialog;
}

//////////////////////////////////////////////////////////////////////////

FieldEditCtrl_File::FieldEditCtrl_File(const wxString& rootDir, const wxString& fileMask)
	: m_RootDir(rootDir), m_FileMask(fileMask)
{
	// Make the rootDir path absolute (where rootDir is relative to binaries/system),
	// defaulting to the 'public' mod:
	wxFileName path (_T("mods/public/") + rootDir);
	wxASSERT(path.IsOk());
	path.MakeAbsolute(Datafile::GetDataDirectory());
	wxASSERT(path.IsOk());
	m_RememberedDir = path.GetPath();
}

void FieldEditCtrl_File::StartEdit(wxWindow* parent, wxRect rect, long row, int col)
{
	ListCtrlValidator validator((EditableListCtrl*)parent, row, col);
	new QuickFileCtrl(parent, rect, m_RootDir, m_FileMask, m_RememberedDir, validator);
}
