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

#include "Cinematic.h"

#include "GameInterface/Messages.h"
#include "CustomControls/Buttons/ActionButton.h"
//#include "CustomControls/Buttons/FloatingSpinCtrl.h"
#include "General/Datafile.h"
#include "ScenarioEditor/ScenarioEditor.h"
#include "HighResTimer/HighResTimer.h"

#include "General/VideoRecorder/VideoRecorder.h"

#include "wx/spinctrl.h"
#include "wx/filename.h"
#include "wx/wfstream.h"
#include "wx/listctrl.h"
#include "wx/imaglist.h"

#include <sstream>

using namespace AtlasMessage;

#define CINEMA_EPSILON .0032f

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
	void AddLists(CinematicSidebar* side, PathListCtrl* paths, NodeListCtrl* nodes);
	void OnText(wxCommandEvent& WXUNUSED(event))
	{
		m_OldScale = CinemaTextFloat(*m_TimeText, 2, -5.f, 5.f, m_OldScale);
		m_Sidebar->UpdatePath(m_Name->GetLineText(0).wc_str(), m_OldScale);
	}
	void Update(std::wstring name, float scale)
	{
		m_Name->SetValue( wxString(name.c_str()) );
		m_TimeText->SetValue( wxString::Format(L"%f", scale) );
		if ( m_OldPathIndex != m_Sidebar->GetSelectedPath() )
		{
			m_OldPathIndex = m_Sidebar->GetSelectedPath();
			m_OldScale = 0.f;
		}
		CinemaTextFloat(*m_TimeText, 2, -5.f, 5.f, 0.f);
	}
private:
	wxStaticBoxSizer* m_Sizer;
	CinematicSidebar* m_Sidebar;
	wxTextCtrl* m_Name, *m_TimeText;
	float m_OldScale;
	ssize_t m_OldPathIndex;
	DECLARE_EVENT_TABLE();
};
BEGIN_EVENT_TABLE(CinematicBottomBar, wxPanel)
EVT_TEXT_ENTER(wxID_ANY, CinematicBottomBar::OnText)
END_EVENT_TABLE()
	
