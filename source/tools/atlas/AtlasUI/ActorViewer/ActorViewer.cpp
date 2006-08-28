#include "stdafx.h"

#include "ActorViewer.h"

#include "wx/treectrl.h"

#include "General/Datafile.h"
#include "ScenarioEditor/Tools/Common/Tools.h"
#include "ScenarioEditor/ScenarioEditor.h"
#include "ScenarioEditor/Sections/Environment/LightControl.h"
#include "GameInterface/Messages.h"
#include "CustomControls/Canvas/Canvas.h"
#include "CustomControls/SnapSplitterWindow/SnapSplitterWindow.h"

using namespace AtlasMessage;

//////////////////////////////////////////////////////////////////////////

class ActorCanvas : public Canvas
{
public:
	ActorCanvas(wxWindow* parent, int* attribList)
		: Canvas(parent, attribList, wxBORDER_SUNKEN),
		m_Distance(20.f), m_Angle(0.f), m_LastIsValid(false)
	{
	}

	void PostLookAt()
	{
		POST_MESSAGE(LookAt, (eRenderView::ACTOR,
			Position(m_Distance*sin(m_Angle), m_Distance/2.f, m_Distance*cos(m_Angle)),
			Position(0, 0, 0)));
	}

protected:
	virtual void HandleMouseEvent(wxMouseEvent& evt)
	{
		bool camera_changed = false;

		if (evt.GetWheelRotation())
		{
			float speed = -1.f * ScenarioEditor::GetSpeedModifier();

			m_Distance += evt.GetWheelRotation() * speed / evt.GetWheelDelta();
			
			camera_changed = true;
		}

		if (evt.ButtonDown(wxMOUSE_BTN_LEFT))
		{
			m_LastX = evt.GetX();
			m_LastY = evt.GetY();
			m_LastIsValid = true;
		}
		else if (evt.Dragging() && evt.ButtonIsDown(wxMOUSE_BTN_LEFT) && m_LastIsValid)
		{
			int dx = evt.GetX() - m_LastX;
			int dy = evt.GetY() - m_LastY;
			m_LastX = evt.GetX();
			m_LastY = evt.GetY();

			m_Angle += dx * M_PI/256.f * ScenarioEditor::GetSpeedModifier();
			m_Distance += dy / 8.f * ScenarioEditor::GetSpeedModifier();
			camera_changed = true;
		}
		else if (evt.ButtonUp(wxMOUSE_BTN_ANY))
		{
			// In some situations (e.g. double-clicking the title bar to
			// maximise the window) we get a dragging event without the matching
			// buttondown; so disallow dragging when there was a buttonup since
			// the last buttondown.
			// (TODO: does this affect the scenario editor too?)
			m_LastIsValid = false;
		}

		m_Distance = std::max(m_Distance, 1/64.f); // don't let people fly through the origin

		if (camera_changed)
			PostLookAt();
	}

private:
	float m_Distance;
	float m_Angle;
	int m_LastX, m_LastY;
	bool m_LastIsValid;
};

//////////////////////////////////////////////////////////////////////////

class StringTreeItemData : public wxTreeItemData
{
public:
	StringTreeItemData(const wxString& string) : m_String(string) {}
	wxString m_String;
};

enum
{
	ID_Actors,
	ID_Animations,
	ID_Play,
	ID_Pause,
	ID_Slow,
};

BEGIN_EVENT_TABLE(ActorViewer, wxFrame)
	EVT_CLOSE(ActorViewer::OnClose)
	EVT_TREE_SEL_CHANGED(ID_Actors, ActorViewer::OnTreeSelection)
	EVT_COMBOBOX(ID_Animations, ActorViewer::OnAnimationSelection)
	EVT_TEXT_ENTER(ID_Animations, ActorViewer::OnAnimationSelection)
	EVT_BUTTON(ID_Play, ActorViewer::OnSpeedButton)
	EVT_BUTTON(ID_Pause, ActorViewer::OnSpeedButton)
	EVT_BUTTON(ID_Slow, ActorViewer::OnSpeedButton)
END_EVENT_TABLE()

static void SendToGame(const AtlasMessage::sEnvironmentSettings& settings)
{
	POST_COMMAND(SetEnvironmentSettings, (settings));
}

