#include "stdafx.h"

#include "Cinematic.h"

#include "GameInterface/Messages.h"
#include "CustomControls/Buttons/ActionButton.h"
//#include "CustomControls/Buttons/FloatingSpinCtrl.h"
#include "General/Datafile.h"
#include "ScenarioEditor/Tools/Common/Tools.h"
#include "HighResTimer/HighResTimer.h"

#include "General/VideoRecorder/VideoRecorder.h"

#include "wx/spinctrl.h"
#include "wx/filename.h"
#include "wx/wfstream.h"

#include <sstream>

using namespace AtlasMessage;

#define CINEMA_EPSILON .032f

struct eCinemaButton 
{ 
	enum { 
		previous/*, rewind, reverse*/, stop, 
		play, pause, /*forward,*/ next, record }; 
};

float CinemaTextFloat(wxTextCtrl&, size_t, float, float, float);

class CinematicBottomBar : public wxPanel
{
	friend void TimescaleSpin(void*);
public:
	CinematicBottomBar(wxWindow* parent, CinematicSidebar* side);
	void AddLists(CinematicSidebar* side, TrackListCtrl* tracks,
				PathListCtrl* paths, NodeListCtrl* nodes);
	void OnText(wxCommandEvent& WXUNUSED(event))
	{
		m_OldScale = CinemaTextFloat(*m_TimeText, 2, -5.f, 5.f, m_OldScale);
		m_Sidebar->UpdateTrack(m_Name->GetLineText(0).wc_str(), m_OldScale);
	}
	void Update(std::wstring name, float scale)
	{
		m_Name->SetValue( wxString(name.c_str()) );
		m_TimeText->SetValue( wxString::Format(L"%f", scale) );
		if ( m_OldTrackIndex != m_Sidebar->GetSelectedTrack() )
		{
			m_OldTrackIndex = m_Sidebar->GetSelectedTrack();
			m_OldScale = 0.f;
		}
		CinemaTextFloat(*m_TimeText, 2, -5.f, 5.f, 0.f);
	}
private:
	wxStaticBoxSizer* m_Sizer;
	CinematicSidebar* m_Sidebar;
	wxTextCtrl* m_Name, *m_TimeText;
	float m_OldScale;
	ssize_t m_OldTrackIndex;
	DECLARE_EVENT_TABLE();
};
BEGIN_EVENT_TABLE(CinematicBottomBar, wxPanel)
EVT_TEXT_ENTER(wxID_ANY, CinematicBottomBar::OnText)
END_EVENT_TABLE()
	
/////////////////////////////////////////////////////////////////////
class TrackListCtrl : public wxListCtrl
{
public:
	TrackListCtrl(wxWindow* parent, CinematicSidebar* side)
		: wxListCtrl(parent, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLC_REPORT|/*wxLC_EDIT_LABELS|*/wxLC_SINGLE_SEL),
		m_Sidebar(side)
	{
		InsertColumn(0, _("Tracks"), wxLIST_FORMAT_LEFT, 180);
	}

	void OnSelect(wxListEvent& event)
	{
		m_Sidebar->SelectTrack(event.GetIndex());
		m_Sidebar->UpdateTexts();
	}
	void AddTrack()
	{
		std::wstringstream message;
		message << "Track " << GetItemCount();
		std::wstring msgString = message.str();
		wxString fmt( msgString.c_str(), msgString.length() );
		InsertItem(GetItemCount(), fmt);
		
		qGetCameraInfo qry;
		qry.Post();
		sCameraInfo info = qry.info;
		m_Sidebar->AddTrack(info.rX, info.rY, info.rZ, msgString, GetItemCount()-1);
	}
	void DeleteTrack()
	{
		DeleteItem(m_Sidebar->GetSelectedTrack());
		m_Sidebar->DeleteTrack();
	}

private:
	CinematicSidebar* m_Sidebar;

	DECLARE_EVENT_TABLE();
};
BEGIN_EVENT_TABLE(TrackListCtrl, wxListCtrl)
	EVT_LIST_ITEM_SELECTED(wxID_ANY, TrackListCtrl::OnSelect)
END_EVENT_TABLE()

class PathListCtrl : public wxListCtrl
{
public:
	PathListCtrl(wxWindow* parent, CinematicSidebar* side)
		: wxListCtrl(parent, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLC_REPORT|wxLC_SINGLE_SEL),
		m_Sidebar(side)
	{
		InsertColumn(0, _("Paths"), wxLIST_FORMAT_LEFT, 180);
	}

	void OnSelect(wxListEvent& event)
	{
		m_Sidebar->SelectPath(event.GetIndex());
		m_Sidebar->UpdateTexts();
	}
	void AddPath()
	{
		std::wstringstream message;
		message << "Path " << GetItemCount();
		std::wstring msgString = message.str();
		wxString fmt( msgString.c_str(), msgString.length() );
		InsertItem(GetItemCount(), fmt);

		qGetCameraInfo qry;
		qry.Post();
		sCameraInfo info = qry.info;
		m_Sidebar->AddPath(info.rX, info.rY, info.rZ, GetItemCount()-1);
	}
	void DeletePath()
	{
		DeleteItem(m_Sidebar->GetSelectedPath());
		m_Sidebar->DeletePath();
	}
	void UpdatePath()
	{
		qGetCameraInfo qry;
		qry.Post();
		sCameraInfo info = qry.info;
		m_Sidebar->UpdatePath(info.rX, info.rY, info.rZ);
	}

private:
	CinematicSidebar* m_Sidebar;
	
	DECLARE_EVENT_TABLE();
};
BEGIN_EVENT_TABLE(PathListCtrl, wxListCtrl)
	EVT_LIST_ITEM_SELECTED(wxID_ANY, PathListCtrl::OnSelect)
END_EVENT_TABLE()

class NodeListCtrl : public wxListCtrl
{
public:
	NodeListCtrl(wxWindow* parent, CinematicSidebar* side)
		: wxListCtrl(parent, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLC_REPORT|wxLC_SINGLE_SEL),
		m_Sidebar(side)
	{
		InsertColumn(0, _("Nodes"), wxLIST_FORMAT_LEFT, 180);
	}
	void OnSelect(wxListEvent& event)
	{
		m_Sidebar->SelectSplineNode(event.GetIndex(), GetItemCount());
		m_Sidebar->UpdateTexts();
	}	
	void AddNode()
	{	
		std::wstringstream message;
		message << "Node " << GetItemCount();
		std::wstring msgString = message.str();
		wxString fmt( msgString.c_str(), msgString.length() );
		InsertItem(GetItemCount(), fmt);

		qGetCameraInfo qry;
		qry.Post();
		sCameraInfo info = qry.info;
		m_Sidebar->AddNode(info.pX, info.pY, info.pZ, GetItemCount()-1);
	}
	void DeleteNode()
	{
		DeleteItem(m_Sidebar->GetSelectedNode());
		m_Sidebar->DeleteNode();
	}
	void UpdateNode()
	{
		qGetCameraInfo qry;
		qry.Post();
		sCameraInfo info = qry.info;
		m_Sidebar->UpdateNode(info.pX, info.pY, info.pZ);
	}
	void GotoNode()
	{
		m_Sidebar->GotoNode();
	}
private:
	CinematicSidebar* m_Sidebar;
	
	DECLARE_EVENT_TABLE();
};
BEGIN_EVENT_TABLE(NodeListCtrl, wxListCtrl)
	EVT_LIST_ITEM_SELECTED(wxID_ANY, NodeListCtrl::OnSelect)
END_EVENT_TABLE()

/////////////////////////////////////////////////////////////////

void CinemaTrackAdd(void* data)
{
	TrackListCtrl* list = reinterpret_cast<TrackListCtrl*>(data);
	list->AddTrack();
}
void CinemaPathAdd(void* data)
{
	PathListCtrl* list = reinterpret_cast<PathListCtrl*>(data);
	list->AddPath();
}
void CinemaNodeAdd(void* data)
{
	NodeListCtrl* list = reinterpret_cast<NodeListCtrl*>(data);
	list->AddNode();
}
void CinemaTrackDel(void* data)
{
	TrackListCtrl* list = reinterpret_cast<TrackListCtrl*>(data);
	list->DeleteTrack();
}
void CinemaPathDel(void* data)
{
	PathListCtrl* list = reinterpret_cast<PathListCtrl*>(data);
	list->DeletePath();
}
void CinemaNodeDel(void* data)
{
	NodeListCtrl* list = reinterpret_cast<NodeListCtrl*>(data);
	list->DeleteNode();
}

