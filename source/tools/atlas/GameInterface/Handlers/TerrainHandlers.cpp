/* Copyright (C) 2021 Wildfire Games.
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

#include "graphics/Patch.h"
#include "graphics/TerrainTextureManager.h"
#include "graphics/TerrainTextureEntry.h"
#include "graphics/Terrain.h"
#include "ps/Game.h"
#include "ps/World.h"
#include "lib/tex/tex.h"
#include "ps/Filesystem.h"
#include "simulation2/Simulation2.h"
#include "simulation2/components/ICmpPathfinder.h"
#include "simulation2/components/ICmpTerrain.h"
#include "simulation2/helpers/Grid.h"

#include "../Brushes.h"
#include "../DeltaArray.h"
#include "../View.h"

#include <queue>

namespace AtlasMessage
{

namespace
{

sTerrainTexturePreview MakeEmptyTerrainTexturePreview()
{
	sTerrainTexturePreview preview{};
	preview.name = std::wstring();
	preview.loaded = false;
	preview.imageHeight = 0;
	preview.imageWidth = 0;
	preview.imageData = {};
	return preview;
}

bool CompareTerrain(const sTerrainTexturePreview& a, const sTerrainTexturePreview& b)
{
	return (wcscmp(a.name.c_str(), b.name.c_str()) < 0);
}

sTerrainTexturePreview GetPreview(CTerrainTextureEntry* tex, size_t width, size_t height)
{
	sTerrainTexturePreview preview;
	preview.name = tex->GetTag().FromUTF8();

	const size_t previewBPP = 3;
	std::vector<u8> buffer(width * height * previewBPP);

	// It's not good to shrink the entire texture to fit the small preview
	// window, since it's the fine details in the texture that are
	// interesting; so just go down one mipmap level, then crop a chunk
	// out of the middle.

	std::shared_ptr<u8> fileData;
	size_t fileSize;
	Tex texture;
	const bool canUsePreview =
		!tex->GetDiffuseTexturePath().empty() &&
		g_VFS->LoadFile(tex->GetDiffuseTexturePath(), fileData, fileSize) == INFO::OK &&
		texture.decode(fileData, fileSize) == INFO::OK &&
		// Check that we can fit the texture into the preview size before any transform.
		texture.m_Width >= width && texture.m_Height >= height &&
		// Transform to a single format that we can process.
		texture.transform_to((texture.m_Flags) & ~(TEX_DXT | TEX_MIPMAPS | TEX_GREY | TEX_BGR)) == INFO::OK &&
		(texture.m_Bpp == 24 || texture.m_Bpp == 32);
	if (canUsePreview)
	{
		size_t level = 0;
		while ((texture.m_Width >> (level + 1)) >= width && (texture.m_Height >> (level + 1)) >= height)
			++level;
		// Extract the middle section (as a representative preview),
		// and copy into buffer.
		u8* data = texture.get_data();
		const size_t dataShiftX = ((texture.m_Width - width) / 2) >> level;
		const size_t dataShiftY = ((texture.m_Height - height) / 2) >> level;
		for (size_t y = 0; y < height; ++y)
			for (size_t x = 0; x < width; ++x)
			{
				const size_t bufferOffset = (y * width + x) * previewBPP;
				const size_t dataOffset = (((y << level) + dataShiftY) * texture.m_Width + (x << level) + dataShiftX) * texture.m_Bpp / 8;
				buffer[bufferOffset + 0] = data[dataOffset + 0];
				buffer[bufferOffset + 1] = data[dataOffset + 1];
				buffer[bufferOffset + 2] = data[dataOffset + 2];
			}
		preview.loaded = true;
	}
	else
	{
		// Too small to preview. Just use a flat color instead.
		const u32 baseColor = tex->GetBaseColor();
		for (size_t i = 0; i < width * height; ++i)
		{
			buffer[i * previewBPP + 0] = (baseColor >> 16) & 0xff;
			buffer[i * previewBPP + 1] = (baseColor >> 8) & 0xff;
			buffer[i * previewBPP + 2] = (baseColor >> 0) & 0xff;
		}
		preview.loaded = tex->GetTexture()->IsLoaded();
	}

	preview.imageWidth = width;
	preview.imageHeight = height;
	preview.imageData = buffer;

	return preview;
}

} // anonymous namespace

QUERYHANDLER(GetTerrainGroups)
{
	const CTerrainTextureManager::TerrainGroupMap &groups = g_TexMan.GetGroups();
	std::vector<std::wstring> groupNames;
	for (CTerrainTextureManager::TerrainGroupMap::const_iterator it = groups.begin(); it != groups.end(); ++it)
		groupNames.push_back(it->first.FromUTF8());
	msg->groupNames = groupNames;
}

QUERYHANDLER(GetTerrainGroupTextures)
{
	std::vector<std::wstring> names;

	CTerrainGroup* group = g_TexMan.FindGroup(CStrW(*msg->groupName).ToUTF8());
	if (group)
	{
		for (std::vector<CTerrainTextureEntry*>::const_iterator it = group->GetTerrains().begin(); it != group->GetTerrains().end(); ++it)
			names.emplace_back((*it)->GetTag().FromUTF8());
	}
	std::sort(names.begin(), names.end());
	msg->names = names;
}

QUERYHANDLER(GetTerrainGroupPreviews)
{
	std::vector<sTerrainTexturePreview> previews;

	CTerrainGroup* group = g_TexMan.FindGroup(CStrW(*msg->groupName).ToUTF8());
	for (std::vector<CTerrainTextureEntry*>::const_iterator it = group->GetTerrains().begin(); it != group->GetTerrains().end(); ++it)
	{
		previews.push_back(GetPreview(*it, msg->imageWidth, msg->imageHeight));
 	}

	// Sort the list alphabetically by name
	std::sort(previews.begin(), previews.end(), CompareTerrain);
	msg->previews = previews;
}

QUERYHANDLER(GetTerrainPassabilityClasses)
{
	CmpPtr<ICmpPathfinder> cmpPathfinder(*AtlasView::GetView_Game()->GetSimulation2(), SYSTEM_ENTITY);
	if (cmpPathfinder)
	{
		std::map<std::string, pass_class_t> nonPathfindingClasses, pathfindingClasses;
		cmpPathfinder->GetPassabilityClasses(nonPathfindingClasses, pathfindingClasses);

		std::vector<std::wstring> classNames;
		for (std::map<std::string, pass_class_t>::iterator it = nonPathfindingClasses.begin(); it != nonPathfindingClasses.end(); ++it)
			classNames.push_back(CStr(it->first).FromUTF8());
		msg->classNames = classNames;
	}
}

QUERYHANDLER(GetTerrainTexture)
{
	ssize_t x, y;
	g_CurrentBrush.m_Centre = msg->pos->GetWorldSpace();
	g_CurrentBrush.GetCentre(x, y);

	CTerrain* terrain = g_Game->GetWorld()->GetTerrain();
	CMiniPatch* tile = terrain->GetTile(x, y);
	if (tile)
	{
		CTerrainTextureEntry* tex = tile->GetTextureEntry();
		msg->texture = tex->GetTag().FromUTF8();
	}
	else
	{
		msg->texture = std::wstring();
	}
}

QUERYHANDLER(GetTerrainTexturePreview)
{
	CTerrainTextureEntry* tex = g_TexMan.FindTexture(CStrW(*msg->name).ToUTF8());
	if (tex)
		msg->preview = GetPreview(tex, msg->imageWidth, msg->imageHeight);
	else
		msg->preview = MakeEmptyTerrainTexturePreview();
}

//////////////////////////////////////////////////////////////////////////

namespace
{

struct TerrainTile
{
	TerrainTile(CTerrainTextureEntry* t, ssize_t p) : tex(t), priority(p) {}
	CTerrainTextureEntry* tex;
	ssize_t priority;
};

class TerrainArray : public DeltaArray2D<TerrainTile>
{
public:
	void Init()
	{
		m_Terrain = g_Game->GetWorld()->GetTerrain();
		m_VertsPerSide = g_Game->GetWorld()->GetTerrain()->GetVerticesPerSide();
	}

	void UpdatePriority(ssize_t x, ssize_t y, CTerrainTextureEntry* tex, ssize_t priorityScale, ssize_t& priority)
	{
		CMiniPatch* tile = m_Terrain->GetTile(x, y);
		if (!tile)
			return; // tile was out-of-bounds

		// If this tile matches the current texture, we just want to match its
		// priority; otherwise we want to exceed its priority
		if (tile->GetTextureEntry() == tex)
			priority = std::max(priority, tile->GetPriority()*priorityScale);
		else
			priority = std::max(priority, tile->GetPriority()*priorityScale + 1);
	}

	CTerrainTextureEntry* GetTexEntry(ssize_t x, ssize_t y)
	{
		if (size_t(x) >= size_t(m_VertsPerSide-1) || size_t(y) >= size_t(m_VertsPerSide-1))
			return NULL;

		return get(x, y).tex;
	}

	ssize_t GetPriority(ssize_t x, ssize_t y)
	{
		if (size_t(x) >= size_t(m_VertsPerSide-1) || size_t(y) >= size_t(m_VertsPerSide-1))
			return 0;

		return get(x, y).priority;
	}

	void PaintTile(ssize_t x, ssize_t y, CTerrainTextureEntry* tex, ssize_t priority)
	{
		// Ignore out-of-bounds tiles
		if (size_t(x) >= size_t(m_VertsPerSide-1) || size_t(y) >= size_t(m_VertsPerSide-1))
			return;

		set(x,y, TerrainTile(tex, priority));
	}

	ssize_t GetTilesPerSide()
	{
		return m_VertsPerSide-1;
	}

protected:
	TerrainTile getOld(ssize_t x, ssize_t y)
	{
		CMiniPatch* mp = m_Terrain->GetTile(x, y);
		ENSURE(mp);
		return TerrainTile(mp->Tex, mp->Priority);
	}
	void setNew(ssize_t x, ssize_t y, const TerrainTile& val)
	{
		CMiniPatch* mp = m_Terrain->GetTile(x, y);
		ENSURE(mp);
		mp->Tex = val.tex;
		mp->Priority = val.priority;
	}

	CTerrain* m_Terrain;
	ssize_t m_VertsPerSide;
};

}

BEGIN_COMMAND(PaintTerrain)
{
	TerrainArray m_TerrainDelta;
	ssize_t m_i0, m_j0, m_i1, m_j1; // dirtied tiles (inclusive lower bound, exclusive upper)

	cPaintTerrain()
	{
		m_TerrainDelta.Init();
	}

	void MakeDirty()
	{
		g_Game->GetWorld()->GetTerrain()->MakeDirty(m_i0, m_j0, m_i1, m_j1, RENDERDATA_UPDATE_INDICES);
	}

	void Do()
	{
		g_CurrentBrush.m_Centre = msg->pos->GetWorldSpace();

		ssize_t x0, y0;
		g_CurrentBrush.GetBottomLeft(x0, y0);

		CTerrainTextureEntry* texentry = g_TexMan.FindTexture(CStrW(*msg->texture).ToUTF8());
		if (! texentry)
		{
			debug_warn(L"Can't find texentry"); // TODO: nicer error handling
			return;
		}

		// Priority system: If the new tile should have a high priority,
		// set it to one plus the maximum priority of all surrounding tiles
		// that aren't included in the brush (so that it's definitely the highest).
		// Similar for low priority.
		ssize_t priorityScale = (msg->priority == ePaintTerrainPriority::HIGH ? +1 : -1);
		ssize_t priority = 0;

		for (ssize_t dy = -1; dy < g_CurrentBrush.m_H+1; ++dy)
		{
			for (ssize_t dx = -1; dx < g_CurrentBrush.m_W+1; ++dx)
			{
				if (!(g_CurrentBrush.Get(dx, dy) > 0.5f)) // ignore tiles that will be painted over
					m_TerrainDelta.UpdatePriority(x0+dx, y0+dy, texentry, priorityScale, priority);
			}
		}

		for (ssize_t dy = 0; dy < g_CurrentBrush.m_H; ++dy)
		{
			for (ssize_t dx = 0; dx < g_CurrentBrush.m_W; ++dx)
			{
				if (g_CurrentBrush.Get(dx, dy) > 0.5f) // TODO: proper solid brushes
					m_TerrainDelta.PaintTile(x0+dx, y0+dy, texentry, priority*priorityScale);
			}
		}

		m_i0 = x0 - 1;
		m_j0 = y0 - 1;
		m_i1 = x0 + g_CurrentBrush.m_W + 1;
		m_j1 = y0 + g_CurrentBrush.m_H + 1;
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

	void MergeIntoPrevious(cPaintTerrain* prev)
	{
		prev->m_TerrainDelta.OverlayWith(m_TerrainDelta);
		prev->m_i0 = std::min(prev->m_i0, m_i0);
		prev->m_j0 = std::min(prev->m_j0, m_j0);
		prev->m_i1 = std::max(prev->m_i1, m_i1);
		prev->m_j1 = std::max(prev->m_j1, m_j1);
	}
};
END_COMMAND(PaintTerrain)

//////////////////////////////////////////////////////////////////////////

BEGIN_COMMAND(ReplaceTerrain)
{
	TerrainArray m_TerrainDelta;
	ssize_t m_i0, m_j0, m_i1, m_j1; // dirtied tiles (inclusive lower bound, exclusive upper)

	cReplaceTerrain()
	{
		m_TerrainDelta.Init();
	}

	void MakeDirty()
	{
		g_Game->GetWorld()->GetTerrain()->MakeDirty(m_i0, m_j0, m_i1, m_j1, RENDERDATA_UPDATE_INDICES);
		CmpPtr<ICmpTerrain> cmpTerrain(*g_Game->GetSimulation2(), SYSTEM_ENTITY);
		if (cmpTerrain)
			cmpTerrain->MakeDirty(m_i0, m_j0, m_i1, m_j1);
	}

	void Do()
	{
		g_CurrentBrush.m_Centre = msg->pos->GetWorldSpace();

		ssize_t x0, y0;
		g_CurrentBrush.GetBottomLeft(x0, y0);

		m_i0 = m_i1 = x0;
		m_j0 = m_j1 = y0;

		CTerrainTextureEntry* texentry = g_TexMan.FindTexture(CStrW(*msg->texture).ToUTF8());
		if (! texentry)
		{
			debug_warn(L"Can't find texentry"); // TODO: nicer error handling
			return;
		}

		CTerrainTextureEntry* replacedTex = m_TerrainDelta.GetTexEntry(x0, y0);

		// Don't bother if we're not making a change
		if (texentry == replacedTex)
		{
			return;
		}

		ssize_t tiles = m_TerrainDelta.GetTilesPerSide();

		for (ssize_t j = 0; j < tiles; ++j)
		{
			for (ssize_t i = 0; i < tiles; ++i)
			{
				if (m_TerrainDelta.GetTexEntry(i, j) == replacedTex)
				{
					m_i0 = std::min(m_i0, i-1);
					m_j0 = std::min(m_j0, j-1);
					m_i1 = std::max(m_i1, i+2);
					m_j1 = std::max(m_j1, j+2);
					m_TerrainDelta.PaintTile(i, j, texentry, m_TerrainDelta.GetPriority(i, j));
				}
			}
		}

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
};
END_COMMAND(ReplaceTerrain)

//////////////////////////////////////////////////////////////////////////

BEGIN_COMMAND(FillTerrain)
{
	TerrainArray m_TerrainDelta;
	ssize_t m_i0, m_j0, m_i1, m_j1; // dirtied tiles (inclusive lower bound, exclusive upper)

	cFillTerrain()
	{
		m_TerrainDelta.Init();
	}

	void MakeDirty()
	{
		g_Game->GetWorld()->GetTerrain()->MakeDirty(m_i0, m_j0, m_i1, m_j1, RENDERDATA_UPDATE_INDICES);
		CmpPtr<ICmpTerrain> cmpTerrain(*g_Game->GetSimulation2(), SYSTEM_ENTITY);
		if (cmpTerrain)
			cmpTerrain->MakeDirty(m_i0, m_j0, m_i1, m_j1);
	}

	void Do()
	{
		g_CurrentBrush.m_Centre = msg->pos->GetWorldSpace();

		ssize_t x0, y0;
		g_CurrentBrush.GetBottomLeft(x0, y0);

		m_i0 = m_i1 = x0;
		m_j0 = m_j1 = y0;

		CTerrainTextureEntry* texentry = g_TexMan.FindTexture(CStrW(*msg->texture).ToUTF8());
		if (! texentry)
		{
			debug_warn(L"Can't find texentry"); // TODO: nicer error handling
			return;
		}

		CTerrainTextureEntry* replacedTex = m_TerrainDelta.GetTexEntry(x0, y0);

		// Don't bother if we're not making a change
		if (texentry == replacedTex)
		{
			return;
		}

		ssize_t tiles = m_TerrainDelta.GetTilesPerSide();

		// Simple 4-way flood fill algorithm using queue and a grid to keep track of visited tiles,
		//	almost as fast as loop for filling whole map, much faster for small patches
		SparseGrid<bool> visited(tiles, tiles);
		std::queue<std::pair<u16, u16> > queue;

		// Initial tile
		queue.push(std::make_pair((u16)x0, (u16)y0));
		visited.set(x0, y0, true);

		while(!queue.empty())
		{
			// Check front of queue
			std::pair<u16, u16> t = queue.front();
			queue.pop();
			u16 i = t.first;
			u16 j = t.second;

			if (m_TerrainDelta.GetTexEntry(i, j) == replacedTex)
			{
				// Found a tile to replace: adjust bounds and paint it
				m_i0 = std::min(m_i0, (ssize_t)i-1);
				m_j0 = std::min(m_j0, (ssize_t)j-1);
				m_i1 = std::max(m_i1, (ssize_t)i+2);
				m_j1 = std::max(m_j1, (ssize_t)j+2);
				m_TerrainDelta.PaintTile(i, j, texentry, m_TerrainDelta.GetPriority(i, j));

				// Visit 4 adjacent tiles (could visit 8 if we want to count diagonal adjacency)
				if (i > 0 && !visited.get(i-1, j))
				{
					visited.set(i-1, j, true);
					queue.push(std::make_pair(i-1, j));
				}
				if (i < (tiles-1) && !visited.get(i+1, j))
				{
					visited.set(i+1, j, true);
					queue.push(std::make_pair(i+1, j));
				}
				if (j > 0 && !visited.get(i, j-1))
				{
					visited.set(i, j-1, true);
					queue.push(std::make_pair(i, j-1));
				}
				if (j < (tiles-1) && !visited.get(i, j+1))
				{
					visited.set(i, j+1, true);
					queue.push(std::make_pair(i, j+1));
				}
			}
		}

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
};
END_COMMAND(FillTerrain)

} // namespace AtlasMessage
