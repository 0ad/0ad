Engine.LoadLibrary("rmgen");
Engine.LoadLibrary("rmgen-common");

{
	const tGrass = ["medit_grass_field", "medit_grass_field_b", "temp_grass_c"];
	const tLushGrass = ["medit_grass_field", "medit_grass_field_a"];

	const tSteepCliffs = ["temp_cliff_b", "temp_cliff_a"];
	const tCliffs = ["temp_cliff_b", "medit_cliff_italia", "medit_cliff_italia_grass"];
	const tHill = [
		"medit_cliff_italia_grass",
		"medit_cliff_italia_grass",
		"medit_grass_field",
		"medit_grass_field",
		"temp_grass"
	];
	const tMountain = ["medit_cliff_italia_grass", "medit_cliff_italia"];

	const tRoad = ["medit_city_tile", "medit_rocks_grass", "medit_grass_field_b"];
	const tRoadWild = ["medit_rocks_grass", "medit_grass_field_b"];

	const tShoreBlend = ["medit_sand_wet", "medit_rocks_wet"];
	const tShore = ["medit_rocks", "medit_sand", "medit_sand"];
	const tSandTransition = ["medit_sand", "medit_rocks_grass", "medit_rocks_grass", "medit_rocks_grass"];
	const tVeryDeepWater = ["medit_sea_depths", "medit_sea_coral_deep"];
	const tDeepWater = ["medit_sea_coral_deep", "tropic_ocean_coral"];
	const tCreekWater = "medit_sea_coral_plants";

	const ePine = "gaia/tree/aleppo_pine";
	const ePalmTall = "gaia/tree/cretan_date_palm_tall";
	const eFanPalm = "gaia/tree/medit_fan_palm";
	const eApple = "gaia/fruit/apple";
	const eBush = "gaia/fruit/berry_01";
	const eFish = "gaia/fish/generic";
	const ePig = "gaia/fauna_pig";
	const eStoneMine = "gaia/rock/mediterranean_large";
	const eMetalMine = "gaia/ore/mediterranean_large";

	const aRock = "actor|geology/stone_granite_med.xml";
	const aLargeRock = "actor|geology/stone_granite_large.xml";
	const aBushA = "actor|props/flora/bush_medit_sm_lush.xml";
	const aBushB = "actor|props/flora/bush_medit_me_lush.xml";
	const aPlantA = "actor|props/flora/plant_medit_artichoke.xml";
	const aPlantB = "actor|props/flora/grass_tufts_a.xml";
	const aPlantC = "actor|props/flora/grass_soft_tuft_a.xml";

	const aStandingStone = "actor|props/special/eyecandy/standing_stones.xml";

	const heightSeaGround = -8;
	const heightCreeks = -5;
	const heightBeaches = -1;
	const heightMain = 5;

	const heightOffsetMainRelief = 30;
	const heightOffsetLevel1 = 9;
	const heightOffsetLevel2 = 8;
	const heightOffsetBumps = 2;
	const heightOffsetAntiBumps = -5;

	globalThis.g_Map = new RandomMap(heightSeaGround, tVeryDeepWater);

	const numPlayers = getNumPlayers();
	const mapSize = g_Map.getSize();
	const mapCenter = g_Map.getCenter();

	const clIsland = g_Map.createTileClass();
	const clCreek = g_Map.createTileClass();
	const clWater = g_Map.createTileClass();
	const clCliffs = g_Map.createTileClass();
	const clForest = g_Map.createTileClass();
	const clShore = g_Map.createTileClass();
	const clPlayer = g_Map.createTileClass();
	const clBaseResource = g_Map.createTileClass();
	const clPassage = g_Map.createTileClass();
	const clSettlement = g_Map.createTileClass();

	const radiusBeach = fractionToTiles(0.57);
	const radiusCreeks = fractionToTiles(0.52);
	const radiusIsland = fractionToTiles(0.4);
	const radiusLevel1 = fractionToTiles(0.35);
	const radiusPlayer = fractionToTiles(0.25);
	const radiusLevel2 = fractionToTiles(0.2);

	const creeksArea = () => randBool() ? randFloat(10, 50) : scaleByMapSize(75, 100) + randFloat(0, 20);

	const nbCreeks = scaleByMapSize(6, 15);
	const nbSubIsland = 5;
	const nbBeaches = scaleByMapSize(2, 5);
	const nbPassagesLevel1 = scaleByMapSize(4, 8);
	const nbPassagesLevel2 = scaleByMapSize(2, 4);

	g_Map.log("Creating Corsica and Sardinia");
	const swapAngle = randBool() ? Math.PI / 2 : 0;
	const islandLocations = [new Vector2D(0.05, 0.05), new Vector2D(0.95, 0.95)]
		.map(v => v.mult(mapSize).rotateAround(-swapAngle, mapCenter));

	for (let island = 0; island < 2; ++island)
	{
		g_Map.log("Creating island area");
		createArea(
			new ClumpPlacer(diskArea(radiusIsland), 1, 0.5, Infinity, islandLocations[island]),
			[
				new LayeredPainter([tCliffs, tGrass], [2]),
				new SmoothElevationPainter(ELEVATION_SET, heightMain, 0),
				new TileClassPainter(clIsland)
			]);

		g_Map.log("Creating subislands");
		for (let i = 0; i < nbSubIsland + 1; ++i)
		{
			const angle = Math.PI * (island + i / (nbSubIsland * 2)) + swapAngle;
			const location =
				Vector2D.add(islandLocations[island], new Vector2D(radiusIsland, 0).rotate(-angle));
			createArea(
				new ClumpPlacer(diskArea(fractionToTiles(0.09)), 0.6, 0.03, Infinity, location),
				[
					new LayeredPainter([tCliffs, tGrass], [2]),
					new SmoothElevationPainter(ELEVATION_SET, heightMain, 1),
					new TileClassPainter(clIsland)
				]);
		}

		g_Map.log("Creating creeks");
		for (let i = 0; i < nbCreeks + 1; ++i)
		{
			const angle = Math.PI * (island + i * (1 / (nbCreeks * 2))) + swapAngle;
			const location =
				Vector2D.add(islandLocations[island], new Vector2D(radiusCreeks, 0).rotate(-angle));
			createArea(
				new ClumpPlacer(creeksArea(), 0.4, 0.01, Infinity, location),
				[
					new TerrainPainter(tSteepCliffs),
					new SmoothElevationPainter(ELEVATION_SET, heightCreeks, 0),
					new TileClassPainter(clCreek)
				]);
		}

		g_Map.log("Creating beaches");
		for (let i = 0; i < nbBeaches + 1; ++i)
		{
			const angle = Math.PI * (island +
				(i / (nbBeaches * 2.5)) + 1 / (nbBeaches * 6) +
				randFloat(-1, 1) / (nbBeaches * 7)) +
				swapAngle;
			const start =
				Vector2D.add(islandLocations[island], new Vector2D(radiusIsland, 0).rotate(-angle));
			const end =
				Vector2D.add(islandLocations[island], new Vector2D(radiusBeach, 0).rotate(-angle));

			createArea(
				new ClumpPlacer(130, 0.7, 0.8, Infinity,
					Vector2D.add(start, Vector2D.mult(end, 3)).div(4)),
				new SmoothElevationPainter(ELEVATION_SET, heightBeaches, 5));

			createPassage({
				"start": start,
				"end": end,
				"startWidth": 18,
				"endWidth": 25,
				"smoothWidth": 4,
				"tileClass": clShore
			});
		}

		g_Map.log("Creating main relief");
		createArea(
			new ClumpPlacer(diskArea(radiusIsland), 1, 0.2, Infinity, islandLocations[island]),
			new SmoothElevationPainter(ELEVATION_MODIFY, heightOffsetMainRelief,
				fractionToTiles(0.45)));

		g_Map.log("Creating first level plateau");
		createArea(
			new ClumpPlacer(diskArea(radiusLevel1), 0.95, 0.02, Infinity, islandLocations[island]),
			new SmoothElevationPainter(ELEVATION_MODIFY, heightOffsetLevel1, 1));

		g_Map.log("Creating first level passages");
		for (let i = 0; i <= nbPassagesLevel1; ++i)
		{
			const angle = Math.PI * (i / 7 + 1 / 9 + island) + swapAngle;
			createPassage({
				"start": Vector2D.add(islandLocations[island],
					new Vector2D(radiusLevel1 + 10, 0).rotate(-angle)),
				"end": Vector2D.add(islandLocations[island],
					new Vector2D(radiusLevel1 - 4, 0).rotate(-angle)),
				"startWidth": 10,
				"endWidth": 6,
				"smoothWidth": 3,
				"tileClass": clPassage
			});
		}

		if (mapSize > 150)
		{
			g_Map.log("Creating second level plateau");
			createArea(
				new ClumpPlacer(diskArea(radiusLevel2), 0.98, 0.04, Infinity,
					islandLocations[island]),
				[
					new LayeredPainter([tCliffs, tGrass], [2]),
					new SmoothElevationPainter(ELEVATION_MODIFY, heightOffsetLevel2, 1)
				]);

			g_Map.log("Creating second level passages");
			for (let i = 0; i < nbPassagesLevel2; ++i)
			{
				const angle =
					Math.PI * (i / (2 * nbPassagesLevel2) + 1 / (4 * nbPassagesLevel2) + island) +
					swapAngle;
				createPassage({
					"start": Vector2D.add(islandLocations[island],
						new Vector2D(radiusLevel2 + 3, 0).rotate(-angle)),
					"end": Vector2D.add(islandLocations[island],
						new Vector2D(radiusLevel2 - 6, 0).rotate(-angle)),
					"startWidth": 4,
					"endWidth": 6,
					"smoothWidth": 2,
					"tileClass": clPassage
				});
			}
		}
	}
	Engine.SetProgress(30);

	g_Map.log("Determining player locations");
	const playerIDs = sortAllPlayers();
	const playerPosition = [];
	const playerAngle = [];
	let p = 0;
	for (let island = 0; island < 2; ++island)
	{
		const playersPerIsland = island == 0 ? Math.ceil(numPlayers / 2) : Math.floor(numPlayers / 2);

		for (let i = 0; i < playersPerIsland; ++i)
		{
			playerAngle[p] = Math.PI * ((i + 0.5) / (2 * playersPerIsland) + island) + swapAngle;
			playerPosition[p] = Vector2D.add(islandLocations[island],
				new Vector2D(radiusPlayer).rotate(-playerAngle[p]));
			addCivicCenterAreaToClass(playerPosition[p], clPlayer);
			++p;
		}
	}

	placePlayerBases({
		"PlayerPlacement": [sortAllPlayers(), playerPosition],
		"BaseResourceClass": clBaseResource,
		"baseResourceConstraint": avoidClasses(clPlayer, 2),
		"Walls": false,
		"CityPatch": {
			"outerTerrain": tRoadWild,
			"innerTerrain": tRoad,
			"coherence": 0.8,
			"radius": 6,
			"painters": [
				new TileClassPainter(clSettlement)
			]
		},
		"StartingAnimal": {
		},
		"Berries": {
			"template": eBush
		},
		"Mines": {
			"types": [
				{ "template": eMetalMine },
				{ "template": eStoneMine }
			]
		}
		// Sufficient starting trees around, no decoratives
	});
	Engine.SetProgress(40);

	g_Map.log("Creating bumps");
	createAreas(
		new ClumpPlacer(70, 0.6, 0.1, Infinity),
		[new SmoothElevationPainter(ELEVATION_MODIFY, heightOffsetBumps, 3)],
		[
			stayClasses(clIsland, 2),
			avoidClasses(clPlayer, 6, clPassage, 2)
		],
		scaleByMapSize(20, 100),
		5);

	g_Map.log("Creating anti bumps");
	createAreas(
		new ClumpPlacer(120, 0.3, 0.1, Infinity),
		new SmoothElevationPainter(ELEVATION_MODIFY, heightOffsetAntiBumps, 6),
		avoidClasses(clPlayer, 6, clPassage, 2, clIsland, 2),
		scaleByMapSize(20, 100),
		5);

	g_Map.log("Painting water");
	paintTileClassBasedOnHeight(-Infinity, 0, Elevation_ExcludeMin_ExcludeMax, clWater);

	g_Map.log("Painting land");
	for (let mapX = 0; mapX < mapSize; ++mapX)
		for (let mapZ = 0; mapZ < mapSize; ++mapZ)
		{
			const position = new Vector2D(mapX, mapZ);
			const terrain = getCosricaSardiniaTerrain(position);
			if (!terrain)
				continue;

			createTerrain(terrain).place(position);

			if (terrain == tCliffs || terrain == tSteepCliffs)
				clCliffs.add(position);
		}

	function getCosricaSardiniaTerrain(position)
	{
		const isWater = clWater.countMembersInRadius(position, 3);
		const isShore = clShore.countMembersInRadius(position, 2);
		const isPassage = clPassage.countMembersInRadius(position, 2);
		const isSettlement = clSettlement.countMembersInRadius(position, 2);

		if (isSettlement)
			return undefined;

		const height = g_Map.getHeight(position);
		const slope = g_Map.getSlope(position);

		if (height >= 0.5 && height < 1.5 && isShore)
			return tSandTransition;

		// Paint land cliffs and grass
		if (height >= 1 && !isWater)
		{
			if (isPassage)
				return tGrass;

			if (slope >= 1.25)
				return height > 25 ? tSteepCliffs : tCliffs;

			if (height < 17)
				return tGrass;

			if (slope < 0.625)
				return tHill;

			return tMountain;
		}

		if (slope >= 1.125)
			return tCliffs;

		if (height >= 1.5)
			return undefined;

		if (height >= -0.75)
			return tShore;

		if (height >= -3)
			return tShoreBlend;

		if (height >= -6)
			return tCreekWater;

		if (height > -10 && slope < 0.75)
			return tDeepWater;

		return undefined;
	}

	Engine.SetProgress(65);

	g_Map.log("Creating mines");
	for (const mine of [eMetalMine, eStoneMine])
		createObjectGroupsDeprecated(
			new SimpleGroup(
				[
					new SimpleObject(mine, 1, 1, 0, 0),
					new SimpleObject(aBushB, 1, 1, 2, 2),
					new SimpleObject(aBushA, 0, 2, 1, 3)
				],
				true,
				clBaseResource),
			0,
			[
				stayClasses(clIsland, 1),
				avoidClasses(
					clWater, 3,
					clPlayer, 6,
					clBaseResource, 4,
					clPassage, 2,
					clCliffs, 1)
			],
			scaleByMapSize(6, 25),
			1000);

	g_Map.log("Creating grass patches");
	createAreas(
		new ClumpPlacer(20, 0.3, 0.06, 0.5),
		[
			new TerrainPainter(tLushGrass),
			new TileClassPainter(clForest)
		],
		avoidClasses(
			clWater, 1,
			clPlayer, 6,
			clBaseResource, 3,
			clCliffs, 1),
		scaleByMapSize(10, 40));

	g_Map.log("Creating forests");
	createObjectGroupsDeprecated(
		new SimpleGroup(
			[
				new SimpleObject(ePine, 3, 6, 1, 3),
				new SimpleObject(ePalmTall, 1, 3, 1, 3),
				new SimpleObject(eFanPalm, 0, 2, 0, 2),
				new SimpleObject(eApple, 0, 1, 1, 2)
			],
			true,
			clForest),
		0,
		[
			stayClasses(clIsland, 3),
			avoidClasses(
				clWater, 1,
				clForest, 0,
				clPlayer, 3,
				clBaseResource, 4,
				clPassage, 2,
				clCliffs, 2)
		],
		scaleByMapSize(350, 2500),
		100);

	Engine.SetProgress(75);

	g_Map.log("Creating small decorative rocks");
	createObjectGroupsDeprecated(
		new SimpleGroup(
			[
				new SimpleObject(aRock, 1, 3, 0, 1),
				new SimpleObject(aStandingStone, 0, 2, 0, 3)
			],
			true),
		0,
		avoidClasses(
			clWater, 0,
			clForest, 0,
			clPlayer, 6,
			clBaseResource, 4,
			clPassage, 2),
		scaleByMapSize(16, 262),
		50);

	g_Map.log("Creating large decorative rocks");
	const rocksGroup = new SimpleGroup(
		[
			new SimpleObject(aLargeRock, 1, 2, 0, 1),
			new SimpleObject(aRock, 1, 3, 0, 2)
		],
		true);

	createObjectGroupsDeprecated(
		rocksGroup,
		0,
		avoidClasses(
			clWater, 0,
			clForest, 0,
			clPlayer, 6,
			clBaseResource, 4,
			clPassage, 2),
		scaleByMapSize(8, 131),
		50);

	createObjectGroupsDeprecated(
		rocksGroup,
		0,
		borderClasses(clWater, 5, 10),
		scaleByMapSize(100, 800),
		500);

	g_Map.log("Creating decorative plants");
	const plantGroups = [
		new SimpleGroup(
			[
				new SimpleObject(aPlantA, 3, 7, 0, 3),
				new SimpleObject(aPlantB, 3, 6, 0, 3),
				new SimpleObject(aPlantC, 1, 4, 0, 4)
			],
			true),
		new SimpleGroup(
			[
				new SimpleObject(aPlantB, 5, 20, 0, 5),
				new SimpleObject(aPlantC, 4, 10, 0, 4)
			],
			true)
	];
	for (const group of plantGroups)
		createObjectGroupsDeprecated(
			group,
			0,
			avoidClasses(
				clWater, 0,
				clBaseResource, 4,
				clShore, 3),
			scaleByMapSize(100, 600),
			50);

	Engine.SetProgress(80);

	g_Map.log("Creating animals");
	createObjectGroupsDeprecated(
		new SimpleGroup([new SimpleObject(ePig, 2, 4, 0, 3)]),
		0,
		avoidClasses(
			clWater, 3,
			clBaseResource, 4,
			clPlayer, 6),
		scaleByMapSize(20, 100),
		50);

	g_Map.log("Creating fish");
	createObjectGroupsDeprecated(
		new SimpleGroup([new SimpleObject(eFish, 1, 2, 0, 3)]),
		0,
		[
			stayClasses(clWater, 3),
			avoidClasses(clCreek, 3, clShore, 3)
		],
		scaleByMapSize(50, 150),
		100);

	Engine.SetProgress(95);

	placePlayersNomad(clPlayer, avoidClasses(clWater, 4, clForest, 1, clBaseResource, 4, clCliffs, 4));

	setSkySet(pickRandom(["cumulus", "sunny"]));

	setSunColor(0.8, 0.66, 0.48);
	setSunElevation(0.828932);
	setSunRotation((swapAngle ? 0.288 : 0.788) * Math.PI);

	setAmbientColor(0.564706, 0.543726, 0.419608);
	setWaterColor(0.2, 0.294, 0.49);
	setWaterTint(0.208, 0.659, 0.925);
	setWaterMurkiness(0.72);
	setWaterWaviness(2.0);
	setWaterType("ocean");

	g_Map.ExportMap();
}