/////////////////////////////////////////////////////////////////////

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
		m_Sidebar->AddPath(msgString, GetItemCount()-1);
	}
	void DeletePath()
	{
		DeleteItem(m_Sidebar->GetSelectedPath());
		m_Sidebar->DeletePath();
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
		m_Sidebar->AddNode(info.pX, info.pY, info.pZ, info.rX, info.rY, info.rZ, GetItemCount()-1);
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
		m_Sidebar->UpdateNode(info.pX, info.pY, info.pZ, info.rX, info.rY, info.rZ, true, -1.f);
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

/////////////////////////////////////////////////////////////

class CinemaSpinnerBox : public wxPanel
{
public:
	enum {	RotationX_ID, RotationY_ID, RotationZ_ID, PositionX_ID, PositionY_ID, PositionZ_ID };
	CinemaSpinnerBox(wxWindow* parent, CinematicSidebar* side) 
		: wxPanel(parent), m_OldT(0), m_OldIndex(0)
	{
		m_MainSizer = new wxBoxSizer(wxHORIZONTAL);
		SetSizer(m_MainSizer);
		m_Sidebar = side;

		wxStaticBoxSizer* rotation = new wxStaticBoxSizer(wxHORIZONTAL,this, _T("Rotation"));
		wxStaticBoxSizer* position = new wxStaticBoxSizer(wxHORIZONTAL,this, _T("Position")); 
		wxStaticBoxSizer* time = new wxStaticBoxSizer(wxHORIZONTAL, this, _T("Time"));

		m_NodeRotationX = new wxSpinCtrl(this, RotationX_ID, _T("x"), 
			wxDefaultPosition, wxSize(50, 15), wxSP_ARROW_KEYS|wxSP_WRAP );
		m_NodeRotationY = new wxSpinCtrl(this, RotationY_ID, _T("y"), 
			wxDefaultPosition, wxSize(50, 15), wxSP_ARROW_KEYS|wxSP_WRAP );
		m_NodeRotationZ = new wxSpinCtrl(this, RotationZ_ID, _T("z"), 
			wxDefaultPosition, wxSize(50, 15), wxSP_ARROW_KEYS|wxSP_WRAP );
		
		m_NodePositionX = new wxSpinCtrl(this, PositionX_ID, _T("x"), 
			wxDefaultPosition, wxSize(55, 15), wxSP_ARROW_KEYS|wxSP_WRAP );
		m_NodePositionY = new wxSpinCtrl(this, PositionY_ID, _T("y"), 
			wxDefaultPosition, wxSize(55, 15), wxSP_ARROW_KEYS|wxSP_WRAP );
		m_NodePositionZ = new wxSpinCtrl(this, PositionZ_ID, _T("z"), 
			wxDefaultPosition, wxSize(55, 15), wxSP_ARROW_KEYS | wxSP_WRAP );
		m_NodeT = new wxTextCtrl(this, wxID_ANY, _T("0.00"),
			wxDefaultPosition, wxSize(55, 15), wxTE_PROCESS_ENTER );

		time->Add(m_NodeT);
			
		m_NodeRotationX->SetRange(-360, 360);
		m_NodeRotationY->SetRange(-360, 360);
		m_NodeRotationZ->SetRange(-360, 360);
		m_NodePositionX->SetRange(-1000, 1000);	//TODO make these more exact
		m_NodePositionY->SetRange(-200, 600);
		m_NodePositionZ->SetRange(-1000, 1000);

		rotation->Add(m_NodeRotationX);
		rotation->Add(m_NodeRotationY);
		rotation->Add(m_NodeRotationZ);
		position->Add(m_NodePositionX);
		position->Add(m_NodePositionY);
		position->Add(m_NodePositionZ);

		m_MainSizer->Add(rotation);
		m_MainSizer->Add(position);
		m_MainSizer->Add(time);
	}

	void OnNodePush(wxSpinEvent& WXUNUSED(event))
	{
		m_Sidebar->UpdateNode( m_NodePositionX->GetValue(), m_NodePositionY->GetValue(), 
							   m_NodePositionZ->GetValue(), m_NodeRotationX->GetValue(), 
							   m_NodeRotationY->GetValue(), m_NodeRotationZ->GetValue(), false, m_OldT);
	}

	void OnText(wxCommandEvent& WXUNUSED(event))
	{
		m_OldT = CinemaTextFloat(*m_NodeT, 2, 0.f, 100.f, m_OldT);
		m_Sidebar->UpdateNode( m_NodePositionX->GetValue(), m_NodePositionY->GetValue(), 
							   m_NodePositionZ->GetValue(), m_NodeRotationX->GetValue(), 
							   m_NodeRotationY->GetValue(), m_NodeRotationZ->GetValue(), false, m_OldT );
	}

	void UpdateRotationSpinners(int x, int y, int z)
	{
		m_NodeRotationX->SetValue(x);
		m_NodeRotationY->SetValue(y);
		m_NodeRotationZ->SetValue(z);
	}

	void UpdatePositionSpinners(int x, int y, int z, float t, ssize_t index)
	{
		m_NodePositionX->SetValue(x);
		m_NodePositionY->SetValue(y);
		m_NodePositionZ->SetValue(z);
		m_NodeT->SetValue(wxString::Format(L"%f", t));

		if ( m_OldIndex != index )
		{
			m_OldT = 0.f;
			m_OldIndex = index;
		}
		m_OldT = CinemaTextFloat(*m_NodeT, 2, 0.f, 100.f, m_OldT);
	}
private:
	wxSpinCtrl* m_NodeRotationX, *m_NodeRotationY, *m_NodeRotationZ, *m_NodePositionX, *m_NodePositionY, *m_NodePositionZ;
	wxTextCtrl* m_NodeT;
	float m_OldT;
	int m_OldIndex;
	wxBoxSizer* m_MainSizer;
	CinematicSidebar* m_Sidebar;
	DECLARE_EVENT_TABLE();
};

BEGIN_EVENT_TABLE(CinemaSpinnerBox, wxPanel)
EVT_SPINCTRL(CinemaSpinnerBox::RotationX_ID, CinemaSpinnerBox::OnNodePush)
EVT_SPINCTRL(CinemaSpinnerBox::RotationY_ID, CinemaSpinnerBox::OnNodePush)
EVT_SPINCTRL(CinemaSpinnerBox::RotationZ_ID, CinemaSpinnerBox::OnNodePush)
EVT_SPINCTRL(CinemaSpinnerBox::PositionX_ID, CinemaSpinnerBox::OnNodePush)
EVT_SPINCTRL(CinemaSpinnerBox::PositionY_ID, CinemaSpinnerBox::OnNodePush)
EVT_SPINCTRL(CinemaSpinnerBox::PositionZ_ID, CinemaSpinnerBox::OnNodePush)
EVT_TEXT_ENTER(wxID_ANY, CinemaSpinnerBox::OnText)
END_EVENT_TABLE()

/////////////////////////////////////////////////////////////
CinematicBottomBar::CinematicBottomBar(wxWindow* parent, CinematicSidebar* side)
: wxPanel(parent), m_Sidebar(side), m_OldScale(0), m_OldPathIndex(-1)
{
	m_Sizer = new wxStaticBoxSizer(wxVERTICAL, this);
	SetSizer(m_Sizer);
}
void CinematicBottomBar::AddLists(CinematicSidebar* side, PathListCtrl* paths, NodeListCtrl* nodes)
{
	wxBoxSizer* top = new wxBoxSizer(wxHORIZONTAL);
	CinemaSpinnerBox* spinners = new CinemaSpinnerBox(this, side);
	side->SetSpinners(spinners);

	wxBoxSizer* pathButtons = new wxBoxSizer(wxVERTICAL);
	wxBoxSizer* nodeButtons = new wxBoxSizer(wxVERTICAL);

	ActionButton* PathAdd = new ActionButton(this, _T("Add"),
							&CinemaPathAdd, paths, wxSize(40, 18));
	ActionButton* NodeAdd = new ActionButton(this, _T("Add"),
							&CinemaNodeAdd, nodes, wxSize(40, 18));
	ActionButton* PathDel = new ActionButton(this, _T("Del"),
							&CinemaPathDel, paths, wxSize(40, 18));
	ActionButton* NodeDel = new ActionButton(this, _T("Del"),
							&CinemaNodeDel, nodes, wxSize(40, 18));	
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

	pathButtons->Add(PathAdd, 0);
	pathButtons->Add(PathDel, 0);
	nodeButtons->Add(NodeAdd, 0);
	nodeButtons->Add(NodeDel, 0);
	nodeButtons->Add(NodeUpdate, 0);
	nodeButtons->Add(NodeGoto, 0);
	
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
	enum { Mode_ID, Style_ID, Rotation_ID, Spline_ID, Reset_ID };

	CinemaInfoBox(CinematicSidebar* side) : wxPanel(side), 
	m_Sidebar(side), m_OldGrowth(0), m_OldSwitch(0), m_OldPathIndex(-1)
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
		m_Sizer->Add(m_DrawCurrent, 0);
		m_Sizer->Add( new wxButton(this, Reset_ID, L"Reset Camera"), 0, wxALIGN_CENTER );
	}
	void Update(const sCinemaPath* path)
	{
		m_ModeBox->SetSelection(path->mode);
		m_StyleBox->SetSelection(path->style);
		float growth = path->growth;	//wxFormat doesn't like to share(able)
		float change = path->change;
		
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
		m_Sidebar->UpdatePathInfo( m_ModeBox->GetSelection(), m_StyleBox->GetSelection(), 
								m_OldGrowth, m_OldSwitch,  m_DrawCurrent->IsChecked(), 
												m_SplineDisplay->GetSelection() != 0 );
	}
	void OnRotation(wxCommandEvent& WXUNUSED(event))
	{
		m_Sidebar->m_RotationAbsolute = m_RotationDisplay->GetSelection()!=0;
		m_Sidebar->UpdateSpinners();
	}
	void ResetCamera(wxCommandEvent& WXUNUSED(event))
	{
		POST_MESSAGE(CinemaEvent, ( m_Sidebar->GetSelectedPathName(), eCinemaEventMode::RESET, 
								0.0f, GetDrawCurrent(), GetDrawLines() ) );
	}
	void UpdateOldIndex()
	{
		if ( m_Sidebar->GetSelectedPath() != m_OldPathIndex )
		{
			m_OldPathIndex = m_Sidebar->GetSelectedPath();
			m_OldGrowth = m_OldSwitch = 0.f;
		}
	}	

	bool GetDrawCurrent() { return m_DrawCurrent->IsChecked(); }
	bool GetDrawLines() { return m_SplineDisplay->GetSelection()!=0; }

private:
	wxBoxSizer* m_Sizer;
	CinematicSidebar* m_Sidebar;
	wxRadioBox* m_ModeBox, *m_StyleBox, *m_RotationDisplay, *m_SplineDisplay;
	wxTextCtrl* m_Growth, *m_Switch;
	wxCheckBox* m_DrawCurrent;

	float m_OldGrowth, m_OldSwitch;
	ssize_t m_OldPathIndex;
	DECLARE_EVENT_TABLE();
};
BEGIN_EVENT_TABLE(CinemaInfoBox, wxPanel)
EVT_RADIOBOX(CinemaInfoBox::Mode_ID, CinemaInfoBox::OnChange)
EVT_RADIOBOX(CinemaInfoBox::Style_ID, CinemaInfoBox::OnChange)
EVT_RADIOBOX(CinemaInfoBox::Spline_ID, CinemaInfoBox::OnChange)
EVT_CHECKBOX(wxID_ANY, CinemaInfoBox::OnChange)
EVT_RADIOBOX(CinemaInfoBox::Rotation_ID, CinemaInfoBox::OnRotation)
EVT_TEXT_ENTER(wxID_ANY, CinemaInfoBox::OnChange)
EVT_BUTTON(CinemaInfoBox::Reset_ID, CinemaInfoBox::ResetCamera)
END_EVENT_TABLE()

