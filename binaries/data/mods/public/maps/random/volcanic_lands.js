Engine.LoadLibrary("rmgen");
Engine.LoadLibrary("rmgen-common");

function* GenerateMap()
{
	const tGrass = ["cliff volcanic light", "ocean_rock_a", "ocean_rock_b"];
	const tGrassA = "cliff volcanic light";
	const tGrassB = "ocean_rock_a";
	const tGrassC = "ocean_rock_b";
	const tCliff = ["cliff volcanic coarse", "cave_walls"];
	const tRoad = "road1";
	const tRoadWild = "road1";
	const tLava1 = "LavaTest05";
	const tLava2 = "LavaTest04";
	const tLava3 = "LavaTest03";

	const oTree = "gaia/tree/dead";
	const oStoneLarge = "gaia/rock/alpine_large";
	const oStoneSmall = "gaia/rock/alpine_small";
	const oMetalLarge = "gaia/ore/alpine_large";

	const aRockLarge = "actor|geology/stone_granite_med.xml";
	const aRockMedium = "actor|geology/stone_granite_med.xml";

	const pForestD = [tGrassC + TERRAIN_SEPARATOR + oTree, tGrassC];
	const pForestP = [tGrassB + TERRAIN_SEPARATOR + oTree, tGrassB];

	const heightLand = 1;
	const heightHill = 18;

	globalThis.g_Map = new RandomMap(heightLand, tGrassB);

	const numPlayers = getNumPlayers();
	const mapCenter = g_Map.getCenter();

	const clPlayer = g_Map.createTileClass();
	const clHill = g_Map.createTileClass();
	const clForest = g_Map.createTileClass();
	const clDirt = g_Map.createTileClass();
	const clRock = g_Map.createTileClass();
	const clMetal = g_Map.createTileClass();
	const clBaseResource = g_Map.createTileClass();

	placePlayerBases({
		"PlayerPlacement": playerPlacementCircle(fractionToTiles(0.35)),
		"PlayerTileClass": clPlayer,
		"BaseResourceClass": clBaseResource,
		"CityPatch": {
			"outerTerrain": tRoadWild,
			"innerTerrain": tRoad
		},
		// No berries, no chicken, no decoratives
		"Mines": {
			"types": [
				{ "template": oMetalLarge },
				{ "template": oStoneLarge }
			]
		},
		"Trees": {
			"template": oTree,
			"count": scaleByMapSize(12, 30)
		}
	});
	yield 15;

	createVolcano(mapCenter, clHill, tCliff, [tLava1, tLava2, tLava3], true, ELEVATION_SET);
	yield 45;

	g_Map.log("Creating hills");
	createAreas(
		new ClumpPlacer(scaleByMapSize(20, 150), 0.2, 0.1, Infinity),
		[
			new LayeredPainter([tCliff, tGrass], [2]),
			new SmoothElevationPainter(ELEVATION_SET, heightHill, 2),
			new TileClassPainter(clHill)
		],
		avoidClasses(clPlayer, 12, clHill, 15, clBaseResource, 2),
		scaleByMapSize(2, 8) * numPlayers
	);

	g_Map.log("Creating forests");
	const [forestTrees, stragglerTrees] = getTreeCounts(200, 1250, 0.7);
	const types = [
		[[tGrassB, tGrassA, pForestD], [tGrassB, pForestD]],
		[[tGrassB, tGrassA, pForestP], [tGrassB, pForestP]]
	];
	const forestSize = forestTrees / (scaleByMapSize(2, 8) * numPlayers);
	const num = Math.floor(forestSize / types.length);
	for (const type of types)
		createAreas(
			new ClumpPlacer(forestTrees / num, 0.1, 0.1, Infinity),
			[
				new LayeredPainter(type, [2]),
				new TileClassPainter(clForest)
			],
			avoidClasses(clPlayer, 12, clForest, 10, clHill, 0, clBaseResource, 6),
			num);

	yield 70;

	g_Map.log("Creating dirt patches");
	for (const size of [scaleByMapSize(3, 48), scaleByMapSize(5, 84), scaleByMapSize(8, 128)])
		createAreas(
			new ClumpPlacer(size, 0.3, 0.06, 0.5),
			[
				new LayeredPainter([tGrassA, tGrassA], [1]),
				new TileClassPainter(clDirt)
			],
			avoidClasses(clForest, 0, clHill, 0, clPlayer, 12),
			scaleByMapSize(20, 80));

	for (const size of [scaleByMapSize(3, 48), scaleByMapSize(5, 84), scaleByMapSize(8, 128)])
		createAreas(
			new ClumpPlacer(size, 0.3, 0.06, 0.5),
			[
				new TerrainPainter(tGrassB),
				new TileClassPainter(clDirt)
			],
			avoidClasses(clForest, 0, clHill, 0, clPlayer, 12),
			scaleByMapSize(20, 80));

	for (const size of [scaleByMapSize(3, 48), scaleByMapSize(5, 84), scaleByMapSize(8, 128)])
		createAreas(
			new ClumpPlacer(size, 0.3, 0.06, 0.5),
			[
				new TerrainPainter(tGrassC),
				new TileClassPainter(clDirt)
			],
			avoidClasses(clForest, 0, clHill, 0, clPlayer, 12),
			scaleByMapSize(20, 80)
		);

	g_Map.log("Creating stone mines");
	let group = new SimpleGroup(
		[
			new SimpleObject(oStoneSmall, 0, 2, 0, 4, 0, 2 * Math.PI, 1),
			new SimpleObject(oStoneLarge, 1, 1, 0, 4, 0, 2 * Math.PI, 4)
		], true, clRock);
	createObjectGroupsDeprecated(group, 0,
		avoidClasses(clForest, 1, clPlayer, 10, clRock, 10, clHill, 1),
		scaleByMapSize(4, 16), 100
	);

	g_Map.log("Creating small stone mines");
	group = new SimpleGroup([new SimpleObject(oStoneSmall, 2, 5, 1, 3)], true, clRock);
	createObjectGroupsDeprecated(group, 0,
		avoidClasses(clForest, 1, clPlayer, 10, clRock, 10, clHill, 1),
		scaleByMapSize(4, 16), 100
	);

	g_Map.log("Creating metal mines");
	group = new SimpleGroup([new SimpleObject(oMetalLarge, 1, 1, 0, 4)], true, clMetal);
	createObjectGroupsDeprecated(group, 0,
		avoidClasses(clForest, 1, clPlayer, 10, clMetal, 10, clRock, 5, clHill, 1),
		scaleByMapSize(4, 16), 100
	);

	yield 90;

	g_Map.log("Creating small decorative rocks");
	group = new SimpleGroup(
		[new SimpleObject(aRockMedium, 1, 3, 0, 1)],
		true
	);
	createObjectGroupsDeprecated(
		group, 0,
		avoidClasses(clForest, 0, clPlayer, 0, clHill, 0),
		scaleByMapSize(16, 262), 50
	);

	g_Map.log("Creating large decorative rocks");
	group = new SimpleGroup(
		[new SimpleObject(aRockLarge, 1, 2, 0, 1), new SimpleObject(aRockMedium, 1, 3, 0, 2)],
		true
	);
	createObjectGroupsDeprecated(
		group, 0,
		avoidClasses(clForest, 0, clPlayer, 0, clHill, 0),
		scaleByMapSize(8, 131), 50
	);

	yield 95;

	createStragglerTrees(
		[oTree],
		avoidClasses(clForest, 1, clHill, 1, clPlayer, 12, clMetal, 6, clRock, 6, clBaseResource, 6),
		clForest,
		stragglerTrees);

	placePlayersNomad(clPlayer, avoidClasses(clForest, 1, clMetal, 4, clRock, 4, clHill, 4));

	return g_Map;
}
