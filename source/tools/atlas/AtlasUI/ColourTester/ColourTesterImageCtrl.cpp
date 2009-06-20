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

#include "ColourTesterImageCtrl.h"

#include "ColourTester.h"

#ifdef _MSC_VER
# ifndef NDEBUG
#  pragma comment(lib, "DevIL_DBG.lib")
#  pragma comment(lib, "DevILU_DBG.lib")
# else
#  pragma comment(lib, "DevIL.lib")
#  pragma comment(lib, "DevILU.lib")
# endif
#endif

#include "IL/il.h"
#include "IL/ilu.h"

BEGIN_EVENT_TABLE(ColourTesterImageCtrl, wxWindow)
	EVT_PAINT(ColourTesterImageCtrl::OnPaint)
END_EVENT_TABLE()

ColourTesterImageCtrl::ColourTesterImageCtrl(wxWindow* parent)
	: wxWindow(parent, wxID_ANY),
	m_Valid(false), m_ZoomAmount(1)
{
	m_Colour[0] = 255;
	m_Colour[1] = 0;
	m_Colour[2] = 0;
	ilInit();
	ilGenImages(1, (ILuint*)&m_OriginalImage);
	ilSetInteger(IL_KEEP_DXTC_DATA, IL_TRUE);
}

void ColourTesterImageCtrl::SetImageFile(const wxFileName& fn)
{
	ilBindImage(m_OriginalImage);

	ilOriginFunc(IL_ORIGIN_UPPER_LEFT);
	ilEnable(IL_ORIGIN_SET);
	if (! ilLoadImage((wchar_t*)fn.GetFullPath().c_str()))
	{
		m_Valid = false;
		Refresh();
		return;
	}
	m_Valid = true;

	m_DxtcFormat = ilGetInteger(IL_DXTC_DATA_FORMAT);

	ilConvertImage(IL_RGBA, IL_UNSIGNED_BYTE);

	// TODO: Check for IL errors

	{
		// I'm too lazy to add zoom buttons, so just use 2x zoom for <=256x256 images
		ILinfo info;
		iluGetImageInfo(&info);
		if (info.Width <= 256 && info.Height <= 256)
			m_ZoomAmount = 2;
		else
			m_ZoomAmount = 1;
	}

	CalculateImage();

	// Send an event to indicate that the image has changed, so that
	// the status-bar text can be updated
	wxCommandEvent evt(wxEVT_MY_IMAGE_CHANGED);
	evt.SetEventObject(this);
	evt.SetString(fn.GetFullPath());
	GetEventHandler()->ProcessEvent(evt);
}

wxString ColourTesterImageCtrl::GetImageFiletype()
{
	wxString fmt = _("Not DXTC");
	switch (ilGetInteger(IL_DXTC_DATA_FORMAT)) {
		case IL_DXT1: fmt = _T("DXT1"); break;
		case IL_DXT2: fmt = _T("DXT2"); break;
		case IL_DXT3: fmt = _T("DXT3"); break;
		case IL_DXT4: fmt = _T("DXT4"); break;
		case IL_DXT5: fmt = _T("DXT5"); break;
	}
	return wxString::Format(_T("%s - %dx%d"), fmt.c_str(), ilGetInteger(IL_IMAGE_WIDTH), ilGetInteger(IL_IMAGE_HEIGHT));
}

void ColourTesterImageCtrl::SetColour(const wxColour& colour)
{
	m_Colour[0] = colour.Red();
	m_Colour[1] = colour.Green();
	m_Colour[2] = colour.Blue();

	CalculateImage();
}

void ColourTesterImageCtrl::SetZoom(int amount)
{
	m_ZoomAmount = amount;
	CalculateImage();
}

void ColourTesterImageCtrl::CalculateImage()
{
	if (! m_Valid)
		return;

	ilBindImage(m_OriginalImage);

	ILubyte* data = ilGetData();
	ILinfo info;
	iluGetImageInfo(&info);

	wxASSERT(info.Bpp == 4);

	unsigned char* newData = (unsigned char*)malloc(info.Width*info.Height*3);
		// wxImage desires malloc, not new[]

	for (ILubyte *p0 = data, *p1 = newData;
		 p0 < data+info.Width*info.Height*4;
		 p0 += 4, p1 += 3)
	{
		// Interpolate between texture and texture*colour, so
		// new = old*alpha + old*colour*(1-alpha)
		float alpha = p0[3]/255.f;
		p1[0] = p0[0]*alpha + p0[0]*m_Colour[0]*(1.f-alpha)/255.f;
		p1[1] = p0[1]*alpha + p0[1]*m_Colour[1]*(1.f-alpha)/255.f;
		p1[2] = p0[2]*alpha + p0[2]*m_Colour[2]*(1.f-alpha)/255.f;
	}

	m_FinalImage.SetData(newData, info.Width, info.Height);

	if (m_ZoomAmount != 1)
		m_FinalImage = m_FinalImage.Scale(info.Width*m_ZoomAmount, info.Height*m_ZoomAmount);

	m_FinalBitmap = wxBitmap(m_FinalImage);

	Refresh();
}

void ColourTesterImageCtrl::OnPaint(wxPaintEvent& WXUNUSED(event))
{
	wxPaintDC dc(this);

	dc.Clear();

	if (m_Valid && m_FinalBitmap.Ok())
		dc.DrawBitmap(m_FinalBitmap, 0, 0);
}
