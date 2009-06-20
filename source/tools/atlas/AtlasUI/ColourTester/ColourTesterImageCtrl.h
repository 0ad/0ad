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

#include "wx/image.h"
#include "wx/filename.h"

class ColourTesterImageCtrl : public wxWindow
{
public:
	ColourTesterImageCtrl(wxWindow* parent);

	void SetImageFile(const wxFileName& fn);
	void SetColour(const wxColour& colour);
	void SetZoom(int amount);
	wxString GetImageFiletype();

	void OnPaint(wxPaintEvent& event);

private:
	void CalculateImage();

	bool m_Valid; // stores whether a valid image is loaded and displayable
	unsigned int m_OriginalImage; // DevIL image id
	unsigned int m_DxtcFormat;
	wxImage m_FinalImage;
	wxBitmap m_FinalBitmap;

	unsigned char m_Colour[3]; // RGB bytes

	int m_ZoomAmount;

	DECLARE_EVENT_TABLE();
};
