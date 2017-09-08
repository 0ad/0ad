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
			"gaia/flora_tree_oak",
			"gaia/flora_tree_oak_large"
		],
		[
			"gaia/flora_tree_poplar",
			"gaia/flora_tree_poplar"
		],
		[
			"gaia/flora_tree_euro_beech",
			"gaia/flora_tree_euro_beech"
		]
	]);

	[g_Gaia.tree4, g_Gaia.tree5] = pickRandom([
		[
			"gaia/flora_tree_pine",
			"gaia/flora_tree_aleppo_pine"
		],
		[
			"gaia/flora_tree_pine",
			"gaia/flora_tree_pine"
		],
		[
			"gaia/flora_tree_aleppo_pine",
			"gaia/flora_tree_aleppo_pine"
		]
	]);
}
