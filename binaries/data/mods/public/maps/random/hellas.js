/**
 * Heightmap image source:
 * Imagery by Jesse Allen, NASA's Earth Observatory,
 * using data from the General Bathymetric Chart of the Oceans (GEBCO)
 * produced by the British Oceanographic Data Centre.
 * https://visibleearth.nasa.gov/view.php?id=73934
 *
 * Licensing: Public Domain, https://visibleearth.nasa.gov/useterms.php
 *
 * The heightmap image is reproduced using:
 * wget https://eoimages.gsfc.nasa.gov/images/imagerecords/73000/73934/gebco_08_rev_elev_C1_grey_geo.tif
 * lat=37; lon=23; width=7; # including crete
 * gdal_translate -projwin $((lon-width/2)) $((lat+width/2)) $((lon+width/2)) $((lat-width/2)) gebco_08_rev_elev_C1_grey_geo.tif hellas.tif
 * convert hellas.tif -contrast-stretch 0 hellas.png
 * No further changes should be applied to the image to keep it easily interchangeable.
 */

Engine.LoadLibrary("rmgen");
Engine.LoadLibrary("rmgen-common");

function* GenerateMap(mapSettings)
{
	TILE_CENTERED_HEIGHT_MAP = true;

	const mapStyles = [
		// mainland
		{
			"minMapSize": 0,
			"enabled": randBool(0.15),
			"landRatio": [0.95, 1]
		},
		// lots of water
		{
			"minMapSize": 384,
			"enabled": randBool(1/4),
			"landRatio": [0.3, 0.5]
		},
		// few water
		{
			"minMapSize": 192,
			"enabled": true,
			"landRatio": [0.65, 0.9]
		}
	];

	const heightmapHellas = convertHeightmap1Dto2D(Engine.LoadHeightmapImage("maps/random/hellas.png"));
	const biomes = Engine.ReadJSONFile("maps/random/hellas_biomes.json");

	const heightScale = num => num * mapSettings.Size / 320;

	const heightSeaGround = heightScale(-6);
	const heightReedsMin = heightScale(-2);
	const heightReedsMax = heightScale(-0.5);
	const heightShoreline = heightScale(1);
	const heightLowlands = heightScale(30);
	const heightHighlands = heightScale(60);

	const heightmapMin = 0;
	const heightmapMax = 100;

	globalThis.g_Map = new RandomMap(0, biomes.lowlands.terrains.main);
	const mapSize = g_Map.getSize();
	const mapCenter = g_Map.getCenter();
	const numPlayers = getNumPlayers();

	let clWater = g_Map.createTileClass();
	let clCliffs = g_Map.createTileClass();
	const clPlayer = g_Map.createTileClass();
	const clForest = g_Map.createTileClass();
	const clDirt = g_Map.createTileClass();
	const clRock = g_Map.createTileClass();
	const clMetal = g_Map.createTileClass();
	const clFood = g_Map.createTileClass();
	const clBaseResource = g_Map.createTileClass();
	const clDock = g_Map.createTileClass();

	const constraintLowlands = new HeightConstraint(heightShoreline, heightLowlands);
	const constraintHighlands = new HeightConstraint(heightLowlands, heightHighlands);
	const constraintMountains = new HeightConstraint(heightHighlands, Infinity);

	const [minLandRatio, maxLandRatio] = mapStyles.filter(mapStyle => mapSize >= mapStyle.minMapSize)
		.sort((a, b) => a.enabled - b.enabled).pop().landRatio;
	const [minCliffRatio, maxCliffRatio] = [maxLandRatio < 0.75 ? 0 : 0.08, 0.18];

	let playerIDs = sortAllPlayers();
	let playerPosition;

	// Pick a random subset of the heightmap that meets the mapStyle and has space for all players
	let subAreaSize;
	let subAreaTopLeft;
	while (true)
	{
		subAreaSize = Math.floor(randFloat(0.01, 0.2) * heightmapHellas.length);
		subAreaTopLeft = new Vector2D(randFloat(0, 1), randFloat(0, 1))
			.mult(heightmapHellas.length - subAreaSize).floor();

		const heightmap = extractHeightmap(heightmapHellas, subAreaTopLeft, subAreaSize);
		const heightmapPainter = new HeightmapPainter(heightmap, heightmapMin, heightmapMax);

		// Quick area test
		const points = new DiskPlacer(heightmap.length / 2 - MAP_BORDER_WIDTH,
			new Vector2D(1, 1).mult(heightmap.length / 2)).place(new NullConstraint());
		let landArea = 0;
		for (const point of points)
			if (heightmapPainter.scaleHeight(heightmap[point.x][point.y]) > heightShoreline)
				++landArea;

		let landRatio = landArea / points.length;
		g_Map.log("Chosen heightmap at " + uneval(subAreaTopLeft) + " of size " + subAreaSize +
			", land-ratio: " + landRatio.toFixed(3));
		if (landRatio < minLandRatio || landRatio > maxLandRatio)
			continue;

		g_Map.log("Copying heightmap");
		createArea(
			new MapBoundsPlacer(),
			heightmapPainter);

		g_Map.log("Measuring land area");
		const passableLandArea = createArea(
			new DiskPlacer(fractionToTiles(0.5), mapCenter),
			undefined,
			new HeightConstraint(heightShoreline, Infinity));

		if (!passableLandArea)
			continue;

		landRatio = passableLandArea.getPoints().length / diskArea(fractionToTiles(0.5));
		g_Map.log("Land ratio: " + landRatio.toFixed(3));
		if (landRatio < minLandRatio || landRatio > maxLandRatio)
			continue;

		g_Map.log("Lowering sea ground");
		clWater = g_Map.createTileClass();
		createArea(
			new MapBoundsPlacer(),
			[
				new SmoothElevationPainter(ELEVATION_SET, heightSeaGround, 5),
				new TileClassPainter(clWater)
			],
			new HeightConstraint(-Infinity, heightShoreline));

		let cliffsRatio;
		while (true)
		{
			createArea(
				new DiskPlacer(fractionToTiles(0.5) - MAP_BORDER_WIDTH, mapCenter),
				new SmoothingPainter(1, 0.5, 1));
			yield 25;

			clCliffs = g_Map.createTileClass();

			// Marking cliffs
			const cliffsArea = createArea(
				new MapBoundsPlacer(),
				new TileClassPainter(clCliffs),
				[
					avoidClasses(clWater, 2),
					new SlopeConstraint(2, Infinity)
				]);

			cliffsRatio = cliffsArea.getPoints().length / Math.square(g_Map.getSize());
			g_Map.log("Smoothing heightmap, cliff ratio: " + cliffsRatio.toFixed(3));
			if (cliffsRatio < maxCliffRatio)
				break;
		}

		if (cliffsRatio < minCliffRatio)
		{
			g_Map.log("Too few cliffs: " + cliffsRatio);
			continue;
		}

		if (isNomad())
			break;

		g_Map.log("Finding player locations");
		const players = playerPlacementRandom(
			playerIDs,
			avoidClasses(
				clCliffs, scaleByMapSize(6, 15),
				clWater, scaleByMapSize(10, 20)));

		if (players)
		{
			[playerIDs, playerPosition] = players;
			break;
		}

		g_Map.log("Too few player locations, starting over");
	}
	yield 35;

	if (!isNomad())
	{
		g_Map.log("Flattening initial CC area");
		const playerRadius = defaultPlayerBaseRadius() * 0.8;
		for (const position of playerPosition)
			createArea(
				new ClumpPlacer(diskArea(playerRadius), 0.95, 0.6, Infinity, position),
				new SmoothElevationPainter(ELEVATION_SET, g_Map.getHeight(position),
					playerRadius / 2));
		yield 38;
	}

	g_Map.log("Painting lowlands");
	createArea(
		new MapBoundsPlacer(),
		new TerrainPainter(biomes.lowlands.terrains.main),
		constraintLowlands);
	yield 40;

	g_Map.log("Painting highlands");
	createArea(
		new MapBoundsPlacer(),
		new TerrainPainter(biomes.highlands.terrains.main),
		constraintHighlands);
	yield 45;

	g_Map.log("Painting mountains");
	createArea(
		new MapBoundsPlacer(),
		new TerrainPainter(biomes.common.terrains.cliffs),
		[
			avoidClasses(clWater, 2),
			constraintMountains
		]);
	yield 48;

	g_Map.log("Painting water and shoreline");
	createArea(
		new MapBoundsPlacer(),
		new TerrainPainter(biomes.water.terrains.main),
		new HeightConstraint(-Infinity, heightShoreline));
	yield 50;

	g_Map.log("Painting cliffs");
	createArea(
		new MapBoundsPlacer(),
		new TerrainPainter(biomes.common.terrains.cliffs),
		[
			avoidClasses(clWater, 2),
			new SlopeConstraint(2, Infinity)
		]);
	yield 55;

	for (let i = 0; i < numPlayers; ++i)
	{
		if (isNomad())
			break;

		const localBiome = constraintHighlands.allows(playerPosition[i]) ? biomes.highlands :
			biomes.lowlands;
		placePlayerBase({
			"playerID": playerIDs[i],
			"playerPosition": playerPosition[i],
			"PlayerTileClass": clPlayer,
			"Walls": "towers",
			"BaseResourceClass": clBaseResource,
			"baseResourceConstraint": avoidClasses(clPlayer, 4, clWater, 1, clCliffs, 1),
			"CityPatch": {
				"outerTerrain": biomes.common.terrains.roadWild,
				"innerTerrain": biomes.common.terrains.road
			},
			"StartingAnimal": {
				"template": localBiome.gaia.fauna.startingAnimal,
				"groupCount": 1,
				"minGroupCount": 4,
				"maxGroupCount": 4
			},
			"Berries": {
				"template": localBiome.gaia.flora.fruitBush,
				"minCount": 3,
				"maxCount": 3
			},
			"Mines": {
				"types": [
					{ "template": biomes.common.gaia.mines.metalLarge },
					{ "template": biomes.common.gaia.mines.stoneLarge }
				],
				"minAngle": Math.PI / 2,
				"maxAngle": Math.PI
			},
			"Trees": {
				"template": pickRandom(localBiome.gaia.flora.trees),
				"count": 15
			}
			// No decoratives
		});
	}
	yield 60;

	g_Map.log("Placing docks");
	placeDocks(
		biomes.shoreline.gaia.dock,
		0,
		scaleByMapSize(1, 2) * 100,
		clWater,
		clDock,
		heightReedsMax,
		heightShoreline,
		[avoidClasses(clDock, 50), new StaticConstraint(avoidClasses(clPlayer, 30, clCliffs, 8))],
		0,
		50);
	yield 65;

	const [forestTrees, stragglerTrees] = getTreeCounts(600, 4000, 0.7);
	const biomeTreeRatioHighlands = 0.4;
	for (const biome of ["lowlands", "highlands"])
		createForests(
			[
				biomes[biome].terrains.main,
				biomes[biome].terrains.forestFloors[0],
				biomes[biome].terrains.forestFloors[1],
				biomes[biome].terrains.forests[0],
				biomes[biome].terrains.forests[1]
			],
			[
				biome == "highlands" ? constraintHighlands : constraintLowlands,
				avoidClasses(clPlayer, 20, clForest, 18, clCliffs, 1, clWater, 2)
			],
			clForest,
			forestTrees * (biome == "highlands" ? biomeTreeRatioHighlands :
				1 - biomeTreeRatioHighlands));
	yield 70;

	g_Map.log("Creating stone mines");
	const minesStone = [
		[new SimpleObject(biomes.common.gaia.mines.stoneLarge, 1, 1, 0, 4, 0, 2 * Math.PI, 4)],
		[new SimpleObject(biomes.common.gaia.mines.stoneSmall, 2, 3, 1, 3, 0, 2 * Math.PI, 1)]
	];
	for (const mine of minesStone)
		createObjectGroups(
			new SimpleGroup(mine, true, clRock),
			0,
			[avoidClasses(clForest, 1, clPlayer, 20, clRock, 18, clCliffs, 2, clWater, 2, clDock, 6)],
			scaleByMapSize(2, 12),
			50);
	yield 75;

	g_Map.log("Creating metal mines");
	const minesMetal = [
		[new SimpleObject(biomes.common.gaia.mines.metalLarge, 1, 1, 0, 4, 0, 2 * Math.PI, 4)],
		[new SimpleObject(biomes.common.gaia.mines.metalSmall, 2, 3, 1, 3, 0, 2 * Math.PI, 1)]
	];
	for (const mine of minesMetal)
		createObjectGroups(
			new SimpleGroup(mine, true, clMetal),
			0,
			[avoidClasses(
				clForest, 1,
				clPlayer, 20,
				clRock, 8,
				clMetal, 18,
				clCliffs, 2,
				clWater, 2,
				clDock, 6)],
			scaleByMapSize(2, 12),
			50);
	yield 80;

	for (const biome of ["lowlands", "highlands"])
		createStragglerTrees(
			biomes[biome].gaia.flora.trees,
			[
				biome == "highlands" ? constraintHighlands : constraintLowlands,
				avoidClasses(
					clForest, 8,
					clCliffs, 1,
					clPlayer, 12,
					clMetal, 6,
					clRock, 6,
					clCliffs, 2,
					clWater, 2,
					clDock, 6)
			],
			clForest,
			stragglerTrees * (biome == "highlands" ? biomeTreeRatioHighlands * 4 :
				1 - biomeTreeRatioHighlands));
	yield 85;

	createFood(
		[
			[new SimpleObject(biomes.highlands.gaia.fauna.horse, 3, 5, 0, 4)],
			[new SimpleObject(biomes.highlands.gaia.fauna.pony, 2, 3, 0, 4)],
			[new SimpleObject(biomes.highlands.gaia.flora.fruitBush, 5, 7, 0, 4)]
		],
		[
			scaleByMapSize(2, 16),
			scaleByMapSize(2, 12),
			scaleByMapSize(2, 20)
		],
		[
			avoidClasses(
				clForest, 0,
				clPlayer, 20,
				clFood, 16,
				clCliffs, 2,
				clWater, 2,
				clRock, 4,
				clMetal, 4,
				clDock, 6),
			constraintHighlands
		],
		clFood);
	yield 90;

	createFood(
		[
			[new SimpleObject(biomes.lowlands.gaia.fauna.sheep, 2, 3, 0, 2)],
			[new SimpleObject(biomes.lowlands.gaia.fauna.rabbit, 2, 3, 0, 2)],
			[new SimpleObject(biomes.lowlands.gaia.flora.fruitBush, 5, 7, 0, 4)]
		],
		[
			scaleByMapSize(2, 16),
			scaleByMapSize(2, 12),
			scaleByMapSize(1, 20)
		],
		[
			avoidClasses(
				clForest, 0,
				clPlayer, 20,
				clFood, 16,
				clCliffs, 2,
				clWater, 2,
				clRock, 4,
				clMetal, 4,
				clDock, 6),
			constraintLowlands
		],
		clFood);
	yield 93;

	createFood(
		[
			[new SimpleObject(biomes.highlands.gaia.fauna.goat, 3, 5, 0, 4)]
		],
		[
			3 * numPlayers
		],
		[
			avoidClasses(
				clForest, 1,
				clPlayer, 20,
				clFood, 20,
				clCliffs, 1,
				clRock, 4,
				clMetal, 4,
				clDock, 6),
			constraintMountains
		],
		clFood);

	g_Map.log("Creating hawk");
	for (let i = 0; i < scaleByMapSize(0, 2); ++i)
		g_Map.placeEntityAnywhere(biomes.highlands.gaia.fauna.hawk, 0, mapCenter, randomAngle());

	g_Map.log("Creating fish");
	createObjectGroups(
		new SimpleGroup([new SimpleObject(biomes.water.gaia.fauna.fish, 1, 1, 0, 3)], true, clFood),
		0,
		[stayClasses(clWater, 8), avoidClasses(clFood, 8, clDock, 6)],
		scaleByMapSize(15, 50),
		100);
	yield 95;

	g_Map.log("Creating grass patches");
	for (const biome of ["lowlands", "highlands"])
		for (const patch of biomes[biome].terrains.patches)
			createPatches(
				[scaleByMapSize(3, 7), scaleByMapSize(5, 15)],
				patch,
				[
					biome == "highlands" ? constraintHighlands : constraintLowlands,
					avoidClasses(clForest, 0, clDirt, 5, clPlayer, 12, clCliffs, 2, clWater, 2),
				],
				scaleByMapSize(15, 45) / biomes[biome].terrains.patches.length,
				clDirt);
	yield 96;

	for (const biome of ["lowlands", "highlands"])
	{
		createDecoration(
			[
				[new SimpleObject(actorTemplate(biomes[biome].actors.mushroom), 1, 4, 1, 2)],
				[
					new SimpleObject(actorTemplate(biomes.common.actors.grass), 2, 4, 0, 1.8),
					new SimpleObject(actorTemplate(biomes.common.actors.grassShort), 3, 6, 1.2,
						2.5)
				],
				[
					new SimpleObject(actorTemplate(biomes.common.actors.bushMedium), 1, 2, 0, 2),
					new SimpleObject(actorTemplate(biomes.common.actors.bushSmall), 2, 4, 0, 2)
				]
			],
			[
				scaleByMapAreaAbsolute(20),
				scaleByMapAreaAbsolute(13),
				scaleByMapAreaAbsolute(13),
				scaleByMapAreaAbsolute(13)
			],
			[
				biome == "highlands" ? constraintHighlands : constraintLowlands,
				avoidClasses(clCliffs, 1, clPlayer, 15, clForest, 1, clRock, 4, clMetal, 4),
			]);

		createDecoration(
			[
				biomes[biome].actors.stones.map(template => new SimpleObject(actorTemplate(template), 1, 3, 0, 1))
			],
			[
				biomes[biome].actors.stones.map(template => scaleByMapAreaAbsolute(2) * randIntInclusive(1, 3))
			],
			[
				biome == "highlands" ? constraintHighlands : constraintLowlands,
				avoidClasses(clWater, 4, clPlayer, 15, clForest, 1, clRock, 4, clMetal, 4),
			]);
	}
	yield 98;

	g_Map.log("Creating temple");
	createObjectGroups(
		new SimpleGroup([new SimpleObject(biomes.highlands.gaia.athen.temple, 1, 1, 0, 0)], true),
		0,
		[
			avoidClasses(clCliffs, 4, clWater, 4, clPlayer, 40, clForest, 4, clRock, 4, clMetal, 4),
			constraintHighlands
		],
		1,
		200);

	g_Map.log("Creating statues");
	createObjectGroups(
		new SimpleGroup(
			[new SimpleObject(actorTemplate(biomes.lowlands.actors.athen.statue), 1, 1, 0, 0)], true),
		0,
		[
			avoidClasses(
				clCliffs, 2,
				clWater, 4,
				clPlayer, 30,
				clForest, 1,
				clRock, 8,
				clMetal, 8,
				clDock, 6),
			constraintLowlands
		],
		scaleByMapSize(1, 2),
		50);

	g_Map.log("Creating campfire");
	createObjectGroups(
		new SimpleGroup([new SimpleObject(actorTemplate(biomes.common.actors.campfire), 1, 1, 0, 0)],
			true),
		0,
		[avoidClasses(
			clCliffs, 2,
			clWater, 4,
			clPlayer, 30,
			clForest, 1,
			clRock, 8,
			clMetal, 8,
			clDock, 6)],
		scaleByMapSize(0, 2),
		50);

	g_Map.log("Creating oxybeles");
	createObjectGroups(
		new SimpleGroup([new SimpleObject(biomes.highlands.gaia.athen.oxybeles, 1, 1, 0, 0)], true),
		0,
		[
			avoidClasses(clCliffs, 2, clPlayer, 30, clForest, 1, clRock, 4, clMetal, 4),
			constraintHighlands
		],
		scaleByMapSize(0, 2),
		100);

	g_Map.log("Creating handcart");
	createObjectGroups(
		new SimpleGroup([new SimpleObject(actorTemplate(biomes.highlands.actors.handcart), 1, 1, 0, 0)],
			true),
		0,
		[
			avoidClasses(clCliffs, 1, clPlayer, 15, clForest, 1, clRock, 4, clMetal, 4),
			constraintHighlands
		],
		scaleByMapSize(1, 4),
		50);

	g_Map.log("Creating water log");
	createObjectGroups(
		new SimpleGroup([new SimpleObject(actorTemplate(biomes.water.actors.waterlog), 1, 1, 0, 0)],
			true),
		0,
		[stayClasses(clWater, 4)],
		scaleByMapSize(1, 2),
		10);

	g_Map.log("Creating reeds");
	createObjectGroups(
		new SimpleGroup(
			[
				new SimpleObject(actorTemplate(biomes.shoreline.actors.reeds), 5, 12, 1, 4),
				new SimpleObject(actorTemplate(biomes.shoreline.actors.lillies), 1, 2, 1, 5)
			],
			false,
			clDirt),
		0,
		new HeightConstraint(heightReedsMin, heightReedsMax),
		scaleByMapSize(10, 25),
		20);

	placePlayersNomad(clPlayer,
		avoidClasses(clForest, 1, clMetal, 4, clRock, 4, clFood, 2, clCliffs, 2, clWater, 15));
	yield 99;

	setSkySet("sunny");
	setSunColor(0.988166, 0.929297, 0.693819);
	setSunElevation(0.579592);
	setSunRotation(-0.566729);
	setAmbientColor(0.372549, 0.376471, 0.459608);
	setWaterColor(0.024, 0.212, 0.024);
	setWaterTint(0.133, 0.725, 0.855);
	setWaterMurkiness(0.8);
	setWaterWaviness(3);
	setFogFactor(0);
	setPPEffect("hdr");
	setPPSaturation(0.45);
	setPPContrast(0.62);
	setPPBloom(0.12);

	return g_Map;
}
