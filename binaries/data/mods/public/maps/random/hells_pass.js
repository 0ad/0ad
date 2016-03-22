RMS.LoadLibrary("rmgen");
InitMap();

randomizeBiome();
initMapSettings();
initTileClasses();

resetTerrain(g_Terrains.mainTerrain, g_TileClasses.land, 1);
RMS.SetProgress(10);

addBases("line", 0.2, 0.08);
RMS.SetProgress(20);

placeBarriers();
RMS.SetProgress(40);

addElements(shuffleArray([
	{
		"func": addBluffs,
		"avoid": [
			g_TileClasses.bluff, 20,
			g_TileClasses.hill, 5,
			g_TileClasses.mountain, 20,
			g_TileClasses.plateau, 20,
			g_TileClasses.player, 30,
			g_TileClasses.spine, 15,
			g_TileClasses.valley, 5,
			g_TileClasses.water, 7
		],
		"sizes": ["normal", "big"],
		"mixes": ["varied"],
		"amounts": ["few"]
	},
	{
		"func": addHills,
		"avoid": [
			g_TileClasses.bluff, 5,
			g_TileClasses.hill, 15,
			g_TileClasses.mountain, 2,
			g_TileClasses.plateau, 5,
			g_TileClasses.player, 20,
			g_TileClasses.spine, 15,
			g_TileClasses.valley, 2,
			g_TileClasses.water, 2
		],
		"sizes": ["normal", "big"],
		"mixes": ["varied"],
		"amounts": ["few"]
	},
	{
		"func": addLakes,
		"avoid": [
			g_TileClasses.bluff, 7,
			g_TileClasses.hill, 2,
			g_TileClasses.mountain, 15,
			g_TileClasses.plateau, 10,
			g_TileClasses.player, 20,
			g_TileClasses.spine, 15,
			g_TileClasses.valley, 10,
			g_TileClasses.water, 25
		],
		"sizes": ["big", "huge"],
		"mixes": ["varied", "unique"],
		"amounts": ["few"]
	}
]));
RMS.SetProgress(50);

addElements([
	{
		"func": addLayeredPatches,
		"avoid": [g_TileClasses.bluff, 2,
			g_TileClasses.dirt, 5,
			g_TileClasses.forest, 2,
			g_TileClasses.mountain, 2,
			g_TileClasses.plateau, 2,
			g_TileClasses.player, 12,
			g_TileClasses.spine, 5,
			g_TileClasses.water, 3
			],
		"sizes": ["normal"],
		"mixes": ["normal"],
		"amounts": ["normal"]
	},
	{
		"func": addDecoration,
		"avoid": [g_TileClasses.bluff, 2,
			g_TileClasses.forest, 2,
			g_TileClasses.mountain, 2,
			g_TileClasses.plateau, 2,
			g_TileClasses.player, 12,
			g_TileClasses.spine, 5,
			g_TileClasses.water, 3
			],
		"sizes": ["normal"],
		"mixes": ["normal"],
		"amounts": ["normal"]
	}
]);
RMS.SetProgress(60);

addElements(shuffleArray([
	{
		"func": addMetal,
		"avoid": [g_TileClasses.berries, 5,
			g_TileClasses.bluff, 5,
			g_TileClasses.forest, 3,
			g_TileClasses.mountain, 2,
			g_TileClasses.plateau, 2,
			g_TileClasses.player, 30,
			g_TileClasses.rock, 10,
			g_TileClasses.metal, 20,
			g_TileClasses.spine, 5,
			g_TileClasses.water, 3
			],
		"sizes": ["normal"],
		"mixes": ["same"],
		"amounts": g_AllAmounts
	},
	{
		"func": addStone,
		"avoid": [
			g_TileClasses.berries, 5,
			g_TileClasses.bluff, 5,
			g_TileClasses.forest, 3,
			g_TileClasses.mountain, 2,
			g_TileClasses.plateau, 2,
			g_TileClasses.player, 30,
			g_TileClasses.rock, 20,
			g_TileClasses.metal, 10,
			g_TileClasses.spine, 5,
			g_TileClasses.water, 3
			],
		"sizes": ["normal"],
		"mixes": ["same"],
		"amounts": g_AllAmounts
	},
	{
		"func": addForests,
		"avoid": [g_TileClasses.berries, 5,
			g_TileClasses.bluff, 5,
			g_TileClasses.forest, 18,
			g_TileClasses.metal, 3,
			g_TileClasses.mountain, 5,
			g_TileClasses.plateau, 5,
			g_TileClasses.player, 20,
			g_TileClasses.rock, 3,
			g_TileClasses.spine, 5,
			g_TileClasses.water, 2
			],
		"sizes": g_AllSizes,
		"mixes": g_AllMixes,
		"amounts": ["few", "normal", "many", "tons"]
	}
]));
RMS.SetProgress(80);