void CinemaPathUpdate(void* data)
{
	PathListCtrl* list = reinterpret_cast<PathListCtrl*>(data);
	list->UpdatePath();
}
void CinemaNodeUpdate(void* data)
{
	NodeListCtrl* list = reinterpret_cast<NodeListCtrl*>(data);
	list->UpdateNode();
}
void CinemaNodeGoto(void* data)
{
	NodeListCtrl* list = reinterpret_cast<NodeListCtrl*>(data);
	list->GotoNode();
}

///////////////////////////////////////////////////////////

void CinemaStringToFloat(wxString& text, size_t decimals)
{
	size_t i, j;
	if ( (i = text.find(L".")) == wxString::npos)
	{
		text.Append('.');
		text.Append('0', decimals);
	}
	else
	{
		j = text.find_last_not_of(L"0");
		text.Remove(j+1);
		//Too many numbers
		if ( j - i > decimals )
			text.Remove(i+decimals+1);
		else
			text.Append('0', decimals - (j-i));
	}
}
float CinemaTextFloat(wxTextCtrl& ctrl, size_t decimals, float min, 
											float max, float oldval)
{
	wxString text = ctrl.GetLineText(0);
	double val;
	if ( !text.ToDouble(&val) )
	{
		wxBell();
		val = oldval;
		text = wxString::Format(L"%f", val);
		CinemaStringToFloat(text, decimals);
	}
	
	if ( val > max )
		val = oldval;
	else if ( val < min )
		val = oldval;
	text = wxString::Format(L"%f", val);
	CinemaStringToFloat(text, decimals);
	ctrl.SetValue(text);
	return val;
	
}
void WrapCinemaPath(sCinemaPath& path)
{
	if ( path.x > 360 )
		path.x = 360;
	else if ( path.x < -360 )
		path.x = -360;
	
	if ( path.y > 360 )
		path.y = 360;
	else if ( path.y < -360 )
		path.y = -360;
	
	if ( path.y > 360 )
		path.z = 360;
	else if ( path.x < -360 )
		path.z = -360;
}
/////////////////////////////////////////////////////////////

class CinemaSpinnerBox : public wxPanel
{
public:
	enum {  TrackX_ID, TrackY_ID, TrackZ_ID,
			PathX_ID, PathY_ID, PathZ_ID,
			NodeX_ID, NodeY_ID, NodeZ_ID };
	CinemaSpinnerBox(wxWindow* parent, CinematicSidebar* side) 
		: wxPanel(parent), m_OldT(0), m_OldIndex(0)
	{
		m_MainSizer = new wxBoxSizer(wxHORIZONTAL);
		SetSizer(m_MainSizer);
		m_Sidebar = side;
		
		wxStaticBoxSizer* tracks = new wxStaticBoxSizer(wxHORIZONTAL,
											this, _T("Start Rotation"));
		wxStaticBoxSizer* paths = new wxStaticBoxSizer(wxHORIZONTAL,
												this, _T("Rotation"));
		wxStaticBoxSizer* nodes = new wxStaticBoxSizer(wxHORIZONTAL,
												this, _T("Position")); 

			m_TrackX = new wxSpinCtrl(this, TrackX_ID, _T("x"), 
		wxDefaultPosition, wxSize(50, 15), wxSP_ARROW_KEYS|wxSP_WRAP );
			m_TrackY = new wxSpinCtrl(this, TrackY_ID, _T("y"), 
		wxDefaultPosition, wxSize(50, 15), wxSP_ARROW_KEYS|wxSP_WRAP );
			m_TrackZ = new wxSpinCtrl(this, TrackZ_ID, _T("z"), 
		wxDefaultPosition, wxSize(50, 15), wxSP_ARROW_KEYS|wxSP_WRAP );

			m_PathX = new wxSpinCtrl(this, PathX_ID, _T("x"), 
		wxDefaultPosition, wxSize(50, 15), wxSP_ARROW_KEYS|wxSP_WRAP );
			m_PathY = new wxSpinCtrl(this, PathY_ID, _T("y"), 
		wxDefaultPosition, wxSize(50, 15), wxSP_ARROW_KEYS|wxSP_WRAP );
			m_PathZ = new wxSpinCtrl(this, PathZ_ID, _T("z"), 
		wxDefaultPosition, wxSize(50, 15), wxSP_ARROW_KEYS|wxSP_WRAP );
		
			m_NodeX = new wxSpinCtrl(this, NodeX_ID, _T("x"), 
		wxDefaultPosition, wxSize(55, 15), wxSP_ARROW_KEYS|wxSP_WRAP );
			m_NodeY = new wxSpinCtrl(this, NodeY_ID, _T("y"), 
		wxDefaultPosition, wxSize(55, 15), wxSP_ARROW_KEYS|wxSP_WRAP );
			m_NodeZ = new wxSpinCtrl(this, NodeZ_ID, _T("z"), 
		wxDefaultPosition, wxSize(55, 15), wxSP_ARROW_KEYS|wxSP_WRAP );
			m_NodeT = new wxTextCtrl(this, wxID_ANY, _T("0.00"),
		wxDefaultPosition, wxSize(55, 15), wxTE_PROCESS_ENTER );
			
		m_TrackX->SetRange(-360, 360);
		m_TrackY->SetRange(-360, 360);
		m_TrackZ->SetRange(-360, 360);
		m_PathX->SetRange(-360, 360);
		m_PathY->SetRange(-360, 360);
		m_PathZ->SetRange(-360, 360);
		m_NodeX->SetRange(-1000, 1000);	//TODO make these more exact
		m_NodeY->SetRange(-200, 600);
		m_NodeZ->SetRange(-1000, 1000);
		
		tracks->Add(m_TrackX);
		tracks->Add(m_TrackY);
		tracks->Add(m_TrackZ);
		paths->Add(m_PathX);
		paths->Add(m_PathY);
		paths->Add(m_PathZ);
		nodes->Add(m_NodeX);
		nodes->Add(m_NodeY);
		nodes->Add(m_NodeZ);
		nodes->Add(m_NodeT);
		
		m_MainSizer->Add(tracks);
		m_MainSizer->Add(paths);
		m_MainSizer->Add(nodes);
	}

	void OnTrackPush(wxSpinEvent& WXUNUSED(event))
	{
		m_Sidebar->UpdateTrack( m_TrackX->GetValue(), 
				m_TrackY->GetValue(), m_TrackZ->GetValue());
	}
	void OnPathPush(wxSpinEvent& WXUNUSED(event))
	{
		m_Sidebar->UpdatePath( m_PathX->GetValue(), 
					m_PathY->GetValue(), m_PathZ->GetValue());
	}
	void OnNodePush(wxSpinEvent& WXUNUSED(event))
	{
		m_OldT = CinemaTextFloat(*m_NodeT, 2, 0.f, 100.f, m_OldT);
		m_Sidebar->UpdateNode( m_NodeX->GetValue(), 
				m_NodeY->GetValue(), m_NodeZ->GetValue(), m_OldT);
	}
	void OnText(wxCommandEvent& WXUNUSED(event))
	{
		m_OldT = CinemaTextFloat(*m_NodeT, 2, 0.f, 100.f, m_OldT);
		m_Sidebar->UpdateNode( m_NodeX->GetValue(), 
				m_NodeY->GetValue(), m_NodeZ->GetValue(), m_OldT);
	}

