RMS.LoadLibrary("rmgen");
RMS.LoadLibrary("rmgen2");

InitMap();

setSelectedBiome();
initMapSettings();
initTileClasses(["step"]);

var topTerrain = g_Terrains.tier2Terrain;

resetTerrain(topTerrain, g_TileClasses.land, 50);
RMS.SetProgress(10);

var players = addBases("radial", 0.4, randFloat(0.05, 0.1));
RMS.SetProgress(20);

createSunkenTerrain(players);
RMS.SetProgress(30);

addElements([
	{
		"func": addLayeredPatches,
		"avoid": [
			g_TileClasses.dirt, 5,
			g_TileClasses.forest, 2,
			g_TileClasses.mountain, 2,
			g_TileClasses.player, 12,
			g_TileClasses.step, 5
		],
		"stay": [g_TileClasses.valley, 7],
		"sizes": ["normal"],
		"mixes": ["normal"],
		"amounts": ["normal"]
	},
	{
		"func": addLayeredPatches,
		"avoid": [
			g_TileClasses.dirt, 5,
			g_TileClasses.forest, 2,
			g_TileClasses.mountain, 2,
			g_TileClasses.player, 12
		],
		"stay": [g_TileClasses.settlement, 7],
		"sizes": ["normal"],
		"mixes": ["normal"],
		"amounts": ["normal"]
	},
	{
		"func": addLayeredPatches,
		"avoid": [
			g_TileClasses.dirt, 5,
			g_TileClasses.forest, 2
		],
		"stay": [g_TileClasses.player, 1],
		"sizes": ["normal"],
		"mixes": ["normal"],
		"amounts": ["normal"]
	},
	{
		"func": addDecoration,
		"avoid": [g_TileClasses.forest, 2],
		"stay": [g_TileClasses.player, 1],
		"sizes": ["normal"],
		"mixes": ["normal"],
		"amounts": ["normal"]
	},
	{
		"func": addDecoration,
		"avoid": [
			g_TileClasses.forest, 2,
			g_TileClasses.mountain, 2,
			g_TileClasses.player, 12,
			g_TileClasses.step, 2
		 ],
		"stay": [g_TileClasses.valley, 7],
		"sizes": ["normal"],
		"mixes": ["normal"],
		"amounts": ["normal"]
	},
	{
		"func": addDecoration,
		"avoid": [
			g_TileClasses.forest, 2,
			g_TileClasses.mountain, 2,
			g_TileClasses.player, 12
		],
		"stay": [g_TileClasses.settlement, 7],
		"sizes": ["normal"],
		"mixes": ["normal"],
		"amounts": ["normal"]
	},
	{
		"func": addDecoration,
		"avoid": [
			g_TileClasses.forest, 2,
			g_TileClasses.mountain, 2,
			g_TileClasses.player, 12
		],
		"stay": [g_TileClasses.step, 7],
		"sizes": ["normal"],
		"mixes": ["normal"],
		"amounts": ["scarce"]
	}
]);
RMS.SetProgress(40);

addElements(shuffleArray([
	{
		"func": addMetal,
		"avoid": [
			g_TileClasses.berries, 5,
			g_TileClasses.forest, 3,
			g_TileClasses.player, 30,
			g_TileClasses.rock, 10,
			g_TileClasses.metal, 20
		],
		"stay": [g_TileClasses.settlement, 7],
		"sizes": ["normal"],
		"mixes": ["same"],
		"amounts": ["tons"]
	},
	{
		"func": addMetal,
		"avoid": [
			g_TileClasses.berries, 5,
			g_TileClasses.forest, 3,
			g_TileClasses.player, 10,
			g_TileClasses.rock, 10,
			g_TileClasses.metal, 20,
			g_TileClasses.mountain, 5,
			g_TileClasses.step, 5
		],
		"stay": [g_TileClasses.valley, 7],
		"sizes": ["normal"],
		"mixes": ["same"],
		"amounts": g_AllAmounts
	},
	{
		"func": addStone,
		"avoid": [
			g_TileClasses.berries, 5,
			g_TileClasses.forest, 3,
			g_TileClasses.player, 30,
			g_TileClasses.rock, 20,
			g_TileClasses.metal, 10
		],
		"stay": [g_TileClasses.settlement, 7],
		"sizes": ["normal"],
		"mixes": ["same"],
		"amounts": ["tons"]
	},
	{
		"func": addStone,
		"avoid": [
			g_TileClasses.berries, 5,
			g_TileClasses.forest, 3,
			g_TileClasses.player, 10,
			g_TileClasses.rock, 20,
			g_TileClasses.metal, 10,
			g_TileClasses.mountain, 5,
			g_TileClasses.step, 5
		],
		"stay": [g_TileClasses.valley, 7],
		"sizes": ["normal"],
		"mixes": ["same"],
		"amounts": g_AllAmounts
	},
	{
		"func": addForests,
		"avoid": [
			g_TileClasses.berries, 5,
			g_TileClasses.forest, 18,
			g_TileClasses.metal, 3,
			g_TileClasses.player, 20,
			g_TileClasses.rock, 3
		],
		"stay": [g_TileClasses.settlement, 7],
		"sizes": ["normal", "big"],
		"mixes": ["same"],
		"amounts": ["tons"]
	},
	{
		"func": addForests,
		"avoid": [
			g_TileClasses.berries, 3,
			g_TileClasses.forest, 18,
			g_TileClasses.metal, 3,
			g_TileClasses.mountain, 5,
			g_TileClasses.player, 5,
			g_TileClasses.rock, 3,
			g_TileClasses.step, 1
		],
		"stay": [g_TileClasses.valley, 7],
		"sizes": ["normal", "big"],
		"mixes": ["same"],
		"amounts": ["tons"]
	}
]));
RMS.SetProgress(60);

