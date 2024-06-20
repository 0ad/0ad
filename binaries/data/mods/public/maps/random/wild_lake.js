Engine.LoadLibrary("rmgen");
Engine.LoadLibrary("rmgen-common");
Engine.LoadLibrary("rmbiome");
Engine.LoadLibrary("heightmap");

{
	globalThis.g_Map = new RandomMap(0, "whiteness");

	/**
	 * getArray - To ensure a terrain texture is contained within an array
	 */
	function getArray(stringOrArrayOfStrings)
	{
		if (typeof stringOrArrayOfStrings == "string")
			return [stringOrArrayOfStrings];
		return stringOrArrayOfStrings;
	}

	setSelectedBiome();

	// Terrain, entities and actors
	const wildLakeBiome = [
		// 0 Deep water
		{
			"texture": getArray(g_Terrains.water),
			"entity": [[g_Gaia.fish], 0.005],
			"textureHS": getArray(g_Terrains.water),
			"entityHS": [[g_Gaia.fish], 0.01]
		},
		// 1 Shallow water
		{
			"texture": getArray(g_Terrains.water),
			"entity": [[g_Decoratives.lillies, g_Decoratives.reeds], 0.3],
			"textureHS": getArray(g_Terrains.water),
			"entityHS": [[g_Decoratives.lillies], 0.1]
		},
		// 2 Shore
		{
			"texture": getArray(g_Terrains.shore),
			"entity": [
				[
					g_Gaia.tree1,
					g_Gaia.tree1,
					g_Gaia.tree1,
					g_Gaia.tree1,
					g_Gaia.tree1,
					g_Gaia.tree1,
					g_Gaia.tree1,
					g_Gaia.tree1,
					g_Gaia.tree1,
					g_Gaia.tree1,
					g_Gaia.tree1,
					g_Gaia.tree1,
					g_Gaia.tree1,
					g_Gaia.tree1,
					g_Gaia.tree1,
					g_Gaia.tree2,
					g_Gaia.tree2,
					g_Gaia.tree2,
					g_Gaia.tree2,
					g_Gaia.tree2,
					g_Gaia.tree2,
					g_Gaia.tree2,
					g_Gaia.tree2,
					g_Gaia.tree2,
					g_Gaia.tree2,
					g_Gaia.tree2,
					g_Gaia.tree2,
					g_Gaia.tree2,
					g_Gaia.tree2,
					g_Gaia.tree2,
					g_Gaia.mainHuntableAnimal,
					g_Decoratives.grass,
					g_Decoratives.grass,
					g_Decoratives.rockMedium,
					g_Decoratives.rockMedium,
					g_Decoratives.rockMedium,
					g_Decoratives.rockMedium,
					g_Decoratives.rockMedium,
					g_Decoratives.rockMedium,
					g_Decoratives.rockMedium,
					g_Decoratives.rockMedium,
					g_Decoratives.bushMedium,
					g_Decoratives.bushMedium,
					g_Decoratives.bushMedium,
					g_Decoratives.bushMedium,
					g_Decoratives.bushMedium,
					g_Decoratives.bushMedium,
					g_Decoratives.bushMedium,
					g_Decoratives.bushMedium
				],
				0.3
			],
			"textureHS": getArray(g_Terrains.cliff),
			"entityHS": [
				[g_Decoratives.grassShort, g_Decoratives.rockMedium, g_Decoratives.bushSmall],
				0.05
			]
		},
		// 3 Low ground
		{
			"texture": getArray(g_Terrains.tier1Terrain),
			"entity": [
				[
					g_Decoratives.grass,
					g_Decoratives.grassShort,
					g_Decoratives.rockLarge,
					g_Decoratives.rockMedium,
					g_Decoratives.bushMedium,
					g_Decoratives.bushSmall
				],
				0.07
			],
			"textureHS": getArray(g_Terrains.cliff),
			"entityHS": [
				[g_Decoratives.grassShort, g_Decoratives.rockMedium, g_Decoratives.bushSmall],
				0.05
			]
		},
		// 4 Mid ground. Player and path height
		{
			"texture": getArray(g_Terrains.mainTerrain),
			"entity": [
				[
					g_Decoratives.grass,
					g_Decoratives.grassShort,
					g_Decoratives.rockLarge,
					g_Decoratives.rockMedium,
					g_Decoratives.bushMedium,
					g_Decoratives.bushSmall
				],
				0.07
			],
			"textureHS": getArray(g_Terrains.cliff),
			"entityHS": [
				[g_Decoratives.grassShort, g_Decoratives.rockMedium, g_Decoratives.bushSmall],
				0.05
			]
		},
		// 5 High ground
		{
			"texture": getArray(g_Terrains.tier2Terrain),
			"entity": [
				[
					g_Decoratives.grass,
					g_Decoratives.grassShort,
					g_Decoratives.rockLarge,
					g_Decoratives.rockMedium,
					g_Decoratives.bushMedium,
					g_Decoratives.bushSmall
				],
				0.07
			],
			"textureHS": getArray(g_Terrains.cliff),
			"entityHS": [
				[g_Decoratives.grassShort, g_Decoratives.rockMedium, g_Decoratives.bushSmall],
				0.05
			]
		},
		// 6 Lower hilltop forest border
		{
			"texture": getArray(g_Terrains.dirt),
			"entity": [
				[
					g_Gaia.tree1, g_Gaia.tree1,
					g_Gaia.tree3, g_Gaia.tree3,
					g_Gaia.fruitBush,
					g_Gaia.secondaryHuntableAnimal,
					g_Decoratives.grass, g_Decoratives.grass,
					g_Decoratives.rockMedium, g_Decoratives.rockMedium,
					g_Decoratives.bushMedium, g_Decoratives.bushMedium
				],
				0.25
			],
			"textureHS": getArray(g_Terrains.cliff),
			"entityHS": [
				[g_Decoratives.grassShort, g_Decoratives.rockMedium, g_Decoratives.bushSmall],
				0.1
			]
		},
		// 7 Hilltop forest
		{
			"texture": getArray(g_Terrains.forestFloor1),
			"entity": [
				[
					g_Gaia.tree1,
					g_Gaia.tree2,
					g_Gaia.tree3,
					g_Gaia.tree4,
					g_Gaia.tree5,
					g_Decoratives.tree,
					g_Decoratives.grass,
					g_Decoratives.rockMedium,
					g_Decoratives.bushMedium
				],
				0.3
			],
			"textureHS": getArray(g_Terrains.cliff),
			"entityHS": [
				[g_Decoratives.grassShort, g_Decoratives.rockMedium, g_Decoratives.bushSmall],
				0.1
			]
		}
	];

	const wildLakeEntities = Engine.ReadJSONFile("maps/random/wild_lake_biomes.json");
	const farmEntities = wildLakeEntities[currentBiome()].farmEntities;
	const mercenaryCampEntities = wildLakeEntities[currentBiome()].mercenaryCampEntities;
	const guards = mercenaryCampEntities
		.map(ent => ent.Template)
		.filter(ent => ent.indexOf("units/") != -1);
	const campEntities = wildLakeEntities.campEntities.concat(guards);

	/**
	 * Resource spots and other points of interest
	 */

	function placeMine(position, centerEntity,
		decorativeActors = [
			g_Decoratives.grass, g_Decoratives.grassShort,
			g_Decoratives.rockLarge, g_Decoratives.rockMedium,
			g_Decoratives.bushMedium, g_Decoratives.bushSmall
		]
	)
	{
		g_Map.placeEntityPassable(centerEntity, 0, position, randomAngle());

		const quantity = randIntInclusive(11, 23);
		const dAngle = 2 * Math.PI / quantity;
		for (let i = 0; i < quantity; ++i)
			g_Map.placeEntityPassable(
				pickRandom(decorativeActors),
				0,
				Vector2D.add(position,
					new Vector2D(randFloat(2, 5), 0).rotate(-dAngle * randFloat(i, i + 1))),
				randomAngle());
	}

	// Groves, only wood
	const groveActors = [g_Decoratives.grass, g_Decoratives.rockMedium, g_Decoratives.bushMedium];
	const clGaiaCamp = g_Map.createTileClass();

	function placeGrove(point,
		groveEntities = [
			g_Gaia.tree1, g_Gaia.tree1, g_Gaia.tree1, g_Gaia.tree1, g_Gaia.tree1,
			g_Gaia.tree2, g_Gaia.tree2, g_Gaia.tree2, g_Gaia.tree2,
			g_Gaia.tree3, g_Gaia.tree3, g_Gaia.tree3,
			g_Gaia.tree4, g_Gaia.tree4, g_Gaia.tree5
		],
		groveTileClass = undefined,
		groveTerrainTexture = getArray(g_Terrains.forestFloor1)
	)
	{
		const position = new Vector2D(point.x, point.y);
		g_Map.placeEntityPassable(pickRandom(["structures/gaul/outpost", g_Gaia.tree1]), 0, position,
			randomAngle());

		const quantity = randIntInclusive(20, 30);
		const dAngle = 2 * Math.PI / quantity;
		for (let i = 0; i < quantity; ++i)
		{
			const angle = dAngle * randFloat(i, i + 1);
			const dist = randFloat(2, 5);
			let objectList = groveEntities;
			if (i % 3 == 0)
				objectList = groveActors;

			const pos = Vector2D.add(position, new Vector2D(dist, 0).rotate(-angle));
			g_Map.placeEntityPassable(pickRandom(objectList), 0, pos, randomAngle());

			const painters = [new TerrainPainter(groveTerrainTexture)];
			if (groveTileClass)
				painters.push(new TileClassPainter(groveTileClass));

			createArea(
				new ClumpPlacer(5, 1, 1, Infinity, pos),
				painters);
		}
	}



	g_WallStyles.other = {
		"overlap": 0,
		"fence": readyWallElement("structures/fence_long", "gaia"),
		"fence_short": readyWallElement("structures/fence_short", "gaia"),
		"bench": {
			"angle": Math.PI / 2,
			"length": 1.5,
			"indent": 0,
			"bend": 0,
			"templateName": "structures/bench"
		},
		"foodBin": {
			"angle": Math.PI / 2,
			"length": 1.5,
			"indent": 0,
			"bend": 0,
			"templateName": "gaia/treasure/food_bin"
		},
		"animal": {
			"angle": 0,
			"length": 0,
			"indent": 0.75,
			"bend": 0,
			"templateName": farmEntities.animal
		},
		"farmstead": {
			"angle": Math.PI,
			"length": 0,
			"indent": -3,
			"bend": 0,
			"templateName": farmEntities.building
		}
	};

	const fences = [
		new Fortress("fence", [
			"foodBin", "farmstead", "bench",
			"turn_0.25", "animal", "turn_0.25", "fence",
			"turn_0.25", "animal", "turn_0.25", "fence",
			"turn_0.25", "animal", "turn_0.25", "fence"
		]),
		new Fortress("fence", [
			"foodBin", "farmstead", "fence",
			"turn_0.25", "animal", "turn_0.25", "fence",
			"turn_0.25", "animal", "turn_0.25", "bench", "animal", "fence",
			"turn_0.25", "animal", "turn_0.25", "fence"
		]),
		new Fortress("fence", [
			"foodBin", "farmstead", "turn_0.5", "bench", "turn_-0.5", "fence_short",
			"turn_0.25", "animal", "turn_0.25", "fence",
			"turn_0.25", "animal", "turn_0.25", "fence",
			"turn_0.25", "animal", "turn_0.25", "fence_short", "animal", "fence"
		]),
		new Fortress("fence", [
			"foodBin", "farmstead", "turn_0.5", "fence_short", "turn_-0.5", "bench",
			"turn_0.25", "animal", "turn_0.25", "fence",
			"turn_0.25", "animal", "turn_0.25", "fence",
			"turn_0.25", "animal", "turn_0.25", "fence_short", "animal", "fence"
		]),
		new Fortress("fence", [
			"foodBin", "farmstead", "fence",
			"turn_0.25", "animal", "turn_0.25", "bench", "animal", "fence",
			"turn_0.25", "animal", "turn_0.25", "fence_short", "animal", "fence",
			"turn_0.25", "animal", "turn_0.25", "fence_short", "animal", "fence"
		])
	];
	const num = fences.length;
	for (let i = 0; i < num; ++i)
		fences.push(new Fortress("fence", clone(fences[i].wall).reverse()));

	// Camps with fire and gold treasure
	function placeCamp(position,
		centerEntity = "actor|props/special/eyecandy/campfire.xml"
	)
	{
		g_Map.placeEntityPassable(centerEntity, 0, position, randomAngle());

		const quantity = randIntInclusive(5, 11);
		const dAngle = 2 * Math.PI / quantity;
		for (let i = 0; i < quantity; ++i)
		{
			const angle = dAngle * randFloat(i, i + 1);
			const dist = randFloat(1, 3);
			g_Map.placeEntityPassable(pickRandom(campEntities), 0,
				Vector2D.add(position, new Vector2D(dist, 0).rotate(-angle)), randomAngle());
		}

		addCivicCenterAreaToClass(position, clGaiaCamp);
	}

	function placeStartLocationResources(
		point,
		foodEntities = [g_Gaia.fruitBush, g_Gaia.startingAnimal],
		groveEntities = [
			g_Gaia.tree1, g_Gaia.tree1, g_Gaia.tree1, g_Gaia.tree1, g_Gaia.tree1,
			g_Gaia.tree2, g_Gaia.tree2, g_Gaia.tree2, g_Gaia.tree2,
			g_Gaia.tree3, g_Gaia.tree3, g_Gaia.tree3,
			g_Gaia.tree4, g_Gaia.tree4, g_Gaia.tree5
		],
		groveTerrainTexture = getArray(g_Terrains.forestFloor1),
		averageDistToCC = 10,
		dAverageDistToCC = 2
	)
	{
		function getRandDist()
		{
			return averageDistToCC + randFloat(-dAverageDistToCC, dAverageDistToCC);
		}

		let currentAngle = randomAngle();
		// Stone
		let dAngle = 4/9 * Math.PI;
		let angle = currentAngle + randFloat(dAngle / 4, 3 * dAngle / 4);
		placeMine(Vector2D.add(point, new Vector2D(averageDistToCC, 0).rotate(-angle)),
			g_Gaia.stoneLarge);

		currentAngle += dAngle;

		// Wood
		let quantity = 80;
		dAngle = 2/3 * Math.PI / quantity;
		for (let i = 0; i < quantity; ++i)
		{
			angle = currentAngle + randFloat(0, dAngle);
			const dist = getRandDist();
			let objectList = groveEntities;
			if (i % 2 == 0)
				objectList = groveActors;

			const position = Vector2D.add(point, new Vector2D(dist, 0).rotate(-angle));
			g_Map.placeEntityPassable(pickRandom(objectList), 0, position, randomAngle());

			createArea(
				new ClumpPlacer(5, 1, 1, Infinity, position),
				new TerrainPainter(groveTerrainTexture));
			currentAngle += dAngle;
		}

		// Metal
		dAngle = 4/9 * Math.PI;
		angle = currentAngle + randFloat(dAngle / 4, 3 * dAngle / 4);
		placeMine(Vector2D.add(point, new Vector2D(averageDistToCC, 0).rotate(-angle)),
			g_Gaia.metalLarge);
		currentAngle += dAngle;

		// Berries and domestic animals
		quantity = 15;
		dAngle = 4/9 * Math.PI / quantity;
		for (let i = 0; i < quantity; ++i)
		{
			angle = currentAngle + randFloat(0, dAngle);
			const dist = getRandDist();
			g_Map.placeEntityPassable(pickRandom(foodEntities), 0, Vector2D.add(point,
				new Vector2D(dist, 0).rotate(-angle)), randomAngle());
			currentAngle += dAngle;
		}
	}

	/**
	 * Base terrain shape generation and settings
	 */

	// Height range by map size
	const heightScale = (g_Map.size + 512) / 1024 / 5;
	const heightRange = { "min": MIN_HEIGHT * heightScale, "max": MAX_HEIGHT * heightScale };

	// Water coverage
	// NOTE: Since terrain generation is quite unpredictable actual water
	//	coverage might vary much with the same value
	const averageWaterCoverage = 1 / 5;
	 // Water height in environment and the engine
	const heightSeaGround = -MIN_HEIGHT + heightRange.min + averageWaterCoverage *
		(heightRange.max - heightRange.min);
	// Water height as terrain height
	const heightSeaGroundAdjusted = heightSeaGround + MIN_HEIGHT;
	setWaterHeight(heightSeaGround);

	// Generate base terrain shape
	const lowH = heightRange.min;
	const medH = (heightRange.min + heightRange.max) / 2;

	// Lake
	let initialHeightmap = [
		[medH, medH, medH, medH, medH, medH],
		[medH, medH, medH, medH, medH, medH],
		[medH, medH, lowH, lowH, medH, medH],
		[medH, medH, lowH, lowH, medH, medH],
		[medH, medH, medH, medH, medH, medH],
		[medH, medH, medH, medH, medH, medH],
	];
	if (g_Map.size < 256)
	{
		initialHeightmap = [
			[medH, medH, medH, medH, medH],
			[medH, medH, medH, medH, medH],
			[medH, medH, lowH, medH, medH],
			[medH, medH, medH, medH, medH],
			[medH, medH, medH, medH, medH]
		];
	}
	if (g_Map.size >= 384)
	{
		initialHeightmap = [
			[medH, medH, medH, medH, medH, medH, medH, medH],
			[medH, medH, medH, medH, medH, medH, medH, medH],
			[medH, medH, medH, medH, medH, medH, medH, medH],
			[medH, medH, medH, lowH, lowH, medH, medH, medH],
			[medH, medH, medH, lowH, lowH, medH, medH, medH],
			[medH, medH, medH, medH, medH, medH, medH, medH],
			[medH, medH, medH, medH, medH, medH, medH, medH],
			[medH, medH, medH, medH, medH, medH, medH, medH],
		];
	}

	setBaseTerrainDiamondSquare(heightRange.min, heightRange.max, initialHeightmap, 0.8);

	g_Map.log("Eroding map");
	for (let i = 0; i < 5; ++i)
		splashErodeMap(0.1);

	g_Map.log("Smoothing map");
	createArea(
		new MapBoundsPlacer(),
		new SmoothingPainter(1, 0.5, Math.ceil(g_Map.size/128) + 1));

	g_Map.log("Rescaling map");
	rescaleHeightmap(heightRange.min, heightRange.max);

	Engine.SetProgress(25);

	/**
	 * Prepare terrain texture placement
	 */
	const heighLimits = [
		// 0 Deep water
		heightRange.min + 3/4 * (heightSeaGroundAdjusted - heightRange.min),
		// 1 Shallow water
		heightSeaGroundAdjusted,
		// 2 Shore
		heightSeaGroundAdjusted + 2/8 * (heightRange.max - heightSeaGroundAdjusted),
		// 3 Low ground
		heightSeaGroundAdjusted + 3/8 * (heightRange.max - heightSeaGroundAdjusted),
		// 4 Player and path height
		heightSeaGroundAdjusted + 4/8 * (heightRange.max - heightSeaGroundAdjusted),
		// 5 High ground
		heightSeaGroundAdjusted + 6/8 * (heightRange.max - heightSeaGroundAdjusted),
		// 6 Lower forest border
		heightSeaGroundAdjusted + 7/8 * (heightRange.max - heightSeaGroundAdjusted),
		// 7 Forest
		heightRange.max
	];
	const playerHeightRange = { "min": heighLimits[3], "max": heighLimits[4] };
	const resourceSpotHeightRange = {
		"min": (heighLimits[2] + heighLimits[3]) / 2,
		"max": (heighLimits[4] + heighLimits[5]) / 2
	};
	// Average player height
	const playerHeight = (playerHeightRange.min + playerHeightRange.max) / 2;

	g_Map.log("Chosing starting locations");
	const [playerIDs, playerPosition] =
		groupPlayersCycle(getStartLocationsByHeightmap(playerHeightRange, 1000, 30));

	g_Map.log("Smoothing starting locations before height calculation");
	for (const position of playerPosition)
		createArea(
			new ClumpPlacer(diskArea(20), 0.8, 0.8, Infinity, position),
			new SmoothElevationPainter(ELEVATION_SET, g_Map.getHeight(position), 20));

	Engine.SetProgress(30);

	/**
	 * Calculate tile centered height map after start position smoothing but
	 * before placing paths
	 * This has nothing to to with TILE_CENTERED_HEIGHT_MAP which should be
	 * false!
	 */
	const tchm = getTileCenteredHeightmap();

	g_Map.log("Get points per height");
	const areas = heighLimits.map(heightLimit => []);
	for (let x = 0; x < tchm.length; ++x)
		for (let y = 0; y < tchm[0].length; ++y)
		{
			let minHeight = heightRange.min;
			for (let h = 0; h < heighLimits.length; ++h)
			{
				if (tchm[x][y] >= minHeight && tchm[x][y] <= heighLimits[h])
				{
					areas[h].push(new Vector2D(x, y));
					break;
				}

				minHeight = heighLimits[h];
			}
		}

	g_Map.log("Get slope limits per heightrange");
	const slopeMap = getSlopeMap();
	const minSlope = [];
	const maxSlope = [];
	for (let h = 0; h < heighLimits.length; ++h)
	{
		minSlope[h] = Infinity;
		maxSlope[h] = 0;
		for (const point of areas[h])
		{
			const slope = slopeMap[point.x][point.y];

			if (slope > maxSlope[h])
				maxSlope[h] = slope;

			if (slope < minSlope[h])
				minSlope[h] = slope;
		}
	}

	g_Map.log("Paint areas by height and slope");
	for (let h = 0; h < heighLimits.length; ++h)
		for (const point of areas[h])
		{
			let entity;
			let texture = pickRandom(wildLakeBiome[h].texture);

			if (slopeMap[point.x][point.y] < (minSlope[h] + maxSlope[h]) / 2)
			{
				if (randBool(wildLakeBiome[h].entity[1]))
					entity = pickRandom(wildLakeBiome[h].entity[0]);
			}
			else
			{
				texture = pickRandom(wildLakeBiome[h].textureHS);
				if (randBool(wildLakeBiome[h].entityHS[1]))
					entity = pickRandom(wildLakeBiome[h].entityHS[0]);
			}

			g_Map.setTexture(point, texture);

			if (entity)
				g_Map.placeEntityPassable(entity, 0, randomPositionOnTile(point), randomAngle());
		}
	Engine.SetProgress(40);

	g_Map.log("Placing resources");
	const avoidPoints = playerPosition.map(pos => pos.clone());
	for (let i = 0; i < avoidPoints.length; ++i)
		avoidPoints[i].dist = 30;
	const resourceSpots = getPointsByHeight(resourceSpotHeightRange, avoidPoints).map(point =>
		new Vector2D(point.x, point.y));

	Engine.SetProgress(55);

	g_Map.log("Placing players");
	if (isNomad())
		placePlayersNomad(
			g_Map.createTileClass(),
			[
				new HeightConstraint(playerHeightRange.min, playerHeightRange.max),
				avoidClasses(clGaiaCamp, 8)
			]);
	else
		for (let p = 0; p < playerIDs.length; ++p)
		{
			placeCivDefaultStartingEntities(playerPosition[p], playerIDs[p], g_Map.size > 192);
			placeStartLocationResources(playerPosition[p]);
		}

	let mercenaryCamps = isNomad() ? 0 : Math.ceil(g_Map.size / 256);
	g_Map.log("Placing at most " + mercenaryCamps + " mercenary camps");
	for (let i = 0; i < resourceSpots.length; ++i)
	{
		let radius;
		const choice = i % (isNomad() ? 4 : 5);
		if (choice == 0)
			placeMine(resourceSpots[i], g_Gaia.stoneLarge);
		if (choice == 1)
			placeMine(resourceSpots[i], g_Gaia.metalLarge);
		if (choice == 2)
			placeGrove(resourceSpots[i]);
		if (choice == 3)
		{
			placeCamp(resourceSpots[i]);
			radius = 5;
		}
		if (choice == 4)
		{
			if (mercenaryCamps)
			{
				placeStartingEntities(resourceSpots[i], 0, mercenaryCampEntities);
				radius = 15;
				--mercenaryCamps;
			}
			else
			{
				placeCustomFortress(resourceSpots[i], pickRandom(fences), "other", 0,
					randomAngle());
				radius = 10;
			}
		}

		if (radius)
			createArea(
				new DiskPlacer(radius, resourceSpots[i]),
				new SmoothElevationPainter(ELEVATION_SET, g_Map.getHeight(resourceSpots[i]),
					radius / 3));
	}

	g_Map.ExportMap();
}
