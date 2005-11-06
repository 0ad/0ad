#include "stdafx.h"

#include "Terrain.h"

#include "Buttons/ActionButton.h"
#include "Buttons/ToolButton.h"
#include "General/Datafile.h"
#include "ScenarioEditor/Tools/Common/Brushes.h"
#include "ScenarioEditor/Tools/Common/Tools.h"
#include "ScenarioEditor/Tools/Common/MiscState.h"

#include "GameInterface/Messages.h"

#include "wx/spinctrl.h"

TerrainSidebar::TerrainSidebar(wxWindow* parent)
: Sidebar(parent), m_BottomBar(NULL)
{
	// TODO: Less ugliness

	m_MainSizer->Add(new wxStaticText(this, wxID_ANY, _T("TODO: Make this much less ugly\n")));

	{
		wxSizer* sizer = new wxStaticBoxSizer(wxHORIZONTAL, this, _("Elevation tools"));
		sizer->Add(new ToolButton(this, _("Modify"), _T("AlterElevation"), wxSize(50,20)));
//		sizer->Add(new ToolButton(this, _("Smooth"), _T(""), wxSize(50,20)));
//		sizer->Add(new ToolButton(this, _("Sample"), _T(""), wxSize(50,20)));
		sizer->Add(new ToolButton(this, _("Paint"), _T("PaintTerrain"), wxSize(50,20)));
		m_MainSizer->Add(sizer);
	}

	{
		wxSizer* sizer = new wxStaticBoxSizer(wxVERTICAL, this, _("Brush"));
		g_Brush_Elevation.CreateUI(this, sizer);
		m_MainSizer->Add(sizer);
	}

}

wxWindow* TerrainSidebar::GetBottomBar(wxWindow* parent)
{
	if (m_BottomBar)
		return m_BottomBar;

	m_BottomBar = new TerrainBottomBar(parent);
	return m_BottomBar;
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

		m_TextureNames.Clear();

		wxSizer* sizer = new wxBoxSizer(wxVERTICAL);

		m_ListCtrl = new wxListCtrl(this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLC_ICON | wxLC_SINGLE_SEL | wxLC_AUTOARRANGE);

		const int imageWidth = 64;
		const int imageHeight = 32;
		wxImageList* imglist = new wxImageList(imageWidth, imageHeight, false, 0);

		AtlasMessage::qGetTerrainGroupPreviews qry(m_Name.c_str(), imageWidth, imageHeight);
		qry.Post();

		int i = 0;
		for (std::vector<AtlasMessage::sTerrainGroupPreview>::iterator it = qry.previews.begin(); it != qry.previews.end(); ++it)
		{
			wxImage img (imageWidth, imageHeight, it->imagedata);
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
		// Get the list of terrain groups from the engine
		AtlasMessage::qGetTerrainGroups qry;
		qry.Post();
		for (std::vector<std::wstring>::iterator it = qry.groupnames.begin(); it != qry.groupnames.end(); ++it)
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
		if (event.GetSelection() != -1)
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
	wxNotebook* notebook = new TextureNotebook(this);
	sizer->Add(notebook, wxSizerFlags().Expand().Proportion(1));
	SetSizer(sizer);
}