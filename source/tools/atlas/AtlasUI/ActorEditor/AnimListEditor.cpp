#include "stdafx.h"

#include "AnimListEditor.h"

#include "FieldEditCtrl.h"
#include "AtlasObject/AtlasObject.h"

//////////////////////////////////////////////////////////////////////////

IMPLEMENT_DYNAMIC_CLASS(AnimListEditor, AtlasDialog);

AnimListEditor::AnimListEditor()
	: AtlasDialog(NULL, _("Animation editor"))
{
	m_MainListBox = new AnimListEditorListCtrl(m_MainPanel);

	wxBoxSizer* sizer = new wxBoxSizer(wxVERTICAL);
	sizer->Add(m_MainListBox,
		wxSizerFlags().Proportion(1).Expand().Border(wxALL, 5));

	m_MainPanel->SetSizer(sizer);
}

void AnimListEditor::Import(AtObj& in)
{
	m_MainListBox->Import(in);
}

AtObj AnimListEditor::Export()
{
	return m_MainListBox->Export();
}

//////////////////////////////////////////////////////////////////////////

AnimListEditorListCtrl::AnimListEditorListCtrl(wxWindow* parent)
: DraggableListCtrl(parent, wxID_ANY, wxDefaultPosition, wxDefaultSize,
					wxLC_REPORT | wxLC_HRULES | wxLC_VRULES | wxLC_SINGLE_SEL)
{
	AddColumnType(_("Anim name"), 100, "@name",  new FieldEditCtrl_List("animations"));
	AddColumnType(_("File"),	  200, "@file",  new FieldEditCtrl_File(_T("art/animation"), _("Animation files (*.psa)|*.psa|All files (*.*)|*.*")));
	AddColumnType(_("Speed"),	  50,  "@speed", new FieldEditCtrl_Text());
}

void AnimListEditorListCtrl::DoImport(AtObj& in)
{
	for (AtIter prop = in["animation"]; prop.defined(); ++prop)
		AddRow(prop);

	UpdateDisplay();
}

AtObj AnimListEditorListCtrl::DoExport()
{
	AtObj out;
	for (size_t i = 0; i < m_ListData.size(); ++i)
		out.add("animation", m_ListData[i]);
	return out;
}
