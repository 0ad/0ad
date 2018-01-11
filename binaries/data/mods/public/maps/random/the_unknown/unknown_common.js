/**
 * @file This library is used to generate different map variations on the map Unknown, Unknown Land and Unknown Nomad.
 */

/**
 * True if city centers should be placed or false for nomad.
 */
var g_PlayerBases;

/**
 * True if all players should be connected via land and false if river or islands can split some if not all the players.
 */
var g_AllowNaval;

TILE_CENTERED_HEIGHT_MAP = true;

setSelectedBiome();

const tMainTerrain = g_Terrains.mainTerrain;
const tForestFloor1 = g_Terrains.forestFloor1;
const tForestFloor2 = g_Terrains.forestFloor2;
const tCliff = g_Terrains.cliff;
const tTier1Terrain = g_Terrains.tier1Terrain;
const tTier2Terrain = g_Terrains.tier2Terrain;
const tTier3Terrain = g_Terrains.tier3Terrain;
const tHill = g_Terrains.hill;
const tRoad = g_Terrains.road;
const tRoadWild = g_Terrains.roadWild;
const tTier4Terrain = g_Terrains.tier4Terrain;
const tShore = g_Terrains.shore;
const tWater = g_Terrains.water;

const oTree1 = g_Gaia.tree1;
const oTree2 = g_Gaia.tree2;
const oTree4 = g_Gaia.tree4;
const oTree5 = g_Gaia.tree5;
const oFruitBush = g_Gaia.fruitBush;
const oMainHuntableAnimal = g_Gaia.mainHuntableAnimal;
const oSecondaryHuntableAnimal = g_Gaia.secondaryHuntableAnimal;
const oFish = g_Gaia.fish;
const oStoneLarge = g_Gaia.stoneLarge;
const oStoneSmall = g_Gaia.stoneSmall;
const oMetalLarge = g_Gaia.metalLarge;
const oWoodTreasure = "gaia/special_treasure_wood";

const aGrass = g_Decoratives.grass;
const aGrassShort = g_Decoratives.grassShort;
const aReeds = g_Decoratives.reeds;
const aLillies = g_Decoratives.lillies;
const aRockLarge = g_Decoratives.rockLarge;
const aRockMedium = g_Decoratives.rockMedium;
const aBushMedium = g_Decoratives.bushMedium;
const aBushSmall = g_Decoratives.bushSmall;

const pForest1 = [tForestFloor2 + TERRAIN_SEPARATOR + oTree1, tForestFloor2 + TERRAIN_SEPARATOR + oTree2, tForestFloor2];
const pForest2 = [tForestFloor1 + TERRAIN_SEPARATOR + oTree4, tForestFloor1 + TERRAIN_SEPARATOR + oTree5, tForestFloor1];

InitMap();

const numPlayers = getNumPlayers();
const mapSize = getMapSize();
const mapArea = getMapArea();
const mapCenter = getMapCenter();
const lSize = Math.pow(scaleByMapSize(1, 6), 1/8);

var clPlayer = createTileClass();
var clPlayerTerritory = createTileClass();
var clHill = createTileClass();
var clForest = createTileClass();
var clWater = createTileClass();
var clDirt = createTileClass();
var clRock = createTileClass();
var clMetal = createTileClass();
var clFood = createTileClass();
var clPeninsulaSteam = createTileClass();
var clBaseResource = createTileClass();
var clLand = createTileClass();
var clShallow = createTileClass();

var landHeight = 3;
var cliffHeight = 3.12;
var landElevationPainter = new SmoothElevationPainter(ELEVATION_SET, landHeight, 4);

var unknownMapFunctions = {
	"land": [
		"Continent",
		"CentralSea",
		"CentralRiver",
		"EdgeSeas",
		"Gulf",
		"Lakes",
		"Passes",
		"Lowlands",
		"Mainland"
	],
	"naval": [
		"Archipelago",
		"RiversAndLake"
	]
};

/**
 * The player IDs and locations shall only be determined by the landscape functions if it's not a nomad game,
 * because nomad maps randomize the locations after the terrain generation.
 * The locations should only determined by the landscape functions to avoid placing bodies of water and resources into civic centers and the starting resources.
 */
var playerIDs = sortAllPlayers();
var playerX = [];
var playerZ = [];

var g_StartingTreasures = false;
var g_StartingWalls = true;

function createUnknownMap()
{
	let funcs = unknownMapFunctions.land;

	if (g_AllowNaval)
		funcs = funcs.concat(unknownMapFunctions.naval);

	global["unknown" + pickRandom(funcs)]();

	paintUnknownMapBasedOnHeight();

	if (g_PlayerBases)
		createUnknownPlayerBases();
}

/**
 * Chain of islands or many disconnected islands.
 */
