#include "stdafx.h"

#include "Sidebar.h"

IMPLEMENT_DYNAMIC_CLASS(Sidebar, wxPanel)

Sidebar::Sidebar(wxWindow* parent)
	: wxPanel(parent), m_AlreadyDisplayed(false)
{
	m_MainSizer = new wxBoxSizer(wxVERTICAL);
	SetSizer(m_MainSizer);
}

wxWindow* Sidebar::GetBottomBar(wxWindow* WXUNUSED(parent))
{
	return NULL;
}

void Sidebar::OnSwitchTo()
{
	if (m_AlreadyDisplayed)
		return;

	m_AlreadyDisplayed = true;
	OnFirstDisplay();
}
