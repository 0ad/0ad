#include "stdafx.h"

#include "ColourTesterColourCtrl.h"

#include "ColourTesterImageCtrl.h"

//////////////////////////////////////////////////////////////////////////
// A couple of buttons:

class ColouredButton : public wxButton
{
public:
	ColouredButton(wxWindow* parent, int colour[], const wxString& caption, ColourTesterImageCtrl* imgctrl)
		: wxButton(parent, wxID_ANY, caption),
		m_ImageCtrl(imgctrl), m_Colour(colour[0], colour[1], colour[2])
	{
		SetBackgroundColour(m_Colour);
		// TODO: More readable text colours
	}
	void OnButton(wxCommandEvent& WXUNUSED(event))
	{
		m_ImageCtrl->SetColour(m_Colour);
	}
private:
	wxColour m_Colour;
	ColourTesterImageCtrl* m_ImageCtrl;
	DECLARE_EVENT_TABLE();
};
BEGIN_EVENT_TABLE(ColouredButton, wxButton)
	EVT_BUTTON(wxID_ANY, ColouredButton::OnButton)
END_EVENT_TABLE()


class CustomColourButton : public wxButton
{
public:
	CustomColourButton(wxWindow* parent, const wxString& caption, ColourTesterImageCtrl* imgctrl)
		: wxButton(parent, wxID_ANY, caption),
		m_ImageCtrl(imgctrl), m_Colour(127,127,127)
	{
		SetBackgroundColour(m_Colour);
	}
	void OnButton(wxCommandEvent& WXUNUSED(event))
	{
		wxColour c = wxGetColourFromUser(this, m_Colour);
		if (c.Ok())
		{
			m_Colour = c;
			m_ImageCtrl->SetColour(m_Colour);
			SetBackgroundColour(m_Colour);
		}
	}
private:
	wxColour m_Colour;
	ColourTesterImageCtrl* m_ImageCtrl;
	DECLARE_EVENT_TABLE();
};
BEGIN_EVENT_TABLE(CustomColourButton, wxButton)
	EVT_BUTTON(wxID_ANY, CustomColourButton::OnButton)
END_EVENT_TABLE()

//////////////////////////////////////////////////////////////////////////

ColourTesterColourCtrl::ColourTesterColourCtrl(wxWindow* parent, ColourTesterImageCtrl* imgctrl)
	: wxPanel(parent, wxID_ANY)
{
	wxBoxSizer* mainSizer = new wxBoxSizer(wxHORIZONTAL);

	wxGridSizer* presetColourSizer = new wxGridSizer(2);
	mainSizer->Add(presetColourSizer);

	// TODO: make configurable
	int presetColours[][3] = {
		{255,255,255}, // Gaia
		{255,0,0}, // Player 1
		{0,255,0}, // etc
		{0,0,255},
		{255,255,0},
		{255,0,255},
		{0,255,255},
		{255,127,255},
		{255,204,127}
	};

	for (int i = 0; i < sizeof(presetColours)/sizeof(presetColours[0]); ++i)
	{
		wxButton* button = new ColouredButton(this, presetColours[i],
			i ? wxString::Format(_("Player %d"), i) : _("Gaia"), imgctrl);
		presetColourSizer->Add(button);
	}
	presetColourSizer->Add(new CustomColourButton(this, _("Custom"), imgctrl));

	SetSizer(mainSizer);
	mainSizer->SetSizeHints(this);
}