function unknownArchipelago()
{
	g_StartingWalls = "towers";
	g_StartingTreasures = true;

	let [pIDs, islandX, islandZ] = playerPlacementCircle(0.35);
	if (g_PlayerBases)
	{
		[playerIDs, playerX, playerZ] = [pIDs, islandX, islandZ];
		markPlayerArea("large");
	}

	log("Creating islands...");
	let islandSize = diskArea(scaleByMapSize(17, 29));
	for (let i = 0; i < numPlayers; ++i)
		createArea(
			new ClumpPlacer(islandSize, 0.8, 0.1, 10, fractionToTiles(islandX[i]), fractionToTiles(islandZ[i])),
			landElevationPainter);

	let type = randIntInclusive(1, 3);
	if (type == 1)
	{
		log("Creating archipelago...");
		createAreas(
			new ClumpPlacer(Math.floor(islandSize * randFloat(0.8, 1.2)), 0.8, 0.1, 10),
			[
				landElevationPainter,
				paintClass(clLand)
			],
			null,
			scaleByMapSize(2, 5) * randIntInclusive(8, 14));

		log("Creating shore jaggedness with small puddles...");
		createAreas(
			new ClumpPlacer(scaleByMapSize(15, 80), 0.2, 0.1, 1),
			[
				new SmoothElevationPainter(ELEVATION_SET, landHeight, 4),
				paintClass(clLand)
			],
			borderClasses(clLand, 6, 3),
			scaleByMapSize(12, 130) * 2,
			150);
	}
	else if (type == 2)
	{
		log("Creating islands...");
		createAreas(
			new ClumpPlacer(Math.floor(islandSize * randFloat(0.6, 1.4)), 0.8, 0.1, randFloat(0.0, 0.2)),
			[
				landElevationPainter,
				paintClass(clLand)
			],
			avoidClasses(clLand, 3, clPlayerTerritory, 3),
			scaleByMapSize(6, 10) * randIntInclusive(8, 14));

		log("Creating small islands...");
		createAreas(
			new ClumpPlacer(Math.floor(islandSize * randFloat(0.3, 0.7)), 0.8, 0.1, 0.07),
			[
				new SmoothElevationPainter(ELEVATION_SET, landHeight, 6),
				paintClass(clLand)
			],
			avoidClasses(clLand, 3, clPlayerTerritory, 3),
			scaleByMapSize(2, 6) * randIntInclusive(6, 15),
			25);
	}
	else if (type == 3)
	{
		log("Creating tight islands...");
		createAreas(
			new ClumpPlacer(Math.floor(islandSize * randFloat(0.8, 1.2)), 0.8, 0.1, 10),
			[
				landElevationPainter,
				paintClass(clLand)
			],
			avoidClasses(clLand, randIntInclusive(8, 16), clPlayerTerritory, 3),
			scaleByMapSize(2, 5) * randIntInclusive(8, 14));
	}
}

/**
 * Disk shaped mainland with water on the edge.
 */
function unknownContinent()
{
	let waterHeight = -5;

	if (g_PlayerBases)
	{
		log("Ensuring player area...");
		[playerIDs, playerX, playerZ] = playerPlacementCircle(0.25);
		markPlayerArea("small");

		for (let i = 0; i < numPlayers; ++i)
			createArea(
				new ChainPlacer(
					2,
					Math.floor(scaleByMapSize(5, 9)),
					Math.floor(scaleByMapSize(5, 20)),
					1,
					Math.round(fractionToTiles(playerX[i])),
					Math.round(fractionToTiles(playerZ[i])),
					0,
					[Math.floor(scaleByMapSize(23, 50))]),
				[
					landElevationPainter,
					paintClass(clLand)
				]);
	}

	log("Creating continent...");
	createArea(
		new ClumpPlacer(mapArea * 0.45, 0.9, 0.09, 10, Math.round(fractionToTiles(0.5)), Math.round(fractionToTiles(0.5))),
		[
			landElevationPainter,
			paintClass(clLand)
		]);

	if (randBool(1/3))
	{
		log("Creating peninsula (i.e. half the map not being surrounded by water)...");
		let angle = randFloat(0, 2 * Math.PI);
		createArea(
			new ClumpPlacer(
				mapArea * 0.45,
				0.9,
				0.09,
				10,
				Math.round(fractionToTiles(0.5 + 0.25 * Math.cos(angle))),
				Math.round(fractionToTiles(0.5 + 0.25 * Math.sin(angle)))),
			[
				landElevationPainter,
				paintClass(clLand)
			]);

		log("Remembering to not paint shorelines into the peninsula...");
		createArea(
			new ClumpPlacer(
				mapArea * 0.3,
				0.9,
				0.01,
				10,
				Math.round(fractionToTiles(0.5 + 0.35 * Math.cos(angle))),
				Math.round(fractionToTiles(0.5 + 0.35 * Math.sin(angle)))),
			paintClass(clPeninsulaSteam));
	}

	createShoreJaggedness(waterHeight, clLand, 7);
}

/**
 * Creates a huge central river, possibly connecting the riversides with a narrow piece of land.
 */