addElements(shuffleArray([
	{
		"func": addBerries,
		"avoid": [
			g_TileClasses.berries, 30,
			g_TileClasses.forest, 5,
			g_TileClasses.metal, 10,
			g_TileClasses.player, 20,
			g_TileClasses.rock, 10
		],
		"stay": [g_TileClasses.settlement, 7],
		"sizes": g_AllSizes,
		"mixes": g_AllMixes,
		"amounts": ["tons"]
	},
	{
		"func": addBerries,
		"avoid": [
			g_TileClasses.berries, 30,
			g_TileClasses.forest, 5,
			g_TileClasses.metal, 10,
			g_TileClasses.mountain, 5,
			g_TileClasses.player, 10,
			g_TileClasses.rock, 10,
			g_TileClasses.step, 5
		],
		"stay": [g_TileClasses.valley, 7],
		"sizes": g_AllSizes,
		"mixes": g_AllMixes,
		"amounts": g_AllAmounts
	},
	{
		"func": addAnimals,
		"avoid": [
			g_TileClasses.animals, 20,
			g_TileClasses.forest, 0,
			g_TileClasses.metal, 1,
			g_TileClasses.player, 20,
			g_TileClasses.rock, 1
		],
		"stay": [g_TileClasses.settlement, 7],
		"sizes": g_AllSizes,
		"mixes": g_AllMixes,
		"amounts": ["tons"]
	},
	{
		"func": addAnimals,
		"avoid": [
			g_TileClasses.animals, 20,
			g_TileClasses.forest, 0,
			g_TileClasses.metal, 1,
			g_TileClasses.mountain, 5,
			g_TileClasses.player, 10,
			g_TileClasses.rock, 1,
			g_TileClasses.step, 5
		],
		"stay": [g_TileClasses.valley, 7],
		"sizes": g_AllSizes,
		"mixes": g_AllMixes,
		"amounts": g_AllAmounts
	},
	{
		"func": addStragglerTrees,
		"avoid": [
			g_TileClasses.berries, 5,
			g_TileClasses.forest, 7,
			g_TileClasses.metal, 3,
			g_TileClasses.player, 12,
			g_TileClasses.rock, 3
		],
		"stay": [g_TileClasses.settlement, 7],
		"sizes": g_AllSizes,
		"mixes": g_AllMixes,
		"amounts": ["tons"]
	},
	{
		"func": addStragglerTrees,
		"avoid": [
			g_TileClasses.berries, 5,
			g_TileClasses.forest, 7,
			g_TileClasses.metal, 3,
			g_TileClasses.mountain, 5,
			g_TileClasses.player, 10,
			g_TileClasses.rock, 3,
			g_TileClasses.step, 5
		],
		"stay": [g_TileClasses.valley, 7],
		"sizes": g_AllSizes,
		"mixes": g_AllMixes,
		"amounts": ["normal", "many", "tons"]
	},
	{
		"func": addStragglerTrees,
		"avoid": [
			g_TileClasses.berries, 5,
			g_TileClasses.forest, 3,
			g_TileClasses.metal, 5,
			g_TileClasses.rock, 5
		],
		"stay": [g_TileClasses.player, 1],
		"sizes": ["huge"],
		"mixes": ["same"],
		"amounts": ["tons"]
	}
]));
RMS.SetProgress(75);

