Engine.LoadLibrary("rmgen");
Engine.LoadLibrary("rmgen-common");

{
	const tPrimary = ["alpine_snow_01"];
	const tSecondary = "alpine_snow_02";
	const tShore = "alpine_ice_01";
	const tWater = "alpine_ice_01";

	const oArcticFox = "gaia/fauna_fox_arctic";
	const oArcticWolf = "gaia/fauna_wolf_arctic_violent";
	const oMuskox = "gaia/fauna_muskox";
	const oWalrus = "gaia/fauna_walrus";
	const oWhaleFin = "gaia/fauna_whale_fin";
	const oWhaleHumpback = "gaia/fauna_whale_humpback";
	const oFish = "gaia/fish/generic";
	const oStoneLarge = "gaia/rock/polar_01";
	const oStoneSmall = "gaia/rock/alpine_small";
	const oMetalLarge = "gaia/ore/polar_01";
	const oWoodTreasure = "gaia/treasure/wood";
	const oMarket = "skirmish/structures/default_market";

	const aRockLarge = "actor|geology/stone_granite_med.xml";
	const aRockMedium = "actor|geology/stone_granite_med.xml";
	const aIceberg = "actor|props/special/eyecandy/iceberg.xml";

	const heightSeaGround = -10;
	const heightLand = 2;
	const heightCliff = 3;

	globalThis.g_Map = new RandomMap(heightLand, tPrimary);

	const numPlayers = getNumPlayers();
	const mapSize = g_Map.getSize();
	const mapCenter = g_Map.getCenter();

	const clPlayer = g_Map.createTileClass();
	const clWater = g_Map.createTileClass();
	const clDirt = g_Map.createTileClass();
	const clRock = g_Map.createTileClass();
	const clMetal = g_Map.createTileClass();
	const clHill = g_Map.createTileClass();
	const clFood = g_Map.createTileClass();
	const clBaseResource = g_Map.createTileClass();
	const clArcticWolf = g_Map.createTileClass();

	let [playerIDs, playerPosition] = playerPlacementCircle(fractionToTiles(0.35));

	const treasures = [{
		"template": oWoodTreasure,
		"count": isNomad() ? 16 : 14
	}];

	g_Map.log("Creating player markets");
	if (!isNomad())
		for (let i = 0; i < numPlayers; ++i)
		{
			const marketPos =
				Vector2D.add(playerPosition[i], new Vector2D(12, 0).rotate(randomAngle())).round();
			g_Map.placeEntityPassable(oMarket, playerIDs[i], marketPos, BUILDING_ORIENTATION);
			addCivicCenterAreaToClass(marketPos, clBaseResource);
		}

	placePlayerBases({
		"PlayerPlacement": [playerIDs, playerPosition],
		"PlayerTileClass": clPlayer,
		"BaseResourceClass": clBaseResource,
		"Walls": "towers",
		"CityPatch": {
			"outerTerrain": tSecondary,
			"innerTerrain": tSecondary
		},
		"StartingAnimal": {
			"template": oMuskox
		},
		// No berries, no trees, no decoratives
		"Mines": {
			"types": [
				{ "template": oMetalLarge },
				{ "template": oStoneLarge }
			]
		},
		"Treasures": {
			"types": treasures
		},
	});
	Engine.SetProgress(30);

	g_Map.log("Creating central lake");
	createArea(
		new ChainPlacer(
			2,
			Math.floor(scaleByMapSize(5, 16)),
			Math.floor(scaleByMapSize(35, 200)),
			Infinity,
			mapCenter,
			0,
			[Math.floor(fractionToTiles(0.17))]),
		[
			new LayeredPainter([tShore, tWater], [1]),
			new SmoothElevationPainter(ELEVATION_SET, heightSeaGround, 4),
			new TileClassPainter(clWater)
		],
		avoidClasses(clPlayer, 20));

	Engine.SetProgress(40);

	g_Map.log("Creating small lakes");
	createAreas(
		new ChainPlacer(1, Math.floor(scaleByMapSize(2, 4)), Math.floor(scaleByMapSize(20, 140)), 0.7),
		[
			new LayeredPainter([tShore, tWater], [1]),
			new SmoothElevationPainter(ELEVATION_SET, heightSeaGround, 5),
			new TileClassPainter(clWater)
		],
		avoidClasses(clPlayer, 20),
		scaleByMapSize(10, 16),
		1);
	Engine.SetProgress(50);

	createBumps(avoidClasses(clWater, 2, clPlayer, 20));
	Engine.SetProgress(60);

	createHills(
		[tSecondary, tSecondary, tSecondary],
		avoidClasses(clPlayer, 20, clHill, 35),
		clHill,
		scaleByMapSize(20, 240));
	Engine.SetProgress(65);

	g_Map.log("Creating glacier patches");
	createPatches(
		[scaleByMapSize(10, 20), scaleByMapSize(20, 30)],
		tSecondary,
		avoidClasses(clWater, 3, clDirt, 5, clPlayer, 12),
		scaleByMapSize(15, 45),
		clDirt);
	Engine.SetProgress(70);

	g_Map.log("Creating stone mines");
	createMines(
		[
			[
				new SimpleObject(oStoneSmall, 0, 2, 0, 4, 0, 2 * Math.PI, 1),
				new SimpleObject(oStoneLarge, 1, 1, 0, 4, 0, 2 * Math.PI, 4)
			],
			[new SimpleObject(oStoneSmall, 2, 5, 1, 3)]
		],
		avoidClasses(clWater, 3, clPlayer, 20, clRock, 18, clHill, 2),
		clRock);

	g_Map.log("Creating metal mines");
	createMines(
		[
			[new SimpleObject(oMetalLarge, 1, 1, 0, 4)]
		],
		avoidClasses(clWater, 3, clPlayer, 20, clMetal, 18, clRock, 5, clHill, 2),
		clMetal);
	Engine.SetProgress(75);

	createDecoration(
		[
			[
				new SimpleObject(aRockMedium, 1, 3, 0, 1)
			],
			[
				new SimpleObject(aRockLarge, 1, 2, 0, 1),
				new SimpleObject(aRockMedium, 1, 3, 0, 2)
			]
		],
		[
			scaleByMapAreaAbsolute(16),
			scaleByMapAreaAbsolute(8),
		],
		avoidClasses(clWater, 0, clPlayer, 0));

	createDecoration(
		[
			[new SimpleObject(aIceberg, 1, 1, 1, 1)]
		],
		[
			scaleByMapAreaAbsolute(8)
		],
		[stayClasses(clWater, 4), avoidClasses(clHill, 2)]);
	Engine.SetProgress(80);

	createFood(
		[
			[new SimpleObject(oArcticFox, 1, 2, 0, 3)],
			[new SimpleObject(isNomad() ? oArcticFox : oArcticWolf, 4, 6, 0, 4)],
			[new SimpleObject(oWalrus, 2, 3, 0, 2)],
			[new SimpleObject(oMuskox, 2, 3, 0, 2)]
		],
		[
			3 * numPlayers,
			5 * numPlayers,
			5 * numPlayers,
			12 * numPlayers
		],
		avoidClasses(clPlayer, 35, clFood, 16, clWater, 2, clMetal, 4, clRock, 4, clHill, 2),
		clFood);

	createFood(
		[
			[new SimpleObject(oWhaleFin, 1, 2, 0, 2)],
			[new SimpleObject(oWhaleHumpback, 1, 2, 0, 2)]
		],
		[
			scaleByMapSize(1, 6) * 3,
			scaleByMapSize(1, 6) * 3,
		],
		[avoidClasses(clFood, 20, clHill, 5), stayClasses(clWater, 6)],
		clFood);

	createFood(
		[
			[new SimpleObject(oFish, 2, 3, 0, 2)]
		],
		[
			100
		],
		[avoidClasses(clFood, 12, clHill, 5), stayClasses(clWater, 6)],
		clFood);
	Engine.SetProgress(85);

	// Create trigger points where wolves spawn
	createObjectGroupsDeprecated(
		new SimpleGroup([new SimpleObject("trigger/trigger_point_A", 1, 1, 0, 0)], true, clArcticWolf),
		0,
		avoidClasses(clWater, 2, clMetal, 4, clRock, 4, clPlayer, 15, clHill, 2, clArcticWolf, 20),
		1000,
		100);
	Engine.SetProgress(95);

	if (g_MapSettings.Daytime !== undefined ? g_MapSettings.Daytime == "dawn" : randBool(1/3))
	{
		setSkySet("sunset 1");
		setSunColor(0.8, 0.7, 0.6);
		setAmbientColor(0.7, 0.6, 0.7);
		setSunElevation(Math.PI * randFloat(1/24, 1/7));
	}
	else
	{
		setSkySet(pickRandom(["cumulus", "rain", "mountainous", "overcast", "rain", "stratus"]));
		setSunElevation(Math.PI * randFloat(1/9, 1/7));
	}

	if (isNomad())
	{
		const constraint = avoidClasses(clWater, 4, clMetal, 4, clRock, 4, clHill, 4, clFood, 2);
		[playerIDs, playerPosition] = placePlayersNomad(clPlayer, constraint);

		for (let i = 0; i < numPlayers; ++i)
			placePlayerBaseTreasures({
				"playerID": playerIDs[i],
				"playerPosition": playerPosition[i],
				"BaseResourceClass": clBaseResource,
				"baseResourceConstraint": constraint,
				"types": treasures
			});
	}

	setSunRotation(randomAngle());

	setWaterColor(0.3, 0.3, 0.4);
	setWaterTint(0.75, 0.75, 0.75);
	setWaterMurkiness(0.92);
	setWaterWaviness(0.5);
	setWaterType("clap");

	setFogThickness(0.76);
	setFogFactor(0.7);

	setPPEffect("hdr");
	setPPContrast(0.6);
	setPPSaturation(0.45);
	setPPBloom(0.4);

	g_Map.ExportMap();
}