function unknownCentralSea()
{
	let waterHeight = -3;
	let horizontal = randBool();

	let [start, end] = centralRiverCoordinates(horizontal);
	paintRiver({
		"parallel": false,
		"startX": tilesToFraction(start[0]),
		"startZ": tilesToFraction(start[1]),
		"endX": tilesToFraction(end[0]),
		"endZ": tilesToFraction(end[1]),
		"width": randFloat(0.22, 0.3) + scaleByMapSize(0.05, 0.2),
		"fadeDist": 0.025,
		"deviation": 0,
		"waterHeight": waterHeight,
		"landHeight": landHeight,
		"meanderShort": 20,
		"meanderLong": 0,
		"waterFunc": (ix, iz, height, riverFraction) => {
			if (height < 0)
				addToClass(ix, iz, clWater);
		},
		"landFunc": (ix, iz, shoreDist1, shoreDist2) => {
			setHeight(ix, iz, 3.1);
			addToClass(ix, iz, clLand);
		}
	});

	if (g_PlayerBases)
	{
		[playerIDs, playerX, playerZ] = playerPlacementRiver(horizontal ? Math.PI / 2 : 0, 0.6);
		markPlayerArea("small");
	}

	if (!g_AllowNaval || randBool())
	{
		log("Creating isthmus (i.e. connecting the two riversides with a big land passage)...");
		let [coord1, coord2] = centralRiverCoordinates(!horizontal);
		createArea(
			new PathPlacer(
				...coord1,
				...coord2,
				scaleByMapSize(randIntInclusive(16, 24),
				randIntInclusive(100, 140)),
				0.5,
				3 * scaleByMapSize(1, 4),
				0.1,
				0.01),
			[
				landElevationPainter,
				paintClass(clLand),
				unPaintClass(clWater)
			]);
	}

	createExtensionsOrIslands();
	// Don't createShoreJaggedness since it doesn't fit artistically here
}

/**
 * Creates a very small central river.
 */
function unknownCentralRiver()
{
	let waterHeight = -4;
	let shallowHeight = -2;
	initHeight(landHeight);

	let horizontal = randBool();
	let riverAngle = horizontal ? 0 : Math.PI / 2;

	if (g_PlayerBases)
	{
		[playerIDs, playerX, playerZ] = playerPlacementRiver(horizontal ? Math.PI / 2 : 0, 0.5);
		markPlayerArea("large");
	}

	log("Creating the main river...");
	let [coord1, coord2] = centralRiverCoordinates(horizontal);
	createArea(
		new PathPlacer(...coord1, ...coord2, scaleByMapSize(14, 24), 0.5, scaleByMapSize(3, 12), 0.1, 0.01),
		new SmoothElevationPainter(ELEVATION_SET, waterHeight, 4),
		avoidClasses(clPlayerTerritory, 4));

	log("Creating small water spots at the map border to ensure separation of players...");
	for (let coord of [coord1, coord2])
		createArea(
			new ClumpPlacer(Math.floor(diskArea(scaleByMapSize(5, 10))), 0.95, 0.6, 10, ...coord),
			new SmoothElevationPainter(ELEVATION_SET, waterHeight, 2),
			avoidClasses(clPlayerTerritory, 8));

	if (!g_AllowNaval || randBool())
	{
		log("Creating the shallows of the main river...");
		for (let i = 0; i <= randIntInclusive(1, scaleByMapSize(4, 8)); ++i)
		{
			let location = randFloat(0.15, 0.85);
			createPassage({
				"start": new Vector2D(location, 0).mult(mapSize).rotateAround(riverAngle, mapCenter),
				"end": new Vector2D(location, 1).mult(mapSize).rotateAround(riverAngle, mapCenter),
				"startWidth": scaleByMapSize(8, 12),
				"endWidth": scaleByMapSize(8, 12),
				"smoothWidth": 2,
				"startHeight": shallowHeight,
				"endHeight": shallowHeight,
				"maxHeight": shallowHeight,
				"tileClass": clShallow
			});
		}
	}

	if (randBool(2/3))
		createTributaryRivers(
			horizontal,
			randIntInclusive(8, scaleByMapSize(12, 16)),
			scaleByMapSize(10, 20),
			-4,
			[-6, -1.5],
			Math.PI / 5,
			clWater,
			clShallow,
			avoidClasses(clPlayerTerritory, 3));
}

/**
 * Creates a circular lake in the middle and possibly a river between each player ("pizza slices").
 */