addElements([
	{
		"func": addDecoration,
		"avoid": [
			g_TileClasses.valley, 4,
			g_TileClasses.player, 4,
			g_TileClasses.settlement, 4,
			g_TileClasses.step, 4
		],
		"stay": [g_TileClasses.land, 2],
		"sizes": ["normal"],
		"mixes": ["normal"],
		"amounts": ["tons"]
	}
]);
RMS.SetProgress(80);

addElements([
	{
		"func": addProps,
		"avoid": [
			g_TileClasses.valley, 4,
			g_TileClasses.player, 4,
			g_TileClasses.settlement, 4,
			g_TileClasses.step, 4
		],
		"stay": [g_TileClasses.land, 2],
		"sizes": ["normal"],
		"mixes": ["normal"],
		"amounts": ["scarce"]
	}
]);
RMS.SetProgress(85);

addElements([
	{
		"func": addDecoration,
		"avoid": [
			g_TileClasses.player, 4,
			g_TileClasses.settlement, 4,
			g_TileClasses.step, 4
		],
		"stay": [g_TileClasses.mountain, 2],
		"sizes": ["normal"],
		"mixes": ["normal"],
		"amounts": ["tons"]
	}
]);
RMS.SetProgress(90);

addElements([
	{
		"func": addProps,
		"avoid": [
			g_TileClasses.player, 4,
			g_TileClasses.settlement, 4,
			g_TileClasses.step, 4
		],
		"stay": [g_TileClasses.mountain, 2],
		"sizes": ["normal"],
		"mixes": ["normal"],
		"amounts": ["scarce"]
	}
]);
RMS.SetProgress(95);

ExportMap();

