#include "precompiled.h"

#include "MessageHandler.h"

#include "../CommandProc.h"

#include "graphics/Terrain.h"
#include "ps/Game.h"

#include "../Brushes.h"

namespace AtlasMessage {


BEGIN_COMMAND(AlterElevation)

	// TODO: much more efficient version of this, and without the memory leaks
	u16* OldTerrain;
	u16* NewTerrain;

	void Construct()
	{
		OldTerrain = NewTerrain = NULL;
	}
	void Destruct()
	{
		delete OldTerrain;
		delete NewTerrain;
	}

	void Do()
	{

		CTerrain* terrain = g_Game->GetWorld()->GetTerrain();

		int verts = terrain->GetVerticesPerSide()*terrain->GetVerticesPerSide();
		OldTerrain = new u16[verts];
		memcpy(OldTerrain, terrain->GetHeightMap(), verts*sizeof(u16));

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
					terrain->RaiseVertex(x0+dx, y0+dy, amount*b);
			}

		terrain->MakeDirty(x0, y0, x0+g_CurrentBrush.m_W, y0+g_CurrentBrush.m_H);
	}

	void Undo()
	{
		CTerrain* terrain = g_Game->GetWorld()->GetTerrain();
		if (! NewTerrain)
		{
			int verts = terrain->GetVerticesPerSide()*terrain->GetVerticesPerSide();
			NewTerrain = new u16[verts];
			memcpy(NewTerrain, terrain->GetHeightMap(), verts*sizeof(u16));
		}
		terrain->SetHeightMap(OldTerrain); // CTerrain duplicates the data
	}

	void Redo()
	{
		CTerrain* terrain = g_Game->GetWorld()->GetTerrain();
		terrain->SetHeightMap(NewTerrain); // CTerrain duplicates the data
	}

	void MergeWithSelf(cAlterElevation* prev)
	{
		std::swap(prev->NewTerrain, NewTerrain);
	}

END_COMMAND(AlterElevation);

}
