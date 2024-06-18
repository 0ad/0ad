Engine.LoadLibrary("rmgen");
Engine.LoadLibrary("rmgen-common");
Engine.LoadLibrary("rmbiome");

{
	setSelectedBiome();

	const tPrimary = g_Terrains.mainTerrain;
	const tGrass = [g_Terrains.tier1Terrain, g_Terrains.tier2Terrain];
	const tGrassPForest = g_Terrains.forestFloor1;
	const tGrassDForest = g_Terrains.forestFloor2;
	const tGrassA = g_Terrains.tier2Terrain;
	const tGrassB = g_Terrains.tier3Terrain;
	const tGrassC = g_Terrains.tier4Terrain;
	const tHill = g_Terrains.hill;
	const tCliff = g_Terrains.cliff;
	const tRoad = g_Terrains.road;
	const tRoadWild = g_Terrains.roadWild;
	const tGrassPatchBlend = g_Terrains.tier2Terrain;
	const tGrassPatch = g_Terrains.tier1Terrain;
	const tShoreBlend = g_Terrains.shoreBlend;
	const tShore = g_Terrains.shore;
	const tWater = g_Terrains.water;

	const oOak = g_Gaia.tree1;
	const oOakLarge = g_Gaia.tree2;
	const oApple = g_Gaia.tree3;
	const oPine = g_Gaia.tree4;
	const oAleppoPine = g_Gaia.tree5;
	const oBerryBush = g_Gaia.fruitBush;
	const oDeer = g_Gaia.mainHuntableAnimal;
	const oFish = g_Gaia.fish;
	const oSheep = g_Gaia.secondaryHuntableAnimal;
	const oStoneLarge = g_Gaia.stoneLarge;
	const oStoneSmall = g_Gaia.stoneSmall;
	const oMetalLarge = g_Gaia.metalLarge;
	const oMetalSmall = g_Gaia.metalSmall;

	const aGrass = g_Decoratives.grass;
	const aGrassShort = g_Decoratives.grassShort;
	const aReeds = g_Decoratives.reeds;
	const aLillies = g_Decoratives.lillies;
	const aRockLarge = g_Decoratives.rockLarge;
	const aRockMedium = g_Decoratives.rockMedium;
	const aBushMedium = g_Decoratives.bushMedium;
	const aBushSmall = g_Decoratives.bushSmall;

	const pForestD = [
		tGrassDForest + TERRAIN_SEPARATOR + oOak,
		tGrassDForest + TERRAIN_SEPARATOR + oOakLarge,
		tGrassDForest
	];
	const pForestP = [
		tGrassPForest + TERRAIN_SEPARATOR + oPine,
		tGrassPForest + TERRAIN_SEPARATOR + oAleppoPine,
		tGrassPForest
	];

	const heightSeaGround = -7;
	const heightLand = 3;
	const heightHill = 20;

	globalThis.g_Map = new RandomMap(heightLand, tPrimary);

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

	const playerHillRadius = defaultPlayerBaseRadius() / (isNomad() ? 1.5 : 1);

	const [playerIDs, playerPosition, playerAngle] = playerPlacementCircle(fractionToTiles(0.35));

	g_Map.log("Creating player hills and ramps");
	for (let i = 0; i < numPlayers; ++i)
	{
		createArea(
			new ClumpPlacer(diskArea(playerHillRadius), 0.95, 0.6, Infinity, playerPosition[i]),
			[
				new LayeredPainter([tCliff, tHill], [2]),
				new SmoothElevationPainter(ELEVATION_SET, heightHill, 2),
				new TileClassPainter(clPlayer)
			]);

		const angle = playerAngle[i] + Math.PI * (1 + randFloat(-1, 1) / 8);
		createPassage({
			"start": Vector2D.add(playerPosition[i], new Vector2D(playerHillRadius + 15, 0)
				.rotate(-angle)),
			"end": Vector2D.add(playerPosition[i], new Vector2D(playerHillRadius - 3, 0)
				.rotate(-angle)),
			"startWidth": 10,
			"endWidth": 10,
			"smoothWidth": 2,
			"tileClass": clPlayer,
			"terrain": tHill,
			"edgeTerrain": tCliff
		});
	}

	placePlayerBases({
		"PlayerPlacement": [playerIDs, playerPosition],
		"PlayerTileClass": clPlayer,
		"BaseResourceClass": clBaseResource,
		"Walls": false,
		"CityPatch": {
			"outerTerrain": tRoadWild,
			"innerTerrain": tRoad
		},
		"StartingAnimal": {
		},
		"Berries": {
			"template": oBerryBush
		},
		"Mines": {
			"types": [
				{ "template": oMetalLarge },
				{ "template": oStoneLarge }
			]
		},
		"Trees": {
			"template": oOak,
			"count": 2
		},
		"Decoratives": {
			"template": aGrassShort
		}
	});
	Engine.SetProgress(10);

	g_Map.log("Creating lakes");
	const numLakes = Math.round(scaleByMapSize(1, 4) * numPlayers);
	const waterAreas = createAreas(
		new ClumpPlacer(scaleByMapSize(100, 250), 0.8, 0.1, Infinity),
		[
			new LayeredPainter([tShoreBlend, tShore, tWater], [1, 1]),
			new SmoothElevationPainter(ELEVATION_SET, heightSeaGround, 6),
			new TileClassPainter(clWater)
		],
		avoidClasses(clPlayer, 2, clWater, 20),
		numLakes
	);
	Engine.SetProgress(15);

	g_Map.log("Creating reeds");
	const group = new SimpleGroup(
		[new SimpleObject(aReeds, 5, 10, 0, 4), new SimpleObject(aLillies, 0, 1, 0, 4)], true
	);
	createObjectGroupsByAreas(group, 0,
		[borderClasses(clWater, 3, 0), stayClasses(clWater, 1)],
		numLakes, 100,
		waterAreas
	);
	Engine.SetProgress(20);

	g_Map.log("Creating fish");
	createObjectGroupsByAreas(
		new SimpleGroup(
			[new SimpleObject(oFish, 1, 1, 0, 1)],
			true, clFood
		),
		0,
		[stayClasses(clWater, 4), avoidClasses(clFood, 8)],
		numLakes / 4,
		50,
		waterAreas
	);
	Engine.SetProgress(25);

	createBumps(avoidClasses(clWater, 2, clPlayer, 0));
	Engine.SetProgress(30);

	createHills([tCliff, tCliff, tHill], avoidClasses(clPlayer, 2, clWater, 5, clHill, 15), clHill,
		scaleByMapSize(1, 4) * numPlayers);
	Engine.SetProgress(35);

	const [forestTrees, stragglerTrees] = getTreeCounts(...rBiomeTreeCount(1));
	createDefaultForests(
		[tGrass, tGrassDForest, tGrassPForest, pForestP, pForestD],
		avoidClasses(clPlayer, 1, clWater, 3, clForest, 17, clHill, 1),
		clForest,
		forestTrees);
	Engine.SetProgress(40);

	g_Map.log("Creating dirt patches");
	createLayeredPatches(
		[scaleByMapSize(3, 6), scaleByMapSize(5, 10), scaleByMapSize(8, 21)],
		[[tGrass, tGrassA], [tGrassA, tGrassB], [tGrassB, tGrassC]],
		[1, 1],
		avoidClasses(clWater, 1, clForest, 0, clHill, 0, clDirt, 5, clPlayer, 0),
		scaleByMapSize(15, 45),
		clDirt);
	Engine.SetProgress(45);

	g_Map.log("Creating grass patches");
	createLayeredPatches(
		[scaleByMapSize(2, 4), scaleByMapSize(3, 7), scaleByMapSize(5, 15)],
		[tGrassPatchBlend, tGrassPatch],
		[1],
		avoidClasses(clWater, 1, clForest, 0, clHill, 0, clDirt, 5, clPlayer, 0),
		scaleByMapSize(15, 45),
		clDirt);
	Engine.SetProgress(50);

	g_Map.log("Creating metal mines");
	createBalancedMetalMines(
		oMetalSmall,
		oMetalLarge,
		clMetal,
		avoidClasses(clWater, 0, clForest, 0, clPlayer, scaleByMapSize(15, 25), clHill, 1)
	);

	g_Map.log("Creating stone mines");
	createBalancedStoneMines(
		oStoneSmall,
		oStoneLarge,
		clRock,
		avoidClasses(clWater, 0, clForest, 0, clPlayer, scaleByMapSize(15, 25), clHill, 1, clMetal, 10)
	);

	Engine.SetProgress(60);

	createDecoration(
		[
			[new SimpleObject(aRockMedium, 1, 3, 0, 1)],
			[new SimpleObject(aRockLarge, 1, 2, 0, 1), new SimpleObject(aRockMedium, 1, 3, 0, 2)],
			[new SimpleObject(aGrassShort, 1, 2, 0, 1)],
			[new SimpleObject(aGrass, 2, 4, 0, 1.8), new SimpleObject(aGrassShort, 3, 6, 1.2, 2.5)],
			[new SimpleObject(aBushMedium, 1, 2, 0, 2), new SimpleObject(aBushSmall, 2, 4, 0, 2)]
		],
		[
			scaleByMapAreaAbsolute(16),
			scaleByMapAreaAbsolute(8),
			scaleByMapAreaAbsolute(13),
			scaleByMapAreaAbsolute(13),
			scaleByMapAreaAbsolute(13)
		],
		avoidClasses(clWater, 0, clForest, 0, clPlayer, 0, clHill, 0));
	Engine.SetProgress(70);

	createFood(
		[
			[new SimpleObject(oSheep, 2, 3, 0, 2)],
			[new SimpleObject(oDeer, 5, 7, 0, 4)]
		],
		[
			3 * numPlayers,
			3 * numPlayers
		],
		avoidClasses(clWater, 0, clForest, 0, clPlayer, 1, clHill, 1, clFood, 20),
		clFood);
	Engine.SetProgress(80);

	createFood(
		[
			[new SimpleObject(oBerryBush, 5, 7, 0, 4)]
		],
		[
			randIntInclusive(1, 4) * numPlayers + 2
		],
		avoidClasses(clWater, 3, clForest, 0, clPlayer, 20, clHill, 1, clFood, 10),
		clFood);
	Engine.SetProgress(85);

	createStragglerTrees(
		[oOak, oOakLarge, oPine, oApple],
		avoidClasses(clWater, 1, clForest, 1, clHill, 1, clPlayer, 1, clMetal, 6, clRock, 6),
		clForest,
		stragglerTrees);
	Engine.SetProgress(90);

	placePlayersNomad(clPlayer,
		avoidClasses(clWater, 4, clForest, 1, clMetal, 4, clRock, 4, clHill, 4, clFood, 2));

	setSkySet("cirrus");
	// muddy brown
	setWaterColor(0.447, 0.412, 0.322);
	setWaterTint(0.447, 0.412, 0.322);
	setWaterMurkiness(1.0);
	setWaterWaviness(3.0);
	setWaterType("lake");

	setFogThickness(0.25);
	setFogFactor(0.4);

	g_Map.ExportMap();
}