// Create the sunken terrain
function createSunkenTerrain(players)
{
	var base = g_Terrains.mainTerrain;
	var middle = g_Terrains.dirt;
	var lower = g_Terrains.tier2Terrain;
	var road = g_Terrains.road;

	if (currentBiome() == g_BiomeSnowy)
	{
		middle = g_Terrains.tier2Terrain;
		lower = g_Terrains.tier1Terrain;
	}

	if (currentBiome() == g_BiomeAlpine)
	{
		middle = g_Terrains.shore;
		lower = g_Terrains.tier4Terrain;
	}

	if (currentBiome() == g_BiomeMediterranean)
	{
		middle = g_Terrains.tier1Terrain;
		lower = g_Terrains.forestFloor1;
	}

	if (currentBiome() == g_BiomeSavanna)
	{
		middle = g_Terrains.tier2Terrain;
		lower = g_Terrains.tier4Terrain;
	}

	if (currentBiome() == g_BiomeTropic || currentBiome() == g_BiomeAutumn)
		road = g_Terrains.roadWild;

	if (currentBiome() == g_BiomeAutumn)
		middle = g_Terrains.shore;

	var expSize = g_MapInfo.mapArea * 0.015 / (g_MapInfo.numPlayers / 4);
	var expDist = 0.1 + (g_MapInfo.numPlayers / 200);
	var expAngle = 0.75;
	if (g_MapInfo.numPlayers == 2)
	{
		expSize = g_MapInfo.mapArea * 0.015 / 0.8;
		expAngle = 0.72;
	}

	var nRoad = 0.44;
	var nExp = 0.425;

	if (g_MapInfo.numPlayers < 4)
	{
		nRoad = 0.42;
		nExp = 0.4;
	}

	// Create central valley
	var placer = new ClumpPlacer(g_MapInfo.mapArea * 0.26, 1, 1, 1, g_MapInfo.centerOfMap, g_MapInfo.centerOfMap);
	var terrainPainter = new LayeredPainter([g_Terrains.cliff, lower], [3]);
	var elevationPainter = new SmoothElevationPainter(ELEVATION_SET, 0, 3);
	createArea(placer, [terrainPainter, elevationPainter, paintClass(g_TileClasses.valley)]);

	// Create the center hill
	var placer = new ClumpPlacer(g_MapInfo.mapArea * 0.14, 1, 1, 1, g_MapInfo.centerOfMap, g_MapInfo.centerOfMap);
	var terrainPainter = new LayeredPainter([g_Terrains.cliff, topTerrain], [3]);
	var elevationPainter = new SmoothElevationPainter(ELEVATION_SET, g_MapInfo.mapHeight, 3);
	createArea(placer, [terrainPainter, elevationPainter, paintClass(g_TileClasses.mountain)]);

	for(var i = 0; i < players.length; ++i)
	{
		var playerAngle = g_MapInfo.startAngle + i * TWO_PI / g_MapInfo.numPlayers;
		var pX = round(fractionToTiles(0.5 + 0.4 * cos(playerAngle)));
		var pZ = round(fractionToTiles(0.5 + 0.4 * sin(playerAngle)));
		var expX = round(fractionToTiles(0.5 + expDist * cos(g_MapInfo.startAngle + (i + expAngle) * TWO_PI / g_MapInfo.numPlayers)));
		var expZ = round(fractionToTiles(0.5 + expDist * sin(g_MapInfo.startAngle + (i + expAngle) * TWO_PI / g_MapInfo.numPlayers)));
		var rearX = round(fractionToTiles(0.5 + 0.47 * cos(playerAngle)));
		var rearZ = round(fractionToTiles(0.5 + 0.47 * sin(playerAngle)));
		var prePlayerAngle = g_MapInfo.startAngle + (i - 0.5) * TWO_PI / g_MapInfo.numPlayers;
		var preX = round(fractionToTiles(0.5 + nRoad * cos(prePlayerAngle)));
		var preZ = round(fractionToTiles(0.5 + nRoad * sin(prePlayerAngle)));
		var nextPlayerAngle = g_MapInfo.startAngle + (i + 0.5) * TWO_PI / g_MapInfo.numPlayers;
		var nextX = round(fractionToTiles(0.5 + nRoad * cos(nextPlayerAngle)));
		var nextZ = round(fractionToTiles(0.5 + nRoad * sin(nextPlayerAngle)));

		// Create path to expansion
		var placer = new PathPlacer(pX, pZ, expX, expZ, scaleByMapSize(12, 12), 0.7, 0.5, 0.1, -1);
		var terrainPainter = new LayeredPainter([g_Terrains.cliff, middle, road], [3, 4]);
		var elevationPainter = new SmoothElevationPainter(ELEVATION_SET, 10, 3);
		createArea(placer, [terrainPainter, elevationPainter, paintClass(g_TileClasses.step)]);

		// Create path to neighbor
		var placer = new PathPlacer(rearX, rearZ, nextX, nextZ, scaleByMapSize(19, 19), 0.4, 0.5, 0.1, -0.6);
		var terrainPainter = new LayeredPainter([g_Terrains.cliff, middle, road], [3, 6]);
		var elevationPainter = new SmoothElevationPainter(ELEVATION_SET, 10, 3);
		createArea(placer, [terrainPainter, elevationPainter, paintClass(g_TileClasses.step)]);

		// Create path to neighbor
		var placer = new PathPlacer(rearX, rearZ, preX, preZ, scaleByMapSize(19, 19), 0.4, 0.5, 0.1, -0.6);
		var terrainPainter = new LayeredPainter([g_Terrains.cliff, middle, road], [3, 6]);
		var elevationPainter = new SmoothElevationPainter(ELEVATION_SET, 10, 3);
		createArea(placer, [terrainPainter, elevationPainter, paintClass(g_TileClasses.step)]);

		// Create the den
		var placer = new ClumpPlacer(g_MapInfo.mapArea * 0.03, 0.9, 0.3, 1, pX, pZ);
		var terrainPainter = new LayeredPainter([g_Terrains.cliff, base], [3]);
		var elevationPainter = new SmoothElevationPainter(ELEVATION_SET, 15, 3);
		createArea(placer, [terrainPainter, elevationPainter, paintClass(g_TileClasses.valley)]);

		// Create the expansion
		var placer = new ClumpPlacer(expSize, 0.9, 0.3, 1, expX, expZ);
		var terrainPainter = new LayeredPainter([g_Terrains.cliff, base], [3]);
		var elevationPainter = new SmoothElevationPainter(ELEVATION_SET, 15, 3);
		var area = createArea(placer, [terrainPainter, elevationPainter, paintClass(g_TileClasses.settlement)], [avoidClasses(g_TileClasses.settlement, 2)]);
		var unpainter = new TileClassUnPainter(new TileClass(g_MapInfo.mapSize, g_TileClasses.mountain));
		unpainter.paint(area);
	}

	// Create the neighbor expansions
	for (var i = 0; i < g_MapInfo.numPlayers; ++i)
	{
		var nextPlayerAngle = g_MapInfo.startAngle + (i + 0.5) * TWO_PI / g_MapInfo.numPlayers;
		var nextX = round(fractionToTiles(0.5 + nExp * cos(nextPlayerAngle)));
		var nextZ = round(fractionToTiles(0.5 + nExp * sin(nextPlayerAngle)));

		// Create the neightbor expansion
		var placer = new ClumpPlacer(expSize, 0.9, 0.3, 1, nextX, nextZ);
		var terrainPainter = new LayeredPainter([g_Terrains.cliff, lower], [3]);
		var elevationPainter = new SmoothElevationPainter(ELEVATION_SET, 0, 3);
		var area = createArea(placer, [terrainPainter, elevationPainter, paintClass(g_TileClasses.settlement)]);
	}
}
