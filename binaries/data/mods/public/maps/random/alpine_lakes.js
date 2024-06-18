Engine.LoadLibrary("rmbiome");
Engine.LoadLibrary("rmgen");
Engine.LoadLibrary("rmgen-common");

{
	setBiome(g_MapSettings.Biome ?? "alpine/winter");
	const isLateSpringBiome = g_MapSettings.Biome !== "alpine/winter";

	setFogThickness(isLateSpringBiome ? 0.26 : 0.19);
	setFogFactor(isLateSpringBiome ? 0.4 : 0.35);

	setPPEffect("hdr");
	setPPSaturation(isLateSpringBiome ? 0.48 : 0.37);
	if (isLateSpringBiome)
	{
		setPPContrast(0.53);
		setPPBloom(0.12);
	}

	const pForest = [g_Terrains.forestFloor + TERRAIN_SEPARATOR + g_Gaia.tree1, g_Terrains.forestFloor];

	const heightSeaGround = -5;
	const heightLand = 3;

	globalThis.g_Map = new RandomMap(heightLand, g_Terrains.mainTerrain);

	const numPlayers = getNumPlayers();
	const mapSize = g_Map.getSize();

	const clPlayer = g_Map.createTileClass();
	const clHill = g_Map.createTileClass();
	const clForest = g_Map.createTileClass();
	const clWater = g_Map.createTileClass();
	const clDirt = g_Map.createTileClass();
	const clRock = g_Map.createTileClass();
	const clMetal = g_Map.createTileClass();
	const clFood = g_Map.createTileClass();
	const clBaseResource = g_Map.createTileClass();

	placePlayerBases({
		"PlayerPlacement": playerPlacementCircle(fractionToTiles(0.35)),
		"PlayerTileClass": clPlayer,
		"BaseResourceClass": clBaseResource,
		"CityPatch": {
			"outerTerrain": g_Terrains.roadWild,
			"innerTerrain": g_Terrains.road
		},
		"StartingAnimal": {
		},
		"Berries": {
			"template": g_Gaia.fruitBush
		},
		"Mines": {
			"types": [
				{ "template": g_Gaia.metalLarge },
				{ "template": g_Gaia.stoneLarge }
			]
		},
		"Trees": {
			"template": g_Gaia.tree1,
			"count": scaleByMapSize(3, 12)
		},
		"Decoratives": {
			"template": g_Decoratives.grassShort
		}
	});
	Engine.SetProgress(20);

	createMountains(g_Terrains.cliff,
		avoidClasses(clPlayer, 20, clHill, 8),
		clHill,
		scaleByMapSize(10, 40) * numPlayers,
		Math.floor(scaleByMapSize(40, 60)),
		Math.floor(scaleByMapSize(4, 5)),
		Math.floor(scaleByMapSize(7, 15)),
		Math.floor(scaleByMapSize(5, 15)));

	Engine.SetProgress(30);

	g_Map.log("Creating lakes");
	createAreas(
		new ChainPlacer(1, Math.floor(scaleByMapSize(4, 8)), Math.floor(scaleByMapSize(40, 180)), 0.7),
		[
			new LayeredPainter([g_Terrains.shore, g_Terrains.water], [1]),
			new SmoothElevationPainter(ELEVATION_SET, heightSeaGround, 5),
			new TileClassPainter(clWater)
		],
		avoidClasses(clPlayer, 20, clWater, 8),
		scaleByMapSize(5, 16),
		1);

	paintTerrainBasedOnHeight(3, Math.floor(scaleByMapSize(20, 40)), 0, g_Terrains.cliff);
	paintTerrainBasedOnHeight(Math.floor(scaleByMapSize(20, 40)), 100, 3, g_Terrains.snowLimited);

	createBumps(avoidClasses(clWater, 2, clPlayer, 20));

	const [forestTrees, stragglerTrees] = getTreeCounts(500, 3000, 0.7);
	createForests(
		[g_Terrains.mainTerrain, g_Terrains.forestFloor, g_Terrains.forestFloor, pForest, pForest],
		avoidClasses(clPlayer, 20, clForest, 17, clHill, 0, clWater, 2),
		clForest,
		forestTrees);

	Engine.SetProgress(60);

	g_Map.log("Creating dirt patches");
	createLayeredPatches(
		[scaleByMapSize(3, 6), scaleByMapSize(5, 10), scaleByMapSize(8, 21)],
		[[g_Terrains.dirt, g_Terrains.halfSnow], [g_Terrains.halfSnow, g_Terrains.snowLimited]],
		[2],
		avoidClasses(clWater, 3, clForest, 0, clHill, 0, clDirt, 5, clPlayer, 12),
		scaleByMapSize(15, 45),
		clDirt);

	g_Map.log("Creating grass patches");
	createPatches(
		[scaleByMapSize(2, 4), scaleByMapSize(3, 7), scaleByMapSize(5, 15)],
		g_Terrains.tier2Terrain,
		avoidClasses(clWater, 3, clForest, 0, clHill, 0, clDirt, 5, clPlayer, 12),
		scaleByMapSize(15, 45),
		clDirt);
	Engine.SetProgress(65);

	g_Map.log("Creating stone mines");
	createMines(
		[
			[
				new SimpleObject(g_Gaia.stoneSmall, 0, 2, 0, 4, 0, 2 * Math.PI, 1),
				new SimpleObject(g_Gaia.stoneSmall, 1, 1, 0, 4, 0, 2 * Math.PI, 4)
			],
			[
				new SimpleObject(g_Gaia.stoneSmall, 2, 5, 1, 3)
			]
		],
		avoidClasses(clWater, 3, clForest, 1, clPlayer, 20, clRock, 10, clHill, 1),
		clRock);

	g_Map.log("Creating metal mines");
	createMines(
		[
			[new SimpleObject(g_Gaia.metalLarge, 1, 1, 0, 4)]
		],
		avoidClasses(clWater, 3, clForest, 1, clPlayer, 20, clMetal, 10, clRock, 5, clHill, 1),
		clMetal);

	Engine.SetProgress(70);

	createDecoration(
		[
			[
				new SimpleObject(g_Decoratives.rockMedium, 1, 3, 0, 1)
			],
			[
				new SimpleObject(g_Decoratives.rockLarge, 1, 2, 0, 1),
				new SimpleObject(g_Decoratives.rockMedium, 1, 3, 0, 2)
			],
			[
				new SimpleObject(g_Decoratives.grassShort, 1, 2, 0, 1)
			],
			[
				new SimpleObject(g_Decoratives.grass, 2, 4, 0, 1.8),
				new SimpleObject(g_Decoratives.grassShort, 3, 6, 1.2, 2.5)
			],
			[
				new SimpleObject(g_Decoratives.bushMedium, 1, 2, 0, 2),
				new SimpleObject(g_Decoratives.bushSmall, 2, 4, 0, 2)
			]
		],
		[
			scaleByMapAreaAbsolute(16),
			scaleByMapAreaAbsolute(8),
			scaleByMapAreaAbsolute(13),
			scaleByMapAreaAbsolute(13),
			scaleByMapAreaAbsolute(13)
		],
		avoidClasses(clWater, 0, clForest, 0, clPlayer, 0, clHill, 0));

	Engine.SetProgress(75);

	createFood(
		[
			[new SimpleObject(g_Gaia.mainHuntableAnimal, 5, 7, 0, 4)],
			[new SimpleObject(g_Gaia.secondaryHuntableAnimal, 2, 3, 0, 2)]
		],
		[
			3 * numPlayers,
			3 * numPlayers
		],
		avoidClasses(clWater, 3, clForest, 0, clPlayer, 20, clHill, 1, clFood, 20),
		clFood);

	createFood(
		[
			[new SimpleObject(g_Gaia.fruitBush, 5, 7, 0, 4)]
		],
		[
			randIntInclusive(1, 4) * numPlayers + 2
		],
		avoidClasses(clWater, 3, clForest, 0, clPlayer, 20, clHill, 1, clFood, 10),
		clFood);

	createFood(
		[
			[new SimpleObject(g_Gaia.fish, 2, 3, 0, 2)]
		],
		[
			15 * numPlayers
		],
		[avoidClasses(clFood, 20), stayClasses(clWater, 6)],
		clFood);

	Engine.SetProgress(85);

	createStragglerTrees(
		[g_Gaia.tree1],
		avoidClasses(clWater, 5, clForest, 3, clHill, 1, clPlayer, 12, clMetal, 6, clRock, 6),
		clForest,
		stragglerTrees);

	placePlayersNomad(clPlayer,
		avoidClasses(
			clWater, 4,
			clForest, 1,
			clMetal, 4,
			clRock, 4,
			clHill, 4,
			clFood, 2));

	setSkySet(pickRandom(["cirrus", "cumulus", "sunny"]));
	setSunRotation(randomAngle());
	setSunElevation(Math.PI * randFloat(1/5, 1/3));
	// dark majestic blue
	setWaterColor(0.0, 0.047, 0.286);
	// light blue
	setWaterTint(0.471, 0.776, 0.863);
	setWaterMurkiness(0.82);
	setWaterWaviness(3.0);
	setWaterType("clap");

	g_Map.ExportMap();
}
