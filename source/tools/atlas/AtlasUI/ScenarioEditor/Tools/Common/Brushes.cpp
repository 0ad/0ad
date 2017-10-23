/* Copyright (C) 2015 Wildfire Games.
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

#include "Brushes.h"

#include "GameInterface/Messages.h"

#include "wx/spinctrl.h"

Brush g_Brush_Elevation; // shared between several elevation-related tools; other tools have their own brushes

static Brush* g_Brush_CurrentlyActive = NULL; // only one brush can be active at once

const float Brush::STRENGTH_MULTIPLIER = 1024.f;

Brush::Brush()
: m_Shape(CIRCLE), m_Size(4), m_Strength(1.f), m_IsActive(false)
{
}

Brush::~Brush()
{
	// Avoid dangling pointers
	if (g_Brush_CurrentlyActive == this)
		g_Brush_CurrentlyActive = NULL;
}

void Brush::MakeActive()
{
	if (g_Brush_CurrentlyActive)
		g_Brush_CurrentlyActive->m_IsActive = false;

	g_Brush_CurrentlyActive = this;
	m_IsActive = true;

	Send();
}

void Brush::Send()
{
	if (m_IsActive)
		POST_MESSAGE(Brush, (GetWidth(), GetHeight(), GetData()));
}

int Brush::GetWidth() const
{
	switch (m_Shape)
	{
	case CIRCLE:
		return m_Size;
	case SQUARE:
		return m_Size;
	default:
		wxFAIL;
		return -1;
	}
}

int Brush::GetHeight() const
{
/*
	switch (m_Shape)
	{
	case RECTANGLE or something:
	default:
		return GetWidth();
	}
*/
	return GetWidth();
}

std::vector<float> Brush::GetData() const
{
	int width = GetWidth();
	int height = GetHeight();

	std::vector<float> data (width*height);

	switch (m_Shape)
	{
	case CIRCLE:
		{
			int i = 0;
			// All calculations are done in units of half-tiles, since that
			// is the required precision
			int mid_x = m_Size-1;
			int mid_y = m_Size-1;
			for (int y = 0; y < m_Size; ++y)
			{
				for (int x = 0; x < m_Size; ++x)
				{
					float dist_sq = // scaled to 0 in centre, 1 on edge
						((2*x - mid_x)*(2*x - mid_x) +
						 (2*y - mid_y)*(2*y - mid_y)) / (float)(m_Size*m_Size);
					if (dist_sq <= 1.f)
						data[i++] = (sqrtf(2.f - dist_sq) - 1.f) / (sqrt(2.f) - 1.f);
					else
						data[i++] = 0.f;
				}
			}
			break;
		}

	case SQUARE:
		{
			int i = 0;
			for (int y = 0; y < height; ++y)
				for (int x = 0; x < width; ++x)
					data[i++] = 1.f;
			break;
		}
	}

	return data;
}

float Brush::GetStrength() const
{
	return m_Strength;
}

void Brush::SetStrength(float strength)
{
	m_Strength = strength;
}

void Brush::SetCircle(int size)
{
	m_Shape = CIRCLE;
	m_Size = size;
}

void Brush::SetSquare(int size)
{
	m_Shape = SQUARE;
	m_Size = size;
}

//////////////////////////////////////////////////////////////////////////

class BrushShapeCtrl : public wxRadioBox
{
public:
	BrushShapeCtrl(wxWindow* parent, wxArrayString& shapes, Brush& brush)
		: wxRadioBox(parent, wxID_ANY, _("Shape"), wxDefaultPosition, wxDefaultSize, shapes, 0, wxRA_SPECIFY_ROWS),
		m_Brush(brush)
	{
		SetSelection(m_Brush.m_Shape);
	}

private:
	Brush& m_Brush;

	void OnChange(wxCommandEvent& WXUNUSED(evt))
	{
		m_Brush.m_Shape = (Brush::BrushShape)GetSelection();
		m_Brush.Send();
	}

	DECLARE_EVENT_TABLE();
};
BEGIN_EVENT_TABLE(BrushShapeCtrl, wxRadioBox)
	EVT_RADIOBOX(wxID_ANY, BrushShapeCtrl::OnChange)
END_EVENT_TABLE()


class BrushSizeCtrl: public wxSpinCtrl
{
public:
	BrushSizeCtrl(wxWindow* parent, Brush& brush)
		: wxSpinCtrl(parent, wxID_ANY, wxString::Format(_T("%d"), brush.m_Size), wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, 0, 100, brush.m_Size),
		m_Brush(brush)
	{
	}

private:
	Brush& m_Brush;

	void OnChange(wxSpinEvent& WXUNUSED(evt))
	{
		m_Brush.m_Size = GetValue();
		m_Brush.Send();
	}

	DECLARE_EVENT_TABLE();
};
BEGIN_EVENT_TABLE(BrushSizeCtrl, wxSpinCtrl)
	EVT_SPINCTRL(wxID_ANY, BrushSizeCtrl::OnChange)
END_EVENT_TABLE()


class BrushStrengthCtrl : public wxSpinCtrl
{
public:
	BrushStrengthCtrl(wxWindow* parent, Brush& brush)
		: wxSpinCtrl(parent, wxID_ANY, wxString::Format(_T("%d"), (int)(10.f*brush.m_Strength)), wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, 0, 100, (int)(10.f*brush.m_Strength)),
		m_Brush(brush)
	{
	}

private:
	Brush& m_Brush;

	void OnChange(wxSpinEvent& WXUNUSED(evt))
	{
		m_Brush.m_Strength = GetValue()/10.f;
		m_Brush.Send();
	}

	DECLARE_EVENT_TABLE();
};
BEGIN_EVENT_TABLE(BrushStrengthCtrl, wxSpinCtrl)
	EVT_SPINCTRL(wxID_ANY, BrushStrengthCtrl::OnChange)
END_EVENT_TABLE()



void Brush::CreateUI(wxWindow* parent, wxSizer* sizer)
{
	wxArrayString shapes; // Must match order of BrushShape enum
	shapes.Add(_("Circle"));
	shapes.Add(_("Square"));
	// TODO (maybe): get rid of the extra static box, by not using wxRadioBox
	sizer->Add(new BrushShapeCtrl(parent, shapes, *this), wxSizerFlags().Expand());

	sizer->AddSpacer(5);

	// TODO: These are yucky
	wxFlexGridSizer* spinnerSizer = new wxFlexGridSizer(2, 5, 5);
	spinnerSizer->AddGrowableCol(1);
	spinnerSizer->Add(new wxStaticText(parent, wxID_ANY, _("Size")), wxSizerFlags().Align(wxALIGN_CENTER_VERTICAL|wxALIGN_RIGHT));
	spinnerSizer->Add(new BrushSizeCtrl(parent, *this), wxSizerFlags().Expand());
	spinnerSizer->Add(new wxStaticText(parent, wxID_ANY, _("Strength")), wxSizerFlags().Align(wxALIGN_CENTER_VERTICAL|wxALIGN_RIGHT));
	spinnerSizer->Add(new BrushStrengthCtrl(parent, *this), wxSizerFlags().Expand());
	sizer->Add(spinnerSizer, wxSizerFlags().Expand());
}
