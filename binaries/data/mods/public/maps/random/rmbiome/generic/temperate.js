function setupBiome_temperate()
{
	if (randBool())
	{
		g_Terrains.mainTerrain = "alpine_grass";
		g_Terrains.forestFloor1 = "temp_forestfloor_pine";
		g_Terrains.forestFloor2 = "temp_grass_clovers_2";
		g_Terrains.tier1Terrain = "alpine_grass_a";
		g_Terrains.tier2Terrain = "alpine_grass_b";
		g_Terrains.tier3Terrain = "alpine_grass_c";
		g_Terrains.tier4Terrain = "temp_grass_mossy";
	}
	else
	{
		g_Terrains.mainTerrain = "temp_grass_long_b";
		g_Terrains.forestFloor1 = "temp_forestfloor_pine";
		g_Terrains.forestFloor2 = "temp_plants_bog";
		g_Terrains.tier1Terrain = "temp_grass_d";
		g_Terrains.tier2Terrain = "temp_grass_c";
		g_Terrains.tier3Terrain = "temp_grass_clovers_2";
		g_Terrains.tier4Terrain = "temp_grass_plants";
	}

	[g_Gaia.tree1, g_Gaia.tree2] = pickRandom([
		[
			"gaia/tree/oak",
			"gaia/tree/oak_large"
		],
		[
			"gaia/tree/poplar",
			"gaia/tree/poplar"
		],
		[
			"gaia/tree/euro_beech",
			"gaia/tree/euro_beech"
		]
	]);

	[g_Gaia.tree4, g_Gaia.tree5] = pickRandom([
		[
			"gaia/tree/pine",
			"gaia/tree/aleppo_pine"
		],
		[
			"gaia/tree/pine",
			"gaia/tree/pine"
		],
		[
			"gaia/tree/aleppo_pine",
			"gaia/tree/aleppo_pine"
		]
	]);
}
