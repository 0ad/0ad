#include "stdafx.h"

#include "ToolButton.h"
#include "ScenarioEditor/Tools/Common/Tools.h"

BEGIN_EVENT_TABLE(ToolButton, wxButton)
	EVT_BUTTON(wxID_ANY, ToolButton::OnClick)
END_EVENT_TABLE()

void ToolButton::OnClick(wxCommandEvent& evt)
{
	if (g_Current)
		g_Current->SetSelectedAppearance(false);
	
	// Toggle on/off
	if (g_Current == this)
	{
		g_Current = NULL;
		SetSelectedAppearance(false);
		SetCurrentTool(_T(""));
	}
	else
	{
		g_Current = this;
		SetSelectedAppearance(true);
		SetCurrentTool(m_Tool);
	}
}

void ToolButton::SetSelectedAppearance(bool selected)
{
	if (selected)
		SetBackgroundColour(wxColour(0xee, 0xcc, 0x55));
	else
		SetBackgroundColour(wxSystemSettings::GetColour(wxSYS_COLOUR_BTNFACE));
}

ToolButton* ToolButton::g_Current = NULL;