class PathSlider : public wxSlider
{
	static const int range=1024;
public:
	PathSlider(CinematicSidebar* side) 
		: wxSlider(side, wxID_ANY, 0, 0, range, wxDefaultPosition,
			wxDefaultSize, wxSL_HORIZONTAL, wxDefaultValidator, _("Path")),
			m_Sidebar(side), m_OldTime(0), m_NewTime(0)
	{
		m_Timer.SetOwner(this);
	}
	
	void Update()
	{
		if ( m_Sidebar->m_SelectedPath < 0 )
			return;
		int sliderValue = (m_Sidebar->m_TimeElapsed / m_Sidebar->GetCurrentPath()->duration) * range;
		SetValue(sliderValue);
	}
	
	void OnTick(wxTimerEvent& WXUNUSED(event))
	{
		m_NewTime = m_HighResTimer.GetTime();
		m_Sidebar->m_TimeElapsed += m_NewTime - m_OldTime;
		
		if ( m_Sidebar->m_TimeElapsed >= m_Sidebar->GetCurrentPath()->duration )
		{
			m_Timer.Stop();
			m_Sidebar->m_TimeElapsed = 0.0f;
			POST_MESSAGE(CinemaEvent, 
				( *m_Sidebar->GetCurrentPath()->name, eCinemaEventMode::IMMEDIATE_PATH, 0.0f,
				m_Sidebar->m_InfoBox->GetDrawCurrent(), m_Sidebar->m_InfoBox->GetDrawLines()) );
		}

		Update();
		m_OldTime = m_NewTime;
	}
	void OnScroll(wxScrollEvent& WXUNUSED(event));

