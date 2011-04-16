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

#include "precompiled.h"

#include "NewDialog.h"

#include "AtlasScript/ScriptInterface.h"
#include "General/Datafile.h"


enum {
	ID_CH_SIZE = wxID_HIGHEST+1,
	ID_SP_HEIGHT
};

IMPLEMENT_CLASS(NewDialog, wxDialog)

BEGIN_EVENT_TABLE(NewDialog, wxDialog)
	EVT_CHOICE(ID_CH_SIZE, NewDialog::OnSizeChange)
	EVT_SPINCTRL(ID_SP_HEIGHT, NewDialog::OnHeightChange)
END_EVENT_TABLE()


NewDialog::NewDialog(wxWindow* parent, const wxString& title, const wxSize& size, ScenarioEditor& scenarioEditor)
	: wxDialog(parent, -1, title, wxDefaultPosition, size,
			   wxCAPTION | wxRESIZE_BORDER | wxSYSTEM_MENU | wxCLOSE_BOX)
{
	wxBoxSizer* mainSizer = new wxBoxSizer(wxVERTICAL);
	SetSizer(mainSizer);

	m_SelectedSize = 0;
	m_BaseHeight = 0;

	m_Panel = new wxPanel(this);
	mainSizer->Add(m_Panel, wxSizerFlags().Expand().Border(wxLEFT|wxRIGHT, 5));

	// Get available map sizes
	AtObj sizes(Datafile::ReadList("mapsizes"));
	for (AtIter s = sizes["size"]; s.defined(); ++s)
	{
		if (s["@name"].defined() && s["@tiles"].defined())
		{
			m_SizeArray.Add(wxString(s["@name"]));

			size_t size;
			std::wstringstream stream;
			stream << (std::wstring)s["@tiles"];
			stream >> size;
			
			m_TilesArray.push_back(size);
		}
	}

	// Map size
	wxBoxSizer* mapSizeSizer = new wxBoxSizer(wxHORIZONTAL);
	mainSizer->Add(mapSizeSizer, wxSizerFlags().Expand().Align(wxALIGN_TOP|wxALIGN_RIGHT).Border(wxALL, 5));

	mapSizeSizer->Add(new wxStaticText(this, wxID_ANY, _("Choose map size")), wxSizerFlags().Border(wxRIGHT, 25));
	wxChoice* sizeCtrl = new wxChoice(this, ID_CH_SIZE, wxDefaultPosition, wxDefaultSize, m_SizeArray);
	sizeCtrl->SetSelection(m_SelectedSize);
	mapSizeSizer->Add(sizeCtrl, wxSizerFlags().Align(wxALIGN_CENTER_VERTICAL));

	// Base height
	wxBoxSizer* baseHeightSizer = new wxBoxSizer(wxHORIZONTAL);
	mainSizer->Add(baseHeightSizer, wxSizerFlags().Expand().Align(wxALIGN_TOP|wxALIGN_RIGHT).Border(wxALL, 5));

	baseHeightSizer->Add(new wxStaticText(this, wxID_ANY, _("Choose base height")), wxSizerFlags().Border(wxRIGHT, 25));
	wxSpinCtrl* heightCtrl = new wxSpinCtrl(this, ID_SP_HEIGHT, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, 0, 65535);
	baseHeightSizer->Add(heightCtrl, wxSizerFlags().Align(wxALIGN_CENTER_VERTICAL));

	// Map base terrain
	wxBoxSizer* mapTerrainSizer = new wxBoxSizer(wxVERTICAL);
	mainSizer->Add(mapTerrainSizer, wxSizerFlags().Proportion(1).Expand().Border(wxALL, 5));

	mapTerrainSizer->Add(new wxStaticText(this, wxID_ANY, _("Choose map terrain")), wxSizerFlags().Border(wxRIGHT, 25));
	wxPanel* terrainPanel = scenarioEditor.GetScriptInterface().LoadScriptAsPanel(_T("terrainpreview"), this);
	mapTerrainSizer->Add(terrainPanel, wxSizerFlags().Proportion(1).Expand().Align(wxALIGN_BOTTOM|wxALIGN_RIGHT).Border(wxALL, 5));

	// OK/Cancel buttons
	wxBoxSizer* buttonSizer = new wxBoxSizer(wxHORIZONTAL);
	mainSizer->Add(buttonSizer, wxSizerFlags().Expand().Align(wxALIGN_RIGHT).Border(wxALL, 5));

	buttonSizer->Add(new wxButton(this, wxID_OK, _("OK")), wxSizerFlags().Border(wxRIGHT, 25));
	buttonSizer->Add(new wxButton(this, wxID_CANCEL, _("Cancel")), wxSizerFlags().Border(wxRIGHT, 5));

}

void NewDialog::OnSizeChange(wxCommandEvent& event)
{
	m_SelectedSize = (size_t)event.GetSelection();
}

void NewDialog::OnHeightChange(wxSpinEvent& event)
{
	m_BaseHeight = (size_t)event.GetSelection();
}

size_t NewDialog::GetSelectedSize()
{
	return m_TilesArray[m_SelectedSize];
}

size_t NewDialog::GetBaseHeight()
{
	return m_BaseHeight;
}
