/* Copyright (C) 2020 Wildfire Games.
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

#include "PseudoMiniMapPanel.h"

#include "GameInterface/MessagePasser.h"
#include "GameInterface/Messages.h"
#include "ScenarioEditor/Tools/Common/Tools.h"

#include <algorithm>
#include <cmath>
#include <wx/dcbuffer.h>
#include <wx/dcgraph.h>
#include <wx/defs.h>
#include <wx/event.h>

namespace
{
	const int PanelRadius = 64 + 1;
	const wxPoint PanelCenter = wxPoint(PanelRadius + 1, PanelRadius + 1);
	const wxPoint ScreenToneOffset(-2 * PanelRadius, -2 * PanelRadius);
	const wxPen Rim(*wxBLACK, 3);
	const wxPen BackgroundMask(*wxBLACK, 2 * PanelRadius);

	bool Within(const wxPoint& test, const wxPoint& center, int radius)
	{
		int dx = abs(test.x - center.x);
		if (dx > radius)
			return false;
		int dy = abs(test.y - center.y);
		if (dy > radius)
			return false;
		if (dx + dy <= radius)
			return true;
		return dx * dx + dy * dy <= radius * radius;
	}
}

PseudoMiniMapPanel::PseudoMiniMapPanel(wxWindow* parent, int currentSize)
	: wxPanel(parent, wxID_ANY, wxDefaultPosition, wxSize(PanelRadius * 2 + 1, PanelRadius * 2 + 1)),
	m_CurrentSize(currentSize), m_ScreenTones(),
	m_LastMousePos(-1, -1), m_Dragging(false),
	m_SelectionRadius(PanelRadius), m_SelectionCenter(PanelCenter), m_SameOrGrowing(true), m_NewSize(currentSize)
{
	AtlasMessage::qRasterizeMinimap qryBackground;
	qryBackground.Post();
	int dim = qryBackground.dimension;
	std::vector<uint8_t> imageBytes = *qryBackground.imageBytes;

	// Data is destined for a wxImage, which uses free.
	uint8_t* data = new uint8_t[imageBytes.size()];
	std::copy(imageBytes.cbegin(), imageBytes.cend(), data);
	m_Background = wxImage(dim, dim, data);
	m_Background.Rescale(PanelRadius * 2, PanelRadius * 2, wxIMAGE_QUALITY_BOX_AVERAGE);
	m_Backgrounds[PanelRadius] = wxBitmap(m_Background);

	SetBackgroundStyle(wxBG_STYLE_PAINT);
}

wxPoint PseudoMiniMapPanel::GetOffset() const
{
	// Since offset is from center, amplitude is (at most) half the largest size.
	int size = std::max(m_CurrentSize, m_NewSize) / 2;
	// If the map is growing, the display is opposite what the actual offset is.
	float scalar = (m_SameOrGrowing ? 1.0 : -1.0) / PanelRadius * size;
	// Rebase offsets to center.
	int hOffset = m_SelectionCenter.x - PanelCenter.x;
	int vOffset = m_SelectionCenter.y - PanelCenter.y;
	return wxPoint(scalar * hOffset, scalar * vOffset);
}

void PseudoMiniMapPanel::OnNewSize(wxCommandEvent& evt)
{
	if (!evt.IsSelection())
		return;

	evt.Skip();

	m_NewSize = wxAtoi(static_cast<wxStringClientData*>(evt.GetClientObject())->GetData());

	m_SameOrGrowing = m_NewSize >= m_CurrentSize;
	m_SelectionRadius = std::min(m_NewSize, m_CurrentSize) * PanelRadius / std::max(m_NewSize, m_CurrentSize);
	if (!m_SameOrGrowing && m_ScreenTones.find(m_SelectionRadius) == m_ScreenTones.cend())
	{
		wxImage overlay = wxImage(PanelRadius * 4, PanelRadius * 4);
		overlay.InitAlpha();
		wxGraphicsContext* gc = wxGraphicsContext::Create(overlay);
		gc->SetBrush(*wxGREY_BRUSH);
		gc->DrawRectangle(0, 0, PanelRadius * 4, PanelRadius * 4);
		gc->SetBrush(*wxBLACK_BRUSH);
		gc->DrawEllipse(PanelRadius * 2 - m_SelectionRadius, PanelRadius * 2  - m_SelectionRadius, m_SelectionRadius * 2, m_SelectionRadius * 2);
		gc->SetPen(*wxWHITE_PEN);
		gc->DrawEllipse(PanelRadius * 2 - m_SelectionRadius, PanelRadius * 2 - m_SelectionRadius, m_SelectionRadius * 2, m_SelectionRadius * 2);
		delete gc;
		// Black -> Converted to transparent.
		// White -> converted to black.
		overlay.ConvertColourToAlpha(0, 0, 0);

		m_ScreenTones[m_SelectionRadius] = wxBitmap(overlay);
	}
	else if (m_SameOrGrowing && m_Backgrounds.find(m_SelectionRadius) == m_Backgrounds.cend())
	{
		wxImage rescaled = wxImage(m_Background);
		rescaled.Rescale(2 * m_SelectionRadius, 2 * m_SelectionRadius, wxIMAGE_QUALITY_BOX_AVERAGE);
		m_Backgrounds[m_SelectionRadius] = wxBitmap(rescaled);
	}

	Refresh();
}

void PseudoMiniMapPanel::OnMouseDown(wxMouseEvent& evt)
{
	// Capture on button-down, so we can respond even when the mouse
	// moves off the window
	if (!m_Dragging && evt.ButtonDown() &&
		Within(evt.GetPosition(), PanelCenter, PanelRadius) &&
		Within(evt.GetPosition(), m_SelectionCenter, m_SelectionRadius))
	{
		m_LastMousePos = evt.GetPosition();
		m_Dragging = true;
	}
}

void PseudoMiniMapPanel::OnMouseUp(wxMouseEvent& evt)
{
	if (m_Dragging &&
		!(evt.ButtonIsDown(wxMOUSE_BTN_LEFT) || evt.ButtonIsDown(wxMOUSE_BTN_MIDDLE) || evt.ButtonIsDown(wxMOUSE_BTN_RIGHT))
		)
	{
		m_Dragging = false;
	}
}

void PseudoMiniMapPanel::OnMouseMove(wxMouseEvent& evt)
{
	if (m_Dragging && evt.Dragging())
	{
		if (m_LastMousePos == evt.GetPosition())
			return;

		wxPoint delta = evt.GetPosition() - m_LastMousePos;
		wxPoint moved = m_SelectionCenter + delta;

		if (!Within(moved, PanelCenter, PanelRadius))
			return;

		m_SelectionCenter = moved;
		m_LastMousePos = evt.GetPosition();
		Refresh();
	}
}

void PseudoMiniMapPanel::OnMouseLeave(wxMouseEvent& WXUNUSED(evt))
{
	m_Dragging = false;
}

void PseudoMiniMapPanel::PaintEvent(wxPaintEvent& WXUNUSED(evt))
{
	wxAutoBufferedPaintDC dca(this);
	// Background must be grabbed from paint dc, not gc, or color may be transparent.
	wxColor background = dca.GetBackground().GetColour();
	wxGCDC dc(dca);
	if (m_SameOrGrowing)
	{
		dc.DrawBitmap(m_Backgrounds[m_SelectionRadius], m_SelectionCenter - wxSize(m_SelectionRadius, m_SelectionRadius));

		dc.SetBrush(*wxTRANSPARENT_BRUSH);
		dc.SetPen(BackgroundMask);
		dc.DrawCircle(m_SelectionCenter, PanelRadius + m_SelectionRadius);

		const wxPen BorderPen(*wxWHITE, 2);
		dc.SetPen(BorderPen);
		dc.DrawCircle(m_SelectionCenter, m_SelectionRadius);
	}
	else
	{
		dc.DrawBitmap(m_Backgrounds[PanelRadius], 0, 0);
		// "fade out" trimmed areas by drawing a screentone ring ring.
		dc.DrawBitmap(m_ScreenTones[m_SelectionRadius], ScreenToneOffset + m_SelectionCenter);
	}

	// Centering markers.
	dc.SetBrush(*wxBLACK_BRUSH);
	dc.SetPen(*wxBLACK_PEN);
	dc.DrawCircle(m_SelectionCenter, 2);
	dc.SetPen(*wxWHITE_PEN);
	dc.DrawLine(PanelRadius - 10, PanelRadius, PanelRadius + 10, PanelRadius);
	dc.DrawLine(PanelRadius, PanelRadius + 10, PanelRadius, PanelRadius - 10);

	// Round border.
	dc.SetBrush(*wxTRANSPARENT_BRUSH);
	dc.SetPen(Rim);
	dc.DrawCircle(PanelCenter, PanelRadius - 1);
	wxPen mask(background, PanelRadius);
	dc.SetPen(mask);
	dc.DrawCircle(PanelCenter, PanelRadius + PanelRadius / 2 - 1);
}

void PseudoMiniMapPanel::EraseBackground(wxEraseEvent& WXUNUSED(evt))
{
	// Do nothing - don't erase to remove flicker.
}

BEGIN_EVENT_TABLE(PseudoMiniMapPanel, wxPanel)
	EVT_LEAVE_WINDOW(PseudoMiniMapPanel::OnMouseUp)
	EVT_LEFT_DOWN(PseudoMiniMapPanel::OnMouseDown)
	EVT_LEFT_UP(PseudoMiniMapPanel::OnMouseUp)
	EVT_RIGHT_DOWN(PseudoMiniMapPanel::OnMouseDown)
	EVT_RIGHT_UP(PseudoMiniMapPanel::OnMouseUp)
	EVT_MIDDLE_DOWN(PseudoMiniMapPanel::OnMouseDown)
	EVT_MIDDLE_UP(PseudoMiniMapPanel::OnMouseUp)
	EVT_MOTION(PseudoMiniMapPanel::OnMouseMove)
	EVT_LEAVE_WINDOW(PseudoMiniMapPanel::OnMouseLeave)
	EVT_PAINT(PseudoMiniMapPanel::PaintEvent)
END_EVENT_TABLE()