	void PrepareTimers()
	{
		m_OldTime = m_NewTime = m_HighResTimer.GetTime();
		m_Timer.Start(10);
	}
	void Reset()
	{
		SetValue(0);
	}
	
	float m_OldTime;
	float m_NewTime;
	wxTimer m_Timer;

private:
	HighResTimer m_HighResTimer;
	CinematicSidebar* m_Sidebar;

	DECLARE_EVENT_TABLE();
};
BEGIN_EVENT_TABLE(PathSlider, wxSlider)
	EVT_SCROLL(PathSlider::OnScroll)
	EVT_TIMER(wxID_ANY, PathSlider::OnTick)
END_EVENT_TABLE()


void PathSlider::OnScroll(wxScrollEvent& WXUNUSED(event))
{
	m_Timer.Stop();
	if ( m_Sidebar->m_SelectedPath < 0 )
	{
		SetValue(0);
		return;		
	}

	//Move path and send movement message.  If blank path, ignore
	if ( m_Sidebar->GetCurrentPath()->duration < .0001f )
		return;

	float ratio = (float)GetValue() / (float)range;
	float time = ratio * m_Sidebar->GetCurrentPath()->duration;
	
	POST_MESSAGE(CinemaEvent,
	( m_Sidebar->GetSelectedPathName(), eCinemaEventMode::IMMEDIATE_PATH, time, 
		  m_Sidebar->m_InfoBox->GetDrawCurrent(),  m_Sidebar->m_InfoBox->GetDrawLines()) );

	m_Sidebar->m_TimeElapsed = time;
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
		if ( m_Parent->m_SelectedPath < 0 )
			return;
		
		m_Parent->m_PathSlider->m_Timer.Stop();
		m_Parent->m_Playing = false;
		float timeSet = 0.0f;
		std::vector<sCinemaSplineNode> nodes = *m_Parent->GetCurrentPath()->nodes;

		for ( size_t i = 0; i < nodes.size(); ++i )
		{
			timeSet += nodes[i].t;
			if ( fabs((timeSet - m_Parent->m_TimeElapsed)) < .0001f )
			{
				timeSet -= nodes[i].t;
				break;
			}
		}

		m_Parent->m_TimeElapsed = timeSet;
		POST_MESSAGE(CinemaEvent, 
			( m_Parent->GetSelectedPathName(), eCinemaEventMode::IMMEDIATE_PATH, timeSet, 
			m_Parent->m_InfoBox->GetDrawCurrent(), m_Parent->m_InfoBox->GetDrawLines()) );

		m_Parent->m_PathSlider->Update();
	}
	void OnStop(wxCommandEvent& WXUNUSED(event))
	{
		if ( m_Parent->m_SelectedPath < 0)
			return;

		m_Parent->m_PathSlider->m_Timer.Stop();
		m_Parent->m_Playing = false;
		m_Parent->m_TimeElapsed = 0.0f;
		m_Parent->m_PathSlider->Update();

		POST_MESSAGE(CinemaEvent,
			(m_Parent->GetSelectedPathName(), eCinemaEventMode::IMMEDIATE_PATH, 0.0f, 
			m_Parent->m_InfoBox->GetDrawCurrent(), m_Parent->m_InfoBox->GetDrawLines()) );
	}
	void OnPlay(wxCommandEvent& WXUNUSED(event))
	{
		if ( m_Parent->m_SelectedPath < 0 )
			return;
	
		m_Parent->m_PathSlider->m_Timer.Stop();
		m_Parent->m_PathSlider->PrepareTimers();

		POST_MESSAGE(CinemaEvent,
			(m_Parent->GetSelectedPathName(), 
			eCinemaEventMode::SMOOTH, 0.0f, m_Parent->m_InfoBox->GetDrawCurrent(),
			m_Parent->m_InfoBox->GetDrawLines()) );
			
		m_Parent->m_Playing = true;
	}
	void OnRecord(wxCommandEvent& WXUNUSED(event))
	{
		if ( m_Parent->m_SelectedPath < 0 )
			return;
		m_Parent->m_PathSlider->m_Timer.Stop();
		m_Parent->m_PathSlider->PrepareTimers();

		VideoRecorder::RecordCinematic(this,
			wxString( m_Parent->GetSelectedPathName().c_str() ), m_Parent->GetCurrentPath()->duration);
	}
	void OnPause(wxCommandEvent& WXUNUSED(event))
	{
		if ( m_Parent->m_SelectedPath < 0 )
			return;
		m_Parent->m_PathSlider->m_Timer.Stop();
		m_Parent->m_Playing = false;

		POST_MESSAGE(CinemaEvent,
			(m_Parent->GetSelectedPathName(), 
			eCinemaEventMode::IMMEDIATE_PATH, m_Parent->m_TimeElapsed,
			m_Parent->m_InfoBox->GetDrawCurrent(), m_Parent->m_InfoBox->GetDrawLines()) );
	}

	void OnNext(wxCommandEvent& WXUNUSED(event))
	{
		if ( m_Parent->m_SelectedPath < 0 )
			return;

		m_Parent->m_PathSlider->m_Timer.Stop();
		m_Parent->m_Playing = false;
		std::wstring name = m_Parent->GetSelectedPathName();
		std::vector<sCinemaSplineNode> nodes = *m_Parent->GetCurrentPath()->nodes;
		float timeSet = 0.0f;

		for ( size_t i = 0; i < nodes.size(); ++i )
		{
			timeSet += nodes[i].t;
			if ( timeSet > m_Parent->m_TimeElapsed )
				break;
		}
		m_Parent->m_TimeElapsed = timeSet;

		POST_MESSAGE(CinemaEvent, 
		( name, eCinemaEventMode::IMMEDIATE_PATH, timeSet,
		m_Parent->m_InfoBox->GetDrawCurrent(), m_Parent->m_InfoBox->GetDrawLines()) );
		
		m_Parent->m_PathSlider->Update();
	}
	CinematicSidebar* m_Parent;
	wxStaticBoxSizer* m_Sizer;
	DECLARE_EVENT_TABLE();
};

