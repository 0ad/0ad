/* Copyright (C) 2011 Wildfire Games.
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

#ifndef INCLUDED_NEWDIALOG
#define INCLUDED_NEWDIALOG

#include "ScenarioEditor/ScenarioEditor.h"

#include "wx/dialog.h"

class NewDialog : public wxDialog
{
	DECLARE_CLASS(NewDialog)
	DECLARE_EVENT_TABLE()

public:
	NewDialog(wxWindow* parent, const wxString& title, const wxSize& size, ScenarioEditor& scenarioEditor);
	virtual ~NewDialog() {}

	size_t GetSelectedSize();
	size_t GetBaseHeight();

protected:
	wxPanel* m_Panel;

private:
	void OnSizeChange(wxCommandEvent& event);
	void OnHeightChange(wxSpinEvent& event);

	size_t m_SelectedSize;
	size_t m_BaseHeight;

	wxArrayString m_SizeArray;
	std::vector<size_t> m_PatchesArray;

};

#endif // INCLUDED_NEWDIALOG
