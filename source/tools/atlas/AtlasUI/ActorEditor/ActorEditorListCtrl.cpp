#include "stdafx.h"

#include "ActorEditorListCtrl.h"

#include "AtlasObject/AtlasObject.h"
#include "FieldEditCtrl.h"

ActorEditorListCtrl::ActorEditorListCtrl(wxWindow* parent)
	: DraggableListCtrl(parent, wxID_ANY, wxDefaultPosition, wxDefaultSize,
		wxLC_REPORT | wxLC_HRULES | wxLC_VRULES | wxLC_SINGLE_SEL)
{
	#define COLOUR(name, c0, c1) \
	m_ListItemAttr_##name[0].SetBackgroundColour(wxColour c0); \
	m_ListItemAttr_##name[1].SetBackgroundColour(wxColour c1)
	
	const int f=0xFF, e=0xEE, c=0xCC;
	COLOUR(Model,   (f,f,e), (f,f,c));
	COLOUR(Texture, (f,e,e), (f,c,c));
	COLOUR(Anim,    (e,f,e), (c,f,c));
	COLOUR(Prop,    (e,e,f), (c,c,f));

	#undef COLOUR

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

wxListItemAttr* ActorEditorListCtrl::OnGetItemAttr(long item) const
{
	if (item >= 0 && item < (int)m_ListData.size())
	{
		AtObj row (m_ListData[item]);

		// Colour-code the row, depending on the first non-empty column,
		// and also alternate between light/dark.

		if (row["mesh"].hasContent())
			return const_cast<wxListItemAttr*>(&m_ListItemAttr_Model[item%2]);
		else if (row["texture"].hasContent())
			return const_cast<wxListItemAttr*>(&m_ListItemAttr_Texture[item%2]);
		else if (row["animations"].hasContent())
			return const_cast<wxListItemAttr*>(&m_ListItemAttr_Anim[item%2]);
		else if (row["props"].hasContent())
			return const_cast<wxListItemAttr*>(&m_ListItemAttr_Prop[item%2]);
	}

	return const_cast<wxListItemAttr*>(&m_ListItemAttr[item%2]);
}
