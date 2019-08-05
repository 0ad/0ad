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

#include "Terrain.h"

#include "Buttons/ToolButton.h"
#include "ScenarioEditor/ScenarioEditor.h"
#include "ScenarioEditor/Tools/Common/Brushes.h"
#include "ScenarioEditor/Tools/Common/MiscState.h"

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

// Helper function for adding tooltips
static wxWindow* Tooltipped(wxWindow* window, const wxString& tip)
{
	window->SetToolTip(tip);
	return window;
}

// Add spaces into the displayed name so there are more wrapping opportunities
static wxString FormatTextureName(wxString name)
{
	if (name.Len())
		name[0] = wxToupper(name[0]);
	name.Replace(_T("_"), _T(" "));

	return name;
}

//////////////////////////////////////////////////////////////////////////

class TexturePreviewPanel : public wxPanel
{
private:
	static const int imageWidth = 120;
	static const int imageHeight = 40;

public:
	TexturePreviewPanel(wxWindow* parent)
		: wxPanel(parent, wxID_ANY), m_Timer(this)
	{
		m_Conn = g_SelectedTexture.RegisterObserver(0, &TexturePreviewPanel::OnTerrainChange, this);
		m_Sizer = new wxStaticBoxSizer(wxVERTICAL, this, _T("Texture"));
		SetSizer(m_Sizer);

		// Use placeholder bitmap for now
		m_Sizer->Add(new wxStaticBitmap(this, wxID_ANY, wxNullBitmap), wxSizerFlags(1).Expand());
	}

	void LoadPreview()
	{
		if (m_TextureName.IsEmpty())
		{
			// If we haven't got a texture yet, copy the global
			m_TextureName = g_SelectedTexture;
		}

		Freeze();

		m_Sizer->Clear(true);

		AtlasMessage::qGetTerrainTexturePreview qry((std::wstring)m_TextureName.wc_str(), imageWidth, imageHeight);
		qry.Post();

		AtlasMessage::sTerrainTexturePreview preview = qry.preview;

		// Check for invalid/missing texture - shouldn't happen
		if (!wxString(qry.preview->name.c_str()).IsEmpty())
		{
			// Construct the wrapped-text label
			wxStaticText* label = new wxStaticText(this, wxID_ANY, FormatTextureName(*qry.preview->name), wxDefaultPosition, wxDefaultSize, wxALIGN_CENTER);
			label->Wrap(m_Sizer->GetSize().GetX());

			unsigned char* buf = (unsigned char*)(malloc(preview.imageData.GetSize()));
			// imagedata.GetBuffer() gives a Shareable<unsigned char>*, which
			// is stored the same as a unsigned char*, so we can just copy it.
			memcpy(buf, preview.imageData.GetBuffer(), preview.imageData.GetSize());
			wxImage img(qry.preview->imageWidth, qry.preview->imageHeight, buf);

			wxStaticBitmap* bitmap = new wxStaticBitmap(this, wxID_ANY, wxBitmap(img), wxDefaultPosition, wxSize(qry.preview->imageWidth, qry.preview->imageHeight), wxBORDER_SIMPLE);
			m_Sizer->Add(bitmap, wxSizerFlags(1).Align(wxALIGN_CENTER));
			m_Sizer->Add(label, wxSizerFlags().Expand());

			// We have to force the sidebar to layout manually
			GetParent()->Layout();

			if (preview.loaded && m_Timer.IsRunning())
			{
				m_Timer.Stop();
			}
			else if (!preview.loaded && !m_Timer.IsRunning())
			{
				m_Timer.Start(2000);
			}
		}

		Layout();
		Thaw();
	}

	void OnTerrainChange(const wxString& texture)
	{
		// Check if texture really changed, to avoid doing this too often
		if (texture != m_TextureName)
		{
			// Load new texture preview
			m_TextureName = texture;
			LoadPreview();
		}
	}

	void OnTimer(wxTimerEvent& WXUNUSED(evt))
	{
		LoadPreview();
	}

private:
	ObservableScopedConnection m_Conn;
	wxSizer* m_Sizer;
	wxTimer m_Timer;
	wxString m_TextureName;

	DECLARE_EVENT_TABLE();
};

BEGIN_EVENT_TABLE(TexturePreviewPanel, wxPanel)
	EVT_TIMER(wxID_ANY, TexturePreviewPanel::OnTimer)
END_EVENT_TABLE();

//////////////////////////////////////////////////////////////////////////

