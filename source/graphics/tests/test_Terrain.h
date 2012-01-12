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

#include "lib/self_test.h"

#include "graphics/Terrain.h"

#include "graphics/Patch.h"
#include "graphics/RenderableObject.h"
#include "maths/FixedVector3D.h"
#include "maths/Fixed.h"

class TestTerrain : public CxxTest::TestSuite
{
	void SetVertex(CTerrain& terrain, ssize_t i, ssize_t j, u16 height)
	{
		terrain.GetHeightMap()[j*terrain.GetVerticesPerSide() + i] = height;
		terrain.MakeDirty(RENDERDATA_UPDATE_VERTICES);
	}

	u16 GetVertex(CTerrain& terrain, ssize_t i, ssize_t j)
	{
		return terrain.GetHeightMap()[j*terrain.GetVerticesPerSide() + i];
	}

	void Set45Slope(CTerrain& terrain)
	{
		SetVertex(terrain, 0, 0, 100);
		SetVertex(terrain, 0, 1, 100);
		SetVertex(terrain, 0, 2, 100);
		SetVertex(terrain, 1, 0, 100 + TERRAIN_TILE_SIZE*HEIGHT_UNITS_PER_METRE);
		SetVertex(terrain, 1, 1, 100 + TERRAIN_TILE_SIZE*HEIGHT_UNITS_PER_METRE);
		SetVertex(terrain, 1, 2, 100 + TERRAIN_TILE_SIZE*HEIGHT_UNITS_PER_METRE);
		SetVertex(terrain, 2, 0, 100 + 2*TERRAIN_TILE_SIZE*HEIGHT_UNITS_PER_METRE);
		SetVertex(terrain, 2, 1, 100 + 2*TERRAIN_TILE_SIZE*HEIGHT_UNITS_PER_METRE);
		SetVertex(terrain, 2, 2, 100 + 2*TERRAIN_TILE_SIZE*HEIGHT_UNITS_PER_METRE);
		SetVertex(terrain, 3, 0, 100 + 2*TERRAIN_TILE_SIZE*HEIGHT_UNITS_PER_METRE);
		SetVertex(terrain, 3, 1, 100 + 2*TERRAIN_TILE_SIZE*HEIGHT_UNITS_PER_METRE);
		SetVertex(terrain, 3, 2, 100 + 2*TERRAIN_TILE_SIZE*HEIGHT_UNITS_PER_METRE);

	}

	void SetHighPlateau(CTerrain& terrain, int height)
	{
		SetVertex(terrain, 4, 0, 100 + height*TERRAIN_TILE_SIZE*HEIGHT_UNITS_PER_METRE);
		SetVertex(terrain, 4, 1, 100 + height*TERRAIN_TILE_SIZE*HEIGHT_UNITS_PER_METRE);
		SetVertex(terrain, 4, 2, 100 + height*TERRAIN_TILE_SIZE*HEIGHT_UNITS_PER_METRE);
		SetVertex(terrain, 5, 0, 100 + height*TERRAIN_TILE_SIZE*HEIGHT_UNITS_PER_METRE);
		SetVertex(terrain, 5, 1, 100 + height*TERRAIN_TILE_SIZE*HEIGHT_UNITS_PER_METRE);
		SetVertex(terrain, 5, 2, 100 + height*TERRAIN_TILE_SIZE*HEIGHT_UNITS_PER_METRE);
	}

public:
	void test_GetExactGroundLevel()
	{
		CTerrain terrain;
		terrain.Initialize(4, NULL);
		Set45Slope(terrain);
		SetHighPlateau(terrain, 20);

		float ground;

		ground = terrain.GetExactGroundLevel(0.f, 1.5f*TERRAIN_TILE_SIZE);
		TS_ASSERT_DELTA(ground, 100.f/HEIGHT_UNITS_PER_METRE, 0.01f);

		ground = terrain.GetExactGroundLevel(0.5f*TERRAIN_TILE_SIZE, 1.5f*TERRAIN_TILE_SIZE);
		TS_ASSERT_DELTA(ground, 100.f/HEIGHT_UNITS_PER_METRE+0.5f*TERRAIN_TILE_SIZE, 0.01f);

		ground = terrain.GetExactGroundLevel(1.5f*TERRAIN_TILE_SIZE, 1.5f*TERRAIN_TILE_SIZE);
		TS_ASSERT_DELTA(ground, 100.f/HEIGHT_UNITS_PER_METRE+1.5f*TERRAIN_TILE_SIZE, 0.01f);

		ground = terrain.GetExactGroundLevel(2.5f*TERRAIN_TILE_SIZE, 1.5f*TERRAIN_TILE_SIZE);
		TS_ASSERT_DELTA(ground, 100.f/HEIGHT_UNITS_PER_METRE+2.f*TERRAIN_TILE_SIZE, 0.01f);

		ground = terrain.GetExactGroundLevel(3.5f*TERRAIN_TILE_SIZE, 1.5f*TERRAIN_TILE_SIZE);
		TS_ASSERT_DELTA(ground, 100.f/HEIGHT_UNITS_PER_METRE+11.f*TERRAIN_TILE_SIZE, 0.01f);

		ground = terrain.GetExactGroundLevel(4.5f*TERRAIN_TILE_SIZE, 1.5f*TERRAIN_TILE_SIZE);
		TS_ASSERT_DELTA(ground, 100.f/HEIGHT_UNITS_PER_METRE+20.f*TERRAIN_TILE_SIZE, 0.01f);
	}