	void UpdateTrackSpinners(int x, int y, int z)
	{
		m_TrackX->SetValue(x);
		m_TrackY->SetValue(y);
		m_TrackZ->SetValue(z);
	}
	void UpdatePathSpinners(int x, int y, int z)
	{
		m_PathX->SetValue(x);
		m_PathY->SetValue(y);
		m_PathZ->SetValue(z);
	}
	void UpdateNodeSpinners(int x, int y, int z, float t, ssize_t index)
	{
		m_NodeX->SetValue(x);
		m_NodeY->SetValue(y);
		m_NodeZ->SetValue(z);
		m_NodeT->SetValue(wxString::Format(L"%f", t));
		if ( m_OldIndex != index )
		{
			m_OldT = 0.f;
			m_OldIndex = index;
		}
		m_OldT = CinemaTextFloat(*m_NodeT, 2, 0.f, 100.f, m_OldT);
	}
private:
	wxSpinCtrl* m_TrackX, *m_TrackY, *m_TrackZ, *m_PathX, *m_PathY, *m_PathZ, *m_NodeX, *m_NodeY, *m_NodeZ;
	wxTextCtrl* m_NodeT;
	float m_OldT;
	int m_OldIndex;
	wxBoxSizer* m_MainSizer;
	CinematicSidebar* m_Sidebar;
	DECLARE_EVENT_TABLE();
};
BEGIN_EVENT_TABLE(CinemaSpinnerBox, wxPanel)
EVT_SPINCTRL(CinemaSpinnerBox::TrackX_ID, CinemaSpinnerBox::OnTrackPush)
EVT_SPINCTRL(CinemaSpinnerBox::TrackY_ID, CinemaSpinnerBox::OnTrackPush)
EVT_SPINCTRL(CinemaSpinnerBox::TrackZ_ID, CinemaSpinnerBox::OnTrackPush)
EVT_SPINCTRL(CinemaSpinnerBox::PathX_ID, CinemaSpinnerBox::OnPathPush)
EVT_SPINCTRL(CinemaSpinnerBox::PathY_ID, CinemaSpinnerBox::OnPathPush)
EVT_SPINCTRL(CinemaSpinnerBox::PathZ_ID, CinemaSpinnerBox::OnPathPush)
EVT_SPINCTRL(CinemaSpinnerBox::NodeX_ID, CinemaSpinnerBox::OnNodePush)
EVT_SPINCTRL(CinemaSpinnerBox::NodeY_ID, CinemaSpinnerBox::OnNodePush)
EVT_SPINCTRL(CinemaSpinnerBox::NodeZ_ID, CinemaSpinnerBox::OnNodePush)
EVT_TEXT_ENTER(wxID_ANY, CinemaSpinnerBox::OnText)
END_EVENT_TABLE()

/////////////////////////////////////////////////////////////
CinematicBottomBar::CinematicBottomBar(wxWindow* parent, CinematicSidebar* side)
: wxPanel(parent), m_Sidebar(side), m_OldScale(0), m_OldTrackIndex(-1)
{
	m_Sizer = new wxStaticBoxSizer(wxVERTICAL, this);
	SetSizer(m_Sizer);
}
void CinematicBottomBar::AddLists(CinematicSidebar* side, 
		TrackListCtrl* tracks, PathListCtrl* paths, NodeListCtrl* nodes)
{
	wxBoxSizer* top = new wxBoxSizer(wxHORIZONTAL);
	CinemaSpinnerBox* spinners = new CinemaSpinnerBox(this, side);
	side->SetSpinners(spinners);
	wxBoxSizer* trackButtons = new wxBoxSizer(wxVERTICAL);
	wxBoxSizer* pathButtons = new wxBoxSizer(wxVERTICAL);
	wxBoxSizer* nodeButtons = new wxBoxSizer(wxVERTICAL);

	ActionButton* TrackAdd = new ActionButton(this, _T("Add"), 
							&CinemaTrackAdd, tracks, wxSize(40, 18));
	ActionButton* PathAdd = new ActionButton(this, _T("Add"),
							&CinemaPathAdd, paths, wxSize(40, 18));
	ActionButton* NodeAdd = new ActionButton(this, _T("Add"),
							&CinemaNodeAdd, nodes, wxSize(40, 18));
	ActionButton* TrackDel = new ActionButton(this, _T("Del"),
							&CinemaTrackDel, tracks, wxSize(40, 18));
	ActionButton* PathDel = new ActionButton(this, _T("Del"),
							&CinemaPathDel, paths, wxSize(40, 18));
	ActionButton* NodeDel = new ActionButton(this, _T("Del"),
							&CinemaNodeDel, nodes, wxSize(40, 18));	
	ActionButton* PathUpdate =  new ActionButton(this, _T("Mod"),
							&CinemaPathUpdate, paths, wxSize(40, 18));
	ActionButton* NodeUpdate =  new ActionButton(this, _T("Mod"),
							&CinemaNodeUpdate, nodes, wxSize(40, 18));
	ActionButton* NodeGoto = new ActionButton(this, _T("Goto"),
							&CinemaNodeGoto, nodes, wxSize(44, 18));
	
	wxBoxSizer* textBoxes = new wxBoxSizer(wxVERTICAL);
	wxBoxSizer* timescale = new wxBoxSizer(wxHORIZONTAL);

	timescale->Add( new wxStaticText(this, wxID_ANY, _T("Timescale:")) );
	m_TimeText = new wxTextCtrl(this, wxID_ANY, _T("1.00"),
						wxDefaultPosition, wxSize(55, 20));
	timescale->Add( m_TimeText );

	wxBoxSizer* nameBox = new wxBoxSizer(wxHORIZONTAL);
	nameBox->Add( new wxStaticText(this, wxID_ANY, _T("Name:")), 0,
						wxALIGN_CENTER | wxLEFT | wxRIGHT, 5);
	m_Name = new wxTextCtrl(this, wxID_ANY, _T(""), wxDefaultPosition, 
														wxSize(55, 20));
	nameBox->Add( m_Name );
	
	textBoxes->Add(timescale);
	textBoxes->Add(nameBox, 0, wxEXPAND);

	trackButtons->Add(TrackAdd, 0);
	trackButtons->Add(TrackDel, 0);
	pathButtons->Add(PathAdd, 0);
	pathButtons->Add(PathDel, 0);
	pathButtons->Add(PathUpdate, 0);
	nodeButtons->Add(NodeAdd, 0);
	nodeButtons->Add(NodeDel, 0);
	nodeButtons->Add(NodeUpdate, 0);
	nodeButtons->Add(NodeGoto, 0);
	
	top->Add(tracks, 0);
	top->Add(trackButtons, 0);
	top->Add(textBoxes, 0);
	top->Add(paths, 0);
	top->Add(pathButtons, 0);
	top->Add(nodes, 0);
	top->Add(nodeButtons, 0);

	m_Sizer->Add(top, 0);
	m_Sizer->Add(spinners, 0);
}

class CinemaInfoBox : public wxPanel
{
public:
	enum { Mode_ID, Style_ID, Rotation_ID, Spline_ID };