function unknownRiversAndLake()
{
	let waterHeight = -4;
	initHeight(landHeight);

	let startAngle;
	if (g_PlayerBases)
	{
		let playerAngle;
		[playerIDs, playerX, playerZ, playerAngle, startAngle] = playerPlacementCircle(0.35);
		markPlayerArea("small");
	}

	let lake = randBool(3/4);
	if (lake)
	{
		log("Creating lake...");
		createArea(
			new ClumpPlacer(mapArea * 0.09 * lSize, 0.7, 0.1, 10, mapCenter.x, mapCenter.y),
			[
				new SmoothElevationPainter(ELEVATION_SET, waterHeight, 4),
				paintClass(clWater)
			]);

		createShoreJaggedness(waterHeight, clWater, 3);
	}

	// Don't do this on nomad because the imbalances on the different islands are too drastic
	if (g_PlayerBases && (!lake || randBool(1/3)))
	{
		log("Creating small rivers separating players...");
		for (let river of distributePointsOnCircle(numPlayers, startAngle + Math.PI / numPlayers, fractionToTiles(0.5), mapCenter)[0])
		{
			createArea(
				new PathPlacer(mapCenter.x, mapCenter.y, river.x, river.y, scaleByMapSize(14, 24), 0.4, 3 * scaleByMapSize(1, 3), 0.2, 0.05),
				[
					new SmoothElevationPainter(ELEVATION_SET, waterHeight, 4),
					paintClass(clWater)
				],
				avoidClasses(clPlayer, 5));

			createArea(
				new ClumpPlacer(Math.floor(diskArea(scaleByMapSize(10, 50)) / 5), 0.95, 0.6, 10, river.x, river.y),
				[
					new SmoothElevationPainter(ELEVATION_SET, waterHeight, 0),
					paintClass(clWater)
				],
				avoidClasses(clPlayer, 5));
		}

		log("Creating lake...");
		createArea(
			new ClumpPlacer(mapArea * 0.005, 0.7, 0.1, 10, mapCenter.x, mapCenter.y),
			[
				new SmoothElevationPainter(ELEVATION_SET, waterHeight, 4),
				paintClass(clWater)
			]);
	}

	if (lake && randBool())
	{
		log("Creating small central island...");
		createArea(
			new ClumpPlacer(mapArea * 0.006 * lSize, 0.7, 0.1, 10, mapCenter.x, mapCenter.y),
			[
				landElevationPainter,
				paintClass(clWater)
			]);
	}
}

/**
 * Align players on a land strip with seas bordering on one or both sides that can hold islands.
 */
function unknownEdgeSeas()
{
	let waterHeight = -4;
	initHeight(landHeight);

	let horizontal = randBool();
	if (g_PlayerBases)
	{
		[playerIDs, playerX, playerZ] = playerPlacementLine(horizontal, 0.5, 0.2);
		// Don't place the shoreline inside the CC, but possibly into the players territory
		markPlayerArea("small");
	}

	for (let location of pickRandom([["first"], ["second"], ["first", "second"]]))
	{
		let positionX = location == "first" ? [0, 0] : [1, 1];
		let positionZ = [0, 1];

		if (horizontal)
			[positionX, positionZ] = [positionZ, positionX];

		paintRiver({
			"parallel": true,
			"startX": positionX[0],
			"startZ": positionZ[0],
			"endX": positionX[1],
			"endZ": positionZ[1],
			"width": 0.62 - randFloat(0, scaleByMapSize(0, 0.1)),
			"fadeDist": 0.015,
			"deviation": 0,
			"waterHeight": waterHeight,
			"landHeight": landHeight,
			"meanderShort": 20,
			"meanderLong": 0
		});
	}

	createExtensionsOrIslands();
	paintTileClassBasedOnHeight(0, cliffHeight, 1, clLand);
	createShoreJaggedness(waterHeight, clLand, 7, false);
}

/**
 * Land shaped like a concrescent moon around a central lake.
 */
function unknownGulf()
{
	let waterHeight = -3;
	initHeight(landHeight);

	let startAngle = randFloat(0, 2) * Math.PI;
	if (g_PlayerBases)
	{
		log("Determining player locations...");

		[playerX, playerZ] = playerPlacementCustomAngle(
			0.35,
			tilesToFraction(mapCenter.x),
			tilesToFraction(mapCenter.y),
			i => startAngle + 2/3 * Math.PI * (-1 + (numPlayers == 1 ? 1 : 2 * i / (numPlayers - 1))));

		markPlayerArea("large");
	}

	let placers = [
		new ClumpPlacer(mapArea * 0.08, 0.7, 0.05, 10, Math.round(fractionToTiles(0.5)), Math.round(fractionToTiles(0.5))),
		new ClumpPlacer(mapArea * 0.13 * lSize, 0.7, 0.05, 10, Math.round(fractionToTiles(0.5 - 0.2 * Math.cos(startAngle))), Math.round(fractionToTiles(0.5 - 0.2 * Math.sin(startAngle)))),
		new ClumpPlacer(mapArea * 0.15 * lSize, 0.7, 0.05, 10, Math.round(fractionToTiles(0.5 - 0.49 * Math.cos(startAngle))), Math.round(fractionToTiles(0.5 - 0.49 * Math.sin(startAngle)))),
	];

	for (let placer of placers)
		createArea(
			placer,
			[
				new SmoothElevationPainter(ELEVATION_SET, waterHeight, 4),
				paintClass(clWater)
			],
			avoidClasses(clPlayerTerritory, scaleByMapSize(15, 25)));
}

/**
 * Mainland style with some small random lakes.
 */