	void test_GetExactGroundLevelFixed()
	{
		CTerrain terrain;
		terrain.Initialize(4, NULL);
		Set45Slope(terrain);
		SetHighPlateau(terrain, 20);

		const double maxDelta = 0.0001;

		fixed ground;

		ground = terrain.GetExactGroundLevelFixed(fixed::FromFloat(0.f), fixed::FromFloat(1.5f*TERRAIN_TILE_SIZE));
		TS_ASSERT_DELTA(ground.ToDouble(), 100.0/HEIGHT_UNITS_PER_METRE, maxDelta);

		ground = terrain.GetExactGroundLevelFixed(fixed::FromFloat(0.5f*TERRAIN_TILE_SIZE), fixed::FromFloat(1.5f*TERRAIN_TILE_SIZE));
		TS_ASSERT_DELTA(ground.ToDouble(), 100.0/HEIGHT_UNITS_PER_METRE+0.5*TERRAIN_TILE_SIZE, maxDelta);

		ground = terrain.GetExactGroundLevelFixed(fixed::FromFloat(1.5f*TERRAIN_TILE_SIZE), fixed::FromFloat(1.5f*TERRAIN_TILE_SIZE));
		TS_ASSERT_DELTA(ground.ToDouble(), 100.0/HEIGHT_UNITS_PER_METRE+1.5*TERRAIN_TILE_SIZE, maxDelta);

		ground = terrain.GetExactGroundLevelFixed(fixed::FromFloat(2.5f*TERRAIN_TILE_SIZE), fixed::FromFloat(1.5f*TERRAIN_TILE_SIZE));
		TS_ASSERT_DELTA(ground.ToDouble(), 100.0/HEIGHT_UNITS_PER_METRE+2.0*TERRAIN_TILE_SIZE, maxDelta);

		ground = terrain.GetExactGroundLevelFixed(fixed::FromFloat(3.5f*TERRAIN_TILE_SIZE), fixed::FromFloat(1.5f*TERRAIN_TILE_SIZE));
		TS_ASSERT_DELTA(ground.ToDouble(), 100.0/HEIGHT_UNITS_PER_METRE+11.0*TERRAIN_TILE_SIZE, maxDelta);

		ground = terrain.GetExactGroundLevelFixed(fixed::FromFloat(4.5f*TERRAIN_TILE_SIZE), fixed::FromFloat(1.5f*TERRAIN_TILE_SIZE));
		TS_ASSERT_DELTA(ground.ToDouble(), 100.0/HEIGHT_UNITS_PER_METRE+20.0*TERRAIN_TILE_SIZE, maxDelta);
	}

	void test_GetExactGroundLevelFixed_max()
	{
		CTerrain terrain;
		terrain.Initialize(4, NULL);
		SetVertex(terrain, 0, 0, 65535);
		SetVertex(terrain, 0, 1, 65535);
		SetVertex(terrain, 1, 0, 65535);
		SetVertex(terrain, 1, 1, 65535);

		const double maxDelta = 0.003;

		int p = 255;
		for (int zi = 0; zi < p; ++zi)
		{
			for (int xi = 0; xi < p; ++xi)
			{
				fixed ground = terrain.GetExactGroundLevelFixed(fixed::FromFloat(xi/(float)p*TERRAIN_TILE_SIZE), fixed::FromFloat(zi/(float)p*TERRAIN_TILE_SIZE));
				TS_ASSERT_DELTA(ground.ToDouble(), 65535.0/HEIGHT_UNITS_PER_METRE, maxDelta);
			}
		}
	}

