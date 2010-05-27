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

#include "MessageHandler.h"

#include "../CommandProc.h"

#include "graphics/Terrain.h"
#include "ps/CStr.h"
#include "ps/Game.h"
#include "ps/World.h"
#include "maths/MathUtil.h"
#include "graphics/RenderableObject.h"
#include "simulation2/Simulation2.h"
#include "simulation2/components/ICmpTerrain.h"

#include "../Brushes.h"
#include "../DeltaArray.h"

namespace AtlasMessage {

class TerrainArray : public DeltaArray2D<u16>
{
public:
	void Init()
	{
		m_Heightmap = g_Game->GetWorld()->GetTerrain()->GetHeightMap();
		m_VertsPerSide = g_Game->GetWorld()->GetTerrain()->GetVerticesPerSide();
	}

	void RaiseVertex(ssize_t x, ssize_t y, int amount)
	{
		// Ignore out-of-bounds vertices
		if (size_t(x) >= size_t(m_VertsPerSide) || size_t(y) >= size_t(m_VertsPerSide))
			return;

		set(x,y, (u16)clamp(get(x,y) + amount, 0, 65535));
	}

	void MoveVertexTowards(ssize_t x, ssize_t y, int target, int amount)
	{
		if (size_t(x) >= size_t(m_VertsPerSide) || size_t(y) >= size_t(m_VertsPerSide))
			return;

		int h = get(x,y);
		if (h < target)
			h = std::min(target, h + amount);
		else if (h > target)
			h = std::max(target, h - amount);
		else
			return;

		set(x,y, (u16)clamp(h, 0, 65535));
	}

	void SetVertex(ssize_t x, ssize_t y, u16 value)
	{
		if (size_t(x) >= size_t(m_VertsPerSide) || size_t(y) >= size_t(m_VertsPerSide))
			return;

		set(x,y, value);
	}

	u16 GetVertex(ssize_t x, ssize_t y)
	{
		return get(clamp(x, ssize_t(0), ssize_t(m_VertsPerSide-1)), clamp(y, ssize_t(0), ssize_t(m_VertsPerSide-1)));
	}

protected:
	u16 getOld(ssize_t x, ssize_t y)
	{
		return m_Heightmap[y*m_VertsPerSide + x];
	}
	void setNew(ssize_t x, ssize_t y, const u16& val)
	{
		m_Heightmap[y*m_VertsPerSide + x] = val;
	}

