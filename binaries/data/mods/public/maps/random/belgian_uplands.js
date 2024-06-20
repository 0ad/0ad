Engine.LoadLibrary("rmgen");
Engine.LoadLibrary("rmgen-common");
Engine.LoadLibrary("heightmap");

{
	const tPrimary = ["temp_grass", "temp_grass_b", "temp_grass_c", "temp_grass_d",
		"temp_grass_long_b", "temp_grass_clovers_2", "temp_grass_mossy", "temp_grass_plants"];

	globalThis.g_Map = new RandomMap(0, tPrimary);
	const mapSize = g_Map.getSize();
	const mapCenter = g_Map.getCenter();

	// Set target min and max height depending on map size to make average
	// stepness the same on all map sizes
	const heightRange = { "min": MIN_HEIGHT * mapSize / 8192, "max": MAX_HEIGHT * mapSize / 8192 };

	// Since erosion is not predictable, actual water coverage can differ much
	// with the same value
	const averageWaterCoverage = scaleByMapSize(1/5, 1/3);

	const heightSeaGround = -MIN_HEIGHT + heightRange.min + averageWaterCoverage *
		(heightRange.max - heightRange.min);
	const heightSeaGroundAdjusted = heightSeaGround + MIN_HEIGHT;
	setWaterHeight(heightSeaGround);

	const terrainTypes = [
		// Deep water
		{
			"upperHeightLimit": heightRange.min + 1/3 * (heightSeaGroundAdjusted - heightRange.min),
			"terrain": "temp_sea_rocks",
			"actors": [
				[100, "actor|props/flora/pond_lillies_large.xml"],
				[40, "actor|props/flora/water_lillies.xml"]
			]
		},

		// Medium deep water (with fish)
		{
			"upperHeightLimit": heightRange.min + 2/3 * (heightSeaGroundAdjusted - heightRange.min),
			"terrain": [
				Array(25).fill("temp_sea_weed"),
				"temp_sea_weed|gaia/fish/generic"
			].flat(),
			"actors": [
				[200, "actor|props/flora/pond_lillies_large.xml"],
				[100, "actor|props/flora/water_lillies.xml"]
			]
		},

		// Flat Water
		{
			"upperHeightLimit": heightRange.min + 3/3 * (heightSeaGroundAdjusted - heightRange.min),
			"terrain": "temp_mud_a",
			"actors": [
				[200, "actor|props/flora/water_log.xml"],
				[100, "actor|props/flora/water_lillies.xml"],
				[40, "actor|geology/highland_c.xml"],
				[20, "actor|props/flora/reeds_pond_lush_b.xml"],
				[10, "actor|props/flora/reeds_pond_lush_a.xml"]
			]
		},

		// Water surroundings/bog (with stone/metal some rabits and bushes)
		{
			"upperHeightLimit": heightSeaGroundAdjusted + 1/6 *
				(heightRange.max - heightSeaGroundAdjusted),
			"terrain": [
				Array(48).fill([
					"temp_plants_bog",
					"temp_plants_bog_aut",
					"temp_dirt_gravel_plants",
					"temp_grass_d"
				]),
				Array(4).fill("temp_plants_bog|gaia/tree/bush_temperate"),
				Array(2).fill([
					"temp_dirt_gravel_plants|gaia/ore/temperate_small",
					"temp_dirt_gravel_plants|gaia/rock/temperate_small",
					"temp_plants_bog|gaia/fauna_rabbit"
				]),
				"temp_plants_bog_aut|gaia/tree/dead"
			].flat(2),
			"actors": [
				[200, "actor|props/flora/water_log.xml"],
				[100, "actor|geology/highland_c.xml"],
				[40, "actor|props/flora/reeds_pond_lush_a.xml"]
			]
		},

		// Juicy grass near bog
		{
			"upperHeightLimit": heightSeaGroundAdjusted + 2/6 *
				(heightRange.max - heightSeaGroundAdjusted),
			"terrain": [
				"temp_grass",
				"temp_grass_d",
				"temp_grass_long_b",
				"temp_grass_plants"
			],
			"actors": [
				[800, "actor|props/flora/grass_field_flowering_tall.xml"],
				[400, "actor|geology/gray_rock1.xml"],
				[200, "actor|props/flora/bush_tempe_sm_lush.xml"],
				[100, "actor|props/flora/bush_tempe_b.xml"],
				[40, "actor|props/flora/grass_soft_small_tall.xml"]
			]
		},

		// Medium level grass
		{
			"upperHeightLimit": heightSeaGroundAdjusted + 3/6 *
				(heightRange.max - heightSeaGroundAdjusted),
			"terrain": [
				"temp_grass",
				"temp_grass_b",
				"temp_grass_c",
				"temp_grass_mossy"
			],
			"actors": [
				[800, "actor|geology/decal_stone_medit_a.xml"],
				[400, "actor|props/flora/decals_flowers_daisies.xml"],
				[200, "actor|props/flora/bush_tempe_underbrush.xml"],
				[100, "actor|props/flora/grass_soft_small_tall.xml"],
				[40, "actor|props/flora/grass_temp_field.xml"]
			]
		},

		// Long grass near forest border
		{
			"upperHeightLimit": heightSeaGroundAdjusted + 4/6 *
				(heightRange.max - heightSeaGroundAdjusted),
			"terrain": [
				"temp_grass",
				"temp_grass_b",
				"temp_grass_c",
				"temp_grass_d",
				"temp_grass_long_b",
				"temp_grass_clovers_2",
				"temp_grass_mossy",
				"temp_grass_plants"
			],
			"actors": [
				[400, "actor|geology/stone_granite_boulder.xml"],
				[200, "actor|props/flora/foliagebush.xml"],
				[100, "actor|props/flora/bush_tempe_underbrush.xml"],
				[40, "actor|props/flora/grass_soft_small_tall.xml"],
				[20, "actor|props/flora/ferns.xml"]
			]
		},

		// Forest border (With wood/food plants/deer/rabits)
		{
			"upperHeightLimit": heightSeaGroundAdjusted + 5/6 *
				(heightRange.max - heightSeaGroundAdjusted),
			"terrain": [
				"temp_grass_plants|gaia/tree/euro_beech",
				"temp_grass_mossy|gaia/tree/poplar",
				"temp_grass_mossy|gaia/tree/poplar_lombardy",
				"temp_grass_long|gaia/tree/bush_temperate",
				"temp_mud_plants|gaia/tree/bush_temperate",
				"temp_mud_plants|gaia/tree/bush_badlands",
				"temp_grass_long|gaia/fruit/apple",
				"temp_grass_clovers|gaia/fruit/berry_01",
				"temp_grass_clovers_2|gaia/fruit/grapes",
				"temp_grass_plants|gaia/fauna_deer",
				"temp_grass_long_b|gaia/fauna_rabbit"
			].flatMap(t => [t, "temp_grass_plants"]),
			"actors": [
				[400, "actor|geology/highland_c.xml"],
				[200, "actor|props/flora/bush_tempe_a.xml"],
				[100, "actor|props/flora/ferns.xml"],
				[40, "actor|props/flora/grass_soft_tuft_a.xml"]
			]
		},

		// Unpassable woods
		{
			"upperHeightLimit": heightSeaGroundAdjusted + 6/6 *
				(heightRange.max - heightSeaGroundAdjusted),
			"terrain": [
				"temp_grass_mossy|gaia/tree/oak",
				"temp_forestfloor_pine|gaia/tree/pine",
				"temp_grass_mossy|gaia/tree/oak",
				"temp_forestfloor_pine|gaia/tree/pine",
				"temp_mud_plants|gaia/tree/dead",
				"temp_plants_bog|gaia/tree/oak_large",
				"temp_dirt_gravel_plants|gaia/tree/aleppo_pine",
				"temp_forestfloor_autumn|gaia/tree/carob"
			],
			"actors": [
				[200, "actor|geology/highland2_moss.xml"],
				[100, "actor|props/flora/grass_soft_tuft_a.xml"],
				[40, "actor|props/flora/ferns.xml"]
			]
		}
	];

	Engine.SetProgress(5);

	const lowerHeightLimit = terrainTypes[3].upperHeightLimit;
	const upperHeightLimit = terrainTypes[6].upperHeightLimit;

	const [playerIDs, playerPosition] = (() => {
		while (true)
		{
			g_Map.log("Randomizing heightmap");
			createArea(
				new MapBoundsPlacer(),
				new RandomElevationPainter(heightRange.min, heightRange.max));

			// More cycles yield bigger structures
			g_Map.log("Smoothing map");
			createArea(
				new MapBoundsPlacer(),
				new SmoothingPainter(2, 1, 20));

			g_Map.log("Rescaling map");
			rescaleHeightmap(heightRange.min, heightRange.max, g_Map.height);

			g_Map.log("Mark valid heightrange for player starting positions");
			const tHeightRange = g_Map.createTileClass();
			const area = createArea(
				new DiskPlacer(fractionToTiles(0.5) - MAP_BORDER_WIDTH, mapCenter),
				new TileClassPainter(tHeightRange),
				new HeightConstraint(lowerHeightLimit, upperHeightLimit));

			const players = area &&
				playerPlacementRandom(sortAllPlayers(), stayClasses(tHeightRange, 15), true);
			if (players)
				return players;

			g_Map.log("Too few starting locations");
		}
	})();

	Engine.SetProgress(60);

	g_Map.log("Painting terrain by height and add props");
	// 1 means as determined in the loop, less for large maps as set below
	const propDensity = mapSize > 500 ? 1 / 4 : mapSize > 400 ? 3 / 4 : 1;

	for (let x = 0; x < mapSize; ++x)
		for (let y = 0; y < mapSize; ++y)
		{
			const position = new Vector2D(x, y);
			if (!g_Map.validHeight(position) || g_Map.getHeight(position) < heightRange.min)
				continue;

			const elem = terrainTypes.find(e => g_Map.getHeight(position) <= e.upperHeightLimit);
			createTerrain(elem.terrain).place(position);
			const template = elem.actors.find(([propability]) => randBool(propDensity / propability));
			if (template)
				g_Map.placeEntityAnywhere(template[1], 0, position, randomAngle());
		}

	Engine.SetProgress(90);

	if (isNomad())
		placePlayersNomad(g_Map.createTileClass(),
			new HeightConstraint(lowerHeightLimit, upperHeightLimit));
	else
	{
		g_Map.log("Placing players and starting resources");

		const resourceDistance = 8;
		const resourceSpacing = 1;
		const resourceCount = 4;

		playerPosition.forEach((position, i) => {
			placeCivDefaultStartingEntities(position, playerIDs[i], false);

			for (let j = 1; j <= 4; ++j)
			{
				const uAngle = BUILDING_ORIENTATION - Math.PI * (2 - j) / 2;
				for (let k = 0; k < resourceCount; ++k)
					g_Map.placeEntityPassable(
						j % 2 ? "gaia/tree/cypress" : "gaia/fruit/berry_01",
						0,
						Vector2D.sum([
							position,
							new Vector2D(resourceDistance, 0).rotate(-uAngle),
							new Vector2D(k * resourceSpacing, 0)
								.rotate(-uAngle - Math.PI / 2),
							new Vector2D(
								-0.75 * resourceSpacing *
									Math.floor(resourceCount / 2),
								0).rotate(-uAngle - Math.PI/2)
						]),
						randomAngle());
			}
		});
	}

	g_Map.ExportMap();
}