function unknownLakes()
{
	let waterHeight = -5;

	initHeight(landHeight);

	if (g_PlayerBases)
	{
		[playerIDs, playerX, playerZ] = playerPlacementCircle(0.35);
		markPlayerArea("large");
	}

	log("Creating lakes...");
	createAreas(
		new ClumpPlacer(scaleByMapSize(160, 700), 0.2, 0.1, 1),
		[
			new SmoothElevationPainter(ELEVATION_SET, waterHeight, 5),
			paintClass(clWater)
		],
		[avoidClasses(clPlayerTerritory, 12), randBool() ? avoidClasses(clWater, 8) : new NullConstraint()],
		scaleByMapSize(5, 16));
}

/**
 * A large hill leaving players only a small passage to each of the the two neighboring players.
 */
function unknownPasses()
{
	let mountainHeight = 24;
	let waterHeight = -4;
	initHeight(landHeight);

	let playerAngle;
	let startAngle;
	if (g_PlayerBases)
	{
		[playerIDs, playerX, playerZ, playerAngle, startAngle] = playerPlacementCircle(0.35);
		markPlayerArea("small");
	}
	else
		startAngle = Math.random(0, 2 * Math.PI);

	for (let mountain of distributePointsOnCircle(numPlayers, startAngle + Math.PI / numPlayers, fractionToTiles(0.5), mapCenter)[0])
	{
		log("Creating a mountain range between neighboring players...");
		createArea(
			new PathPlacer(mapCenter.x, mapCenter.y, mountain.x, mountain.y, scaleByMapSize(14, 24), 0.4, 3 * scaleByMapSize(1, 3), 0.2, 0.05),
			[
				// More smoothing than this often results in the mountainrange becoming passable to one player.
				new SmoothElevationPainter(ELEVATION_SET, mountainHeight, 1),
				paintClass(clWater)
			],
			avoidClasses(clPlayer, 5));

		log("Creating small mountain at the map border between the players to ensure separation of players...");
		createArea(
			new ClumpPlacer(Math.floor(diskArea(scaleByMapSize(10, 50)) / 5), 0.95, 0.6, 10, mountain.x, mountain.y),
			new SmoothElevationPainter(ELEVATION_SET, mountainHeight, 0),
			avoidClasses(clPlayer, 5));
	}

	let passes = distributePointsOnCircle(numPlayers * 2, startAngle, fractionToTiles(0.35), mapCenter)[0];
	for (let i = 0; i < numPlayers; ++i)
	{
		log("Create passages between neighboring players...");
		createArea(
			new PathPlacer(
				passes[2 * i].x,
				passes[2 * i].y,
				passes[2 * ((i + 1) % numPlayers)].x,
				passes[2 * ((i + 1) % numPlayers)].y,
				scaleByMapSize(14, 24),
				0.4,
				3 * scaleByMapSize(1, 3),
				0.2,
				0.05),
			new SmoothElevationPainter(ELEVATION_SET, landHeight, 2));
	}

	if (randBool(2/5))
	{
		log("Create central lake...");
		createArea(
			new ClumpPlacer(mapArea * 0.03 * lSize, 0.7, 0.1, 10, mapCenter.x, mapCenter.y),
			[
				new SmoothElevationPainter(ELEVATION_SET, waterHeight, 3),
				paintClass(clWater)
			]);
	}
	else
	{
		log("Fill area between the paths...");
		createArea(
			new ClumpPlacer(mapArea * 0.005, 0.7, 0.1, 10, mapCenter.x, mapCenter.y),
			[
				new SmoothElevationPainter(ELEVATION_SET, mountainHeight, 4),
				paintClass(clWater)
			]);
	}
}

/**
 * Land enclosed by a hill that leaves small areas for civic centers and large central place.
 */
function unknownLowlands()
{
	let mountainHeight = 30;

	log("Creating mountain that is going to separate players...");
	initHeight(mountainHeight);

	let playerAngle;
	let startAngle;
	if (g_PlayerBases)
	{
		[playerIDs, playerX, playerZ, playerAngle, startAngle] = playerPlacementCircle(0.35);
		markPlayerArea("small");
	}
	else
		startAngle = Math.random(0, 2 * Math.PI);

	log("Creating valleys enclosed by the mountain...");
	let valleys = numPlayers;
	if (mapSize >= 128 && numPlayers <= 2 ||
	    mapSize >= 192 && numPlayers <= 3 ||
	    mapSize >= 320 && numPlayers <= 4 ||
	    mapSize >= 384 && numPlayers <= 5 ||
	    mapSize >= 448 && numPlayers <= 6)
		valleys *= 2;

	for (let valley of distributePointsOnCircle(valleys, startAngle, fractionToTiles(0.35), mapCenter)[0])
	{
		log("Creating player valley...");
		createArea(
			new ClumpPlacer(diskArea(scaleByMapSize(18, 32)), 0.65, 0.1, 10, valley.x, valley.y),
			[
				new SmoothElevationPainter(ELEVATION_SET, landHeight, 2),
				paintClass(clLand)
			]);

		log("Creating passes from player areas to the center...");
		createArea(
			new PathPlacer(mapCenter.x, mapCenter.y, valley.x, valley.y, scaleByMapSize(14, 24), 0.4, 3 * scaleByMapSize(1, 3), 0.2, 0.05),
			[
				landElevationPainter,
				paintClass(clWater)
			]);
	}

	log("Creating the big central area...");
	createArea(
		new ClumpPlacer(mapArea * 0.091 * lSize, 0.7, 0.1, 10, mapCenter.x, mapCenter.y),
		[
			landElevationPainter,
			paintClass(clWater)
		]);
}

