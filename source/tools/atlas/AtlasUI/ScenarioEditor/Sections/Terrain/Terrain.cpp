/* Copyright (C) 2011 Wildfire Games.
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

#include "Terrain.h"

#include "Buttons/ToolButton.h"
#include "ScenarioEditor/ScenarioEditor.h"
#include "ScenarioEditor/Tools/Common/Brushes.h"
#include "ScenarioEditor/Tools/Common/MiscState.h"
#include "AtlasScript/ScriptInterface.h"

#include "GameInterface/Messages.h"

#include "wx/spinctrl.h"
#include "wx/listctrl.h"
#include "wx/image.h"
#include "wx/imaglist.h"
#include "wx/busyinfo.h"
#include "wx/notebook.h"

class TextureNotebook;

class TerrainBottomBar : public wxPanel
{
public:
	TerrainBottomBar(ScenarioEditor& scenarioEditor, wxWindow* parent);
	void LoadTerrain();
private:
	TextureNotebook* m_Textures;
};


enum
{
	ID_Passability = 1,
	ID_ShowPriorities,
	ID_ResizeMap
};

TerrainSidebar::TerrainSidebar(ScenarioEditor& scenarioEditor, wxWindow* sidebarContainer, wxWindow* bottomBarContainer) :
	Sidebar(scenarioEditor, sidebarContainer, bottomBarContainer)
{
	{
		/////////////////////////////////////////////////////////////////////////
		// Terrain elevation
		wxSizer* sizer = new wxStaticBoxSizer(wxVERTICAL, this, _("Elevation tools"));
		wxSizer* gridSizer = new wxGridSizer(3);
		gridSizer->Add(new ToolButton(scenarioEditor.GetToolManager(), this, _("Modify"), _T("AlterElevation")), wxSizerFlags().Expand());
		gridSizer->Add(new ToolButton(scenarioEditor.GetToolManager(), this, _("Smooth"), _T("SmoothElevation")), wxSizerFlags().Expand());
		gridSizer->Add(new ToolButton(scenarioEditor.GetToolManager(), this, _("Flatten"), _T("FlattenElevation")), wxSizerFlags().Expand());
		sizer->Add(gridSizer, wxSizerFlags().Expand());
		m_MainSizer->Add(sizer, wxSizerFlags().Expand().Border(wxTOP, 10));
	}

	{
		/////////////////////////////////////////////////////////////////////////
		// Terrain texture
		wxSizer* sizer = new wxStaticBoxSizer(wxVERTICAL, this, _("Texture tools"));
		wxSizer* gridSizer = new wxGridSizer(3);
		gridSizer->Add(new ToolButton(scenarioEditor.GetToolManager(), this, _("Paint"), _T("PaintTerrain")), wxSizerFlags().Expand());
		gridSizer->Add(new ToolButton(scenarioEditor.GetToolManager(), this, _("Replace"), _T("ReplaceTerrain")), wxSizerFlags().Expand());
		gridSizer->Add(new ToolButton(scenarioEditor.GetToolManager(), this, _("Fill"), _T("FillTerrain")), wxSizerFlags().Expand());
		sizer->Add(gridSizer, wxSizerFlags().Expand());
		m_MainSizer->Add(sizer, wxSizerFlags().Expand().Border(wxTOP, 10));
	}

	{
		/////////////////////////////////////////////////////////////////////////
		// Brush settings
		wxSizer* sizer = new wxStaticBoxSizer(wxVERTICAL, this, _("Brush"));
		g_Brush_Elevation.CreateUI(this, sizer);
		m_MainSizer->Add(sizer, wxSizerFlags().Expand().Border(wxTOP, 10));
	}

	{
		/////////////////////////////////////////////////////////////////////////
		// Visualise
		wxSizer* sizer = new wxStaticBoxSizer(wxVERTICAL, this, _("Visualise"));
		m_MainSizer->Add(sizer, wxSizerFlags().Expand().Border(wxTOP, 10));

		wxSizer* visSizer = new wxFlexGridSizer(2, 2, 5, 5);
		sizer->Add(visSizer);

		wxArrayString defaultChoices;
		defaultChoices.Add(_("(none)"));
		m_PassabilityChoice = new wxChoice(this, ID_Passability, wxDefaultPosition, wxDefaultSize, defaultChoices);
		m_PassabilityChoice->SetSelection(0);

		visSizer->Add(new wxStaticText(this, wxID_ANY, _("Passability")), wxSizerFlags().Align(wxALIGN_CENTER|wxALIGN_RIGHT));
		visSizer->Add(m_PassabilityChoice);

		visSizer->Add(new wxStaticText(this, wxID_ANY, _("Priorities")), wxSizerFlags().Align(wxALIGN_CENTER|wxALIGN_RIGHT));
		visSizer->Add(new wxCheckBox(this, ID_ShowPriorities, _("")));
	}

	{
		/////////////////////////////////////////////////////////////////////////
		// Misc tools
		wxSizer* sizer = new wxStaticBoxSizer(wxVERTICAL, this, _("Misc tools"));
		sizer->Add(new wxButton(this, ID_ResizeMap, _("Resize map")), wxSizerFlags().Expand());
		m_MainSizer->Add(sizer, wxSizerFlags().Expand().Border(wxTOP, 10));
	}

	m_BottomBar = new TerrainBottomBar(scenarioEditor, bottomBarContainer);
}

void TerrainSidebar::OnFirstDisplay()
{
	AtlasMessage::qGetTerrainPassabilityClasses qry;
	qry.Post();
	std::vector<std::wstring> passClasses = *qry.classNames;
	for (size_t i = 0; i < passClasses.size(); ++i)
		m_PassabilityChoice->Append(passClasses[i].c_str());

	static_cast<TerrainBottomBar*>(m_BottomBar)->LoadTerrain();
}

void TerrainSidebar::OnPassabilityChoice(wxCommandEvent& evt)
{
	if (evt.GetSelection() == 0)
		POST_MESSAGE(SetViewParamS, (AtlasMessage::eRenderView::GAME, L"passability", L""));
	else
		POST_MESSAGE(SetViewParamS, (AtlasMessage::eRenderView::GAME, L"passability", evt.GetString().c_str()));
}

void TerrainSidebar::OnShowPriorities(wxCommandEvent& evt)
{
	POST_MESSAGE(SetViewParamB, (AtlasMessage::eRenderView::GAME, L"priorities", evt.IsChecked()));
}

void TerrainSidebar::OnResizeMap(wxCommandEvent& WXUNUSED(evt))
{
	wxArrayString sizeNames;
	std::vector<size_t> sizeTiles;

	// Load the map sizes list
	AtlasMessage::qGetMapSizes qrySizes;
	qrySizes.Post();
	AtObj sizes = AtlasObject::LoadFromJSON(m_ScenarioEditor.GetScriptInterface().GetContext(), *qrySizes.sizes);
	for (AtIter s = sizes["Sizes"]["item"]; s.defined(); ++s)
	{
		long tiles = 0;
		wxString(s["Tiles"]).ToLong(&tiles);
		sizeNames.Add(wxString(s["Name"]));
		sizeTiles.push_back((size_t)tiles);
	}

	// TODO: set default based on current map size

	wxSingleChoiceDialog dlg(this, _("Select new map size. WARNING: This probably only works reliably on blank maps, and cannot be undone."),
			_("Resize map"), sizeNames);

	if (dlg.ShowModal() != wxID_OK)
		return;

	size_t tiles = sizeTiles.at(dlg.GetSelection());
	POST_COMMAND(ResizeMap, (tiles));
}

BEGIN_EVENT_TABLE(TerrainSidebar, Sidebar)
	EVT_CHOICE(ID_Passability, TerrainSidebar::OnPassabilityChoice)
	EVT_CHECKBOX(ID_ShowPriorities, TerrainSidebar::OnShowPriorities)
	EVT_BUTTON(ID_ResizeMap, TerrainSidebar::OnResizeMap)
END_EVENT_TABLE();

//////////////////////////////////////////////////////////////////////////

class TextureNotebookPage : public wxPanel
{
private:
	static const int imageWidth = 120;
	static const int imageHeight = 40;

public:
	TextureNotebookPage(ScenarioEditor& scenarioEditor, wxWindow* parent, const wxString& name)
		: wxPanel(parent, wxID_ANY), m_ScenarioEditor(scenarioEditor), m_Timer(this), m_Name(name), m_Loaded(false)
	{
		m_ScrolledPanel = new wxScrolledWindow(this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxVSCROLL);
		m_ScrolledPanel->SetScrollRate(0, 10);
		m_ScrolledPanel->SetBackgroundColour(wxColour(255, 255, 255));

		wxSizer* sizer = new wxBoxSizer(wxVERTICAL);
		sizer->Add(m_ScrolledPanel, wxSizerFlags().Proportion(1).Expand());
		SetSizer(sizer);

		m_ItemSizer = new wxGridSizer(6, 4, 0);
		m_ScrolledPanel->SetSizer(m_ItemSizer);
	}

	void OnDisplay()
	{
		// Trigger the terrain loading on first display

		if (m_Loaded)
			return;

		m_Loaded = true;

		wxBusyInfo busy (_("Loading terrain previews"));

		ReloadPreviews();
	}

	void ReloadPreviews()
	{
		Freeze();

		m_ScrolledPanel->DestroyChildren();
		m_ItemSizer->Clear();

		m_LastTerrainSelection = NULL; // clear any reference to deleted button

		AtlasMessage::qGetTerrainGroupPreviews qry(m_Name.c_str(), imageWidth, imageHeight);
		qry.Post();

		std::vector<AtlasMessage::sTerrainGroupPreview> previews = *qry.previews;

		bool allLoaded = true;

		for (size_t i = 0; i < previews.size(); ++i)
		{
			if (!previews[i].loaded)
				allLoaded = false;

			// Construct the wrapped-text label
			wxString name = previews[i].name.c_str();

			// Add spaces into the displayed name so there are more wrapping opportunities
			wxString labelText = name;
			labelText.Replace(_T("_"), _T(" "));
			wxStaticText* label = new wxStaticText(m_ScrolledPanel, wxID_ANY, labelText, wxDefaultPosition, wxDefaultSize, wxALIGN_CENTER);
			label->Wrap(imageWidth);

			unsigned char* buf = (unsigned char*)(malloc(previews[i].imageData.GetSize()));
			// imagedata.GetBuffer() gives a Shareable<unsigned char>*, which
			// is stored the same as a unsigned char*, so we can just copy it.
			memcpy(buf, previews[i].imageData.GetBuffer(), previews[i].imageData.GetSize());
			wxImage img (imageWidth, imageHeight, buf);

			wxButton* button = new wxBitmapButton(m_ScrolledPanel, wxID_ANY, wxBitmap(img));
			// Store the texture name in the clientdata slot
			button->SetClientObject(new wxStringClientData(name));

			wxSizer* imageSizer = new wxBoxSizer(wxVERTICAL);
			imageSizer->Add(button, wxSizerFlags().Center());
			imageSizer->Add(label, wxSizerFlags().Proportion(1).Center());
			m_ItemSizer->Add(imageSizer, wxSizerFlags().Expand().Center());
		}

		m_ScrolledPanel->Fit();
		Layout();

		Thaw();

		// If not all textures were loaded yet, run a timer to reload the previews
		// every so often until they've all finished
		if (allLoaded && m_Timer.IsRunning())
		{
			m_Timer.Stop();
		}
		else if (!allLoaded && !m_Timer.IsRunning())
		{
			m_Timer.Start(2000);
		}
	}

	void OnButton(wxCommandEvent& evt)
	{
		wxButton* button = wxDynamicCast(evt.GetEventObject(), wxButton);
		wxString name = static_cast<wxStringClientData*>(button->GetClientObject())->GetData();
		g_SelectedTexture = name;

		if (m_LastTerrainSelection)
			m_LastTerrainSelection->SetBackgroundColour(wxNullColour);

		button->SetBackgroundColour(wxColour(255, 255, 0));
		m_LastTerrainSelection = button;

		// Slight hack: Default to Paint mode because that's probably what the user wanted
		// when they selected a terrain; unless already explicitly in Replace mode, because
		// then the user probably wanted that instead
		if (m_ScenarioEditor.GetToolManager().GetCurrentToolName() != _T("ReplaceTerrain") && m_ScenarioEditor.GetToolManager().GetCurrentToolName() != _T("FillTerrain"))
			m_ScenarioEditor.GetToolManager().SetCurrentTool(_T("PaintTerrain"));
	}

	void OnSize(wxSizeEvent& evt)
	{
		int numCols = std::max(1, (int)(evt.GetSize().GetWidth() / (imageWidth + 16)));
		m_ItemSizer->SetCols(numCols);
		evt.Skip();
	}

	void OnTimer(wxTimerEvent& WXUNUSED(evt))
	{
		ReloadPreviews();
	}

private:
	ScenarioEditor& m_ScenarioEditor;
	bool m_Loaded;
	wxTimer m_Timer;
	wxString m_Name;
	wxScrolledWindow* m_ScrolledPanel;
	wxGridSizer* m_ItemSizer;
	wxButton* m_LastTerrainSelection; // button that was last selected, so we can undo its colouring

	DECLARE_EVENT_TABLE();
};

BEGIN_EVENT_TABLE(TextureNotebookPage, wxPanel)
	EVT_BUTTON(wxID_ANY, TextureNotebookPage::OnButton)
	EVT_SIZE(TextureNotebookPage::OnSize)
	EVT_TIMER(wxID_ANY, TextureNotebookPage::OnTimer)
END_EVENT_TABLE();


class TextureNotebook : public wxNotebook
{
public:
	TextureNotebook(ScenarioEditor& scenarioEditor, wxWindow *parent)
		: wxNotebook(parent, wxID_ANY/*, wxDefaultPosition, wxDefaultSize, wxNB_FIXEDWIDTH*/),
		  m_ScenarioEditor(scenarioEditor)
	{
	}

	void LoadTerrain()
	{
		wxBusyInfo busy (_("Loading terrain groups"));

		DeleteAllPages();
		m_TerrainGroups.Clear();

		// Get the list of terrain groups from the engine
		AtlasMessage::qGetTerrainGroups qry;
		qry.Post();
		std::vector<std::wstring> groupnames = *qry.groupNames;
		for (std::vector<std::wstring>::iterator it = groupnames.begin(); it != groupnames.end(); ++it)
			m_TerrainGroups.Add(it->c_str());

		for (size_t i = 0; i < m_TerrainGroups.GetCount(); ++i)
		{
			wxString visibleName = m_TerrainGroups[i];
			if (visibleName.Len())
				visibleName[0] = wxToupper(visibleName[0]);
			AddPage(new TextureNotebookPage(m_ScenarioEditor, this, m_TerrainGroups[i]), visibleName);
		}
	}

