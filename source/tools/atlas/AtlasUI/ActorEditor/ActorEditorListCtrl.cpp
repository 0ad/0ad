#include "stdafx.h"

#include "ActorEditorListCtrl.h"

#include "AtlasObject/AtlasObject.h"
#include "FieldEditCtrl.h"

ActorEditorListCtrl::ActorEditorListCtrl(wxWindow* parent)
	: DraggableListCtrl(parent, wxID_ANY, wxDefaultPosition, wxDefaultSize,
		wxLC_REPORT | wxLC_HRULES | wxLC_VRULES | wxLC_SINGLE_SEL)
{
	AddColumnType(_("Variant"),		100, "name",		new FieldEditCtrl_Text());
	AddColumnType(_("Freq"),		50,  "frequency",	new FieldEditCtrl_Text());
	AddColumnType(_("Model"),		160, "mesh",		new FieldEditCtrl_Text());
	AddColumnType(_("Texture"),		160, "texture",		new FieldEditCtrl_Text());
	AddColumnType(_("Animations"),	250, "animations",	new FieldEditCtrl_Dialog(_T("AnimListEditor")));
	AddColumnType(_("Props"),		250, "props",		new FieldEditCtrl_Dialog(_T("PropListEditor")));
}

void ActorEditorListCtrl::Import(AtObj& in)
{
	DeleteData();

	for (AtIter group = in["group"]; group.defined(); ++group)
	{
		for (AtIter variant = group["variant"]; variant.defined(); ++variant)
			AddRow(variant);

		AtObj blank;
		AddRow(blank);
	}

	UpdateDisplay();
}

void ActorEditorListCtrl::Export(AtObj& out)
{
	AtObj group;

	for (size_t i = 0; i < m_ListData.size(); ++i)
	{
		if (IsRowBlank((int)i))
		{
			if (! group.isNull())
				out.add("group", group);
			group = AtObj();
		}
		else
		{
			AtObj variant = m_ListData[i];
			group.add("variant", variant);
		}
	}

	if (! group.isNull())
		out.add("group", group);
}
