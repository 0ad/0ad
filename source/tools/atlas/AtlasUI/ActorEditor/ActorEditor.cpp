#include "stdafx.h"

#include "ActorEditor.h"

#include "ActorEditorListCtrl.h"

#include "AtlasObject/AtlasObject.h"

#include "wx/panel.h"
#include "wx/sizer.h"

ActorEditor::ActorEditor(wxWindow* parent)
	: AtlasWindow(parent, _("Actor Editor"), wxSize(1024, 450))
{
	wxPanel* mainPanel = new wxPanel(this);

	m_ActorEditorListCtrl = new ActorEditorListCtrl(mainPanel);

	wxBoxSizer* vertSizer = new wxBoxSizer(wxVERTICAL);
	mainPanel->SetSizer(vertSizer);

	wxBoxSizer* topSizer = new wxBoxSizer(wxHORIZONTAL);
	vertSizer->Add(topSizer,
		wxSizerFlags().Border(wxLEFT|wxRIGHT, 5));

	vertSizer->Add(
		m_ActorEditorListCtrl,
		wxSizerFlags().Proportion(1).Expand().Border(wxALL, 10));

	//////////////////////////////////////////////////////////////////////////
	// Properties panel:

	wxPanel* propertiesPanel = new wxPanel(mainPanel);
	topSizer->Add(propertiesPanel, wxSizerFlags().Expand().Border(wxLEFT|wxRIGHT, 5));

	wxSizer* propertiesSizer = new wxStaticBoxSizer(
		new wxStaticBox(propertiesPanel, wxID_ANY, _("Actor properties")),
		wxHORIZONTAL);

	propertiesPanel->SetSizer(propertiesSizer);


	m_CastShadows = new wxCheckBox(propertiesPanel, wxID_ANY, _("Cast shadow"));
	propertiesSizer->Add(m_CastShadows, wxSizerFlags().Border(wxALL, 5));

	// TODO: Orientation property.

	//////////////////////////////////////////////////////////////////////////
	// Materials box:

	wxPanel* materialsPanel = new wxPanel(mainPanel);
	topSizer->Add(materialsPanel, wxSizerFlags().Expand().Border(wxLEFT|wxRIGHT, 5));

	wxSizer* materialsSizer = new wxStaticBoxSizer(
		new wxStaticBox(materialsPanel, wxID_ANY, _("Material")),
		wxHORIZONTAL);

	materialsPanel->SetSizer(materialsSizer);

	m_Material = new wxTextCtrl(materialsPanel, wxID_ANY, _T(""));
	materialsSizer->Add(m_Material, wxSizerFlags().Border(wxALL, 2));

	//////////////////////////////////////////////////////////////////////////

}


void ActorEditor::Import(AtObj& in)
{
	if (! in["actor"].defined())
	{
		// TODO: report error
		return;
	}

	AtObj actor (in["actor"]);
	m_ActorEditorListCtrl->Import(actor);

	if (actor["castshadow"].defined())
		m_CastShadows->SetValue(true);
	else
		m_CastShadows->SetValue(false);

	m_Material->SetValue(actor["material"]);
}

AtObj ActorEditor::Export()
{
	AtObj actor (m_ActorEditorListCtrl->Export());

	if (m_CastShadows->IsChecked())
		actor.set("castshadow", L"true");

	actor.set("material", m_Material->GetValue().c_str());

	AtObj out;
	out.set("actor", actor);
	return out;
}
