/* Copyright (C) 2022 Wildfire Games.
 * This file is part of 0 A.D.
 *
 * 0 A.D. is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * 0 A.D. is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with 0 A.D.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "precompiled.h"

#include "simulation2/system/Component.h"
#include "ICmpTerrain.h"

#include "graphics/Terrain.h"
#include "renderer/Renderer.h"
#include "renderer/SceneRenderer.h"
#include "renderer/WaterManager.h"
#include "maths/Vector3D.h"
#include "simulation2/components/ICmpObstructionManager.h"
#include "simulation2/components/ICmpRangeManager.h"
#include "simulation2/MessageTypes.h"

class CCmpTerrain final : public ICmpTerrain
{
public:
	static void ClassInit(CComponentManager& UNUSED(componentManager))
	{
	}

	DEFAULT_COMPONENT_ALLOCATOR(Terrain)

	CTerrain* m_Terrain; // not null

	static std::string GetSchema()
	{
		return "<a:component type='system'/><empty/>";
	}

	void Init(const CParamNode& UNUSED(paramNode)) override
	{
		m_Terrain = &GetSimContext().GetTerrain();
	}

	void Deinit() override
	{
	}

	void Serialize(ISerializer& UNUSED(serialize)) override
	{
	}

	void Deserialize(const CParamNode& paramNode, IDeserializer& UNUSED(deserialize)) override
	{
		Init(paramNode);
	}

	bool IsLoaded() const override
	{
		return m_Terrain->GetVerticesPerSide() != 0;
	}

	CFixedVector3D CalcNormal(entity_pos_t x, entity_pos_t z) const override
	{
		CFixedVector3D normal;
		m_Terrain->CalcNormalFixed((x / (int)TERRAIN_TILE_SIZE).ToInt_RoundToZero(), (z / (int)TERRAIN_TILE_SIZE).ToInt_RoundToZero(), normal);
		return normal;
	}

	CVector3D CalcExactNormal(float x, float z) const override
	{
		return m_Terrain->CalcExactNormal(x, z);
	}

	entity_pos_t GetGroundLevel(entity_pos_t x, entity_pos_t z) const override
	{
		// TODO: this can crash if the terrain heightmap isn't initialised yet

		return m_Terrain->GetExactGroundLevelFixed(x, z);
	}

	float GetExactGroundLevel(float x, float z) const override
	{
		return m_Terrain->GetExactGroundLevel(x, z);
	}

	u16 GetTilesPerSide() const override
	{
		ssize_t tiles = m_Terrain->GetTilesPerSide();

		if (tiles == -1)
			return 0;
		ENSURE(1 <= tiles && tiles <= 65535);
		return (u16)tiles;
	}

	u32 GetMapSize() const override
	{
		return GetTilesPerSide() * TERRAIN_TILE_SIZE;
	}

	u16 GetVerticesPerSide() const override
	{
		ssize_t vertices = m_Terrain->GetVerticesPerSide();
		ENSURE(1 <= vertices && vertices <= 65535);
		return (u16)vertices;
	}

	CTerrain* GetCTerrain() override
	{
		return m_Terrain;
	}

	void ReloadTerrain(bool ReloadWater) override
	{
		// TODO: should refactor this code to be nicer

		u16 tiles = GetTilesPerSide();
		u16 vertices = GetVerticesPerSide();

		CmpPtr<ICmpObstructionManager> cmpObstructionManager(GetSystemEntity());
		if (cmpObstructionManager)
		{
			cmpObstructionManager->SetBounds(entity_pos_t::Zero(), entity_pos_t::Zero(),
					entity_pos_t::FromInt(tiles*(int)TERRAIN_TILE_SIZE),
					entity_pos_t::FromInt(tiles*(int)TERRAIN_TILE_SIZE));
		}

		CmpPtr<ICmpRangeManager> cmpRangeManager(GetSystemEntity());
		if (cmpRangeManager)
		{
			cmpRangeManager->SetBounds(entity_pos_t::Zero(), entity_pos_t::Zero(),
					entity_pos_t::FromInt(tiles*(int)TERRAIN_TILE_SIZE),
					entity_pos_t::FromInt(tiles*(int)TERRAIN_TILE_SIZE));
		}

		if (ReloadWater && CRenderer::IsInitialised())
		{
			g_Renderer.GetSceneRenderer().GetWaterManager().SetMapSize(vertices);
			g_Renderer.GetSceneRenderer().GetWaterManager().RecomputeWaterData();
		}
		MakeDirty(0, 0, tiles+1, tiles+1);
	}

	void MakeDirty(i32 i0, i32 j0, i32 i1, i32 j1) override
	{
		CMessageTerrainChanged msg(i0, j0, i1, j1);
		GetSimContext().GetComponentManager().BroadcastMessage(msg);
	}
};

REGISTER_COMPONENT_TYPE(Terrain)
