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

	RegisterToolButton(this, toolName);
}

void ToolButton::OnClick(wxCommandEvent& WXUNUSED(evt))
{
	// Toggle on/off
	if (m_Selected)
		SetCurrentTool(_T(""));
	else
		SetCurrentTool(m_Tool);
}

void ToolButton::SetSelectedAppearance(bool selected)
{
	m_Selected = selected;
	if (selected)
		SetBackgroundColour(wxColour(0xee, 0xcc, 0x55));
	else
		SetBackgroundColour(wxSystemSettings::GetColour(wxSYS_COLOUR_BTNFACE));
}

//////////////////////////////////////////////////////////////////////////

BEGIN_EVENT_TABLE(ToolButtonBar, wxToolBar)
	EVT_TOOL(wxID_ANY, ToolButtonBar::OnTool)
END_EVENT_TABLE()

ToolButtonBar::ToolButtonBar(wxWindow* parent, int baseID)
: wxToolBar(parent, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxNO_BORDER|wxTB_FLAT|wxTB_HORIZONTAL), m_Id(baseID)
{
}

void ToolButtonBar::AddToolButton(const wxString& shortLabel, const wxString& longLabel, const wxBitmap& bitmap, const wxString& toolName)
{
	AddCheckTool(m_Id, shortLabel, bitmap, wxNullBitmap, longLabel);
	m_Buttons[m_Id] = toolName;
	
	RegisterToolBarButton(this, m_Id, toolName);

	++m_Id;
}

void ToolButtonBar::OnTool(wxCommandEvent& evt)
{
	std::map<int, wxString>::iterator it = m_Buttons.find(evt.GetId());
	wxCHECK_RET(it != m_Buttons.end(), _T("Invalid toolbar button"));
	SetCurrentTool(it->second);
}
