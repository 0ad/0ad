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

#include "PropListEditor.h"

#include "EditableListCtrl/FieldEditCtrl.h"
#include "AtlasObject/AtlasObject.h"

//////////////////////////////////////////////////////////////////////////

PropListEditor::PropListEditor(wxWindow* parent)
	: AtlasDialog(parent, _("Prop editor"), wxSize(400, 280))
{
	m_MainListBox = new PropListEditorListCtrl(m_MainPanel);

	wxBoxSizer* sizer = new wxBoxSizer(wxVERTICAL);
	sizer->Add(m_MainListBox,
		wxSizerFlags().Proportion(1).Expand().Border(wxALL, 5));

	m_MainPanel->SetSizer(sizer);
}

void PropListEditor::ThawData(AtObj& in)
{
	m_MainListBox->ThawData(in);
}

AtObj PropListEditor::FreezeData()
{
	return m_MainListBox->FreezeData();
}

void PropListEditor::ImportData(AtObj& in)
{
	m_MainListBox->ImportData(in);
}

AtObj PropListEditor::ExportData()
{
	return m_MainListBox->ExportData();
}

//////////////////////////////////////////////////////////////////////////

PropListEditorListCtrl::PropListEditorListCtrl(wxWindow* parent)
: DraggableListCtrl(parent, wxID_ANY, wxDefaultPosition, wxDefaultSize,
					wxLC_REPORT | wxLC_HRULES | wxLC_VRULES | wxLC_SINGLE_SEL)
{
	AddColumnType(_("Attachment point"), 100, "@attachpoint",	new FieldEditCtrl_List("attachpoints"));
	AddColumnType(_("Prop model"),		 200, "@actor",			new FieldEditCtrl_File(_T("art/actors/"), _("Actor files (*.xml)|*.xml|All files (*.*)|*.*")));
}

void PropListEditorListCtrl::DoImport(AtObj& in)
{
	for (AtIter prop = in["prop"]; prop.defined(); ++prop)
		AddRow(prop);

	UpdateDisplay();
}

AtObj PropListEditorListCtrl::DoExport()
{
	AtObj out;
	for (size_t i = 0; i < m_ListData.size(); ++i)
		out.add("prop", m_ListData[i]);
	return out;
}
