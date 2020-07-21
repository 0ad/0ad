/* Copyright (C) 2020 Wildfire Games.
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

#include "MapResizeDialog.h"
#include "GameInterface/MessagePasser.h"
#include "GameInterface/Messages.h"
#include "ScenarioEditor/ScenarioEditor.h"

#include <wx/statline.h>

MapResizeDialog::MapResizeDialog(wxWindow* parent)
	: wxDialog(parent, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxCAPTION | wxRESIZE_BORDER)
{
	Freeze();

	AtlasMessage::qGetCurrentMapSize qrySize;
	qrySize.Post();
	m_NewSize = qrySize.size;

	SetTitle(_("Resize/Recenter map"));
	wxSizer* sizer = new wxBoxSizer(wxVERTICAL);
	wxStaticText* label = new wxStaticText(this, wxID_ANY, _("Select new map size and center."), wxDefaultPosition, wxDefaultSize);
	sizer->Add(label, wxSizerFlags().Align(wxALIGN_CENTER_HORIZONTAL).Border(wxALL, 10));

	wxBoxSizer* listAndMap = new wxBoxSizer(wxHORIZONTAL);

	wxListBox* listBox = new wxListBox(this, wxID_ANY, wxDefaultPosition, wxDefaultSize, 0, nullptr, wxLB_SINGLE | wxLB_HSCROLL);
	// Load the map sizes list
	AtlasMessage::qGetMapSizes qrySizes;
	qrySizes.Post();
	AtObj sizes = AtlasObject::LoadFromJSON(*qrySizes.sizes);
	for (AtIter s = sizes["Data"]["item"]; s.defined(); ++s)
	{
		wxString size = wxString::FromUTF8(s["Tiles"]);
		listBox->Append(wxString::FromUTF8(s["Name"]), new wxStringClientData(size));
		if (m_NewSize == static_cast<ssize_t>(wxAtoi(size)))
			listBox->SetSelection(listBox->GetCount() - 1);
	}
	listAndMap->Add(listBox, wxSizerFlags().Align(wxALIGN_LEFT).Proportion(1).Expand());
	listAndMap->AddSpacer(10);

	m_MiniMap = new PseudoMiniMapPanel(this, m_NewSize);
	listBox->Bind(wxEVT_LISTBOX, &PseudoMiniMapPanel::OnNewSize, m_MiniMap);

	listAndMap->Add(m_MiniMap, wxSizerFlags());
	sizer->Add(listAndMap, wxSizerFlags().Proportion(1).Expand().Border(wxLEFT | wxRIGHT, 10));

	sizer->AddSpacer(5);
	sizer->Add(new wxStaticLine(this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL), wxSizerFlags().Expand().Border(wxRIGHT | wxLEFT, 7));
	sizer->AddSpacer(5);

	wxSizer* buttonSizer = new wxBoxSizer(wxHORIZONTAL);
	buttonSizer->Add(new wxButton(this, wxID_OK, _("OK")));
	buttonSizer->AddSpacer(5);
	buttonSizer->Add(new wxButton(this, wxID_CANCEL, _("Cancel")));

	sizer->Add(buttonSizer, wxSizerFlags().Align(wxALIGN_RIGHT).Border(wxRIGHT | wxBOTTOM, 10));

	SetSizerAndFit(sizer);
	Layout();
	Thaw();
}

ssize_t MapResizeDialog::GetNewSize() const
{
	return m_NewSize;
}

wxPoint MapResizeDialog::GetOffset() const
{
	return m_MiniMap->GetOffset();
}

void MapResizeDialog::OnListBox(wxCommandEvent& evt)
{
	if (!evt.IsSelection())
		return;

	const wxString str = static_cast<wxStringClientData*>(evt.GetClientObject())->GetData();
	long value = 0;
	if (str.ToLong(&value))
		m_NewSize = static_cast<ssize_t>(value);

	if (evt.GetEventType() == wxEVT_COMMAND_LISTBOX_DOUBLECLICKED)
		EndModal(wxID_OK);
}

void MapResizeDialog::OnCancel(wxCommandEvent& WXUNUSED(evt))
{
	EndModal(wxID_CANCEL);
}

void MapResizeDialog::OnOK(wxCommandEvent& WXUNUSED(evt))
{
	EndModal(wxID_OK);
}

BEGIN_EVENT_TABLE(MapResizeDialog, wxDialog)
	EVT_BUTTON(wxID_CANCEL, MapResizeDialog::OnCancel)
	EVT_BUTTON(wxID_OK, MapResizeDialog::OnOK)
	EVT_LISTBOX(wxID_ANY, MapResizeDialog::OnListBox)
	EVT_LISTBOX_DCLICK(wxID_ANY, MapResizeDialog::OnListBox)
END_EVENT_TABLE()
