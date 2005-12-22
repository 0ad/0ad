#include "stdafx.h"

#include "Object.h"

#include "Buttons/ActionButton.h"
#include "Buttons/ToolButton.h"
#include "ScenarioEditor/Tools/Common/Tools.h"

#include "GameInterface/Messages.h"

class ObjectSelectListBox : public wxListBox
{
public:
	ObjectSelectListBox(wxWindow* parent)
		: wxListBox(parent, wxID_ANY, wxDefaultPosition, wxDefaultSize, 0, NULL, wxLB_SINGLE|wxLB_HSCROLL)
	{
	}

	void OnSelect(wxCommandEvent& evt)
	{
		wxString id = static_cast<wxStringClientData*>(evt.GetClientObject())->GetData();
		SetCurrentTool(_T("PlaceObject"), &id);
	}
private:
	DECLARE_EVENT_TABLE();
};
BEGIN_EVENT_TABLE(ObjectSelectListBox, wxListBox)
	EVT_LISTBOX(wxID_ANY, ObjectSelectListBox::OnSelect)
END_EVENT_TABLE();


class ObjectChoiceCtrl : public wxChoice
{
public:
	ObjectChoiceCtrl(wxWindow* parent, const wxArrayString& strings, ObjectSidebar& sidebar)
		: wxChoice(parent, wxID_ANY, wxDefaultPosition, wxDefaultSize, strings),
		m_Sidebar(sidebar)
	{
		SetSelection(0);
	}

	void OnSelect(wxCommandEvent& evt)
	{
		m_Sidebar.SetObjectFilter(evt.GetSelection());
	}
private:
	ObjectSidebar& m_Sidebar;
	DECLARE_EVENT_TABLE();
};
BEGIN_EVENT_TABLE(ObjectChoiceCtrl, wxChoice)
	EVT_CHOICE(wxID_ANY, ObjectChoiceCtrl::OnSelect)
END_EVENT_TABLE();

//////////////////////////////////////////////////////////////////////////

struct ObjectSidebarImpl
{
	ObjectSidebarImpl() : m_BottomBar(NULL), m_ObjectListBox(NULL) { }
	wxWindow* m_BottomBar;
	wxListBox* m_ObjectListBox;
	std::vector<AtlasMessage::sObjectsListItem> m_Objects;
};

ObjectSidebar::ObjectSidebar(wxWindow* parent)
: Sidebar(parent), p(new ObjectSidebarImpl())
{
	wxArrayString strings;
	strings.Add(_("Entities"));
	strings.Add(_("Actors (all)"));
	m_MainSizer->Add(new ObjectChoiceCtrl(this, strings, *this), wxSizerFlags().Expand());

	p->m_ObjectListBox = new ObjectSelectListBox(this);
	m_MainSizer->Add(p->m_ObjectListBox, wxSizerFlags().Proportion(1).Expand());
}

ObjectSidebar::~ObjectSidebar()
{
	delete p;
}

wxWindow* ObjectSidebar::GetBottomBar(wxWindow* WXUNUSED(parent))
{
	if (p->m_BottomBar)
		return p->m_BottomBar;

//	m_BottomBar = new ObjectBottomBar(parent);
	return p->m_BottomBar;
}

void ObjectSidebar::OnFirstDisplay()
{
	AtlasMessage::qGetObjectsList qry;
	qry.Post();
	p->m_Objects = qry.objects;
	SetObjectFilter(0);
}

void ObjectSidebar::SetObjectFilter(int type)
{
	p->m_ObjectListBox->Freeze();
	p->m_ObjectListBox->Clear();
	for (std::vector<AtlasMessage::sObjectsListItem>::iterator it = p->m_Objects.begin(); it != p->m_Objects.end(); ++it)
	{
		if (it->type == type)
		{
			wxString id = it->id.c_str();
			wxString name = it->name.c_str();
			p->m_ObjectListBox->Append(name, new wxStringClientData(id));
		}
	}
	p->m_ObjectListBox->Thaw();
}

//////////////////////////////////////////////////////////////////////////


//TerrainBottomBar::TerrainBottomBar(wxWindow* parent)
//	: wxPanel(parent, wxID_ANY)
//{
//	wxSizer* sizer = new wxBoxSizer(wxVERTICAL);
//	wxNotebook* notebook = new TextureNotebook(this);
//	sizer->Add(notebook, wxSizerFlags().Expand().Proportion(1));
//	SetSizer(sizer);
//}
