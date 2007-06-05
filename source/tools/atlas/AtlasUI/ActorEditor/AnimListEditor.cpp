#include "precompiled.h"

#include "AnimListEditor.h"

#include "EditableListCtrl/FieldEditCtrl.h"
#include "AtlasObject/AtlasObject.h"

//////////////////////////////////////////////////////////////////////////

AnimListEditor::AnimListEditor(wxWindow* parent)
	: AtlasDialog(parent, _("Animation editor"), wxSize(480, 280))
{
	m_MainListBox = new AnimListEditorListCtrl(m_MainPanel);

	wxBoxSizer* sizer = new wxBoxSizer(wxVERTICAL);
	sizer->Add(m_MainListBox,
		wxSizerFlags().Proportion(1).Expand().Border(wxALL, 5));

	m_MainPanel->SetSizer(sizer);
}

void AnimListEditor::ThawData(AtObj& in)
{
	m_MainListBox->ThawData(in);
}

AtObj AnimListEditor::FreezeData()
{
	return m_MainListBox->FreezeData();
}

void AnimListEditor::ImportData(AtObj& in)
{
	m_MainListBox->ImportData(in);
}

AtObj AnimListEditor::ExportData()
{
	return m_MainListBox->ExportData();
}

//////////////////////////////////////////////////////////////////////////

AnimListEditorListCtrl::AnimListEditorListCtrl(wxWindow* parent)
: DraggableListCtrl(parent, wxID_ANY, wxDefaultPosition, wxDefaultSize,
					wxLC_REPORT | wxLC_HRULES | wxLC_VRULES | wxLC_SINGLE_SEL)
{
	AddColumnType(_("Anim name"), 100, "@name",  new FieldEditCtrl_List("animations"));
	AddColumnType(_("File"),	  200, "@file",  new FieldEditCtrl_File(_T("art/animation/"), _("Animation files (*.psa, *.dae)|*.psa;*.dae|All files (*.*)|*.*")));
	AddColumnType(_("Speed"),	  50,  "@speed", new FieldEditCtrl_Text());
	AddColumnType(_("Load"),	  40,  "@load",  new FieldEditCtrl_Text());
	AddColumnType(_("Event"),	  40,  "@event", new FieldEditCtrl_Text());
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
