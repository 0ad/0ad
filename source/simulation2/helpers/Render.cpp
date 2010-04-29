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
#include "graphics/Overlay.h"

static const size_t RENDER_CIRCLE_POINTS = 16;
static const float RENDER_HEIGHT_DELTA = 0.25f; // distance above terrain

void SimRender::ConstructLineOnGround(const CSimContext& context, std::vector<float> xz, SOverlayLine& overlay)
{
	overlay.m_Coords.clear();

	CmpPtr<ICmpTerrain> cmpTerrain(context, SYSTEM_ENTITY);
	if (cmpTerrain.null())
		return;

	overlay.m_Coords.reserve(xz.size()/2 * 3);

	for (size_t i = 0; i < xz.size(); i += 2)
	{
		float px = xz[i];
		float pz = xz[i+1];
		float py = cmpTerrain->GetGroundLevel(px, pz) + RENDER_HEIGHT_DELTA;
		overlay.m_Coords.push_back(px);
		overlay.m_Coords.push_back(py);
		overlay.m_Coords.push_back(pz);
	}
}

void SimRender::ConstructCircleOnGround(const CSimContext& context, float x, float z, float radius, SOverlayLine& overlay)
{
	overlay.m_Coords.clear();

	CmpPtr<ICmpTerrain> cmpTerrain(context, SYSTEM_ENTITY);
	if (cmpTerrain.null())
		return;

	overlay.m_Coords.reserve((RENDER_CIRCLE_POINTS + 1) * 3);

	for (size_t i = 0; i <= RENDER_CIRCLE_POINTS; ++i) // use '<=' so it's a closed loop
	{
		float a = i * 2 * (float)M_PI / RENDER_CIRCLE_POINTS;
		float px = x + radius * sin(a);
		float pz = z + radius * cos(a);
		float py = cmpTerrain->GetGroundLevel(px, pz) + RENDER_HEIGHT_DELTA;
		overlay.m_Coords.push_back(px);
		overlay.m_Coords.push_back(py);
		overlay.m_Coords.push_back(pz);
	}
}

void SimRender::ConstructSquareOnGround(const CSimContext& context, float x, float z, float w, float h, float a, SOverlayLine& overlay)
{
	overlay.m_Coords.clear();

	CmpPtr<ICmpTerrain> cmpTerrain(context, SYSTEM_ENTITY);
	if (cmpTerrain.null())
		return;

	// TODO: might be nicer to split this into little pieces so it copes better with uneven terrain

	overlay.m_Coords.reserve(5 * 3);

	float c = cos(a);
	float s = sin(a);

	std::vector<std::pair<float, float> > coords;
	coords.push_back(std::make_pair(x - w/2*c + h/2*s, z + w/2*s + h/2*c));
	coords.push_back(std::make_pair(x - w/2*c - h/2*s, z + w/2*s - h/2*c));
	coords.push_back(std::make_pair(x + w/2*c - h/2*s, z - w/2*s - h/2*c));
	coords.push_back(std::make_pair(x + w/2*c + h/2*s, z - w/2*s + h/2*c));
	coords.push_back(std::make_pair(x - w/2*c + h/2*s, z + w/2*s + h/2*c));

	for (size_t i = 0; i < coords.size(); ++i)
	{
		float px = coords[i].first;
		float pz = coords[i].second;
		float py = cmpTerrain->GetGroundLevel(px, pz) + RENDER_HEIGHT_DELTA;
		overlay.m_Coords.push_back(px);
		overlay.m_Coords.push_back(py);
		overlay.m_Coords.push_back(pz);
	}
}
