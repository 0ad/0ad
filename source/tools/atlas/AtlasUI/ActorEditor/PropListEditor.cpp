#include "stdafx.h"

#include "PropListEditor.h"

#include "FieldEditCtrl.h"
#include "AtlasObject/AtlasObject.h"

//////////////////////////////////////////////////////////////////////////

IMPLEMENT_DYNAMIC_CLASS(PropListEditor, AtlasDialog);

PropListEditor::PropListEditor()
	: AtlasDialog(NULL, _("Prop editor"))
{
	m_MainListBox = new PropListEditorListCtrl(m_MainPanel);

	wxBoxSizer* sizer = new wxBoxSizer(wxVERTICAL);
	sizer->Add(m_MainListBox,
		wxSizerFlags().Proportion(1).Expand().Border(wxALL, 5));

	m_MainPanel->SetSizer(sizer);
}

void PropListEditor::Import(AtObj& in)
{
	m_MainListBox->Import(in);
}

AtObj PropListEditor::Export()
{
	return m_MainListBox->Export();
}

//////////////////////////////////////////////////////////////////////////

PropListEditorListCtrl::PropListEditorListCtrl(wxWindow* parent)
: DraggableListCtrl(parent, wxID_ANY, wxDefaultPosition, wxDefaultSize,
					wxLC_REPORT | wxLC_HRULES | wxLC_VRULES | wxLC_SINGLE_SEL)
{
	AddColumnType(_("Attachment point"), 100, "attachpoint", new FieldEditCtrl_List("attachpoints"));
	AddColumnType(_("Prop model"),		 200, "model",       new FieldEditCtrl_File(_T("art/actors"), _("Actor files (*.xml)|*.xml"))); // not *.*, in order to hide all the *.xmb files
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
