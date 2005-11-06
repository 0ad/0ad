#include "precompiled.h"

#include "MessageHandler.h"

#include "../CommandProc.h"

#include "graphics/Terrain.h"
#include "ps/Game.h"
#include "maths/MathUtil.h"

#include "../Brushes.h"
#include "../DeltaArray.h"

namespace AtlasMessage {

BEGIN_COMMAND(AlterElevation)

	class TerrainArray : public DeltaArray2D<u16>
	{
	public:
		void Init()
		{
			m_Heightmap = g_Game->GetWorld()->GetTerrain()->GetHeightMap();
			m_VertsPerSide = g_Game->GetWorld()->GetTerrain()->GetVerticesPerSide();
		}

		void RaiseVertex(int x, int y, int amount)
		{
			// Ignore out-of-bounds vertices
			if ((unsigned)x >= m_VertsPerSide || (unsigned)y >= m_VertsPerSide)
				return;

			set(x,y, (u16)clamp(get(x,y) + amount, 0, 65535));
		}

	protected:
		u16 getOld(int x, int y)
		{
			return m_Heightmap[y*m_VertsPerSide + x];
		}
		void setNew(int x, int y, const u16& val)
		{
			m_Heightmap[y*m_VertsPerSide + x] = val;
		}

		u16* m_Heightmap;
		size_t m_VertsPerSide;
	};

	TerrainArray m_TerrainDelta;

	void Construct()
	{
		m_TerrainDelta.Init();
	}
	void Destruct()
	{
	}

	void Do()
	{

		int amount = (int)d->amount;

		// If the framerate is very high, 'amount' is often very
		// small (even zero) so the integer truncation is significant
		static float roundingError = 0.0;
		roundingError += d->amount - (float)amount;
		if (roundingError >= 1.f)
		{
			amount += (int)roundingError;
			roundingError -= (float)(int)roundingError;
		}

		static CVector3D previousPosition;
		d->pos.GetWorldSpace(g_CurrentBrush.m_Centre, previousPosition);
		previousPosition = g_CurrentBrush.m_Centre;

		int x0, y0;
		g_CurrentBrush.GetBottomRight(x0, y0);

		for (int dy = 0; dy < g_CurrentBrush.m_H; ++dy)
			for (int dx = 0; dx < g_CurrentBrush.m_W; ++dx)
			{
				// TODO: proper variable raise amount (store floats in terrain delta array?)
				float b = g_CurrentBrush.Get(dx, dy);
				if (b)
					m_TerrainDelta.RaiseVertex(x0+dx, y0+dy, amount*b);
			}

		g_Game->GetWorld()->GetTerrain()->MakeDirty(x0, y0, x0+g_CurrentBrush.m_W, y0+g_CurrentBrush.m_H, RENDERDATA_UPDATE_VERTICES);
	}

	void Undo()
	{
		m_TerrainDelta.Undo();
		g_Game->GetWorld()->GetTerrain()->MakeDirty(RENDERDATA_UPDATE_VERTICES);
	}

	void Redo()
	{
		m_TerrainDelta.Redo();
		g_Game->GetWorld()->GetTerrain()->MakeDirty(RENDERDATA_UPDATE_VERTICES);
	}

	void MergeWithSelf(cAlterElevation* prev)
	{
		prev->m_TerrainDelta.OverlayWith(m_TerrainDelta);
	}

END_COMMAND(AlterElevation);

}