protected:
	void OnPageChanged(wxNotebookEvent& event)
	{
		if (event.GetSelection() >= 0 && event.GetSelection() < (int)GetPageCount())
		{
			static_cast<TextureNotebookPage*>(GetPage(event.GetSelection()))->OnDisplay();
		}
		event.Skip();
	}

private:
	ScenarioEditor& m_ScenarioEditor;
	wxArrayString m_TerrainGroups;

	DECLARE_EVENT_TABLE();
};

BEGIN_EVENT_TABLE(TextureNotebook, wxNotebook)
	EVT_NOTEBOOK_PAGE_CHANGED(wxID_ANY, TextureNotebook::OnPageChanged)
END_EVENT_TABLE();

//////////////////////////////////////////////////////////////////////////

TerrainBottomBar::TerrainBottomBar(ScenarioEditor& scenarioEditor, wxWindow* parent) :
	wxPanel(parent, wxID_ANY)
{
	wxSizer* sizer = new wxBoxSizer(wxVERTICAL);
	m_Textures = new TextureNotebook(scenarioEditor, this);
	sizer->Add(m_Textures, wxSizerFlags().Expand().Proportion(1));
	SetSizer(sizer);
}

void TerrainBottomBar::LoadTerrain()
{
	m_Textures->LoadTerrain();
}
