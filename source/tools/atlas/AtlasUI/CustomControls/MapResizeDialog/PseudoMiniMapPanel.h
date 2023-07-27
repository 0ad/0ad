/* Copyright (C) 2020 Wildfire Games.
 * This file is part of 0 A.D.
 *
 * 0 A.D. is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * 0 A.D. is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with 0 A.D.  If not, see <http://www.gnu.org/licenses/>.
 */


#ifndef INCLUDED_PSEUDOMINIMAPPANEL
#define INCLUDED_PSEUDOMINIMAPPANEL

#include <map>
#include <wx/panel.h>

class PseudoMiniMapPanel : public wxPanel
{
public:
	PseudoMiniMapPanel(wxWindow* parent, int currentSize);

	void PaintEvent(wxPaintEvent& evt);
	void EraseBackground(wxEraseEvent& evt);

	void OnNewSize(wxCommandEvent& evt);

	wxPoint GetOffset() const;
private:
	void OnMouseDown(wxMouseEvent& evt);
	void OnMouseUp(wxMouseEvent& evt);
	void OnMouseMove(wxMouseEvent& evt);
	void OnMouseLeave(wxMouseEvent& evt);

	const ssize_t m_CurrentSize;
	wxImage m_Background;
	std::map<int, wxBitmap> m_ScreenTones;
	std::map<int, wxBitmap> m_Backgrounds;

	wxPoint m_LastMousePos;
	bool m_Dragging;
	wxPoint m_SelectionCenter;
	int m_SelectionRadius;
	bool m_SameOrGrowing;
	ssize_t m_NewSize;

	DECLARE_EVENT_TABLE();
};

#endif // INCLUDED_PSEUDOMINIMAPPANEL