/**
 * No water, no hills.
 */
function unknownMainland()
{
	initHeight(3);

	if (g_PlayerBases)
	{
		[playerIDs, playerX, playerZ] = playerPlacementCircle(0.35);
		markPlayerArea("small");
	}
}

function centralRiverCoordinates(horizontal)
{
	let coord1 = [0, fractionToTiles(0.5)];
	let coord2 = [fractionToTiles(1), fractionToTiles(0.5)];

	if (!horizontal)
	{
		coord1.reverse();
		coord2.reverse();
	}

	return [coord1, coord2];
}

function createShoreJaggedness(waterHeight, borderClass, shoreDist, inwards = true)
{
	log("Creating shore jaggedness...");
	for (let i = 0; i < 2; ++i)
		if (i || inwards)
			createAreas(
				new ChainPlacer(2, Math.floor(scaleByMapSize(4, 6)), 15, 1),
				[
						new SmoothElevationPainter(ELEVATION_SET, i ? landHeight : waterHeight, 4),
						i ? paintClass(clLand) : unPaintClass(clLand)
				],
				[
						avoidClasses(clPlayer, 20, clPeninsulaSteam, 20),
						borderClasses(borderClass, shoreDist, shoreDist)
				],
				scaleByMapSize(7, 130) * 2,
				150);
}

function createExtensionsOrIslands()
{
	let rnd = randIntInclusive(1, 3);

	if (rnd == 1)
	{
		log("Creating islands...");
		createAreas(
			new ClumpPlacer(Math.square(randIntInclusive(scaleByMapSize(8, 15), scaleByMapSize(15, 23))), 0.8, 0.1, randFloat(0, 0.2)),
			[
				landElevationPainter,
				paintClass(clLand)
			],
			avoidClasses(clLand, 3, clPlayer, 3),
			scaleByMapSize(2, 5) * randIntInclusive(8, 14));
	}
	else if (rnd == 2)
	{
		log("Creating extentions...");
		createAreas(
			new ChainPlacer(Math.floor(scaleByMapSize(4, 7)), Math.floor(scaleByMapSize(7, 10)), Math.floor(scaleByMapSize(16, 40)), 0.07),
			[
				landElevationPainter,
				paintClass(clLand)
			],
			null,
			scaleByMapSize(2, 5) * randIntInclusive(8, 14));
	}
}

/**
 * Prevent impassable terrain and resource collisions at the the civic center and starting resources.
 */
function markPlayerArea(size)
{
	for (let i = 0; i < numPlayers; ++i)
	{
		addCivicCenterAreaToClass(
			Math.round(fractionToTiles(playerX[i])),
			Math.round(fractionToTiles(playerZ[i])),
			clPlayer);

		if (size == "large")
			createArea(
				new ClumpPlacer(diskArea(scaleByMapSize(17, 29) / 3), 0.6, 0.3, 10, fractionToTiles(playerX[i]), fractionToTiles(playerZ[i])),
				paintClass(clPlayerTerritory));
	}
}

function paintUnknownMapBasedOnHeight()
{
	paintTerrainBasedOnHeight(cliffHeight, 40, 1, tCliff);
	paintTerrainBasedOnHeight(3, cliffHeight, 1, tMainTerrain);
	paintTerrainBasedOnHeight(1, 3, 1, tShore);
	paintTerrainBasedOnHeight(-8, 1, 2, tWater);

	unPaintTileClassBasedOnHeight(0, cliffHeight, 1, clWater);
	unPaintTileClassBasedOnHeight(-6, 0, 1, clLand);

	paintTileClassBasedOnHeight(-6, 0, 1, clWater);
	paintTileClassBasedOnHeight(0, cliffHeight, 1, clLand);
	paintTileClassBasedOnHeight(cliffHeight, 40, 1, clHill);
}

/**
 * Place resources and decoratives after the player territory was marked.
 */
