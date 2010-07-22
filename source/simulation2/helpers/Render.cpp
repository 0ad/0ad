/* Copyright (C) 2010 Wildfire Games.
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

#include "Render.h"

#include "simulation2/Simulation2.h"
#include "simulation2/components/ICmpTerrain.h"
#include "simulation2/components/ICmpWaterManager.h"
#include "graphics/Overlay.h"
#include "graphics/Terrain.h"
#include "maths/MathUtil.h"

static const size_t RENDER_CIRCLE_POINTS = 16;
static const float RENDER_HEIGHT_DELTA = 0.25f; // distance above terrain

void SimRender::ConstructLineOnGround(const CSimContext& context, std::vector<float> xz,
		SOverlayLine& overlay, bool floating)
{
	overlay.m_Coords.clear();

	CmpPtr<ICmpTerrain> cmpTerrain(context, SYSTEM_ENTITY);
	if (cmpTerrain.null())
		return;

	if (xz.size() < 2)
		return;

	float water = 0.f;
	if (floating)
	{
		CmpPtr<ICmpWaterManager> cmpWaterMan(context, SYSTEM_ENTITY);
		if (!cmpWaterMan.null())
			water = cmpWaterMan->GetExactWaterLevel(xz[0], xz[1]);
	}

	overlay.m_Coords.reserve(xz.size()/2 * 3);

	for (size_t i = 0; i < xz.size(); i += 2)
	{
		float px = xz[i];
		float pz = xz[i+1];
		float py = std::max(water, cmpTerrain->GetExactGroundLevel(px, pz)) + RENDER_HEIGHT_DELTA;
		overlay.m_Coords.push_back(px);
		overlay.m_Coords.push_back(py);
		overlay.m_Coords.push_back(pz);
	}
}

void SimRender::ConstructCircleOnGround(const CSimContext& context, float x, float z, float radius,
		SOverlayLine& overlay, bool floating)
{
	overlay.m_Coords.clear();

	CmpPtr<ICmpTerrain> cmpTerrain(context, SYSTEM_ENTITY);
	if (cmpTerrain.null())
		return;

	float water = 0.f;
	if (floating)
	{
		CmpPtr<ICmpWaterManager> cmpWaterMan(context, SYSTEM_ENTITY);
		if (!cmpWaterMan.null())
			water = cmpWaterMan->GetExactWaterLevel(x, z);
	}

	overlay.m_Coords.reserve((RENDER_CIRCLE_POINTS + 1) * 3);

	for (size_t i = 0; i <= RENDER_CIRCLE_POINTS; ++i) // use '<=' so it's a closed loop
	{
		float a = i * 2 * (float)M_PI / RENDER_CIRCLE_POINTS;
		float px = x + radius * sin(a);
		float pz = z + radius * cos(a);
		float py = std::max(water, cmpTerrain->GetExactGroundLevel(px, pz)) + RENDER_HEIGHT_DELTA;
		overlay.m_Coords.push_back(px);
		overlay.m_Coords.push_back(py);
		overlay.m_Coords.push_back(pz);
	}
}

// This method splits up a straight line into a number of line segments each having a length ~= CELL_SIZE
static void SplitLine(std::vector<std::pair<float, float> >& coords, float x1, float y1, float x2, float y2)
{
	float length = sqrt(SQR(x1 - x2) + SQR(y1 - y2));
	size_t pieces = ((int)length) / CELL_SIZE;
	if (pieces > 0)
	{
		float xPieceLength = (x1 - x2) / pieces;
		float yPieceLength = (y1 - y2) / pieces;
		for (size_t i = 1; i <= (pieces - 1); ++i)
		{
			coords.push_back(std::make_pair(x1 - (xPieceLength * i), y1 - (yPieceLength * i)));
		}
	}
	coords.push_back(std::make_pair(x2, y2));
}

void SimRender::ConstructSquareOnGround(const CSimContext& context, float x, float z, float w, float h, float a,
		SOverlayLine& overlay, bool floating)
{
	overlay.m_Coords.clear();

	CmpPtr<ICmpTerrain> cmpTerrain(context, SYSTEM_ENTITY);
	if (cmpTerrain.null())
		return;

	float water = 0.f;
	if (floating)
	{
		CmpPtr<ICmpWaterManager> cmpWaterMan(context, SYSTEM_ENTITY);
		if (!cmpWaterMan.null())
			water = cmpWaterMan->GetExactWaterLevel(x, z);
	}

	float c = cos(a);
	float s = sin(a);

	std::vector<std::pair<float, float> > coords;

	// Add the first vertex, since SplitLine will be adding only the second end-point of the each line to
	// the coordinates list. We don't have to worry about the other lines, since the end-point of one line
	// will be the starting point of the next
	coords.push_back(std::make_pair(x - w/2*c + h/2*s, z + w/2*s + h/2*c));

	SplitLine(coords, x - w/2*c + h/2*s, z + w/2*s + h/2*c, x - w/2*c - h/2*s, z + w/2*s - h/2*c);
	SplitLine(coords, x - w/2*c - h/2*s, z + w/2*s - h/2*c, x + w/2*c - h/2*s, z - w/2*s - h/2*c);
	SplitLine(coords, x + w/2*c - h/2*s, z - w/2*s - h/2*c, x + w/2*c + h/2*s, z - w/2*s + h/2*c);
	SplitLine(coords, x + w/2*c + h/2*s, z - w/2*s + h/2*c, x - w/2*c + h/2*s, z + w/2*s + h/2*c);

	overlay.m_Coords.reserve(coords.size() * 3);

	for (size_t i = 0; i < coords.size(); ++i)
	{
		float px = coords[i].first;
		float pz = coords[i].second;
		float py = std::max(water, cmpTerrain->GetExactGroundLevel(px, pz)) + RENDER_HEIGHT_DELTA;
		overlay.m_Coords.push_back(px);
		overlay.m_Coords.push_back(py);
		overlay.m_Coords.push_back(pz);
	}
}
