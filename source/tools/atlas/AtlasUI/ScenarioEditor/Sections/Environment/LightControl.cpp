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

#include <algorithm>

#include "LightControl.h"

using AtlasMessage::Shareable;

class LightSphere : public wxControl
{
public:

	LightSphere(wxWindow* parent, const wxSize& size, LightControl* lightControl)
		: wxControl(parent, wxID_ANY, wxDefaultPosition, size),
		m_LightControl(lightControl)
	{
	}

	void OnPaint(wxPaintEvent& WXUNUSED(event))
	{
		// Draw a lit 3D sphere:

		int w = GetClientSize().GetWidth();
		int h = GetClientSize().GetHeight();

		float lx = sin(-theta)*cos(phi);
		float ly = cos(-theta)*cos(phi);
		float lz = sin(phi);

		wxImage img (w, h);
		unsigned char* imgData = img.GetData();

		for (int y = 0; y < h; ++y)
		{
			for (int x = 0; x < w; ++x)
			{
				// Calculate normal vector of sphere (radius 1)
				float nx = 2*x / (float)(w-1) - 1; // [-1, 1]
				float ny = 2*y / (float)(h-1) - 1;
				float nz2 = 1 - nx*nx - ny*ny;
				if (nz2 >= 0.f)
				{
					float nz = sqrt(nz2);

					// Get reflection vector (r) of camera vector (c) in n
					float cx = 0;
					float cy = 0;
					float cz = 1; // (camera is infinitely far upwards)
					float ndotc = nz;
					float rx = -cx + nx*(2*ndotc);
					float ry = -cy + ny*(2*ndotc);
					float rz = -cz + nz*(2*ndotc);

					float ndotl = nx*lx + ny*ly + nz*lz;
					float rdotl = rx*lx + ry*ly + rz*lz;

					int diffuse = (int)std::max(0.f, ndotl*128.f);
					int specular = (int)std::min(255.f, 64.f*powf(std::max(0.f, rdotl), 16.f));

					imgData[0] = std::min(64+diffuse+specular, 255);
					imgData[1] = std::min(48+diffuse+specular, 255);
					imgData[2] = std::min(48+diffuse+specular, 255);
				}
				else
				{
					imgData[0] = 0;
					imgData[1] = 0;
					imgData[2] = 0;
				}
				imgData += 3;
			}
		}

		wxPaintDC dc(this);
		#ifdef __WXMSW__
		dc.DrawBitmap(wxBitmap(img, dc), 0, 0); // TODO: is this any better than the version below?
		#else
		dc.DrawBitmap(wxBitmap(img), 0, 0);
		#endif
	}

	void OnMouse(wxMouseEvent& event)
	{
		if (event.LeftIsDown())
		{
			int x = event.GetX();
			int y = event.GetY();
			int w = GetClientSize().GetWidth();
			int h = GetClientSize().GetHeight();

			float mx = 2*x / (float)(w-1) - 1; // [-1, 1]
			float my = 2*y / (float)(h-1) - 1;
			float mz2 = 1 - mx*mx - my*my;
			if (mz2 >= 0.f)
			{
 				//float mz = sqrt(mz2);
 				//phi = asin(mz);

				// ^^ That's giving the height of the sphere at that point, so it's
				// matching the point the user clicked on - but that's quite inconvenient
				// when you want to move the sun near the horizon. So just make up
				// some formula that gives a slightly nicer-feeling result:
				phi = asin(mz2*mz2);

				theta = -atan2(mx, my);
			}
			else
			{
				theta = -atan2(mx, my);
				phi = 0;
			}

			Refresh(false);
			m_LightControl->NotifyOtherObservers();
		}
	}

	float theta, phi;
	LightControl* m_LightControl;

	DECLARE_EVENT_TABLE();
};

BEGIN_EVENT_TABLE(LightSphere, wxControl)
	EVT_PAINT(LightSphere::OnPaint)
	EVT_MOTION(LightSphere::OnMouse)
	EVT_LEFT_DOWN(LightSphere::OnMouse)
END_EVENT_TABLE()

LightControl::LightControl(wxWindow* parent, const wxSize& size, Observable<AtlasMessage::sEnvironmentSettings>& environment)
: wxPanel(parent), m_Environment(environment)
{
	wxSizer* sizer = new wxBoxSizer(wxHORIZONTAL);
	sizer->SetMinSize(size);
	m_Sphere = new LightSphere(this, size, this);
	m_Sphere->theta = environment.sunrotation;
	m_Sphere->phi = environment.sunelevation;
	sizer->Add(m_Sphere, wxSizerFlags().Expand().Proportion(1));
	SetSizer(sizer);

	m_Conn = environment.RegisterObserver(0, &LightControl::OnSettingsChange, this);
}

void LightControl::OnSettingsChange(const AtlasMessage::sEnvironmentSettings& settings)
{
	m_Sphere->theta = settings.sunrotation;
	m_Sphere->phi = settings.sunelevation;
	m_Sphere->Refresh(false);
}

void LightControl::NotifyOtherObservers()
{
	m_Environment.sunrotation = m_Sphere->theta;
	m_Environment.sunelevation = m_Sphere->phi;
	m_Environment.NotifyObserversExcept(m_Conn);
}
