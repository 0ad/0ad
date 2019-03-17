/* Copyright (C) 2019 Wildfire Games.
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

#include "ActorEditorListCtrl.h"

#include "AnimListEditor.h"
#include "TexListEditor.h"
#include "PropListEditor.h"

#include "AtlasObject/AtlasObject.h"
#include "EditableListCtrl/FieldEditCtrl.h"

ActorEditorListCtrl::ActorEditorListCtrl(wxWindow* parent)
	: DraggableListCtrl(parent, wxID_ANY, wxDefaultPosition, wxDefaultSize,
		wxLC_REPORT | wxLC_HRULES | wxLC_VRULES | wxLC_SINGLE_SEL)
{
	// Set colors for row backgrounds (which vary depending on what columns
	// have some data)
	#define COLOR(name, c0, c1) \
	m_ListItemAttr_##name[0].SetBackgroundColour(wxColor c0); \
	m_ListItemAttr_##name[1].SetBackgroundColour(wxColor c1)

	const int f=0xFF, e=0xEE, c=0xCC;
	COLOR(Model,   (f,f,e), (f,f,c));
	COLOR(Texture, (f,e,e), (f,c,c));
	COLOR(Anim,    (e,f,e), (c,f,c));
	COLOR(Prop,    (e,e,f), (c,c,f));
	COLOR(Color,  (f,e,f), (f,c,f));
	COLOR(None,    (f,f,f), (f,f,f));

	#undef COLOR

	AddColumnType(_("Variant"),    90,  "@name",      new FieldEditCtrl_Text());
	AddColumnType(_("Base File"),  90,  "@file",      new FieldEditCtrl_File(_T("art/variants/"), _("Variants (*.xml)|*.xml|All files (*.*)|*.*")));
	AddColumnType(_("Ratio"),      50,  "@frequency", new FieldEditCtrl_Text());
	AddColumnType(_("Model"),      140, "mesh",       new FieldEditCtrl_File(_T("art/meshes/"), _("Mesh files (*.pmd, *.dae)|*.pmd;*.dae|All files (*.*)|*.*")));
	AddColumnType(_("Particles"),  90,  "particles",  new FieldEditCtrl_File(_T("art/particles/"), _("Particle file (*.xml)|*.xml|All files (*.*)|*.*")));
	AddColumnType(_("Textures"),   250, "textures",   new FieldEditCtrl_Dialog(&TexListEditor::Create));
	AddColumnType(_("Animations"), 250, "animations", new FieldEditCtrl_Dialog(&AnimListEditor::Create));
	AddColumnType(_("Props"),      220, "props",      new FieldEditCtrl_Dialog(&PropListEditor::Create));
	AddColumnType(_("Color"),      80,  "color",      new FieldEditCtrl_Color());
}

void ActorEditorListCtrl::DoImport(AtObj& in)
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

AtObj ActorEditorListCtrl::DoExport()
{
	AtObj out;

	AtObj group;

	for (size_t i = 0; i < m_ListData.size(); ++i)
	{
		if (IsRowBlank((int)i))
		{
			if (group.defined())
				out.add("group", group);
			group = AtObj();
		}
		else
		{
			AtObj variant = AtlasObject::TrimEmptyChildren(m_ListData[i]);
			group.add("variant", variant);
		}
	}

	if (group.defined())
		out.add("group", group);

	return out;
}

wxListItemAttr* ActorEditorListCtrl::OnGetItemAttr(long item) const
{
	if (item >= 0 && item < (int)m_ListData.size())
	{
		AtObj row (m_ListData[item]);

		// Color-code the row, depending on the first non-empty column,
		// and also alternate between light/dark.

		if (row["mesh"].hasContent())
			return const_cast<wxListItemAttr*>(&m_ListItemAttr_Model[item%2]);
		else if (row["textures"].hasContent())
			return const_cast<wxListItemAttr*>(&m_ListItemAttr_Texture[item%2]);
		else if (row["animations"].hasContent())
			return const_cast<wxListItemAttr*>(&m_ListItemAttr_Anim[item%2]);
		else if (row["props"].hasContent())
			return const_cast<wxListItemAttr*>(&m_ListItemAttr_Prop[item%2]);
		else if (row["color"].hasContent())
			return const_cast<wxListItemAttr*>(&m_ListItemAttr_Color[item%2]);
	}

	return const_cast<wxListItemAttr*>(&m_ListItemAttr_None[item%2]);
}