ActorViewer::ActorViewer(wxWindow* parent)
	: wxFrame(parent, wxID_ANY, _("Actor Viewer"), wxDefaultPosition, wxSize(800, 600)),
	m_CurrentSpeed(0.f)
{
	SetIcon(wxIcon(_T("ICON_ActorEditor")));

	SnapSplitterWindow* splitter = new SnapSplitterWindow(this, 0);
	splitter->SetDefaultSashPosition(250);

	wxPanel* sidePanel = new wxPanel(splitter);

	// TODO: don't have this duplicated from ScenarioEditor.cpp
	int glAttribList[] = {
		WX_GL_RGBA,
		WX_GL_DOUBLEBUFFER,
		WX_GL_DEPTH_SIZE, 24,
		WX_GL_BUFFER_SIZE, 24,
		WX_GL_MIN_ALPHA, 8,
		0
	};

	ActorCanvas* canvas = new ActorCanvas(splitter, glAttribList);

	splitter->SplitVertically(sidePanel, canvas);

	wglMakeCurrent(NULL, NULL);
	POST_MESSAGE(SetContext, (canvas->GetContext()));

	POST_MESSAGE(Init, ());

	canvas->InitSize();
	canvas->PostLookAt();

	//////////////////////////////////////////////////////////////////////////

	// Construct a tree containing all the available actors

	qGetObjectsList qry;
	qry.Post();
	std::vector<sObjectsListItem> objects = *qry.objects;

	m_TreeCtrl = new wxTreeCtrl(sidePanel, ID_Actors);
	wxTreeItemId root = m_TreeCtrl->AddRoot(_("Actors"));

	std::map<std::wstring, wxTreeItemId> treeEntries;

	wxRegEx stripDirs (_T("^([^/]+)/"), wxRE_ADVANCED); // the non-empty string up to the first slash

	for (std::vector<sObjectsListItem>::iterator it = objects.begin(); it != objects.end(); ++it)
	{
		if (it->type != 1)
			continue;

		wxString name = it->name.c_str();
		// Loop through the directory components of the name, stripping them
		// off and search down the tree hierarchy
		wxString path = _T("");
		wxTreeItemId treeItem = root;
		while (stripDirs.Matches(name))
		{
			wxString dir = stripDirs.GetMatch(name, 1);
			path += dir + _T("/");

			// If we've got 'path' in the tree already, use it
			std::map<std::wstring, wxTreeItemId>::iterator entry = treeEntries.find(path.c_str());
			if (entry != treeEntries.end())
			{
				treeItem = entry->second;
			}
			else
			{
				// Add this new path into the tree
				treeItem = m_TreeCtrl->AppendItem(treeItem, dir);
				treeEntries.insert(std::make_pair(path, treeItem));
			}

			// Remove the leading directory name from the full filename
			stripDirs.Replace(&name, _T(""));
		}
		
		m_TreeCtrl->AppendItem(treeItem, name, -1, -1, new StringTreeItemData(it->name.c_str()));
	}

	m_TreeCtrl->Expand(root);


	wxArrayString animations;

	AtObj animationsList (Datafile::ReadList("animations"));
	for (AtIter it = animationsList["item"]; it.defined(); ++it)
		animations.Add(it);

	m_AnimationBox = new wxComboBox(sidePanel, ID_Animations, _T("Idle"), wxDefaultPosition, wxDefaultSize, animations);

	m_EnvironmentSettings.sunelevation = 45 * M_PI/180;
	m_EnvironmentSettings.sunrotation = 315 * M_PI/180;
	m_EnvironmentSettings.suncolour = Colour(255, 255, 255);
	m_EnvironmentSettings.terraincolour = Colour(164, 164, 164);
	m_EnvironmentSettings.unitcolour = Colour(164, 164, 164);
	LightControl* lightControl = new LightControl(sidePanel, wxSize(100, 100), m_EnvironmentSettings);
	m_Conn = m_EnvironmentSettings.RegisterObserver(0, &SendToGame);
	SendToGame(m_EnvironmentSettings);

	wxSizer* mainSizer = new wxBoxSizer(wxVERTICAL);
	wxSizer* bottomSizer = new wxBoxSizer(wxHORIZONTAL);
	wxSizer* bottomRightSizer = new wxBoxSizer(wxVERTICAL);
	wxSizer* playButtonSizer = new wxBoxSizer(wxHORIZONTAL);

	mainSizer->Add(m_TreeCtrl, wxSizerFlags().Expand().Proportion(1));
	mainSizer->Add(bottomSizer, wxSizerFlags().Expand());

	bottomSizer->Add(lightControl, wxSizerFlags().Border(wxRIGHT, 5));
	bottomSizer->Add(bottomRightSizer, wxSizerFlags().Proportion(1));

	playButtonSizer->Add(new wxButton(sidePanel, ID_Play, _("Play")), wxSizerFlags().Proportion(1));
	playButtonSizer->Add(new wxButton(sidePanel, ID_Pause, _("Pause")), wxSizerFlags().Proportion(1));
	playButtonSizer->Add(new wxButton(sidePanel, ID_Slow, _("Slow")), wxSizerFlags().Proportion(1));

	bottomRightSizer->Add(m_AnimationBox, wxSizerFlags().Expand());
	bottomRightSizer->Add(playButtonSizer, wxSizerFlags().Expand());
	sidePanel->SetSizer(mainSizer);

	//////////////////////////////////////////////////////////////////////////

	// Start by displaying the default non-existent actor
	m_CurrentActor = _T("structures/fndn_1x1.xml");
	SetActorView();

	POST_MESSAGE(RenderEnable, (eRenderView::ACTOR));
}

void ActorViewer::OnClose(wxCloseEvent& WXUNUSED(event))
{
	POST_MESSAGE(Shutdown, ());

	AtlasMessage::qExit().Post();
	// blocks until engine has noticed the message, so we won't be
	// destroying the GLCanvas while it's still rendering

	Destroy();
}

void ActorViewer::SetActorView()
{
	POST_MESSAGE(SetActorViewer, (m_CurrentActor.c_str(), m_AnimationBox->GetValue().c_str(), m_CurrentSpeed));
}

void ActorViewer::OnTreeSelection(wxTreeEvent& event)
{
	wxTreeItemData* data = m_TreeCtrl->GetItemData(event.GetItem());
	if (! data)
		return;

	m_CurrentActor = static_cast<StringTreeItemData*>(data)->m_String;
	SetActorView();
}

void ActorViewer::OnAnimationSelection(wxCommandEvent& WXUNUSED(event))
{
	SetActorView();
}

void ActorViewer::OnSpeedButton(wxCommandEvent& event)
{
	if (event.GetId() == ID_Play)
		m_CurrentSpeed = 1.f;
	else if (event.GetId() == ID_Pause)
		m_CurrentSpeed = 0.f;
	else if (event.GetId() == ID_Slow)
		m_CurrentSpeed = 0.1f;
	else
	{
		wxLogDebug(_T("Invalid OnSpeedButton (%d)"), event.GetId());
		m_CurrentSpeed = 1.f;
	}

	SetActorView();
}