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

#include "ColourTesterColourCtrl.h"

#include "ColourTesterImageCtrl.h"
#include "General/Datafile.h"

#include "wx/colordlg.h"

#include <sstream>

//////////////////////////////////////////////////////////////////////////
// A couple of buttons:

class ColouredButton : public wxButton
{
public:
	ColouredButton(wxWindow* parent, const wxColour& colour, const wxColour& textColor,
					const wxString& caption, ColourTesterImageCtrl* imgctrl)
		: wxButton(parent, wxID_ANY, caption),
		m_ImageCtrl(imgctrl), m_Colour(colour)
	{
		SetBackgroundColour(m_Colour);
		SetForegroundColour(textColor);
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
		if (c.IsOk())
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

	AtObj colours (Datafile::ReadList("playercolours"));
	for (AtIter c = colours["colour"]; c.defined(); ++c)
	{
		wxColour back;
		{
			int r,g,b;
			std::wstringstream s;
			s << (std::wstring)c["@rgb"];
			s >> r >> g >> b;
			back = wxColour(r,g,b);
		}

		wxColour text (0, 0, 0);
		if (c["@buttontext"].defined())
		{
			int r,g,b;
			std::wstringstream s;
			s << (std::wstring)c["@buttontext"];
			s >> r >> g >> b;
			text = wxColour(r,g,b);
		}
		presetColourSizer->Add(new ColouredButton(this, back, text, (wxString)c["@name"], imgctrl));
	}

	presetColourSizer->Add(new CustomColourButton(this, _("Custom"), imgctrl));

	SetSizer(mainSizer);
	mainSizer->SetSizeHints(this);
}