	CinemaInfoBox(CinematicSidebar* side) : wxPanel(side), 
	m_Sidebar(side), m_OldGrowth(0), m_OldSwitch(0), m_OldTrackIndex(-1)
	{
		m_Sizer = new wxBoxSizer(wxVERTICAL);
		SetSizer(m_Sizer);
		//Use individual static boxes for nicer looks
		wxBoxSizer* radios = new wxBoxSizer(wxHORIZONTAL);
		wxBoxSizer* spinners = new wxBoxSizer(wxHORIZONTAL);

		wxString style[5] = {_T("Default"), _T("Growth"), _T("Expo"),
			_T("Circle"), _T("Sine") };
		m_StyleBox = new wxRadioBox(this, Style_ID, _T("Style"), wxDefaultPosition, 
			wxDefaultSize, 5, style, 5, wxRA_SPECIFY_ROWS );
		radios->Add(m_StyleBox, 0, wxALIGN_CENTER);
		
		//Mode radio box
		wxString mode[4]= { _T("Ease in"), _T("Ease Out"),
									_T("In-out"), _T("Out-in") };
		m_ModeBox = new wxRadioBox(this, Mode_ID, _T("Mode"), wxDefaultPosition, 
					wxDefaultSize, 4, mode, 4, wxRA_SPECIFY_ROWS );
		radios->Add(m_ModeBox, 0, wxEXPAND | wxALIGN_CENTER);

		wxStaticText* growth = new wxStaticText(this, wxID_ANY, _T("Growth"));
		wxStaticText* change = new wxStaticText(this, wxID_ANY, _T("Switch"));
		m_Growth = new wxTextCtrl(this, wxID_ANY, _T("0.00"), wxDefaultPosition, 
						wxSize(50, 15), wxTE_PROCESS_ENTER);
		m_Switch = new wxTextCtrl(this, wxID_ANY, _T("0.00"), wxDefaultPosition, 
						wxSize(50, 15), wxTE_PROCESS_ENTER);

		wxStaticBoxSizer* growthBox = new wxStaticBoxSizer(wxHORIZONTAL, this);
		growthBox->Add(growth, 0, wxALIGN_CENTER);
		growthBox->Add(m_Growth, 0, wxALIGN_CENTER);
		wxStaticBoxSizer* switchBox = new wxStaticBoxSizer(wxHORIZONTAL, this);
		switchBox->Add(change, 0, wxALIGN_CENTER);
		switchBox->Add(m_Switch, 0, wxALIGN_CENTER);

		spinners->Add(growthBox);
		spinners->Add(switchBox);
		m_Sizer->Add(radios, 0, wxALIGN_CENTER);
		m_Sizer->Add(spinners, 0, wxALIGN_CENTER);
		
		//Put this here since there isn't any other place yet
		wxBoxSizer* displayH = new wxBoxSizer(wxHORIZONTAL);
		m_DrawAll = new wxCheckBox(this, wxID_ANY, _T("Draw all"));
		m_DrawCurrent = new wxCheckBox(this, wxID_ANY, _T("Draw current"));
		
		wxString splineStrings[2] = { _T("Points"), _T("Lines") };
		m_SplineDisplay = new wxRadioBox(this, Spline_ID, _T("Spline Display"), 
			wxDefaultPosition, wxDefaultSize, 2, splineStrings, 2, wxRA_SPECIFY_ROWS);
		wxString angle[2] = { _T("Relative"), _T("Absolute") };
		m_RotationDisplay = new wxRadioBox(this, Rotation_ID, _T("Rotation Display"), 
			wxDefaultPosition, wxDefaultSize, 2, angle, 2, wxRA_SPECIFY_ROWS );
		
		displayH->Add(m_SplineDisplay, 0);
		displayH->Add(m_RotationDisplay, 0);
		m_Sizer->Add(displayH, 0, wxTop | wxALIGN_CENTER, 10);
		m_Sizer->Add(m_DrawAll, 0);
		m_Sizer->Add(m_DrawCurrent, 0);
		
	}
	void Update(const sCinemaPath& path)
	{
		m_ModeBox->SetSelection(path.mode);
		m_StyleBox->SetSelection(path.style);
		float growth = path.growth;	//wxFormat doesn't like to share(able)
		float change = path.change;
		m_Growth->SetValue( wxString::Format(L"%f", growth) );
		m_Switch->SetValue( wxString::Format(L"%f", change) );
		UpdateOldIndex();
		CinemaTextFloat(*m_Growth, 2, 0.f, 10.f, m_OldGrowth);
		CinemaTextFloat(*m_Switch, 2, 0.f, 10.f, m_OldSwitch);
	}
	void OnChange(wxCommandEvent& WXUNUSED(event))
	{	
		m_OldGrowth = CinemaTextFloat(*m_Growth, 2, 0.f, 10.f, m_OldGrowth);
		m_OldSwitch = CinemaTextFloat(*m_Switch, 2, 0.f, 10.f, m_OldSwitch);
		UpdateOldIndex();
		m_Sidebar->UpdatePathInfo( m_ModeBox->GetSelection(),
			m_StyleBox->GetSelection(), m_OldGrowth, m_OldSwitch,
			m_DrawAll->IsChecked(), m_DrawCurrent->IsChecked(), 
							m_SplineDisplay->GetSelection()!=0 );
	}
	void OnRotation(wxCommandEvent& WXUNUSED(event))
	{
		m_Sidebar->m_RotationAbsolute = m_RotationDisplay->GetSelection()!=0;
		m_Sidebar->UpdateSpinners();
	}
	void UpdateOldIndex()
	{
		if ( m_Sidebar->GetSelectedTrack() != m_OldTrackIndex )
		{
			m_OldTrackIndex = m_Sidebar->GetSelectedTrack();
			m_OldGrowth = m_OldSwitch = 0.f;
		}
	}
	bool GetDrawAll() { return m_DrawAll->IsChecked(); }
	bool GetDrawCurrent() { return m_DrawCurrent->IsChecked(); }
	bool GetDrawLines() { return m_SplineDisplay->GetSelection()!=0; }

private:
	wxBoxSizer* m_Sizer;
	CinematicSidebar* m_Sidebar;
	wxRadioBox* m_ModeBox, *m_StyleBox, *m_RotationDisplay, *m_SplineDisplay;
	wxTextCtrl* m_Growth, *m_Switch;
	wxCheckBox* m_DrawAll, *m_DrawCurrent;

	float m_OldGrowth, m_OldSwitch;
	ssize_t m_OldTrackIndex;
	DECLARE_EVENT_TABLE();
};
BEGIN_EVENT_TABLE(CinemaInfoBox, wxPanel)
EVT_RADIOBOX(CinemaInfoBox::Mode_ID, CinemaInfoBox::OnChange)
EVT_RADIOBOX(CinemaInfoBox::Style_ID, CinemaInfoBox::OnChange)
EVT_RADIOBOX(CinemaInfoBox::Spline_ID, CinemaInfoBox::OnChange)
EVT_CHECKBOX(wxID_ANY, CinemaInfoBox::OnChange)
EVT_RADIOBOX(CinemaInfoBox::Rotation_ID, CinemaInfoBox::OnRotation)
EVT_TEXT_ENTER(wxID_ANY, CinemaInfoBox::OnChange)
END_EVENT_TABLE()

class PathSlider;
class CinemaSliderBox;

class TrackSlider : public wxSlider
{
	static const int range=1024;
public:
	TrackSlider(CinematicSidebar* side, CinemaSliderBox* parent) 
		: wxSlider((wxWindow*)parent, wxID_ANY, 0, 0, range),
		m_Sidebar(side), m_Parent(parent)
	{
	}
	void OnScroll(wxScrollEvent& event);
	void Update(float interval);
	void SetPathSlider(PathSlider* slider)
	{
		m_Path = slider;
	}
	
private:
	PathSlider* m_Path;
	CinematicSidebar* m_Sidebar;
	CinemaSliderBox* m_Parent;

	DECLARE_EVENT_TABLE();
};
BEGIN_EVENT_TABLE(TrackSlider, wxSlider)
	EVT_SCROLL(TrackSlider::OnScroll)
END_EVENT_TABLE()

class PathSlider : public wxSlider
{
	static const int range=1024;
public:
	PathSlider(CinematicSidebar* side, wxWindow* parent) 
		: wxSlider(parent, wxID_ANY, 0, 0, range, wxDefaultPosition,
		wxDefaultSize, wxSL_HORIZONTAL, wxDefaultValidator, _("Path")),
		m_Sidebar(side)
	{
	}

	void OnScroll(wxScrollEvent& WXUNUSED(event));

	void SetTrackSlider(TrackSlider* slider)
	{
		m_Track = slider;
	}
private:
	TrackSlider* m_Track;
	CinematicSidebar* m_Sidebar;

	DECLARE_EVENT_TABLE();
};
BEGIN_EVENT_TABLE(PathSlider, wxSlider)
	EVT_SCROLL(PathSlider::OnScroll)
END_EVENT_TABLE()

class CinemaSliderBox : public wxPanel
{
public:
	CinemaSliderBox(CinematicSidebar* parent, const wxString& label) 
		: wxPanel(parent), m_Sidebar(parent), m_OldTime(0), m_NewTime(0)
	{
		m_Sizer = new wxStaticBoxSizer(wxVERTICAL, this, label);
		SetSizer(m_Sizer);
		m_Path = new PathSlider(parent, this);
		m_Track = new TrackSlider(parent, this);
		m_Path->SetTrackSlider(m_Track);
		m_Track->SetPathSlider(m_Path);

		m_Sizer->Add(m_Track, 0);
		m_Sizer->Add(m_Path, 0);

		m_Timer.SetOwner(this);
	}
	void Update()
	{
		m_Track->Update(0);
	}
	void OnTick(wxTimerEvent& WXUNUSED(event))
	{
		m_NewTime = m_HighResTimer.GetTime();
		m_Track->Update(m_NewTime - m_OldTime);
		m_OldTime = m_NewTime;
	}
	void PrepareTimers()
	{
		m_OldTime = m_NewTime = m_HighResTimer.GetTime();
		m_Timer.Start(10);
	}
	void Reset()
	{
		m_Track->SetValue(0);
		m_Path->SetValue(0);
	}
	