TerrainSidebar::TerrainSidebar(ScenarioEditor& scenarioEditor, wxWindow* sidebarContainer, wxWindow* bottomBarContainer) :
	Sidebar(scenarioEditor, sidebarContainer, bottomBarContainer)
{
	wxSizer* scrollSizer = new wxBoxSizer(wxVERTICAL);
	wxScrolledWindow* scrolledWindow = new wxScrolledWindow(this);
	scrolledWindow->SetScrollRate(10, 10);
	scrolledWindow->SetSizer(scrollSizer);
	m_MainSizer->Add(scrolledWindow, wxSizerFlags().Proportion(1).Expand());

	{
		/////////////////////////////////////////////////////////////////////////
		// Terrain elevation
		wxSizer* sizer = new wxStaticBoxSizer(wxVERTICAL, scrolledWindow, _("Elevation tools"));
		wxSizer* gridSizer = new wxGridSizer(4);
		gridSizer->Add(Tooltipped(new ToolButton(scenarioEditor.GetToolManager(), scrolledWindow, _("Modify"), _T("AlterElevation"), wxSize(48, -1)),
			_("Brush with left mouse buttons to raise terrain,\nright mouse button to lower it")), wxSizerFlags().Expand());
		gridSizer->Add(Tooltipped(new ToolButton(scenarioEditor.GetToolManager(), scrolledWindow, _("Ridge"), _T("PikeElevation"), wxSize(48, -1)),
			_("Brush with left mouse buttons to raise terrain,\nright mouse button to lower it")), wxSizerFlags().Expand());
		gridSizer->Add(Tooltipped(new ToolButton(scenarioEditor.GetToolManager(), scrolledWindow, _("Smooth"), _T("SmoothElevation"), wxSize(48, -1)),
			_("Brush with left mouse button to smooth terrain,\nright mouse button to roughen it")), wxSizerFlags().Expand());
		gridSizer->Add(Tooltipped(new ToolButton(scenarioEditor.GetToolManager(), scrolledWindow, _("Flatten"), _T("FlattenElevation"), wxSize(48, -1)),
			_("Brush with left mouse button to flatten terrain")), wxSizerFlags().Expand());
		sizer->Add(gridSizer, wxSizerFlags().Expand());
		scrollSizer->Add(sizer, wxSizerFlags().Expand().Border(wxTOP, 10));
	}

	{
		/////////////////////////////////////////////////////////////////////////
		// Terrain texture
		wxSizer* sizer = new wxStaticBoxSizer(wxVERTICAL, scrolledWindow, _("Texture tools"));
		wxSizer* gridSizer = new wxGridSizer(3);
		gridSizer->Add(Tooltipped(new ToolButton(scenarioEditor.GetToolManager(), scrolledWindow, _("Paint"), _T("PaintTerrain"), wxSize(48, -1)),
			_("Brush with left mouse button to paint texture dominantly,\nright mouse button to paint submissively.\nShift-left-click for eyedropper tool")), wxSizerFlags().Expand());
		gridSizer->Add(Tooltipped(new ToolButton(scenarioEditor.GetToolManager(), scrolledWindow, _("Replace"), _T("ReplaceTerrain"), wxSize(48, -1)),
			_("Replace all of a terrain texture with a new one")), wxSizerFlags().Expand());
		gridSizer->Add(Tooltipped(new ToolButton(scenarioEditor.GetToolManager(), scrolledWindow, _("Fill"), _T("FillTerrain"), wxSize(48, -1)),
			_T("Bucket fill a patch of terrain texture with a new one")), wxSizerFlags().Expand());
		sizer->Add(gridSizer, wxSizerFlags().Expand());
		scrollSizer->Add(sizer, wxSizerFlags().Expand().Border(wxTOP, 10));
	}

	{
		/////////////////////////////////////////////////////////////////////////
		// Brush settings
		wxSizer* sizer = new wxStaticBoxSizer(wxVERTICAL, scrolledWindow, _("Brush"));

		m_TexturePreview = new TexturePreviewPanel(scrolledWindow);
		sizer->Add(m_TexturePreview, wxSizerFlags(1).Expand());

		g_Brush_Elevation.CreateUI(scrolledWindow, sizer);
		scrollSizer->Add(sizer, wxSizerFlags().Expand().Border(wxTOP, 10));
	}

	{
		/////////////////////////////////////////////////////////////////////////
		// Visualise
		wxSizer* sizer = new wxStaticBoxSizer(wxVERTICAL, scrolledWindow, _("Visualise"));
		scrollSizer->Add(sizer, wxSizerFlags().Expand().Border(wxTOP, 10));

		wxFlexGridSizer* visSizer = new wxFlexGridSizer(2, 5, 5);
		visSizer->AddGrowableCol(1);
		sizer->Add(visSizer, wxSizerFlags().Expand());

		wxArrayString defaultChoices;
		defaultChoices.Add(_("(none)"));
		m_PassabilityChoice = new wxChoice(scrolledWindow, ID_Passability, wxDefaultPosition, wxDefaultSize, defaultChoices);
		m_PassabilityChoice->SetSelection(0);

		visSizer->Add(new wxStaticText(scrolledWindow, wxID_ANY, _("Passability")), wxSizerFlags().Align(wxALIGN_CENTER_VERTICAL|wxALIGN_RIGHT));
		visSizer->Add(Tooltipped(m_PassabilityChoice,
			_("View passability classes")), wxSizerFlags().Expand());

		visSizer->Add(new wxStaticText(scrolledWindow, wxID_ANY, _("Priorities")), wxSizerFlags().Align(wxALIGN_CENTER_VERTICAL|wxALIGN_RIGHT));
		visSizer->Add(Tooltipped(new wxCheckBox(scrolledWindow, ID_ShowPriorities, _("")),
			_("Show terrain texture priorities")));
	}

	{
		/////////////////////////////////////////////////////////////////////////
		// Misc tools
		wxSizer* sizer = new wxStaticBoxSizer(wxVERTICAL, scrolledWindow, _("Misc tools"));
		sizer->Add(new wxButton(scrolledWindow, ID_ResizeMap, _("Resize map")), wxSizerFlags().Expand());
		scrollSizer->Add(sizer, wxSizerFlags().Expand().Border(wxTOP, 10));
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
	m_TexturePreview->LoadPreview();
}

void TerrainSidebar::OnPassabilityChoice(wxCommandEvent& evt)
{
	if (evt.GetSelection() == 0)
		POST_MESSAGE(SetViewParamS, (AtlasMessage::eRenderView::GAME, L"passability", L""));
	else
		POST_MESSAGE(SetViewParamS, (AtlasMessage::eRenderView::GAME, L"passability", (std::wstring)evt.GetString().wc_str()));
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
	AtObj sizes = AtlasObject::LoadFromJSON(*qrySizes.sizes);
	for (AtIter s = sizes["Data"]["item"]; s.defined(); ++s)
	{
		sizeNames.Add(wxString::FromUTF8(s["Name"]));
		sizeTiles.push_back((*s["Tiles"]).getLong());
	}

	// TODO: set default based on current map size

	wxSingleChoiceDialog dlg(this, _("Select new map size. WARNING: This probably only works reliably on blank maps."),
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
		m_ScrolledPanel->SetBackgroundColour(wxColor(255, 255, 255));

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

		AtlasMessage::qGetTerrainGroupPreviews qry((std::wstring)m_Name.wc_str(), imageWidth, imageHeight);
		qry.Post();

		std::vector<AtlasMessage::sTerrainTexturePreview> previews = *qry.previews;

		bool allLoaded = true;

		for (size_t i = 0; i < previews.size(); ++i)
		{
			if (!previews[i].loaded)
				allLoaded = false;

			wxString name = previews[i].name.c_str();

			// Construct the wrapped-text label
			wxStaticText* label = new wxStaticText(m_ScrolledPanel, wxID_ANY, FormatTextureName(name), wxDefaultPosition, wxDefaultSize, wxALIGN_CENTER);
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
			m_ItemSizer->Add(imageSizer, wxSizerFlags().Expand());
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
		g_SelectedTexture.NotifyObservers();

		if (m_LastTerrainSelection)
			m_LastTerrainSelection->SetBackgroundColour(wxNullColour);

		button->SetBackgroundColour(wxColor(255, 255, 0));
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
	wxButton* m_LastTerrainSelection; // button that was last selected, so we can undo its coloring

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
			wxString visibleName = FormatTextureName(m_TerrainGroups[i]);
			AddPage(new TextureNotebookPage(m_ScenarioEditor, this, m_TerrainGroups[i]), visibleName);
		}

		// On some platforms (wxOSX) there is no initial OnPageChanged event, so it loads with a blank page
		//	and setting selection to 0 won't trigger it either, so just force first page to display
		// (this is safe because the sidebar has already been displayed)
		if (GetPageCount() > 0)
		{
			static_cast<TextureNotebookPage*>(GetPage(0))->OnDisplay();
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
