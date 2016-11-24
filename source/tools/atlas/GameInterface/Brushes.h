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

#ifndef INCLUDED_BRUSHES
#define INCLUDED_BRUSHES

#include "maths/Vector3D.h"

class TerrainOverlay;

namespace AtlasMessage {

struct Brush
{
	Brush();
	~Brush();

	void SetData(ssize_t w, ssize_t h, const std::vector<float>& data);

	void SetRenderEnabled(bool enabled); // initial state is disabled

	void GetCentre(ssize_t& x, ssize_t& y) const;
	void GetBottomLeft(ssize_t& x, ssize_t& y) const;
	void GetTopRight(ssize_t& x, ssize_t& y) const;

	float Get(ssize_t x, ssize_t y) const
	{
		if (x >= 0 && x < m_W && y >= 0 && y < m_H)
			return m_Data[x + y*m_W];
		else
			return 0.f;
	}

	ssize_t m_W, m_H;
	CVector3D m_Centre;
private:
	TerrainOverlay* m_TerrainOverlay; // NULL if rendering is not enabled
	std::vector<float> m_Data;
};

extern Brush g_CurrentBrush;

}

#endif // INCLUDED_BRUSHES