	void test_CalcNormal()
	{
		CTerrain terrain;
		terrain.Initialize(4, NULL);
		Set45Slope(terrain);

		CVector3D vec;

		terrain.CalcNormal(1, 1, vec);
		TS_ASSERT_DELTA(vec.X, -1.f/sqrt(2.f), 0.01f);
		TS_ASSERT_DELTA(vec.Y, 1.f/sqrt(2.f), 0.01f);
		TS_ASSERT_EQUALS(vec.Z, 0.f);

		terrain.CalcNormal(2, 1, vec);
		TS_ASSERT_DELTA(vec.X, (-1.f/sqrt(2.f)) / sqrt(2.f+sqrt(2.f)), 0.01f);
		TS_ASSERT_DELTA(vec.Y, (1.f+1.f/sqrt(2.f)) / sqrt(2.f+sqrt(2.f)), 0.01f);
		TS_ASSERT_EQUALS(vec.Z, 0);

		terrain.CalcNormal(5, 1, vec);
		TS_ASSERT_EQUALS(vec.X, 0.f);
		TS_ASSERT_EQUALS(vec.Y, 1.f);
		TS_ASSERT_EQUALS(vec.Z, 0.f);
	}

	void test_CalcNormalFixed()
	{
		CTerrain terrain;
		terrain.Initialize(4, NULL);
		Set45Slope(terrain);

		CFixedVector3D vec;

		terrain.CalcNormalFixed(1, 1, vec);
		TS_ASSERT_DELTA(vec.X.ToFloat(), -1.f/sqrt(2.f), 0.01f);
		TS_ASSERT_DELTA(vec.Y.ToFloat(), 1.f/sqrt(2.f), 0.01f);
		TS_ASSERT_EQUALS(vec.Z.ToFloat(), 0.f);

		terrain.CalcNormalFixed(2, 1, vec);
		TS_ASSERT_DELTA(vec.X.ToFloat(), (-1.f/sqrt(2.f)) / sqrt(2.f+sqrt(2.f)), 0.01f);
		TS_ASSERT_DELTA(vec.Y.ToFloat(), (1.f+1.f/sqrt(2.f)) / sqrt(2.f+sqrt(2.f)), 0.01f);
		TS_ASSERT_EQUALS(vec.Z.ToFloat(), 0);

		terrain.CalcNormalFixed(5, 1, vec);
		TS_ASSERT_EQUALS(vec.X.ToFloat(), 0.f);
		TS_ASSERT_EQUALS(vec.Y.ToFloat(), 1.f);
		TS_ASSERT_EQUALS(vec.Z.ToFloat(), 0.f);
	}

	void test_FlattenArea()
	{
		CTerrain terrain;
		terrain.Initialize(4, NULL);
		size_t m = terrain.GetVerticesPerSide() - 1;
		SetVertex(terrain, 0, 0, 25600);
		SetVertex(terrain, 2, 3, 25600);
		SetVertex(terrain, m, m, 25600);

		terrain.FlattenArea(-1000, 1000, -1000, 1000);

		// (25600*3) / (65*65) = 18
		TS_ASSERT_EQUALS(GetVertex(terrain, 0, 0), 18);
		TS_ASSERT_EQUALS(GetVertex(terrain, 4, 5), 18);
		TS_ASSERT_EQUALS(GetVertex(terrain, m, m), 18);
	}

	void test_FlattenArea_dirty()
	{
		CTerrain terrain;
		terrain.Initialize(4, NULL);

		for (ssize_t pj = 0; pj < terrain.GetPatchesPerSide(); ++pj)
			for (ssize_t pi = 0; pi < terrain.GetPatchesPerSide(); ++pi)
				terrain.GetPatch(pi, pj)->SetRenderData(new CRenderData());

#define EXPECT_DIRTY(which, pi, pj) TS_ASSERT_EQUALS((terrain.GetPatch(pi, pj)->GetRenderData()->m_UpdateFlags & RENDERDATA_UPDATE_VERTICES) != 0, which)

		EXPECT_DIRTY(false, 0, 0);
		EXPECT_DIRTY(false, 1, 0);
		EXPECT_DIRTY(false, 2, 0);
		EXPECT_DIRTY(false, 0, 1);
		EXPECT_DIRTY(false, 1, 1);
		EXPECT_DIRTY(false, 2, 1);
		EXPECT_DIRTY(false, 0, 2);
		EXPECT_DIRTY(false, 1, 2);
		EXPECT_DIRTY(false, 2, 2);

		terrain.FlattenArea(PATCH_SIZE*TERRAIN_TILE_SIZE, 2*PATCH_SIZE*TERRAIN_TILE_SIZE, PATCH_SIZE*TERRAIN_TILE_SIZE+1, 2*PATCH_SIZE*TERRAIN_TILE_SIZE-1);

		EXPECT_DIRTY(true, 0, 0);
		EXPECT_DIRTY(true, 1, 0);
		EXPECT_DIRTY(true, 2, 0);
		EXPECT_DIRTY(true, 0, 1);
		EXPECT_DIRTY(true, 1, 1);
		EXPECT_DIRTY(true, 2, 1);
		EXPECT_DIRTY(true, 0, 2);
		EXPECT_DIRTY(true, 1, 2);
		EXPECT_DIRTY(true, 2, 2);
		// This is dirtying more than strictly necessary, but that's okay
	}

};
