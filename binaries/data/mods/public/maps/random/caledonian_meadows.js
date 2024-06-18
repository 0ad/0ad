Engine.LoadLibrary("rmgen");
Engine.LoadLibrary("rmgen-common");
Engine.LoadLibrary("rmbiome");
Engine.LoadLibrary("heightmap");

{
	const tGrove = "temp_grass_plants";
	const tPath = "road_rome_a";

	const oGroveEntities = ["structures/gaul/outpost", "gaia/tree/oak_new"];

	globalThis.g_Map = new RandomMap(0, "whiteness");

	/**
	 * Design resource spots
	 */
	// Mines
	const decorations = [
		"actor|geology/gray1.xml",
		"actor|geology/gray_rock1.xml",
		"actor|geology/highland1.xml",
		"actor|geology/highland2.xml",
		"actor|geology/highland3.xml",
		"actor|geology/highland_c.xml",
		"actor|geology/highland_d.xml",
		"actor|geology/highland_e.xml",
		"actor|props/flora/bush.xml",
		"actor|props/flora/bush_dry_a.xml",
		"actor|props/flora/bush_highlands.xml",
		"actor|props/flora/bush_tempe_a.xml",
		"actor|props/flora/bush_tempe_b.xml",
		"actor|props/flora/ferns.xml"
	];

	function placeMine(point, centerEntity)
	{
		g_Map.placeEntityPassable(centerEntity, 0, point, randomAngle());
		const quantity = randIntInclusive(11, 23);
		const dAngle = 2 * Math.PI / quantity;

		for (let i = 0; i < quantity; ++i)
			g_Map.placeEntityPassable(
				pickRandom(decorations),
				0,
				Vector2D.add(point,
					new Vector2D(randFloat(2, 5), 0).rotate(-dAngle * randFloat(i, i + 1))),
				randomAngle());
	}

	// Food, fences with domestic animals
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
		"sheep": {
			"angle": 0,
			"length": 0,
			"indent": 0.75,
			"bend": 0,
			"templateName": "gaia/fauna_sheep"
		},
		"foodBin": {
			"angle": Math.PI / 2,
			"length": 1.5,
			"indent": 0,
			"bend": 0,
			"templateName": "gaia/treasure/food_bin"
		},
		"farmstead": {
			"angle": Math.PI,
			"length": 0,
			"indent": -3,
			"bend": 0,
			"templateName": "structures/brit/farmstead"
		}
	};

	const fences = [
		new Fortress("fence", [
			"foodBin", "farmstead", "bench",
			"turn_0.25", "sheep", "turn_0.25", "fence",
			"turn_0.25", "sheep", "turn_0.25", "fence",
			"turn_0.25", "sheep", "turn_0.25", "fence"
		]),
		new Fortress("fence", [
			"foodBin", "farmstead", "fence",
			"turn_0.25", "sheep", "turn_0.25", "fence",
			"turn_0.25", "sheep", "turn_0.25", "bench", "sheep", "fence",
			"turn_0.25", "sheep", "turn_0.25", "fence"
		]),
		new Fortress("fence", [
			"foodBin", "farmstead", "turn_0.5", "bench", "turn_-0.5", "fence_short",
			"turn_0.25", "sheep", "turn_0.25", "fence",
			"turn_0.25", "sheep", "turn_0.25", "fence",
			"turn_0.25", "sheep", "turn_0.25", "fence_short", "sheep", "fence"
		]),
		new Fortress("fence", [
			"foodBin", "farmstead", "turn_0.5", "fence_short", "turn_-0.5", "bench",
			"turn_0.25", "sheep", "turn_0.25", "fence",
			"turn_0.25", "sheep", "turn_0.25", "fence",
			"turn_0.25", "sheep", "turn_0.25", "fence_short", "sheep", "fence"
		]),
		new Fortress("fence", [
			"foodBin", "farmstead", "fence",
			"turn_0.25", "sheep", "turn_0.25", "bench", "sheep", "fence",
			"turn_0.25", "sheep", "turn_0.25", "fence_short", "sheep", "fence",
			"turn_0.25", "sheep", "turn_0.25", "fence_short", "sheep", "fence"
		])
	].flatMap(fence => [fence, new Fortress("fence", clone(fence.wall).reverse())]);

	// Groves, only wood
	const groveEntities = ["gaia/tree/bush_temperate", "gaia/tree/euro_beech"];
	const groveActors = [
		"actor|geology/highland1_moss.xml",
		"actor|geology/highland2_moss.xml",
		"actor|props/flora/bush.xml",
		"actor|props/flora/bush_dry_a.xml",
		"actor|props/flora/bush_highlands.xml",
		"actor|props/flora/bush_tempe_a.xml",
		"actor|props/flora/bush_tempe_b.xml",
		"actor|props/flora/ferns.xml"
	];

	function placeGrove(point)
	{
		g_Map.placeEntityPassable(pickRandom(oGroveEntities), 0, point, randomAngle());
		const quantity = randIntInclusive(20, 30);
		const dAngle = 2 * Math.PI / quantity;
		for (let i = 0; i < quantity; ++i)
		{
			const angle = dAngle * randFloat(i, i + 1);
			const dist = randFloat(2, 5);
			let objectList = groveEntities;
			if (i % 3 == 0)
				objectList = groveActors;
			const position = Vector2D.add(point, new Vector2D(dist, 0).rotate(-angle));
			g_Map.placeEntityPassable(pickRandom(objectList), 0, position, randomAngle());
			createArea(
				new ClumpPlacer(5, 1, 1, Infinity, position),
				new TerrainPainter(tGrove));
		}
	}

	// Camps with fire and gold treasure
	function placeCamp(point)
	{
		const centerEntity = "actor|props/special/eyecandy/campfire.xml";
		const otherEntities = [
			"gaia/treasure/metal",
			"gaia/treasure/standing_stone",
			"units/brit/infantry_slinger_b",
			"units/brit/infantry_javelineer_b",
			"units/gaul/infantry_slinger_b",
			"units/gaul/infantry_javelineer_b",
			"units/gaul/champion_fanatic",
			"actor|props/special/common/waypoint_flag.xml",
			"actor|props/special/eyecandy/barrel_a.xml",
			"actor|props/special/eyecandy/basket_celt_a.xml",
			"actor|props/special/eyecandy/crate_a.xml",
			"actor|props/special/eyecandy/dummy_a.xml",
			"actor|props/special/eyecandy/handcart_1.xml",
			"actor|props/special/eyecandy/handcart_1_broken.xml",
			"actor|props/special/eyecandy/sack_1.xml",
			"actor|props/special/eyecandy/sack_1_rough.xml"
		];
		g_Map.placeEntityPassable(centerEntity, 0, point, randomAngle());
		const quantity = randIntInclusive(5, 11);
		const dAngle = 2 * Math.PI / quantity;
		for (let i = 0; i < quantity; ++i)
		{
			const angle = dAngle * randFloat(i, i + 1);
			const dist = randFloat(1, 3);
			g_Map.placeEntityPassable(pickRandom(otherEntities), 0,
				Vector2D.add(point, new Vector2D(dist, 0).rotate(-angle)), randomAngle());
		}
	}

	function placeStartLocationResources(point)
	{
		const foodEntities = ["gaia/fruit/berry_01", "gaia/fauna_chicken", "gaia/fauna_chicken"];
		let currentAngle = randomAngle();
		// Stone and chicken
		let dAngle = 4/9 * Math.PI;
		let angle = currentAngle + randFloat(1, 3) * dAngle / 4;
		const stonePosition = Vector2D.add(point, new Vector2D(12, 0).rotate(-angle));
		placeMine(stonePosition, "gaia/rock/temperate_large");
		currentAngle += dAngle;

		// Wood
		let quantity = 80;
		dAngle = 2 * Math.PI / quantity / 3;
		for (let i = 0; i < quantity; ++i)
		{
			angle = currentAngle + randFloat(0, dAngle);
			let objectList = groveEntities;
			if (i % 2 == 0)
				objectList = groveActors;
			const woodPosition =
				Vector2D.add(point, new Vector2D(randFloat(10, 15), 0).rotate(-angle));
			g_Map.placeEntityPassable(pickRandom(objectList), 0, woodPosition, randomAngle());
			createArea(
				new ClumpPlacer(5, 1, 1, Infinity, woodPosition),
				new TerrainPainter("temp_grass_plants"));
			currentAngle += dAngle;
		}

		// Metal and chicken
		dAngle = 2 * Math.PI * 2 / 9;
		angle = currentAngle + dAngle * randFloat(1, 3) / 4;
		const metalPosition = Vector2D.add(point, new Vector2D(13, 0).rotate(-angle));
		placeMine(metalPosition, "gaia/ore/temperate_large");
		currentAngle += dAngle;

		// Berries
		quantity = 15;
		dAngle = 2 * Math.PI / quantity * 2 / 9;
		for (let i = 0; i < quantity; ++i)
		{
			angle = currentAngle + randFloat(0, dAngle);
			const berriesPosition =
				Vector2D.add(point, new Vector2D(randFloat(10, 15), 0).rotate(-angle));
			g_Map.placeEntityPassable(pickRandom(foodEntities), 0, berriesPosition, randomAngle());
			currentAngle += dAngle;
		}
	}

	/**
	 * Environment settings
	 */
	setBiome("generic/alpine");
	g_Environment.Fog.FogColor = { "r": 0.8, "g": 0.8, "b": 0.8, "a": 0.01 };
	g_Environment.Water.WaterBody.Colour = { "r": 0.3, "g": 0.05, "b": 0.1, "a": 0.1 };
	g_Environment.Water.WaterBody.Murkiness = 0.4;

	/**
	 * Base terrain shape generation and settings
	 */
	const heightScale = (g_Map.size + 256) / 768 / 4;
	const heightRange = { "min": MIN_HEIGHT * heightScale, "max": MAX_HEIGHT * heightScale };

	// Water coverage
	// NOTE: Since terrain generation is quite unpredictable actual water
	// coverage might vary much with the same value
	const averageWaterCoverage = 1 / 5;
	// Water height in environment and the engine
	const heightSeaGround = -MIN_HEIGHT + heightRange.min + averageWaterCoverage *
		(heightRange.max - heightRange.min);
	// Water height in RMGEN
	const heightSeaGroundAdjusted = heightSeaGround + MIN_HEIGHT;
	setWaterHeight(heightSeaGround);

	g_Map.log("Generating terrain using diamon-square");
	const medH = (heightRange.min + heightRange.max) / 2;
	const initialHeightmap = [[medH, medH], [medH, medH]];
	setBaseTerrainDiamondSquare(heightRange.min, heightRange.max, initialHeightmap, 0.8);

	g_Map.log("Apply erosion");
	for (let i = 0; i < 5; ++i)
		splashErodeMap(0.1);

	rescaleHeightmap(heightRange.min, heightRange.max);

	Engine.SetProgress(25);

	const heighLimits = [
		// 0 Deep water
		[true, 1 / 3],
		// 1 Medium Water
		[true, 2 / 3],
		// 2 Shallow water
		[true, 3 / 3],
		// 3 Shore
		[false, 1 / 8],
		// 4 Low ground
		[false, 2 / 8],
		// 5 Player and path height
		[false, 3 / 8],
		// 6 High ground
		[false, 4 / 8],
		// 7 Lower forest border
		[false, 5 / 8],
		// 8 Forest
		[false, 6 / 8],
		// 9 Upper forest border
		[false, 7 / 8],
		// 10 Hilltop
		[false, 8 / 8]
	].map(([underWater, ratio]) => {
		const base = underWater ? heightRange.min : heightSeaGround;
		const factor = underWater ? heightSeaGroundAdjusted - heightRange.min :
			heightRange.max - heightSeaGroundAdjusted;
		return base + ratio * factor;
	});

	const playerHeight = (heighLimits[4] + heighLimits[5]) / 2; // Average player height

	g_Map.log("Determining height-dependent biome");
	// Texture and actor presets
	const myBiome = [
		// 0 Deep water
		{
			"flat": {
				"texture": ["shoreline_stoney_a"],
				"entity": ["gaia/fish/generic", "actor|geology/stone_granite_boulder.xml"],
				"entityPropability": 0.02
			},
			"steep": {
				"texture": ["alpine_mountainside"],
				"entity": ["gaia/fish/generic"],
				"entityPropability": 0.1
			}
		},

		// 1 Medium Water
		{
			"flat": {
				"texture": ["shoreline_stoney_a", "alpine_shore_rocks"],
				"entity":
					[
						"actor|geology/stone_granite_boulder.xml",
						"actor|geology/stone_granite_med.xml"
					],
				"entityPropability": 0.03
			},
			"steep": {
				"texture": ["alpine_mountainside"],
				"entity":
					[
						"actor|geology/stone_granite_boulder.xml",
						"actor|geology/stone_granite_med.xml"
					],
				"entityPropability": 0.0
			}
		},

		// 2 Shallow water
		{
			"flat": {
				"texture": ["alpine_shore_rocks"],
				"entity": [
					"actor|props/flora/reeds_pond_dry.xml",
					"actor|geology/stone_granite_large.xml",
					"actor|geology/stone_granite_med.xml",
					"actor|props/flora/reeds_pond_lush_b.xml"
				],
				"entityPropability": 0.2
			},
			"steep": {
				"texture": ["alpine_mountainside"],
				"entity":
					[
						"actor|props/flora/reeds_pond_dry.xml",
						"actor|geology/stone_granite_med.xml"
					],
				"entityPropability": 0.1
			}
		},

		// 3 Shore
		{
			"flat": {
				"texture": ["alpine_shore_rocks_grass_50", "alpine_grass_rocky"],
				"entity": [
					"gaia/tree/pine",
					"gaia/tree/bush_badlands",
					"actor|geology/highland1_moss.xml",
					"actor|props/flora/grass_soft_tuft_a.xml",
					"actor|props/flora/bush.xml"
				],
				"entityPropability": 0.3
			},
			"steep": {
				"texture": ["alpine_mountainside"],
				"entity": ["actor|props/flora/grass_soft_tuft_a.xml"],
				"entityPropability": 0.1
			}
		},

		// 4 Low ground
		{
			"flat": {
				"texture": ["alpine_dirt_grass_50", "alpine_grass_rocky"],
				"entity": [
					"actor|geology/stone_granite_med.xml",
					"actor|props/flora/grass_soft_tuft_a.xml",
					"actor|props/flora/bush.xml",
					"actor|props/flora/grass_medit_flowering_tall.xml"
				],
				"entityPropability": 0.2
			},
			"steep": {
				"texture": ["alpine_grass_rocky"],
				"entity":
					[
						"actor|geology/stone_granite_med.xml",
						"actor|props/flora/grass_soft_tuft_a.xml"
					],
				"entityPropability": 0.1
			}
		},

		// 5 Player and path height
		{
			"flat": {
				"texture": ["new_alpine_grass_c", "new_alpine_grass_b", "new_alpine_grass_d"],
				"entity": [
					"actor|geology/stone_granite_small.xml",
					"actor|props/flora/grass_soft_small.xml",
					"actor|props/flora/grass_medit_flowering_tall.xml"
				],
				"entityPropability": 0.2
			},
			"steep": {
				"texture": ["alpine_grass_rocky"],
				"entity":
					[
						"actor|geology/stone_granite_small.xml",
						"actor|props/flora/grass_soft_small.xml"
					],
				"entityPropability": 0.1
			}
		},

		// 6 High ground
		{
			"flat": {
				"texture": ["new_alpine_grass_a", "alpine_grass_rocky"],
				"entity": [
					"actor|geology/stone_granite_med.xml",
					"actor|props/flora/grass_tufts_a.xml",
					"actor|props/flora/bush_highlands.xml",
					"actor|props/flora/grass_medit_flowering_tall.xml"
				],
				"entityPropability": 0.2
			},
			"steep": {
				"texture": ["alpine_grass_rocky"],
				"entity":
					[
						"actor|geology/stone_granite_med.xml",
						"actor|props/flora/grass_tufts_a.xml"
					],
				"entityPropability": 0.1
			}
		},

		// 7 Lower forest border
		{
			"flat": {
				"texture": ["new_alpine_grass_mossy", "alpine_grass_rocky"],
				"entity": [
					"gaia/tree/pine",
					"gaia/tree/oak",
					"actor|props/flora/grass_tufts_a.xml",
					"gaia/fruit/berry_01",
					"actor|geology/highland2_moss.xml",
					"gaia/fauna_goat",
					"actor|props/flora/bush_tempe_underbrush.xml"
				],
				"entityPropability": 0.3
			},
			"steep": {
				"texture": ["alpine_cliff_c"],
				"entity":
					[
						"actor|props/flora/grass_tufts_a.xml",
						"actor|geology/highland2_moss.xml"
					],
				"entityPropability": 0.1
			}
		},

		// 8 Forest
		{
			"flat": {
				"texture": ["alpine_forrestfloor"],
				"entity": [
					"gaia/tree/pine",
					"gaia/tree/pine",
					"gaia/tree/pine",
					"gaia/tree/pine",
					"actor|geology/highland2_moss.xml",
					"actor|props/flora/bush_highlands.xml"
				],
				"entityPropability": 0.5
			},
			"steep": {
				"texture": ["alpine_cliff_c"],
				"entity": [
					"actor|geology/highland2_moss.xml", "actor|geology/stone_granite_med.xml"
				],
				"entityPropability": 0.1
			}
		},

		// 9 Upper forest border
		{
			"flat": {
				"texture": ["alpine_forrestfloor_snow", "new_alpine_grass_dirt_a"],
				"entity": ["gaia/tree/pine", "actor|geology/snow1.xml"],
				"entityPropability": 0.3
			},
			"steep": {
				"texture": ["alpine_cliff_b"],
				"entity": ["actor|geology/stone_granite_med.xml", "actor|geology/snow1.xml"],
				"entityPropability": 0.1
			}
		},

		// 10 Hilltop
		{
			"flat": {
				"texture": ["alpine_cliff_a", "alpine_cliff_snow"],
				"entity": ["actor|geology/highland1.xml"],
				"entityPropability": 0.05
			},
			"steep": {
				"texture": ["alpine_cliff_c"],
				"entity": ["actor|geology/highland1.xml"],
				"entityPropability": 0.0
			}
		}
	];

	const [playerIDs, playerPosition] = groupPlayersCycle(
		getStartLocationsByHeightmap({ "min": heighLimits[4], "max": heighLimits[5] }, 1000, 30));
	Engine.SetProgress(30);

	g_Map.log("Smoothing player locations");
	for (const position of playerPosition)
		createArea(
			new DiskPlacer(35, position),
			new SmoothElevationPainter(ELEVATION_SET, g_Map.getHeight(position), 35));

	g_Map.log("Creating paths between players");
	const clPath = g_Map.createTileClass();
	for (let i = 0; i < playerPosition.length; ++i)
		createArea(
			new RandomPathPlacer(playerPosition[i],
				playerPosition[(i + 1) % playerPosition.length], 4, 2, false),
			[
				new TerrainPainter(tPath),
				new ElevationBlendingPainter(playerHeight, 0.4),
				new TileClassPainter(clPath)
			]);

	g_Map.log("Smoothing paths");
	createArea(
		new MapBoundsPlacer(),
		new SmoothingPainter(5, 1, 1),
		new NearTileClassConstraint(clPath, 5));

	Engine.SetProgress(45);

	g_Map.log("Determining resource locations");
	const avoidPoints = playerPosition.map(pos => pos.clone());
	avoidPoints.forEach(point => { point.dist = 30; });
	const resourceSpots = getPointsByHeight(
		{
			"min": (heighLimits[3] + heighLimits[4]) / 2,
			"max": (heighLimits[5] + heighLimits[6]) / 2
		},
		avoidPoints,
		clPath);
	Engine.SetProgress(55);

	/**
	 * Divide tiles in areas by height and avoid paths
	 */
	const tchm = getTileCenteredHeightmap();
	const areas = heighLimits.map(heightLimit => []);
	for (let x = 0; x < tchm.length; ++x)
		for (let y = 0; y < tchm[0].length; ++y)
		{
			const position = new Vector2D(x, y);
			if (!avoidClasses(clPath, 0).allows(position) || tchm[x][y] < heightRange.min)
				continue;

			const index = heighLimits.findIndex(limit => tchm[x][y] <= limit);
			if (index !== -1)
				areas[index].push(position);
		}

	/**
	 * Get midpoint slope of each area
	 */
	const slopeMap = getSlopeMap();
	const slopeMidpoints = areas.map(area => {
		const slopesInThisArea = area.map(({ x, y }) => slopeMap[x][y]);
		return Math.min(...slopesInThisArea) + Math.max(...slopesInThisArea);
	});

	g_Map.log("Painting areas by height and slope");
	for (let h = 0; h < heighLimits.length; ++h)
		for (const point of areas[h])
		{
			const isFlat = slopeMap[point.x][point.y] < 0.4 * slopeMidpoints[h];
			const selectedBiome = myBiome[h][isFlat? "flat" : "steep"];

			g_Map.setTexture(point, pickRandom(selectedBiome.texture));

			if (randBool(selectedBiome.entityPropability))
				g_Map.placeEntityPassable(pickRandom(selectedBiome.entity), 0,
					randomPositionOnTile(point), randomAngle());
		}
	Engine.SetProgress(80);

	g_Map.log("Placing players");
	if (isNomad())
		placePlayersNomad(g_Map.createTileClass(),
			new HeightConstraint(heighLimits[4], heighLimits[5]));
	else
		for (let p = 0; p < playerIDs.length; ++p)
		{
			placeCivDefaultStartingEntities(playerPosition[p], playerIDs[p], true);
			placeStartLocationResources(playerPosition[p]);
		}

	g_Map.log("Placing resources, farmsteads, groves and camps");
	for (let i = 0; i < resourceSpots.length; ++i)
	{
		const pos = new Vector2D(resourceSpots[i].x, resourceSpots[i].y);
		const choice = i % (isNomad() ? 4 : 5);
		if (choice == 0)
			placeMine(pos, "gaia/rock/temperate_large_02");
		if (choice == 1)
			placeMine(pos, "gaia/ore/temperate_large");
		if (choice == 2)
			placeCustomFortress(pos, pickRandom(fences), "other", 0, randomAngle());
		if (choice == 3)
			placeGrove(pos);
		if (choice == 4)
			placeCamp(pos);
	}

	g_Map.ExportMap();
}