	float m_OldTime;
	float m_NewTime;
	wxTimer m_Timer;
private:
	TrackSlider* m_Track;
	HighResTimer m_HighResTimer;
	PathSlider* m_Path;
	wxStaticBoxSizer* m_Sizer;
	CinematicSidebar* m_Sidebar;

	DECLARE_EVENT_TABLE();
};
BEGIN_EVENT_TABLE(CinemaSliderBox, wxPanel)
	EVT_TIMER(wxID_ANY, CinemaSliderBox::OnTick)
END_EVENT_TABLE()

void TrackSlider::OnScroll(wxScrollEvent& WXUNUSED(event))
{
	//Move path and send movement message
	m_Sidebar->m_SliderBox->m_Timer.Stop();
	if ( m_Sidebar->m_SelectedTrack < 0 || m_Sidebar->m_SelectedPath < 0 )
	{
		SetValue(0);
		return;		
	}
	float ratio = (float)GetValue() / (float)range;
	float time = ratio * m_Sidebar->m_Tracks[m_Sidebar->m_SelectedTrack].duration;
	
	POST_MESSAGE(CinemaEvent,
		( *m_Sidebar->m_Tracks[m_Sidebar->m_SelectedTrack].name, 
		eCinemaEventMode::IMMEDIATE_TRACK, time, 
		m_Sidebar->m_InfoBox->GetDrawAll(), m_Sidebar->m_InfoBox->GetDrawCurrent(), 
		m_Sidebar->m_InfoBox->GetDrawLines() ) );
	
	m_Sidebar->m_AbsoluteTime = time;
	m_Path->SetValue( (int)( m_Sidebar->UpdateSelectedPath() /
		m_Sidebar->GetCurrentPath().duration * range ) );
}
void PathSlider::OnScroll(wxScrollEvent& WXUNUSED(event))
{
	m_Sidebar->m_SliderBox->m_Timer.Stop();
	if ( m_Sidebar->m_SelectedTrack < 0 || m_Sidebar->m_SelectedPath < 0 )
	{
		SetValue(0);
		return;		
	}
	//Move path and send movement message
	float ratio = (float)GetValue() / (float)range;
	float time = ratio * m_Sidebar->GetCurrentPath().duration;
	float trackTime = m_Sidebar->m_AbsoluteTime / m_Sidebar->
			m_Tracks[m_Sidebar->m_SelectedTrack].duration * range;
	m_Sidebar->m_AbsoluteTime += time - m_Sidebar->m_TimeElapsed;
	m_Sidebar->m_TimeElapsed = time;

	m_Track->SetValue( trackTime );
	POST_MESSAGE(CinemaEvent,
		( *m_Sidebar->m_Tracks[m_Sidebar->m_SelectedTrack].name, 
		eCinemaEventMode::IMMEDIATE_TRACK, trackTime,
		m_Sidebar->m_InfoBox->GetDrawAll(), m_Sidebar->m_InfoBox->GetDrawCurrent(),
		m_Sidebar->m_InfoBox->GetDrawLines()) );
		
}

void TrackSlider::Update(float interval)
{
	if ( m_Sidebar->m_SelectedTrack < 0 || m_Sidebar->m_SelectedPath < 0 )
	{
		SetValue(0);
		return;		
	}
	interval *= m_Sidebar->m_Tracks[m_Sidebar->m_SelectedTrack].timescale;
	m_Sidebar->m_AbsoluteTime += interval; 
	int move = m_Sidebar->m_AbsoluteTime / m_Sidebar->m_Tracks
		[m_Sidebar->m_SelectedTrack].duration * (float)range;
	if ( move > range )
		move = range;
	SetValue( move );
	m_Path->SetValue( (int)( m_Sidebar->UpdateSelectedPath() /
	m_Sidebar->GetCurrentPath().duration * range ) );
	
	if ( move == range && m_Sidebar->m_Playing )
	{
		m_Parent->m_Timer.Stop();
		SetValue(0);
		m_Path->SetValue(0);
		m_Sidebar->GotoNode(0);
		m_Sidebar->m_Playing = false;
	}
}
//////////////////////////////////////////////////////////////////////////

class CinemaButtonBox : public wxPanel
{
public:
	CinemaButtonBox(CinematicSidebar* parent) :  wxPanel(parent), m_Parent(parent)
	{
		m_Sizer = new wxStaticBoxSizer(wxHORIZONTAL, this);
		SetSizer(m_Sizer);
	}
	void Add(wxBitmapButton* button)
	{
		m_Sizer->Add(button);
	}
	void OnPrevious(wxCommandEvent& WXUNUSED(event))
	{
		if ( m_Parent->m_SelectedTrack < 0 )
			return;
		m_Parent->m_SliderBox->m_Timer.Stop();
		m_Parent->m_Playing = false;
		std::wstring name=*m_Parent->m_Tracks[m_Parent->m_SelectedTrack].name;

		if ( m_Parent->m_SelectedPath > 0 && m_Parent->m_TimeElapsed 
												< CINEMA_EPSILON)
		{
			m_Parent->SelectPath(m_Parent->m_SelectedPath-1);
			float t = -m_Parent->GetCurrentPath().duration;
			m_Parent->m_AbsoluteTime += t;
			m_Parent->m_TimeElapsed = 0.f;
		
			POST_MESSAGE(CinemaEvent, 
			( name, eCinemaEventMode::IMMEDIATE_PATH, t,
			m_Parent->m_InfoBox->GetDrawAll(), m_Parent->m_InfoBox->GetDrawCurrent(),
			m_Parent->m_InfoBox->GetDrawLines() ) );
		}
		else
		{
			m_Parent->m_AbsoluteTime -= m_Parent->m_TimeElapsed;
			m_Parent->m_TimeElapsed = 0.0f;
				POST_MESSAGE(CinemaEvent, 
				( name, eCinemaEventMode::IMMEDIATE_PATH, 0.0f,
				m_Parent->m_InfoBox->GetDrawAll(), m_Parent->m_InfoBox->GetDrawCurrent(),
				m_Parent->m_InfoBox->GetDrawLines()) );
		}
		m_Parent->m_SliderBox->Update();
	}
	//void OnRewind(wxCommandEvent& event)
	
	//void OnReverse(wxCommandEvent& event)
	void OnStop(wxCommandEvent& WXUNUSED(event))
	{
		if ( m_Parent->m_SelectedTrack < 0 || m_Parent->m_SelectedPath < 0)
			return;
		m_Parent->m_SliderBox->m_Timer.Stop();
		m_Parent->m_Playing = false;
		m_Parent->m_AbsoluteTime = m_Parent->m_TimeElapsed = 0.0f;
		m_Parent->SelectPath(0);
		m_Parent->m_SliderBox->Update();

		POST_MESSAGE(CinemaEvent,
			(*m_Parent->m_Tracks[m_Parent->m_SelectedTrack].name, 
			eCinemaEventMode::IMMEDIATE_TRACK, 0.0f,
			m_Parent->m_InfoBox->GetDrawAll(), m_Parent->m_InfoBox->GetDrawCurrent(),
			m_Parent->m_InfoBox->GetDrawLines()) );
	}
	void OnPlay(wxCommandEvent& WXUNUSED(event))
	{
		if ( m_Parent->m_SelectedTrack < 0 )
			return;
		m_Parent->m_SliderBox->m_Timer.Stop();
		m_Parent->m_SliderBox->PrepareTimers();

		POST_MESSAGE(CinemaEvent,
			(*m_Parent->m_Tracks[m_Parent->m_SelectedTrack].name, 
			eCinemaEventMode::SMOOTH, 0.0f,
			m_Parent->m_InfoBox->GetDrawAll(), m_Parent->m_InfoBox->GetDrawCurrent(),
			m_Parent->m_InfoBox->GetDrawLines()) );
			
		m_Parent->m_Playing = true;
	}
	void OnRecord(wxCommandEvent& WXUNUSED(event))
	{
		if ( m_Parent->m_SelectedTrack < 0 )
			return;
		m_Parent->m_SliderBox->m_Timer.Stop();
		m_Parent->m_SliderBox->PrepareTimers();

		VideoRecorder::RecordCinematic(this,
			m_Parent->m_Tracks[m_Parent->m_SelectedTrack].name.c_str(),
			m_Parent->m_Tracks[m_Parent->m_SelectedTrack].duration);
	}
	void OnPause(wxCommandEvent& WXUNUSED(event))
	{
		if ( m_Parent->m_SelectedTrack < 0 )
			return;
		m_Parent->m_SliderBox->m_Timer.Stop();
		m_Parent->m_SliderBox->Reset();
		//m_Parent->m_NodeList->Thaw();
		m_Parent->m_Playing = false;

		POST_MESSAGE(CinemaEvent,
			(*m_Parent->m_Tracks[m_Parent->m_SelectedTrack].name, 
			eCinemaEventMode::IMMEDIATE_PATH, m_Parent->m_TimeElapsed,
			m_Parent->m_InfoBox->GetDrawAll(), m_Parent->m_InfoBox->GetDrawCurrent(),
			m_Parent->m_InfoBox->GetDrawLines()) );
	}
	//void OnForward(wxCommandEvent& event)
	void OnNext(wxCommandEvent& WXUNUSED(event))
	{
		m_Parent->m_SliderBox->m_Timer.Stop();
		m_Parent->m_Playing = false;
		std::wstring name=*m_Parent->m_Tracks[m_Parent->m_SelectedTrack].name;
		sCinemaPath path = m_Parent->GetCurrentPath();
		float t = path.duration - m_Parent->m_TimeElapsed;

		if ( m_Parent->m_SelectedPath < (ssize_t)m_Parent->m_Tracks
				[m_Parent->m_SelectedTrack].paths.GetSize()-1 )
		{
			m_Parent->SelectPath(m_Parent->m_SelectedPath+1);
			m_Parent->m_TimeElapsed = 0.f;
		}
		else
			m_Parent->m_TimeElapsed = path.duration;
		m_Parent->m_AbsoluteTime += t;	
		
		POST_MESSAGE(CinemaEvent, 
		( name, eCinemaEventMode::IMMEDIATE_PATH, path.duration+CINEMA_EPSILON,
		m_Parent->m_InfoBox->GetDrawAll(), m_Parent->m_InfoBox->GetDrawCurrent(),
		m_Parent->m_InfoBox->GetDrawLines()) );
		
		m_Parent->m_SliderBox->Update();
	}
	CinematicSidebar* m_Parent;
	wxStaticBoxSizer* m_Sizer;
	DECLARE_EVENT_TABLE();
};

