#include "stdafx.h"

#include "ToolButton.h"
#include "ScenarioEditor/Tools/Common/Tools.h"

BEGIN_EVENT_TABLE(ToolButton, wxButton)
	EVT_BUTTON(wxID_ANY, ToolButton::OnClick)
END_EVENT_TABLE()

ToolButton::ToolButton
	(wxWindow *parent,
	 const wxString& label,
	 const wxString& toolName,
	 const wxSize& size,
	 long style)
	: wxButton(parent, wxID_ANY, label, wxDefaultPosition, size, style),
	m_Tool(toolName)
{
	// Explicitly set appearance, so that the button is always owner-drawn
	// (by the wxButton code), rather than initially using the native
	// (fixed colour) button appearance.
	SetSelectedAppearance(false);
}

void ToolButton::OnClick(wxCommandEvent& WXUNUSED(evt))
{
	if (g_Current)
		g_Current->SetSelectedAppearance(false);
	// TODO: set disabled when tool is changed via other (non-button) methods
	
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
