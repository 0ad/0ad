#include "precompiled.h"

#include "Sidebar.h"

Sidebar::Sidebar(ScenarioEditor& scenarioEditor, wxWindow* sidebarContainer, wxWindow* WXUNUSED(bottomBarContainer))
	: wxPanel(sidebarContainer), m_ScenarioEditor(scenarioEditor), m_BottomBar(NULL), m_AlreadyDisplayed(false)
{
	m_MainSizer = new wxBoxSizer(wxVERTICAL);
	SetSizer(m_MainSizer);
}

void Sidebar::OnSwitchAway()
{
	if (m_BottomBar)
		m_BottomBar->Show(false);
}

void Sidebar::OnSwitchTo()
{
	if (! m_AlreadyDisplayed)
	{
		m_AlreadyDisplayed = true;
		OnFirstDisplay();
	}

	if (m_BottomBar)
		m_BottomBar->Show(true);
}
