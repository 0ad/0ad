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
	m_ActorEditorListCtrl->Import(in);

	if (in["castshadow"].defined())
		m_CastShadows->SetValue(true);
	else
		m_CastShadows->SetValue(false);

	m_Material->SetValue(in["material"]);
}

void ActorEditor::Export(AtObj& out)
{
	m_ActorEditorListCtrl->Export(out);

	if (m_CastShadows->IsChecked())
		out.set("castshadow", L"true");

	out.set("material", m_Material->GetValue().c_str());
}
