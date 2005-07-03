#include "stdafx.h"

#include "Sidebar.h"

Sidebar::Sidebar(wxWindow* parent)
	: wxPanel(parent)
{
	m_MainSizer = new wxBoxSizer(wxVERTICAL);
	SetSizer(m_MainSizer);
}
