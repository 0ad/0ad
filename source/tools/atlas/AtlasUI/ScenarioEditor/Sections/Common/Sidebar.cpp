#include "stdafx.h"

#include "Sidebar.h"

IMPLEMENT_DYNAMIC_CLASS(Sidebar, wxPanel)

Sidebar::Sidebar(wxWindow* sidebarContainer, wxWindow* WXUNUSED(bottomBarContainer))
	: wxPanel(sidebarContainer), m_BottomBar(NULL), m_AlreadyDisplayed(false)
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