BEGIN_EVENT_TABLE(CinemaButtonBox, wxPanel)
	EVT_BUTTON(eCinemaButton::previous, CinemaButtonBox::OnPrevious)
	//EVT_BUTTON(eCinemaButton::rewind, CinemaButtonBox::OnRewind)
	//EVT_BUTTON(eCinemaButton::reverse, CinemaButtonBox::OnReverse)
	EVT_BUTTON(eCinemaButton::stop, CinemaButtonBox::OnStop)
	EVT_BUTTON(eCinemaButton::play, CinemaButtonBox::OnPlay)
	EVT_BUTTON(eCinemaButton::pause, CinemaButtonBox::OnPause)
//	EVT_BUTTON(eCinemaButton::forward, CinemaButtonBox::OnForward)
	EVT_BUTTON(eCinemaButton::next, CinemaButtonBox::OnNext)
	EVT_BUTTON(eCinemaButton::record, CinemaButtonBox::OnRecord)
END_EVENT_TABLE()
//////////////////////////////////////////////////////////////////////////

CinematicSidebar::CinematicSidebar(wxWindow* sidebarContainer, wxWindow* bottomBarContainer)
: Sidebar(sidebarContainer, bottomBarContainer), m_SelectedTrack(-1),
m_SelectedPath(-1), m_SelectedSplineNode(-1), m_TimeElapsed(0.f), 
m_AbsoluteTime(0.f), m_RotationAbsolute(false), m_UpdatePathEcho(false),
m_Playing(false)
{
	m_SliderBox = new CinemaSliderBox(this, _T("Timeline"));
	m_MainSizer->Add(m_SliderBox, 0, wxALIGN_CENTER);
	m_IconSizer = new CinemaButtonBox(this);
	LoadIcons(); //do this here; buttons must be added before box is
	m_MainSizer->Add(m_IconSizer, 0, wxALIGN_CENTER);
	m_InfoBox = new CinemaInfoBox(this);
	m_MainSizer->Add(m_InfoBox, 0, wxALIGN_CENTER);
	
	CinematicBottomBar* bottom = new CinematicBottomBar(bottomBarContainer, this);
	m_BottomBar = bottom;
	m_CinemaBottomBar = bottom;  //avoid casting later
	m_TrackList = new TrackListCtrl(bottom, this);
	m_PathList = new PathListCtrl(bottom, this);
	m_NodeList = new NodeListCtrl(bottom, this);
	bottom->AddLists(this, m_TrackList, m_PathList, m_NodeList);
}

void CinematicSidebar::OnFirstDisplay()
{
	qGetCinemaTracks qry;
	qry.Post();
	m_Tracks = *qry.tracks;

	m_TrackList->Freeze();
	for (size_t track = 0; track < m_Tracks.size(); ++track)
	{
		m_TrackList->InsertItem((long)track, wxString(m_Tracks[track].name.c_str()));
	}
	m_TrackList->Thaw();

}		

void CinematicSidebar::SelectTrack(ssize_t n)
{
	if (n == -1)
	{
		m_SelectedTrack = -1;
		SelectPath(-1);
		return;
	}
	else
	{
		wxCHECK_RET (n >= 0 && n < (ssize_t)m_Tracks.size(), _T("SelectTrack out of bounds"));
		
		m_SelectedTrack = n;
		SelectPath(-1);
		m_PathList->Freeze();
		m_PathList->DeleteAllItems();

		if (n != -1)
		{
			std::vector<sCinemaPath> paths = *m_Tracks[n].paths;
			for (size_t path = 0; path < paths.size(); ++path)
			{
				m_PathList->InsertItem((long)path, wxString::Format(_("Path %d"), path));
			}
		}
		m_PathList->Thaw();
	}

	UpdateSpinners();
}

void CinematicSidebar::SelectPath(ssize_t n)
{
	if (n == -1 || m_SelectedTrack == -1)
	{
		m_PathList->DeleteAllItems();
		//Do this here to avoid thinking that there's still a path
		m_SelectedPath = n;
		SelectSplineNode(-1);
		return;
	}
	else
	{
		std::vector<sCinemaPath> paths = *m_Tracks[m_SelectedTrack].paths;
		ssize_t size = (ssize_t)paths.size();
		wxCHECK_RET (n >= 0 && n < size, _T("SelectPath out of bounds"));
		
		m_SelectedPath = n;
		SelectSplineNode(-1);
		m_NodeList->Freeze();
		m_NodeList->DeleteAllItems();

		if (n != -1)
		{
			std::vector<sCinemaSplineNode> nodes = *paths[n].nodes;
			for ( size_t i=0; i<nodes.size(); ++i )
			{
				m_NodeList->InsertItem((long)i, wxString::Format(_("Node %d"), i));
			}
		}

		m_NodeList->Thaw();
	}
	POST_MESSAGE(CinemaEvent, ( *GetCurrentTrack()->name, 
		eCinemaEventMode::SELECT, (int)m_SelectedPath, m_InfoBox->GetDrawAll(), 
		m_InfoBox->GetDrawCurrent(), m_InfoBox->GetDrawLines() ));
	
	UpdateSpinners();
}

