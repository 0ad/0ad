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
		: wxListBox(parent, wxID_ANY, wxDefaultPosition, wxDefaultSize, 0, NULL, wxLB_SINGLE)
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

//////////////////////////////////////////////////////////////////////////

ObjectSidebar::ObjectSidebar(wxWindow* parent)
: Sidebar(parent), m_BottomBar(NULL)
{
	m_ObjectListBox = new ObjectSelectListBox(this);
	m_MainSizer->Add(m_ObjectListBox, wxSizerFlags().Proportion(1).Expand());
	
}

wxWindow* ObjectSidebar::GetBottomBar(wxWindow* parent)
{
	if (m_BottomBar)
		return m_BottomBar;

//	m_BottomBar = new ObjectBottomBar(parent);
	return m_BottomBar;
}

void ObjectSidebar::OnFirstDisplay()
{
	AtlasMessage::qGetEntitiesList qry;
	qry.Post();
	for (std::vector<AtlasMessage::sEntitiesListItem>::iterator it = qry.entities.begin(); it != qry.entities.end(); ++it)
	{
		wxString id = it->id.c_str();
		wxString name = it->name.c_str();
		m_ObjectListBox->Append(name, new wxStringClientData(id));
	}
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