BEGIN_EVENT_TABLE(CinemaButtonBox, wxPanel)
	EVT_BUTTON(eCinemaButton::previous, CinemaButtonBox::OnPrevious)
	EVT_BUTTON(eCinemaButton::stop, CinemaButtonBox::OnStop)
	EVT_BUTTON(eCinemaButton::play, CinemaButtonBox::OnPlay)
	EVT_BUTTON(eCinemaButton::pause, CinemaButtonBox::OnPause)
	EVT_BUTTON(eCinemaButton::next, CinemaButtonBox::OnNext)
	EVT_BUTTON(eCinemaButton::record, CinemaButtonBox::OnRecord)
END_EVENT_TABLE()
//////////////////////////////////////////////////////////////////////////

CinematicSidebar::CinematicSidebar(ScenarioEditor& scenarioEditor, wxWindow* sidebarContainer, wxWindow* bottomBarContainer)
: Sidebar(scenarioEditor, sidebarContainer, bottomBarContainer), m_SelectedPath(-1), m_SelectedSplineNode(-1),
m_TimeElapsed(0.f), m_RotationAbsolute(false), m_Playing(false)
{
	m_PathSlider = new PathSlider(this);
	wxStaticBoxSizer* sliderBox = new wxStaticBoxSizer(wxVERTICAL, this, _T("Timeline"));
	sliderBox->Add(m_PathSlider);
	m_MainSizer->Add(sliderBox, 0, wxALIGN_CENTER);
	m_IconSizer = new CinemaButtonBox(this);
	LoadIcons(); //do this here; buttons must be added before box is

	m_MainSizer->Add(m_IconSizer, 0, wxALIGN_CENTER);
	m_InfoBox = new CinemaInfoBox(this);
	m_MainSizer->Add(m_InfoBox, 0, wxALIGN_CENTER);
	
	CinematicBottomBar* bottom = new CinematicBottomBar(bottomBarContainer, this);
	m_BottomBar = bottom;
	m_CinemaBottomBar = bottom;  //avoid casting later

	m_PathList = new PathListCtrl(bottom, this);
	m_NodeList = new NodeListCtrl(bottom, this);
	bottom->AddLists(this, m_PathList, m_NodeList);
}