void CinematicSidebar::SelectSplineNode(ssize_t n, ssize_t size)
{
	if (n == -1 || m_SelectedPath == -1)
	{
		m_NodeList->DeleteAllItems();
	}
	else
		wxCHECK_RET (n < size, _T("SelectNode out of bounds"));
	m_SelectedSplineNode = n;
	UpdateSpinners();
}
const sCinemaTrack* CinematicSidebar::GetCurrentTrack()
{
	return &m_Tracks[m_SelectedTrack];
}
sCinemaPath CinematicSidebar::GetCurrentPath()
{
	if ( m_SelectedPath < 0 )
	{
		wxFAIL_MSG(L"CurrentPath() request out of range.  The game will attempt to continue.");
		return sCinemaPath();
	}
	return (*m_Tracks[m_SelectedTrack].paths)[m_SelectedPath];
}
sCinemaSplineNode CinematicSidebar::GetCurrentNode()
{
	return (* (*m_Tracks[m_SelectedTrack].paths)[m_SelectedPath].nodes )
										[m_SelectedSplineNode];
}
//copied from sectionlayout's addpage()
wxImage CinematicSidebar::LoadIcon(const wxString& filename)
{
	wxImage img (1, 1, true);

	// Load the icon
	wxFileName iconPath (_T("tools/atlas/buttons/"));
	iconPath.MakeAbsolute(Datafile::GetDataDirectory());
	iconPath.SetFullName(filename);
	wxFileInputStream fstr (iconPath.GetFullPath());
	if (! fstr.Ok())
	{
		wxLogError(_("Failed to open cinematic icon file '%s'"), iconPath.GetFullPath().c_str());
	}
	else
	{
		img = wxImage(fstr, wxBITMAP_TYPE_BMP);
		if (! img.Ok())
		{
			wxLogError(_("Failed to load cinematic icon image '%s'"), iconPath.GetFullPath().c_str());
			img = wxImage (1, 1, true);
		}
	}
	return img;
}
void CinematicSidebar::LoadIcons()
{
	wxBitmapButton* previous = new wxBitmapButton(m_IconSizer, 
			eCinemaButton::previous, LoadIcon( _T("previous_s.bmp") ));
	m_IconSizer->Add(previous);
	wxBitmapButton* stop = new wxBitmapButton(m_IconSizer, 
			eCinemaButton::stop, LoadIcon( _T("stop_s.bmp") ));
	m_IconSizer->Add(stop);
	wxBitmapButton* play = new wxBitmapButton(m_IconSizer, 
			eCinemaButton::play, LoadIcon( _T("play_s.bmp") ));
	m_IconSizer->Add(play);
	wxBitmapButton* pause = new wxBitmapButton(m_IconSizer, 
			eCinemaButton::pause, LoadIcon( _T("pause_s.bmp") ));
	m_IconSizer->Add(pause);
	wxBitmapButton* next = new wxBitmapButton(m_IconSizer, 
			eCinemaButton::next, LoadIcon( _T("next_s.bmp") ));
	m_IconSizer->Add(next);
	wxBitmapButton* record = new wxBitmapButton(m_IconSizer, 
			eCinemaButton::record, LoadIcon( _T("record_s.bmp") ));
	m_IconSizer->Add(record);
}
void CinematicSidebar::AddTrack(float x, float y, float z, 
								std::wstring& name, int count)
{
	m_Tracks.push_back(sCinemaTrack(x, y, z, name));
	SelectPath(-1);
	SelectTrack(count);
	UpdateEngineData();
}
void CinematicSidebar::AddPath(int x, int y, int z, int count)  //rotation
{
	if ( m_SelectedTrack < 0 )
		return;
	std::vector<sCinemaPath> paths=*m_Tracks[m_SelectedTrack].paths;
	
	//x, y, and z should always be absolute, so adjust here
	if ( count == 0 )
	{
		paths.push_back( sCinemaPath(x - m_Tracks[m_SelectedTrack].x,
		y-m_Tracks[m_SelectedTrack].y, z-m_Tracks[m_SelectedTrack].z) );
	}
	else
	{
		int x2 = paths[count-1].x, y2 = paths[count-1].y, 
									z2 = paths[count-1].z;
		GetAbsoluteRotation(x2, y2, z2, count-1);
		paths.push_back( sCinemaPath(x - x2, y - y2, z - z2) );
	}
	
	m_Tracks[m_SelectedTrack].paths = paths;
	SelectSplineNode(-1);
	SelectPath(count);
	UpdateEngineData();
}
void CinematicSidebar::AddNode(float x, float y, float z, int count)
{
	if ( m_SelectedTrack < 0 || m_SelectedPath < 0 )
		return;

	std::vector<sCinemaPath> paths=*m_Tracks[m_SelectedTrack].paths;
	std::vector<sCinemaSplineNode> nodes=*(*m_Tracks[m_SelectedTrack].paths)
										[m_SelectedPath].nodes;
	nodes.push_back(sCinemaSplineNode(x, y, z));
	paths[m_SelectedPath].nodes = nodes;
	m_Tracks[m_SelectedTrack].paths = paths;
	UpdateEngineData();
	SelectSplineNode(count, count+1);
}
void CinematicSidebar::DeleteTrack()
{
	m_Tracks.erase( m_Tracks.begin() + m_SelectedTrack );
	ssize_t size = (ssize_t)m_Tracks.size();
	
	m_SelectedPath = -1;
	m_SelectedSplineNode = -1;
	if ( size == 0 )
		SelectTrack(-1);
	else if ( m_SelectedTrack > size-1 )
		SelectTrack(size-1);
	else
		SelectTrack(m_SelectedTrack);

	UpdateEngineData();
}
void CinematicSidebar::DeletePath()
{
	if ( m_SelectedTrack < 0 || m_SelectedPath < 0 )
		return;
	std::vector<sCinemaPath> paths = *m_Tracks[m_SelectedTrack].paths;
	paths.erase( paths.begin() + m_SelectedPath );
	m_Tracks[m_SelectedTrack].paths = paths;
	ssize_t size = (ssize_t)paths.size();
	
	m_SelectedSplineNode = -1;
	if ( size == 0 )
		SelectPath(-1);
	else if ( m_SelectedPath > size-1 )
		SelectPath(size-1);
	else
		SelectPath(m_SelectedPath);
	
	SelectTrack(m_SelectedTrack);
	UpdateEngineData();
}
void CinematicSidebar::DeleteNode()
{
	if ( m_SelectedTrack < 0 || m_SelectedPath < 0 || 
									m_SelectedSplineNode < 0 )
	{
		return;
	}
	std::vector<sCinemaPath> paths = *m_Tracks[m_SelectedTrack].paths;
	std::vector<sCinemaSplineNode> nodes = *paths[m_SelectedPath].nodes;
 	m_Tracks[m_SelectedTrack].duration = m_Tracks[m_SelectedTrack].duration - 
									nodes[m_SelectedSplineNode].t;
	paths[m_SelectedPath].duration = paths[m_SelectedPath].duration - 
									nodes[m_SelectedSplineNode].t;
	
	if ( m_AbsoluteTime > m_Tracks[m_SelectedTrack].duration )
		m_AbsoluteTime = m_Tracks[m_SelectedTrack].duration;
	if ( m_TimeElapsed > paths[m_SelectedPath].duration )
		m_TimeElapsed = paths[m_SelectedPath].duration;

	nodes.erase( nodes.begin() + m_SelectedSplineNode );
	paths[m_SelectedPath].nodes = nodes;
	m_Tracks[m_SelectedTrack].paths = paths;
	ssize_t size = (ssize_t)nodes.size();
	
	if ( size == 0 )
		SelectSplineNode(-1);
	else if ( m_SelectedSplineNode > size-1 )
		SelectSplineNode(size-1, size);
	else
		SelectSplineNode(m_SelectedSplineNode, size);
	SelectPath(m_SelectedPath);	//Correct numbering
	UpdateEngineData();
	
}	

