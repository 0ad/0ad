/* Copyright (C) 2009 Wildfire Games.
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
#include "General/Datafile.h"
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
	TerrainBottomBar(wxWindow* parent);
	void LoadTerrain();
private:
	TextureNotebook* m_Textures;
};


TerrainSidebar::TerrainSidebar(ScenarioEditor& scenarioEditor, wxWindow* sidebarContainer, wxWindow* bottomBarContainer)
: Sidebar(scenarioEditor, sidebarContainer, bottomBarContainer)
{
	// TODO: Less ugliness

	m_MainSizer->Add(new wxStaticText(this, wxID_ANY, _T("TODO: Make this much less ugly\n")));

	{
		wxSizer* sizer = new wxStaticBoxSizer(wxHORIZONTAL, this, _("Elevation tools"));
		sizer->Add(new ToolButton(scenarioEditor.GetToolManager(), this, _("Modify"), _T("AlterElevation"), wxSize(50,20)));
		sizer->Add(new ToolButton(scenarioEditor.GetToolManager(), this, _("Smooth"), _T("SmoothElevation"), wxSize(50,20)));
		sizer->Add(new ToolButton(scenarioEditor.GetToolManager(), this, _("Flatten"), _T("FlattenElevation"), wxSize(50,20)));
//		sizer->Add(new ToolButton(scenarioEditor.GetToolManager(), this, _("Smooth"), _T(""), wxSize(50,20)));
//		sizer->Add(new ToolButton(scenarioEditor.GetToolManager(), this, _("Sample"), _T(""), wxSize(50,20)));
		sizer->Add(new ToolButton(scenarioEditor.GetToolManager(), this, _("Paint"), _T("PaintTerrain"), wxSize(50,20)));
		m_MainSizer->Add(sizer);
	}

	{
		wxSizer* sizer = new wxStaticBoxSizer(wxVERTICAL, this, _("Brush"));
		g_Brush_Elevation.CreateUI(this, sizer);
		m_MainSizer->Add(sizer);
	}

	m_BottomBar = new TerrainBottomBar(bottomBarContainer);
}

void TerrainSidebar::OnFirstDisplay()
{
	static_cast<TerrainBottomBar*>(m_BottomBar)->LoadTerrain();
}

//////////////////////////////////////////////////////////////////////////

class TextureNotebookPage : public wxPanel
{
public:
	TextureNotebookPage(wxWindow* parent, const wxString& name)
		: wxPanel(parent, wxID_ANY), m_Name(name), m_ListCtrl(NULL)
	{
	}

	void OnDisplay()
	{
		if (m_ListCtrl)
		{
			int sel = m_ListCtrl->GetNextItem(-1, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED);
			if (sel != -1)
				SetSelection(m_ListCtrl->GetItemData(sel));
			return;
		}

		wxBusyInfo busy (_("Loading terrain previews"));

		m_TextureNames.Clear();

		wxSizer* sizer = new wxBoxSizer(wxVERTICAL);

		m_ListCtrl = new wxListCtrl(this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLC_ICON | wxLC_SINGLE_SEL | wxLC_AUTOARRANGE);

		const int imageWidth = 64;
		const int imageHeight = 32;
		wxImageList* imglist = new wxImageList(imageWidth, imageHeight, false, 0);

		AtlasMessage::qGetTerrainGroupPreviews qry(m_Name.c_str(), imageWidth, imageHeight);
		qry.Post();

		std::vector<AtlasMessage::sTerrainGroupPreview> previews = *qry.previews;

		int i = 0;
		for (std::vector<AtlasMessage::sTerrainGroupPreview>::iterator it = previews.begin(); it != previews.end(); ++it)
		{
			unsigned char* buf = (unsigned char*)(malloc(it->imagedata.GetSize()));
			// it->imagedata.GetBuffer() gives a Shareable<unsigned char>*, which
			// is stored the same as a unsigned char*, so we can just copy it.
			memcpy(buf, it->imagedata.GetBuffer(), it->imagedata.GetSize());
			wxImage img (imageWidth, imageHeight, buf);
			imglist->Add(wxBitmap(img));

			wxListItem item;

			wxString name = it->name.c_str();
			m_TextureNames.Add(name);
			// Add spaces into the displayed name, so Windows doesn't just say
			// "grass_..." in the list ctrl for every terrain
			name.Replace(_T("_"), _T(" "));
			item.SetText(name);

			item.SetData(i);
			item.SetId(i);
			item.SetImage(i);

			m_ListCtrl->InsertItem(item);
			++i;
		}
		m_ListCtrl->AssignImageList(imglist, wxIMAGE_LIST_NORMAL);

		sizer->Add(m_ListCtrl, wxSizerFlags().Expand().Proportion(1));
		SetSizer(sizer);
		Layout(); // required to make things size correctly
	}

	void OnSelect(wxListEvent& evt)
	{
		SetSelection(evt.GetData());
	}

	void SetSelection(int n)
	{
		if (n >= 0 && n < (int)m_TextureNames.GetCount())
			g_SelectedTexture = m_TextureNames[n];
	}

private:
	wxListCtrl* m_ListCtrl;
	wxString m_Name;
	wxArrayString m_TextureNames;

	DECLARE_EVENT_TABLE();
};

BEGIN_EVENT_TABLE(TextureNotebookPage, wxPanel)
	EVT_LIST_ITEM_SELECTED(wxID_ANY, TextureNotebookPage::OnSelect)
END_EVENT_TABLE();


class TextureNotebook : public wxNotebook
{
public:
	TextureNotebook(wxWindow *parent)
		: wxNotebook(parent, wxID_ANY/*, wxDefaultPosition, wxDefaultSize, wxNB_FIXEDWIDTH*/)
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
		std::vector<std::wstring> groupnames = *qry.groupnames;
		for (std::vector<std::wstring>::iterator it = groupnames.begin(); it != groupnames.end(); ++it)
			m_TerrainGroups.Add(it->c_str());

		for (size_t i = 0; i < m_TerrainGroups.GetCount(); ++i)
		{
			wxString visibleName = m_TerrainGroups[i];
			if (visibleName.Len())
				visibleName[0] = wxToupper(visibleName[0]);
			AddPage(new TextureNotebookPage(this, m_TerrainGroups[i]), visibleName);
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
	wxArrayString m_TerrainGroups;
	DECLARE_EVENT_TABLE();
};

BEGIN_EVENT_TABLE(TextureNotebook, wxNotebook)
	EVT_NOTEBOOK_PAGE_CHANGED(wxID_ANY, TextureNotebook::OnPageChanged)
END_EVENT_TABLE();

//////////////////////////////////////////////////////////////////////////

TerrainBottomBar::TerrainBottomBar(wxWindow* parent)
	: wxPanel(parent, wxID_ANY)
{
	wxSizer* sizer = new wxBoxSizer(wxVERTICAL);
	m_Textures = new TextureNotebook(this);
	sizer->Add(m_Textures, wxSizerFlags().Expand().Proportion(1));
	SetSizer(sizer);
}

void TerrainBottomBar::LoadTerrain()
{
	m_Textures->LoadTerrain();
}