	u16* m_Heightmap;
	ssize_t m_VertsPerSide;
};

	
BEGIN_COMMAND(AlterElevation)
{
	TerrainArray m_TerrainDelta;
	ssize_t m_i0, m_j0, m_i1, m_j1;

	cAlterElevation()
	{
		m_TerrainDelta.Init();
	}

	void MakeDirty()
	{
		g_Game->GetWorld()->GetTerrain()->MakeDirty(m_i0, m_j0, m_i1, m_j1, RENDERDATA_UPDATE_VERTICES);
		CmpPtr<ICmpTerrain> cmpTerrain(*g_Game->GetSimulation2(), SYSTEM_ENTITY);
		if (!cmpTerrain.null())
			cmpTerrain->MakeDirty(m_i0, m_j0, m_i1, m_j1);
	}

	void Do()
	{
		int amount = (int)msg->amount;

		// If the framerate is very high, 'amount' is often very
		// small (even zero) so the integer truncation is significant
		static float roundingError = 0.0;
		roundingError += msg->amount - (float)amount;
		if (roundingError >= 1.f)
		{
			amount += (int)roundingError;
			roundingError -= (float)(int)roundingError;
		}

		static CVector3D previousPosition;
		g_CurrentBrush.m_Centre = msg->pos->GetWorldSpace(previousPosition);
		previousPosition = g_CurrentBrush.m_Centre;

		ssize_t x0, y0;
		g_CurrentBrush.GetBottomLeft(x0, y0);

		for (ssize_t dy = 0; dy < g_CurrentBrush.m_H; ++dy)
		{
			for (ssize_t dx = 0; dx < g_CurrentBrush.m_W; ++dx)
			{
				// TODO: proper variable raise amount (store floats in terrain delta array?)
				float b = g_CurrentBrush.Get(dx, dy);
				if (b)
					m_TerrainDelta.RaiseVertex(x0+dx, y0+dy, (int)(amount*b));
			}
		}

		m_i0 = x0;
		m_j0 = y0;
		m_i1 = x0 + g_CurrentBrush.m_W;
		m_j1 = y0 + g_CurrentBrush.m_H;
		MakeDirty();
	}

	void Undo()
	{
		m_TerrainDelta.Undo();
		MakeDirty();
	}

	void Redo()
	{
		m_TerrainDelta.Redo();
		MakeDirty();
	}

	void MergeIntoPrevious(cAlterElevation* prev)
	{
		prev->m_TerrainDelta.OverlayWith(m_TerrainDelta);
		prev->m_i0 = std::min(prev->m_i0, m_i0);
		prev->m_j0 = std::min(prev->m_j0, m_j0);
		prev->m_i1 = std::max(prev->m_i1, m_i1);
		prev->m_j1 = std::max(prev->m_j1, m_j1);
	}
};
END_COMMAND(AlterElevation)

//////////////////////////////////////////////////////////////////////////

BEGIN_COMMAND(FlattenElevation)
{
	TerrainArray m_TerrainDelta;
	ssize_t m_i0, m_j0, m_i1, m_j1;

	cFlattenElevation()
	{
		m_TerrainDelta.Init();
	}

	void MakeDirty()
	{
		g_Game->GetWorld()->GetTerrain()->MakeDirty(m_i0, m_j0, m_i1, m_j1, RENDERDATA_UPDATE_VERTICES);
		CmpPtr<ICmpTerrain> cmpTerrain(*g_Game->GetSimulation2(), SYSTEM_ENTITY);
		if (!cmpTerrain.null())
			cmpTerrain->MakeDirty(m_i0, m_j0, m_i1, m_j1);
	}

	void Do()
	{
		int amount = (int)msg->amount;

		static CVector3D previousPosition;
		g_CurrentBrush.m_Centre = msg->pos->GetWorldSpace(previousPosition);
		previousPosition = g_CurrentBrush.m_Centre;

		ssize_t xc, yc;
		g_CurrentBrush.GetCentre(xc, yc);
		u16 height = m_TerrainDelta.GetVertex(xc, yc);

		ssize_t x0, y0;
		g_CurrentBrush.GetBottomLeft(x0, y0);

		for (ssize_t dy = 0; dy < g_CurrentBrush.m_H; ++dy)
			for (ssize_t dx = 0; dx < g_CurrentBrush.m_W; ++dx)
			{
				float b = g_CurrentBrush.Get(dx, dy);
				if (b)
					m_TerrainDelta.MoveVertexTowards(x0+dx, y0+dy, height, 1 + (int)(b*amount));
			}

		m_i0 = x0;
		m_j0 = y0;
		m_i1 = x0 + g_CurrentBrush.m_W;
		m_j1 = y0 + g_CurrentBrush.m_H;
		MakeDirty();
	}

	void Undo()
	{
		m_TerrainDelta.Undo();
		MakeDirty();
	}

	void Redo()
	{
		m_TerrainDelta.Redo();
		MakeDirty();
	}

	void MergeIntoPrevious(cFlattenElevation* prev)
	{
		prev->m_TerrainDelta.OverlayWith(m_TerrainDelta);
		prev->m_i0 = std::min(prev->m_i0, m_i0);
		prev->m_j0 = std::min(prev->m_j0, m_j0);
		prev->m_i1 = std::max(prev->m_i1, m_i1);
		prev->m_j1 = std::max(prev->m_j1, m_j1);
	}
};
END_COMMAND(FlattenElevation)

}