void CinematicSidebar::UpdateTrack(std::wstring name, float timescale)
{
	if ( m_SelectedTrack < 0 )
		return;
	m_Tracks[m_SelectedTrack].name = name;
	m_Tracks[m_SelectedTrack].timescale = timescale;
	m_TrackList->SetItemText(m_SelectedTrack, wxString(name.c_str()));
	UpdateEngineData();
}
void CinematicSidebar::UpdateTrack(float x, float y, float z)
{
	if ( m_SelectedTrack < 0 )
		return;

	m_Tracks[m_SelectedTrack].x = x;
	m_Tracks[m_SelectedTrack].y = y;
	m_Tracks[m_SelectedTrack].z = z;
	
	if ( m_Tracks[m_SelectedTrack].paths.GetSize() > 0 )
	{
		//Recalculate relative rotation for path
		int x, y, z;
		bool marker = m_RotationAbsolute;
		m_RotationAbsolute = false;
		GetAbsoluteRotation(x, y, z, 0);
		//(Track rotation is always absolute)
		UpdatePath(x-m_Tracks[m_SelectedTrack].x, y-m_Tracks[m_SelectedTrack].y, 
						z - m_Tracks[m_SelectedTrack].z, 0);
		m_RotationAbsolute = marker;
	}

	UpdateEngineData();
}
void CinematicSidebar::UpdatePath(int x, int y, int z, ssize_t index)
{
	if ( m_SelectedTrack < 0 || m_SelectedPath < 0 )
		return;
	if ( index < 0 )
		index = m_SelectedPath;

	std::vector<sCinemaPath> paths=*m_Tracks[m_SelectedTrack].paths;
	
	//Convert to relative rotation
	if ( m_RotationAbsolute )
	{
		if ( index == 0 )
		{
			paths[index].x = x - m_Tracks[m_SelectedTrack].x;
			paths[index].y = y - m_Tracks[m_SelectedTrack].y;
			paths[index].z = z - m_Tracks[m_SelectedTrack].z;
		}
		else
		{
			int x2, y2, z2;
			GetAbsoluteRotation(x2, y2, z2);
			paths[index].x = x - x2;
			paths[index].y = y - y2;
			paths[index].z = z - z2;
		}
	}
	else
	{
		paths[index].x = x;
		paths[index].y = y;
		paths[index].z = z;
	}
	
	if ( index != (ssize_t)paths.size()-1 && !m_UpdatePathEcho )
	{
		//Recalculate relative rotation for path
		m_Tracks[m_SelectedTrack].paths = paths;
		int x2, y2, z2;
		bool marker = m_RotationAbsolute;
		m_RotationAbsolute = false;
		GetAbsoluteRotation(x, y, z, index);
		GetAbsoluteRotation(x2, y2, z2, index+1);

		m_UpdatePathEcho = true;
		UpdatePath(x2 - x, y2 - y, z2 - z, m_SelectedPath+1);
		m_RotationAbsolute = marker;
	}
	else if ( m_UpdatePathEcho )
		m_UpdatePathEcho = false;


	m_Tracks[m_SelectedTrack].paths = paths;
	UpdateEngineData();
}
void CinematicSidebar::UpdateNode(float x, float y, float z, float t)
{
	if ( m_SelectedTrack < 0 || m_SelectedPath < 0 || 
									m_SelectedSplineNode < 0 )
	{
		return;
	}
	else if ( m_SelectedSplineNode == 0 )
	{
		wxBell();	//Let them know: the first node has no meaning
		return;
	}
	std::vector<sCinemaPath> paths=*m_Tracks[m_SelectedTrack].paths;
	std::vector<sCinemaSplineNode> nodes=*(*m_Tracks[m_SelectedTrack].paths)
										[m_SelectedPath].nodes;
	if ( t < 0 )
		t = nodes[m_SelectedSplineNode].t;
	sCinemaSplineNode newNode(x, y, z);
	newNode.SetTime(t);
	float delta = newNode.t - nodes[m_SelectedSplineNode].t;
	paths[m_SelectedPath].duration = paths[m_SelectedPath].duration 
															+ delta;
	m_Tracks[m_SelectedTrack].duration = m_Tracks[m_SelectedTrack]
													.duration + delta;

	nodes[m_SelectedSplineNode] = newNode;
	paths[m_SelectedPath].nodes = nodes;
	m_Tracks[m_SelectedTrack].paths = paths;
	UpdateEngineData();
}
void CinematicSidebar::UpdateSpinners()
{
	if ( m_SelectedTrack < 0 ) 
		return;
	int x = m_Tracks[m_SelectedTrack].x, y = m_Tracks[m_SelectedTrack].y,
									z = m_Tracks[m_SelectedTrack].z;
	m_SpinnerBox->UpdateTrackSpinners(x, y, z);
	
	if ( m_SelectedPath < 0 )
		return;
	sCinemaPath path = GetCurrentPath();
	x = path.x, y = path.y, z = path.z;
	
	if ( m_RotationAbsolute )
		GetAbsoluteRotation(x, y, z);
	m_SpinnerBox->UpdatePathSpinners(x, y, z);
	
	if ( m_SelectedSplineNode < 0 )
		return;

	sCinemaSplineNode node = (*path.nodes)[m_SelectedSplineNode];
	x = node.x, y = node.y, z = node.z;
	float t = node.t;
	m_SpinnerBox->UpdateNodeSpinners(x, y, z, t, m_SelectedSplineNode);
}
void CinematicSidebar::UpdateTexts()
{
	if ( m_SelectedTrack < 0 )
		return;

	m_CinemaBottomBar->Update(* m_Tracks[m_SelectedTrack].name, 
						m_Tracks[m_SelectedTrack].timescale );
	if ( m_SelectedPath >= 0 )
	{
		sCinemaPath path = GetCurrentPath();
		m_InfoBox->Update(path);
	}
}
void CinematicSidebar::UpdatePathInfo(int mode, int style, float growth, 
				float change, bool drawAll, bool drawCurrent, bool drawLine)
{
	if ( m_SelectedTrack < 0 || m_SelectedPath < 0 )
		return;
	std::vector<sCinemaPath> paths = *m_Tracks[m_SelectedTrack].paths;
	paths[m_SelectedPath].mode = mode;
	paths[m_SelectedPath].style = style;
	paths[m_SelectedPath].growth = growth;
	paths[m_SelectedPath].change = change;
	m_Tracks[m_SelectedTrack].paths = paths;
	UpdateEngineData();
	POST_MESSAGE( CinemaEvent, (*m_Tracks[m_SelectedTrack].name, 
		eCinemaEventMode::SELECT, (int)m_SelectedPath, drawAll, 
							drawCurrent, drawLine) );
}
void CinematicSidebar::UpdateEngineData()
{
	POST_COMMAND(SetCinemaTracks, (m_Tracks) );
	UpdateSpinners();
}
void CinematicSidebar::GotoNode(ssize_t index)
{
	if ( m_SelectedTrack < 0 || m_SelectedPath < 0 ||
								m_SelectedSplineNode < 0 )
	{
		return;
	}
	if ( index < 0 )
		index = m_SelectedSplineNode;

	std::vector<sCinemaPath> paths=*m_Tracks[m_SelectedTrack].paths;
	std::vector<sCinemaSplineNode> nodes=*(*m_Tracks[m_SelectedTrack].paths)
										[m_SelectedPath].nodes;
	float nodeTime=0;
	float pathTime=0;
	for ( ssize_t i=0; i<=index; ++i )
		nodeTime += nodes[i].t;
	for ( ssize_t i=0; i<m_SelectedPath; ++i )
		pathTime += (*m_Tracks[m_SelectedTrack].paths)[m_SelectedPath].duration;
	
	m_TimeElapsed = nodeTime;
	m_AbsoluteTime = pathTime + nodeTime;
	POST_MESSAGE(CinemaEvent,
		( *m_Tracks[m_SelectedTrack].name, 
			eCinemaEventMode::IMMEDIATE_TRACK, m_AbsoluteTime, 
			m_InfoBox->GetDrawAll(), m_InfoBox->GetDrawCurrent(),
			m_InfoBox->GetDrawLines() ) );
	//this is just an echo if false
	if ( m_AbsoluteTime / m_Tracks[m_SelectedTrack].duration + 
			CINEMA_EPSILON < 1.f || !m_Playing )
	{
		m_SliderBox->Update();
	}
}

void CinematicSidebar::GetAbsoluteRotation(int& x, int& y, int& z, ssize_t index)
{
	if ( index < 0 )
		index = m_SelectedPath;
	if ( m_SelectedTrack < 0 || index < 0 )
		return;

	std::vector<sCinemaPath> paths=*m_Tracks[m_SelectedTrack].paths;
	sCinemaPath path;
	for (ssize_t i=0; i <= index; ++i)
			path = path + paths[i];

	path.x = path.x + m_Tracks[m_SelectedTrack].x;
	path.y = path.y + m_Tracks[m_SelectedTrack].y;
	path.z = path.z + m_Tracks[m_SelectedTrack].z;
	WrapCinemaPath(path);
	x = path.x;
	y = path.y;
	z = path.z;
}

float CinematicSidebar::UpdateSelectedPath()
{
	size_t i=0;
	std::vector<sCinemaPath>paths = *m_Tracks[m_SelectedTrack].paths;
	for ( float time=0.0f; i < paths.size(); ++i )
	{
		float duration = paths[i].duration;
		time += duration;
		if ( time > m_AbsoluteTime )
		{
			static size_t index=(size_t)-1;
			if ( index != i )
			{
				SelectPath((ssize_t)i);
				index = i;
			}
			m_TimeElapsed = m_AbsoluteTime-(time-duration);
			return m_TimeElapsed;
		}
	}
	return m_TimeElapsed;
}
