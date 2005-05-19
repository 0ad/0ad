#include "stdafx.h"

#include "PropListEditor.h"

#include "FieldEditCtrl.h"
#include "AtlasObject/AtlasObject.h"

//////////////////////////////////////////////////////////////////////////

IMPLEMENT_DYNAMIC_CLASS(PropListEditor, AtlasDialog);

PropListEditor::PropListEditor()
	: AtlasDialog(NULL, _("Prop editor"), wxSize(400, 280))
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
