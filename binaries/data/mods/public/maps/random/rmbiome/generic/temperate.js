function setupBiome_temperate()
{
	if (randBool())
	{
		g_Terrains.mainTerrain = "temperate_grass_04";
		g_Terrains.forestFloor1 = "temperate_forestfloor_01";
		g_Terrains.forestFloor2 = "temperate_forestfloor_02";
		g_Terrains.tier1Terrain = "temperate_grass_dirt_02";
		g_Terrains.tier2Terrain = "temperate_grass_03";
		g_Terrains.tier3Terrain = "temperate_grass_04";
		g_Terrains.tier4Terrain = "temperate_grass_01";
	}
	else
	{
		g_Terrains.mainTerrain = "temperate_grass_05";
		g_Terrains.forestFloor1 = "temperate_forestfloor_02_autumn";
		g_Terrains.forestFloor2 = "temperate_forestfloor_01_autumn";
		g_Terrains.tier1Terrain = "temperate_grass_dirt_01";
		g_Terrains.tier2Terrain = "temperate_grass_dirt_02";
		g_Terrains.tier3Terrain = "temperate_grass_mud_01";
		g_Terrains.tier4Terrain = "temperate_grass_02";
	}

	[g_Gaia.tree1, g_Gaia.tree2] = pickRandom([
		[
			"gaia/tree/oak",
			"gaia/tree/oak_hungarian"
		],
		[
			"gaia/tree/oak_holly",
			"gaia/tree/maple"
		],
		[
			"gaia/tree/oak_hungarian",
			"gaia/tree/oak_holly"
		]
	]);

	[g_Gaia.tree4, g_Gaia.tree5] = pickRandom([
		[
			"gaia/tree/pine",
			"gaia/tree/pine_maritime"
		],
		[
			"gaia/tree/pine",
			"gaia/tree/pine"
		],
		[
			"gaia/tree/pine_maritime",
			"gaia/tree/pine_maritime"
		]
	]);
}
