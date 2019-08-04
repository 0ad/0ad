/* Copyright (C) 2019 Wildfire Games.
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

#include "GUIMatrix.h"

#include "maths/Matrix3D.h"

extern int g_xres, g_yres;
extern float g_GuiScale;

CMatrix3D GetDefaultGuiMatrix()
{
	float xres = g_xres / g_GuiScale;
	float yres = g_yres / g_GuiScale;

	CMatrix3D m;
	m.SetIdentity();
	m.Scale(1.0f, -1.f, 1.0f);
	m.Translate(0.0f, yres, -1000.0f);

	CMatrix3D proj;
	proj.SetOrtho(0.f, xres, 0.f, yres, -1.f, 1000.f);
	m = proj * m;

	return m;
}
