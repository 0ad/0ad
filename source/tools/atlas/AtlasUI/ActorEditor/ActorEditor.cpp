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
	// Determine the file format version
	long version;

	if (in["Object"].defined())
	{
		// old-style actor format
		version = -1;
	}
	else if (in["actor"].defined())
	{
		if (in["actor"]["@version"].defined())
			wxString(in["actor"]["@version"]).ToLong(&version);
		else
			version = 0;
	}
	else
	{
		// TODO: report error
		return;
	}


	// Do any necessary conversions into the most recent format:

	if (version == -1)
	{
		AtObj actor;

		if (wxString(in["Object"]["Properties"]["@autoflatten"]) == _T("1"))
			actor.add("autoflatten", L"");

		if (wxString(in["Object"]["Properties"]["@castshadows"]) == _T("1"))
			actor.add("castshadow", L"");

		AtObj var;
		var.add("@name", L"Base");
		var.add("@frequency", L"100");

		#define THING1(out,outname, in,inname, prefix) \
			assert( wxString(in["Object"][inname]).StartsWith(_T(prefix)) ); \
			out.add(outname, wxString(in["Object"][inname]).Mid(wxString(_T(prefix)).Length()))
		#define THING2(out,outname, in,inname, prefix) \
			assert( wxString(in[inname]).StartsWith(_T(prefix)) ); \
			out.add(outname, wxString(in[inname]).Mid(wxString(_T(prefix)).Length()))

		THING1(var,"mesh",    in,"ModelName",   "art/meshes/");
		THING1(var,"texture", in,"TextureName", "art/textures/skins/");

		AtObj anims;
		for (AtIter animit = in["Object"]["Animations"]["Animation"]; animit.defined(); ++animit)
		{
			AtObj anim;
			anim.add("@name", animit["@name"]);
			anim.add("@speed", animit["@speed"]);

			THING2(anim,"@file", animit,"@file",  "art/animation/");

			anims.add("animation", anim);
		}
		var.add("animations", anims);

		AtObj props;
		for (AtIter propit = in["Object"]["Props"]["Prop"]; propit.defined(); ++propit)
		{
			AtObj prop;
			prop.add("@attachpoint", propit["@attachpoint"]);
			prop.add("@actor", propit["@model"]);
			props.add("prop", prop);
		}
		var.add("props", props);

		AtObj grp;
		grp.add("variant", var);
		actor.add("group", grp);

		#undef THING1
		#undef THING2

		in.set("actor", actor);
	}
	else if (version == 0)
	{
		AtObj actor;

		if (in["actor"]["castshadow"].defined()) actor.add("castshadow", in["actor"]["castshadow"]);
		if (in["actor"]["material"].defined()) actor.add("material", in["actor"]["material"]);

		for (AtIter grpit = in["actor"]["group"]; grpit.defined(); ++grpit)
		{
			AtObj grp;
			for (AtIter varit = grpit["variant"]; varit.defined(); ++varit)
			{
				AtObj var;
				var.add("@name", varit["name"]);
				var.add("@frequency", varit["frequency"]);
				var.add("mesh", varit["mesh"]);
				var.add("texture", varit["texture"]);

				AtObj anims;
				for (AtIter animit = varit["animations"]["animation"]; animit.defined(); ++animit)
				{
					AtObj anim;
					anim.add("@name", animit["name"]);
					anim.add("@file", animit["file"]);
					anim.add("@speed", animit["speed"]);

					anims.add("animation", anim);
				}
				var.add("animations", anims);

				AtObj props;
				for (AtIter propit = varit["props"]["prop"]; propit.defined(); ++propit)
				{
					AtObj prop;
					prop.add("@attachpoint", propit["attachpoint"]);
					prop.add("@actor", propit["model"]);

					props.add("prop", prop);
				}
				var.add("props", props);

				grp.add("variant", var);
			}
			actor.add("group", grp);
		}

		in.set("actor", actor);
	}
	else if (version == 1)
	{
		// current format
	}
	else
	{
		// ??? unknown format
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
	// Export the group/variant/etc data
	AtObj actor (m_ActorEditorListCtrl->Export());

	actor.set("@version", L"1");

	if (m_CastShadows->IsChecked())
		actor.set("castshadow", L"");

	if (m_Material->GetValue().length())
		actor.set("material", m_Material->GetValue().c_str());

	AtObj out;
	out.set("actor", actor);
	return out;
}