addElements(shuffleArray([
	{
		"func": addBerries,
		"avoid": [
			g_TileClasses.berries, 30,
			g_TileClasses.bluff, 5,
			g_TileClasses.forest, 5,
			g_TileClasses.metal, 10,
			g_TileClasses.mountain, 2,
			g_TileClasses.plateau, 2,
			g_TileClasses.player, 20,
			g_TileClasses.rock, 10,
			g_TileClasses.spine, 2,
			g_TileClasses.water, 3
		],
		"sizes": g_AllSizes,
		"mixes": g_AllMixes,
		"amounts": g_AllAmounts
	},
	{
		"func": addAnimals,
		"avoid": [
			g_TileClasses.animals, 20,
			g_TileClasses.bluff, 5,
			g_TileClasses.forest, 2,
			g_TileClasses.metal, 2,
			g_TileClasses.mountain, 1,
			g_TileClasses.plateau, 2,
			g_TileClasses.player, 20,
			g_TileClasses.rock, 2,
			g_TileClasses.spine, 2,
			g_TileClasses.water, 3
		],
		"sizes": g_AllSizes,
		"mixes": g_AllMixes,
		"amounts": g_AllAmounts
	},
	{
		"func": addStragglerTrees,
		"avoid": [
			g_TileClasses.berries, 5,
			g_TileClasses.bluff, 5,
			g_TileClasses.forest, 7,
			g_TileClasses.metal, 2,
			g_TileClasses.mountain, 1,
			g_TileClasses.plateau, 2,
			g_TileClasses.player, 12,
			g_TileClasses.rock, 2,
			g_TileClasses.spine, 2,
			g_TileClasses.water, 5
		],
		"sizes": g_AllSizes,
		"mixes": g_AllMixes,
		"amounts": g_AllAmounts
	}
]));
RMS.SetProgress(90);

ExportMap();

// place the mountainous barriers between the teams
function placeBarriers()
{
	var spineTerrain = g_Terrains.dirt;

	if (g_MapInfo.biome == 2)
		spineTerrain = g_Terrains.tier1Terrain;

	if (g_MapInfo.biome == 4 || g_MapInfo.biome == 6)
		spineTerrain = g_Terrains.tier2Terrain;

	if (g_MapInfo.biome == 8)
		spineTerrain = g_Terrains.tier4Terrain;

	// create mountain ranges
	for (var i = 0; i < g_MapInfo.teams.length; ++i)
	{
		var tang = g_MapInfo.startAngle + (i + 0.5) * TWO_PI / g_MapInfo.teams.length;

		var fx = fractionToTiles(0.5);
		var fz = fractionToTiles(0.5);
		var ix = round(fx);
		var iz = round(fz);

		var mStartCo = 0.07;
		var mStopCo = 0.42;
		var mSize = 8;
		var mWaviness = 0.6;
		var mOffset = 0.5;
		var mTaper = -1.5;

		if (g_MapInfo.teams.length > 3 || g_MapInfo.mapSize <= 192)
		{
			mWaviness = 0.2;
			mOffset = 0.2;
			mTaper = -1;
		}

		if (g_MapInfo.teams.length >= 5)
		{
			mSize = 4;
			mWaviness = 0.2;
			mOffset = 0.2;
			mTaper = -0.7;
		}

		// place barrier
		var placer = new PathPlacer(fractionToTiles(0.5 + mStartCo * cos(tang)), fractionToTiles(0.5 + mStartCo * sin(tang)), fractionToTiles(0.5 + mStopCo * cos(tang)), fractionToTiles(0.5 + mStopCo * sin(tang)), scaleByMapSize(14, mSize), mWaviness, 0.1, mOffset, mTaper);
		var terrainPainter = new LayeredPainter([g_Terrains.cliff, spineTerrain], [2]);
		var elevationPainter = new SmoothElevationPainter(ELEVATION_SET, 30, 2);
		createArea(placer, [terrainPainter, elevationPainter, paintClass(g_TileClasses.spine)], avoidClasses(g_TileClasses.player, 5, g_TileClasses.baseResource, 5));
	}

	addElements([
		{
			"func": addDecoration,
			"avoid": [
				g_TileClasses.bluff, 2,
				g_TileClasses.forest, 2,
				g_TileClasses.mountain, 2,
				g_TileClasses.player, 12,
				g_TileClasses.water, 3
			],
			"stay": [g_TileClasses.spine, 5],
			"sizes": ["huge"],
			"mixes": ["unique"],
			"amounts": ["tons"]
		}
	]);

	addElements([
		{
			"func": addProps,
			"avoid": [
				g_TileClasses.forest, 2,
				g_TileClasses.player, 2,
				g_TileClasses.prop, 20,
				g_TileClasses.water, 3
			],
			"stay": [g_TileClasses.spine, 8],
			"sizes": ["normal"],
			"mixes": ["normal"],
			"amounts": ["scarce"]
		}
	]);
}