void CinematicSidebar::OnFirstDisplay()
{
	qGetCinemaPaths qry;
	qry.Post();
	m_Paths = *qry.paths;

	m_PathList->Freeze();
	for (size_t path = 0; path < m_Paths.size(); ++path)
	{
		m_PathList->InsertItem((long)path, wxString(m_Paths[path].name.c_str()));
	}
	m_PathList->Thaw();

}		

void CinematicSidebar::SelectPath(ssize_t n)
{
	if (n == -1)
	{
		m_PathList->DeleteAllItems();
		//Do this here to avoid thinking that there's still a path
		m_SelectedPath = n;
		SelectSplineNode(-1);
		return;
	}
	else
	{
		wxCHECK_RET (n >= 0 && n < (ssize_t)m_Paths.size(), _T("SelectPath out of bounds"));
		
		m_SelectedPath = n;
		SelectSplineNode(-1);
		m_NodeList->Freeze();
		m_NodeList->DeleteAllItems();

		if (n != -1)
		{
			size_t size = GetCurrentPath()->nodes.GetSize();
			for ( size_t i=0; i<size; ++i )
			{
				m_NodeList->InsertItem((long)i, wxString::Format(_("Node %d"), i));
			}
		}

		m_NodeList->Thaw();
	}
	POST_MESSAGE(CinemaEvent, ( GetSelectedPathName(), eCinemaEventMode::SELECT, 
		(int)m_SelectedPath, m_InfoBox->GetDrawCurrent(), m_InfoBox->GetDrawLines() ));
	
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
const sCinemaPath* CinematicSidebar::GetCurrentPath() const
{
	return &m_Paths[m_SelectedPath];
}

sCinemaSplineNode CinematicSidebar::GetCurrentNode() const
{
	if ( m_SelectedSplineNode < 0 )
	{
		wxLogError(_("Invalid request for current spline node (no node selected)"));
		return sCinemaSplineNode();
	}
	return (*GetCurrentPath()->nodes)[m_SelectedSplineNode];
}
//copied from sectionlayout's addpage()
wxImage CinematicSidebar::LoadIcon(const wxString& filename)
{
	wxImage img (1, 1, true);

	// Load the icon
	wxFileName iconPath (_T("tools/atlas/buttons/"));
	iconPath.MakeAbsolute(Datafile::GetDataDirectory());
	iconPath.SetFullName(filename);
	wxFFileInputStream fstr (iconPath.GetFullPath());
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

void CinematicSidebar::AddPath(std::wstring& name, int count)  //rotation
{
	m_Paths.push_back( sCinemaPath(name) );
	SelectSplineNode(-1);
	m_SelectedSplineNode = -1;
	m_SelectedPath = count;	  //Ensure that selection is valid for UpdateEngineData
	UpdateEngineData();	  //Make sure the path exists in the engine before we send a select message to it
	SelectPath(count);
	
}
void CinematicSidebar::AddNode(float px, float py, float pz, float rx, float ry, float rz, int count)
{
	if ( m_SelectedPath < 0 )
		return;

	std::vector<sCinemaSplineNode> nodes = *GetCurrentPath()->nodes;
	sCinemaSplineNode newNode(px, py, pz, rx, ry, rz);
	if ( !nodes.empty() )
	{
		newNode.SetTime(1.0f);
		m_Paths[m_SelectedPath].duration = m_Paths[m_SelectedPath].duration + 1.0f;
	}
	
	nodes.push_back(newNode);
	m_Paths[m_SelectedPath].nodes = nodes;
	UpdateEngineData();
	SelectSplineNode(count, count+1);
}

void CinematicSidebar::DeletePath()
{
	if ( m_SelectedPath < 0 )
		return;

	m_Paths.erase( m_Paths.begin() + m_SelectedPath );
	ssize_t size = (ssize_t)m_Paths.size();
	
	m_SelectedSplineNode = -1;
	if ( size == 0 )
		SelectPath(-1);
	else if ( m_SelectedPath > size-1 )
		SelectPath(size-1);
	else
		SelectPath(m_SelectedPath);
	
	UpdateEngineData();
}

void CinematicSidebar::DeleteNode()
{
	if ( m_SelectedPath < 0 || m_SelectedSplineNode < 0 )
		return;

	std::vector<sCinemaSplineNode> nodes = *m_Paths[m_SelectedPath].nodes;
	m_Paths[m_SelectedPath].duration = m_Paths[m_SelectedPath].duration - nodes[m_SelectedSplineNode].t;
	
	if ( m_TimeElapsed > m_Paths[m_SelectedPath].duration )
		m_TimeElapsed = m_Paths[m_SelectedPath].duration;

	nodes.erase( nodes.begin() + m_SelectedSplineNode );
	ssize_t size = (ssize_t)nodes.size();

	if ( m_SelectedSplineNode == 0 && size != 0 )
		nodes[m_SelectedSplineNode].t = 0;	//Reset the first node's time to 0
	m_Paths[m_SelectedPath].nodes = nodes;
	
	
	if ( size == 0 )
		SelectSplineNode(-1);
	else if ( m_SelectedSplineNode > size-1 )
		SelectSplineNode(size-1, size);
	else
		SelectSplineNode(m_SelectedSplineNode, size);
	
	SelectPath(m_SelectedPath);	//Correct numbering
	UpdateEngineData();
}	

void CinematicSidebar::UpdatePath(std::wstring name, float timescale)
{
	if ( m_SelectedPath < 0 )
		return;
	
	m_Paths[m_SelectedPath].name = name;
	m_Paths[m_SelectedPath].timescale = timescale;
	m_PathList->SetItemText(m_SelectedPath, name.c_str());
	UpdateEngineData();
}

void CinematicSidebar::UpdateNode(float px, float py, float pz, float rx, float ry, float rz, bool absoluteOveride, float t)
{
	if ( m_SelectedPath < 0 || m_SelectedSplineNode < 0 )
		return;
	else if ( m_SelectedSplineNode == 0 && t > 0.0f )
	{
		wxBell();	//Let them know: the first node has no meaning
		return;
	}

	std::vector<sCinemaSplineNode> nodes = *GetCurrentPath()->nodes;
	if ( t < 0 )
		t = nodes[m_SelectedSplineNode].t;
	
	if ( m_RotationAbsolute || m_SelectedSplineNode == 0 || absoluteOveride )
	{
		nodes[m_SelectedSplineNode].rx = rx;
		nodes[m_SelectedSplineNode].ry = ry;
		nodes[m_SelectedSplineNode].rz = rz;
	}
	else
	{
		nodes[m_SelectedSplineNode].rx = rx + nodes[m_SelectedSplineNode-1].rx;
		nodes[m_SelectedSplineNode].ry = ry + nodes[m_SelectedSplineNode-1].ry;
		nodes[m_SelectedSplineNode].rz = rz + nodes[m_SelectedSplineNode-1].rz;
	}

	sCinemaSplineNode newNode(px, py, pz, nodes[m_SelectedSplineNode].rx, 
				nodes[m_SelectedSplineNode].ry, nodes[m_SelectedSplineNode].rz);
	newNode.SetTime(t);
	float delta = newNode.t - nodes[m_SelectedSplineNode].t;
	m_Paths[m_SelectedPath].duration = m_Paths[m_SelectedPath].duration + delta;

	nodes[m_SelectedSplineNode] = newNode;
	m_Paths[m_SelectedPath].nodes = nodes;

	UpdateEngineData();
}

void CinematicSidebar::UpdateSpinners()
{
	if ( m_SelectedPath < 0 || m_SelectedSplineNode < 0 ) 
		return;
	
	std::vector<sCinemaSplineNode> nodes = *GetCurrentPath()->nodes;
	sCinemaSplineNode node = nodes[m_SelectedSplineNode];
	
	if ( !m_RotationAbsolute && m_SelectedSplineNode != 0 )
	{
		m_SpinnerBox->UpdateRotationSpinners(node.rx - nodes[m_SelectedSplineNode-1].rx, 
									 	 node.ry - nodes[m_SelectedSplineNode-1].ry, 
										 node.rz - nodes[m_SelectedSplineNode-1].rz);
	}
	else
		m_SpinnerBox->UpdateRotationSpinners(node.rx, node.ry, node.rz);

	m_SpinnerBox->UpdatePositionSpinners(node.px, node.py, node.pz, node.t, m_SelectedSplineNode);
}

void CinematicSidebar::UpdateTexts()
{
	if ( m_SelectedPath < 0 )
		return;

	m_CinemaBottomBar->Update(GetSelectedPathName(), m_Paths[m_SelectedPath].timescale );
	if ( m_SelectedPath >= 0 )
	{
		const sCinemaPath* path = GetCurrentPath();
		m_InfoBox->Update(path);
	}
}
void CinematicSidebar::UpdatePathInfo(int mode, int style, float growth, float change, 
													bool drawCurrent, bool drawLine)
{
	if ( m_SelectedPath < 0 )
		return;

	m_Paths[m_SelectedPath].mode = mode;
	m_Paths[m_SelectedPath].style = style;
	m_Paths[m_SelectedPath].growth = growth;
	m_Paths[m_SelectedPath].change = change;

	UpdateEngineData();
	POST_MESSAGE( CinemaEvent, (GetSelectedPathName(), 
		eCinemaEventMode::SELECT, (int)m_SelectedPath, drawCurrent, drawLine) );
}

void CinematicSidebar::UpdateEngineData()
{
	POST_COMMAND(SetCinemaPaths, (m_Paths) );
	SendEngineSelection();
	UpdateSpinners();
}
void CinematicSidebar::SendEngineSelection()
{
	POST_MESSAGE(CinemaEvent, ( GetSelectedPathName(), eCinemaEventMode::SELECT, 
		(int)m_SelectedPath, m_InfoBox->GetDrawCurrent(), m_InfoBox->GetDrawLines() ));
}
void CinematicSidebar::GotoNode(ssize_t index)
{
	if ( m_SelectedPath < 0 || m_SelectedSplineNode < 0 )
		return;
	if ( index < 0 )
		index = m_SelectedSplineNode;

	std::vector<sCinemaSplineNode> nodes = *GetCurrentPath()->nodes;
	float time = 0;

	for ( ssize_t i=0; i<=index; ++i )
		time += nodes[i].t;
	
	m_TimeElapsed = time;
	POST_MESSAGE( CinemaEvent, (GetSelectedPathName(), eCinemaEventMode::IMMEDIATE_PATH, time, 
								m_InfoBox->GetDrawCurrent(), m_InfoBox->GetDrawLines()) );
	
	//this is just an echo if false
	if ( m_TimeElapsed / GetCurrentPath()->duration < 1.f )
		m_PathSlider->Update();
}

std::wstring CinematicSidebar::GetSelectedPathName() const 
{ 
	if ( m_SelectedPath < 0 )
		return std::wstring(L"Invalid path");
	return *m_Paths[m_SelectedPath].name; 
}