function createUnknownObjects()
{
	log("Creating bumps...");
	createAreas(
		new ClumpPlacer(scaleByMapSize(20, 50), 0.3, 0.06, 1),
		new SmoothElevationPainter(ELEVATION_MODIFY, 2, 2),
		[avoidClasses(clWater, 2, clPlayer, 10), stayClasses(clLand, 3)],
		randIntInclusive(0, scaleByMapSize(1, 2) * 200));

	log("Creating hills...");
	createAreas(
		new ClumpPlacer(scaleByMapSize(20, 150), 0.2, 0.1, 1),
		[
			new LayeredPainter([tCliff, tHill], [2]),
			new SmoothElevationPainter(ELEVATION_SET, 18, 2),
			paintClass(clHill)
		],
		[avoidClasses(clPlayer, 15, clHill, randIntInclusive(6, 18)), stayClasses(clLand, 0)],
		randIntInclusive(0, scaleByMapSize(4, 8))*randIntInclusive(1, scaleByMapSize(4, 9))
	);
	Engine.SetProgress(50);

	log("Creating forests...");
	let [numForest, numStragglers] = getTreeCounts(...rBiomeTreeCount(1));
	let types = [
		[[tForestFloor2, tMainTerrain, pForest1], [tForestFloor2, pForest1]],
		[[tForestFloor1, tMainTerrain, pForest2], [tForestFloor1, pForest2]]
	];

	let size = numForest / (scaleByMapSize(2, 8) * numPlayers);
	let num = Math.floor(size / types.length);
	for (let type of types)
		createAreas(
			new ClumpPlacer(numForest / num, 0.1, 0.1, 1),
			[
				new LayeredPainter(type, [2]),
				paintClass(clForest)
			],
			[avoidClasses(clPlayer, 20, clForest, randIntInclusive(5, 15), clHill, 2), stayClasses(clLand, 4)],
			num);
	Engine.SetProgress(50);

	log("Creating dirt patches...");
	let patchCount = (currentBiome() == "savanna" ? 3 : 1) * scaleByMapSize(15, 45);
	for (let size of [scaleByMapSize(3, 48), scaleByMapSize(5, 84), scaleByMapSize(8, 128)])
		createAreas(
			new ClumpPlacer(size, 0.3, 0.06, 0.5),
			[
				new LayeredPainter([[tMainTerrain, tTier1Terrain], [tTier1Terrain, tTier2Terrain], [tTier2Terrain, tTier3Terrain]], [1, 1]),
				paintClass(clDirt)
			],
			[avoidClasses(clForest, 0, clHill, 2, clDirt, 5, clPlayer, 7), stayClasses(clLand, 4)],
			patchCount);

	log("Creating grass patches...");
	for (let size of [scaleByMapSize(2, 32), scaleByMapSize(3, 48), scaleByMapSize(5, 80)])
		createAreas(
				new ClumpPlacer(size, 0.3, 0.06, 0.5),
			new TerrainPainter(tTier4Terrain),
			[avoidClasses(clForest, 0, clHill, 2, clDirt, 5, clPlayer, 7), stayClasses(clLand, 4)],
			patchCount);

	Engine.SetProgress(55);

	log("Creating stone mines...");
	createObjectGroupsDeprecated(
		new SimpleGroup([new SimpleObject(oStoneSmall, 0, 2, 0, 4), new SimpleObject(oStoneLarge, 1, 1, 0, 4)], true, clRock),
		0,
		[avoidClasses(clForest, 1, clPlayer, 10, clRock, 10, clHill, 2), stayClasses(clLand, 3)],
		randIntInclusive(scaleByMapSize(2, 9), scaleByMapSize(9, 40)),
		100);

	log("Creating small stone quarries...");
	createObjectGroupsDeprecated(
		new SimpleGroup([new SimpleObject(oStoneSmall, 2, 5, 1, 3)], true, clRock),
		0,
		[avoidClasses(clForest, 1, clPlayer, 10, clRock, 10, clHill, 2), stayClasses(clLand, 3)],
		randIntInclusive(scaleByMapSize(2, 9),scaleByMapSize(9, 40)),
		100);

	log("Creating metal mines...");
	createObjectGroupsDeprecated(
		new SimpleGroup([new SimpleObject(oMetalLarge, 1, 1, 0, 4)], true, clMetal),
		0,
		[avoidClasses(clForest, 1, clPlayer, 10, clMetal, 10, clRock, 5, clHill, 2), stayClasses(clLand, 3)],
		randIntInclusive(scaleByMapSize(2, 9), scaleByMapSize(9, 40)),
		100);
	Engine.SetProgress(65);

	log("Creating small decorative rocks...");
	createObjectGroupsDeprecated(
		new SimpleGroup([new SimpleObject(aRockMedium, 1, 3, 0, 1)], true),
		0,
		[avoidClasses(clWater, 0, clForest, 0, clPlayer, 0, clHill, 2), stayClasses(clLand, 3)],
		scaleByMapSize(16, 262),
		50);

	log("Creating large decorative rocks...");
	createObjectGroupsDeprecated(
		new SimpleGroup([new SimpleObject(aRockLarge, 1, 2, 0, 1), new SimpleObject(aRockMedium, 1, 3, 0, 2)], true),
		0,
		[avoidClasses(clWater, 0, clForest, 0, clPlayer, 0, clHill, 2), stayClasses(clLand, 3)],
		scaleByMapSize(8, 131),
		50);
	Engine.SetProgress(70);

	log("Creating deer...");
	createObjectGroupsDeprecated(
		new SimpleGroup([new SimpleObject(oMainHuntableAnimal, 5, 7, 0, 4)], true, clFood),
		0,
		[avoidClasses(clWater, 0, clForest, 0, clPlayer, 8, clHill, 2, clFood, 20), stayClasses(clLand, 2)],
		randIntInclusive(numPlayers + 3, 5 * numPlayers + 4),
		50);

	log("Creating berry bush...");
	createObjectGroupsDeprecated(
		new SimpleGroup([new SimpleObject(oFruitBush, 5, 7, 0, 4)], true, clFood),
		0,
		[avoidClasses(clWater, 0, clForest, 0, clPlayer, 8, clHill, 2, clFood, 20), stayClasses(clLand, 2)],
		randIntInclusive(1, 4) * numPlayers + 2,
		50);
	Engine.SetProgress(75);

	log("Creating sheep...");
	createObjectGroupsDeprecated(
		new SimpleGroup([new SimpleObject(oSecondaryHuntableAnimal, 2, 3, 0, 2)], true, clFood),
		0,
		[avoidClasses(clWater, 0, clForest, 0, clPlayer, 8, clHill, 2, clFood, 20), stayClasses(clLand, 2)],
		randIntInclusive(numPlayers + 3, 5 * numPlayers + 4),
		50);

	log("Creating fish...");
	createObjectGroupsDeprecated(
		new SimpleGroup([new SimpleObject(oFish, 2, 3, 0, 2)], true, clFood),
		0,
		avoidClasses(clLand, 4, clForest, 0, clPlayer, 0, clHill, 2, clFood, 20),
		randIntInclusive(15, 40) * numPlayers,
		60);
	Engine.SetProgress(85);

	log("Creating straggler trees...");
	types = [g_Gaia.tree1, g_Gaia.tree2, g_Gaia.tree3, g_Gaia.tree4];

	num = Math.floor(numStragglers / types.length);
	for (let type of types)
		createObjectGroupsDeprecated(
			new SimpleGroup([new SimpleObject(type, 1, 1, 0, 3)], true, clForest),
			0,
			[avoidClasses(clWater, 1, clForest, 1, clHill, 2, clPlayer, 0, clMetal, 6, clRock, 6, clBaseResource, 6), stayClasses(clLand, 4)],
			num);

	let planetm = currentBiome() == "tropic" ? 8 : 1;

	log("Creating small grass tufts...");
	createObjectGroupsDeprecated(
		new SimpleGroup([new SimpleObject(aGrassShort, 1, 2, 0, 1, -Math.PI / 8, Math.PI / 8)]),
		0,
		[avoidClasses(clWater, 2, clHill, 2, clPlayer, 2, clDirt, 0), stayClasses(clLand, 3)],
		planetm * scaleByMapSize(13, 200));
	Engine.SetProgress(90);

	log("Creating large grass tufts...");
	createObjectGroupsDeprecated(
		new SimpleGroup([new SimpleObject(aGrass, 2, 4, 0, 1.8, -Math.PI / 8, Math.PI / 8), new SimpleObject(aGrassShort, 3, 6, 1.2, 2.5, -Math.PI / 8, Math.PI / 8)]),
		0,
		[avoidClasses(clWater, 3, clHill, 2, clPlayer, 2, clDirt, 1, clForest, 0), stayClasses(clLand, 3)],
		planetm * scaleByMapSize(13, 200));
	Engine.SetProgress(95);

	log("Creating shallow flora...");
	createObjectGroupsDeprecated(
		new SimpleGroup([new SimpleObject(aLillies, 1, 2, 0, 2), new SimpleObject(aReeds, 2, 4, 0, 2)]),
		0,
		stayClasses(clShallow, 1),
		60 * scaleByMapSize(13, 200),
		80);

	log("Creating bushes...");
	createObjectGroupsDeprecated(
		new SimpleGroup([new SimpleObject(aBushMedium, 1, 2, 0, 2), new SimpleObject(aBushSmall, 2, 4, 0, 2)]),
		0,
		[avoidClasses(clWater, 1, clHill, 2, clPlayer, 1, clDirt, 1), stayClasses(clLand, 3)],
		planetm * scaleByMapSize(13, 200),
		50);

	setSkySet(pickRandom(["cirrus", "cumulus", "sunny", "sunny 1", "mountainous", "stratus"]));
	setSunRotation(randFloat(0, 2 * Math.PI));
	setSunElevation(Math.PI * randFloat(1/5, 1/3));
}

function createUnknownPlayerBases()
{
	placePlayerBases({
		"PlayerPlacement": [playerIDs, playerX, playerZ],
		"BaseResourceClass": clBaseResource,
		"Walls": g_StartingWalls,
		"CityPatch": {
			"outerTerrain": tRoadWild,
			"innerTerrain": tRoad,
			"painters": [
				paintClass(clPlayer)
			]
		},
		"Chicken": {
		},
		"Berries": {
			"template": oFruitBush
		},
		"Mines": {
			"types": [
				{ "template": oMetalLarge },
				{ "template": oStoneLarge }
			]
		},
		"Treasures": {
			"types": [
				{
					"template": oWoodTreasure,
					"count": g_StartingTreasures ? 14 : 0
				}
			]
		},
		"Trees": {
			"template": oTree1
		},
		"Decoratives": {
			"template": aGrassShort
		}
	});
}
