/**
 * @file This library is used to generate different map variations on the map Unknown, Unknown Land and Unknown Nomad.
 */

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

const heightSeaGround = -5;
const heightLand = 3;
const heightCliff = 3.12;
const heightHill = 18;
const heightOffsetBump = 2;

var g_Map = new RandomMap(heightSeaGround, tWater);

const numPlayers = getNumPlayers();
const mapSize = g_Map.getSize();
const mapCenter = g_Map.getCenter();
const mapBounds = g_Map.getBounds();

var clPlayer = g_Map.createTileClass();
var clPlayerTerritory = g_Map.createTileClass();
var clHill = g_Map.createTileClass();
var clForest = g_Map.createTileClass();
var clWater = g_Map.createTileClass();
var clDirt = g_Map.createTileClass();
var clRock = g_Map.createTileClass();
var clMetal = g_Map.createTileClass();
var clFood = g_Map.createTileClass();
var clPeninsulaSteam = g_Map.createTileClass();
var clBaseResource = g_Map.createTileClass();
var clLand = g_Map.createTileClass();
var clShallow = g_Map.createTileClass();

var landElevationPainter = new SmoothElevationPainter(ELEVATION_SET, heightLand, 4);

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
var playerPosition = [];

var g_StartingTreasures = false;
var g_StartingWalls = true;

function createUnknownMap()
{
	let funcs = unknownMapFunctions.land;

	if (g_AllowNaval)
		funcs = funcs.concat(unknownMapFunctions.naval);

	global["unknown" + pickRandom(funcs)]();

	paintUnknownMapBasedOnHeight();

	createUnknownPlayerBases();

	createUnknownObjects();

	placePlayersNomad(clPlayer, avoidClasses(clForest, 1, clMetal, 4, clRock, 4, clHill, 4, clFood, 2, clWater, 10));
}

/**
 * Chain of islands or many disconnected islands.
 */
function unknownArchipelago()
{
	g_StartingWalls = "towers";
	g_StartingTreasures = true;

	let [pIDs, islandPosition] = playerPlacementCircle(fractionToTiles(0.35));
	if (!isNomad())
	{
		[playerIDs, playerPosition] = [pIDs, islandPosition];
		markPlayerArea("large");
	}

	log("Creating islands...");
	let islandSize = diskArea(scaleByMapSize(17, 29));
	for (let i = 0; i < numPlayers; ++i)
		createArea(
			new ClumpPlacer(islandSize, 0.8, 0.1, 10, islandPosition[i]),
			landElevationPainter);

	let type = isNomad() ? randIntInclusive(1, 2) : randIntInclusive(1, 3);
	if (type == 1)
	{
		log("Creating archipelago...");
		createAreas(
			new ClumpPlacer(islandSize * randFloat(0.8, 1.2), 0.8, 0.1, 10),
			[
				landElevationPainter,
				new TileClassPainter(clLand)
			],
			null,
			scaleByMapSize(2, 5) * randIntInclusive(8, 14));

		log("Creating shore jaggedness with small puddles...");
		createAreas(
			new ClumpPlacer(scaleByMapSize(15, 80), 0.2, 0.1, 1),
			[
				new SmoothElevationPainter(ELEVATION_SET, heightLand, 4),
				new TileClassPainter(clLand)
			],
			borderClasses(clLand, 6, 3),
			scaleByMapSize(12, 130) * 2,
			150);
	}
	else if (type == 2)
	{
		log("Creating islands...");
		createAreas(
			new ClumpPlacer(islandSize * randFloat(0.6, 1.4), 0.8, 0.1, randFloat(0.0, 0.2)),
			[
				landElevationPainter,
				new TileClassPainter(clLand)
			],
			avoidClasses(clLand, 3, clPlayerTerritory, 3),
			scaleByMapSize(6, 10) * randIntInclusive(8, 14));

		log("Creating small islands...");
		createAreas(
			new ClumpPlacer(islandSize * randFloat(0.3, 0.7), 0.8, 0.1, 0.07),
			[
				new SmoothElevationPainter(ELEVATION_SET, heightLand, 6),
				new TileClassPainter(clLand)
			],
			avoidClasses(clLand, 3, clPlayerTerritory, 3),
			scaleByMapSize(2, 6) * randIntInclusive(6, 15),
			25);
	}
	else if (type == 3)
	{
		log("Creating tight islands...");
		createAreas(
			new ClumpPlacer(islandSize * randFloat(0.8, 1.2), 0.8, 0.1, 10),
			[
				landElevationPainter,
				new TileClassPainter(clLand)
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

	if (!isNomad())
	{
		log("Ensuring player area...");
		[playerIDs, playerPosition] = playerPlacementCircle(fractionToTiles(0.25));
		markPlayerArea("small");

		for (let i = 0; i < numPlayers; ++i)
			createArea(
				new ChainPlacer(
					2,
					Math.floor(scaleByMapSize(5, 9)),
					Math.floor(scaleByMapSize(5, 20)),
					1,
					playerPosition[i],
					0,
					[Math.floor(scaleByMapSize(23, 50))]),
				[
					landElevationPainter,
					new TileClassPainter(clLand)
				]);
	}

	log("Creating continent...");
	createArea(
		new ClumpPlacer(diskArea(fractionToTiles(0.38)), 0.9, 0.09, 10, mapCenter),
		[
			landElevationPainter,
			new TileClassPainter(clLand)
		]);

	if (randBool(1/3))
	{
		log("Creating peninsula (i.e. half the map not being surrounded by water)...");
		let angle = randomAngle();
		let peninsulaPosition1 = Vector2D.add(mapCenter, new Vector2D(fractionToTiles(0.25), 0).rotate(-angle));
		createArea(
			new ClumpPlacer(diskArea(fractionToTiles(0.38)), 0.9, 0.09, 10, peninsulaPosition1),
			[
				landElevationPainter,
				new TileClassPainter(clLand)
			]);

		log("Remembering to not paint shorelines into the peninsula...");
		let peninsulaPosition2 = Vector2D.add(mapCenter, new Vector2D(fractionToTiles(0.35), 0).rotate(-angle));
		createArea(
			new ClumpPlacer(diskArea(fractionToTiles(0.33)), 0.9, 0.01, 10, peninsulaPosition2),
			new TileClassPainter(clPeninsulaSteam));
	}

	createShoreJaggedness(waterHeight, clLand, 7);
}

/**
 * Creates a huge central river, possibly connecting the riversides with a narrow piece of land.
 */
function unknownCentralSea()
{
	let waterHeight = -3;

	let startAngle = randomAngle();

	let [riverStart, riverEnd] = centralRiverCoordinates(startAngle);

	paintRiver({
		"parallel": false,
		"start": riverStart,
		"end": riverEnd,
		"width": fractionToTiles(scaleByMapSize(0.27, 0.42) + randFloat(0, 0.08)),
		"fadeDist": scaleByMapSize(3, 12),
		"deviation": 0,
		"heightRiverbed": waterHeight,
		"heightLand": heightLand,
		"meanderShort": 20,
		"meanderLong": 0,
		"waterFunc": (position, height, riverFraction) => {
			if (height < 0)
				addToClass(position.x, position.y, clWater);
		},
		"landFunc": (position, shoreDist1, shoreDist2) => {
			g_Map.setHeight(position, 3.1);
			addToClass(position.x, position.y, clLand);
		}
	});

	if (!isNomad())
	{
		[playerIDs, playerPosition] = playerPlacementRiver(startAngle + Math.PI / 2, fractionToTiles(0.6));
		markPlayerArea("small");
	}

	if (!g_AllowNaval || randBool())
	{
		log("Creating isthmus (i.e. connecting the two riversides with a big land passage)...");
		let [isthmusStart, isthmusEnd] = centralRiverCoordinates(startAngle + Math.PI / 2);
		createArea(
			new PathPlacer(
				isthmusStart,
				isthmusEnd,
				scaleByMapSize(randIntInclusive(16, 24), randIntInclusive(100, 140)),
				0.5,
				3 * scaleByMapSize(1, 4),
				0.1,
				0.01),
			[
				landElevationPainter,
				new TileClassPainter(clLand),
				new TileClassUnPainter(clWater)
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
	let heightShallow = -2;

	createArea(
		new MapBoundsPlacer(),
		new ElevationPainter(heightLand));

	let startAngle = randomAngle();

	if (!isNomad())
	{
		[playerIDs, playerPosition] = playerPlacementRiver(startAngle + Math.PI / 2, fractionToTiles(0.5));
		markPlayerArea("large");
	}

	log("Creating the main river...");
	let [coord1, coord2] = centralRiverCoordinates(startAngle);
	createArea(
		new PathPlacer(coord1, coord2, scaleByMapSize(14, 24), 0.5, scaleByMapSize(3, 12), 0.1, 0.01),
		new SmoothElevationPainter(ELEVATION_SET, waterHeight, 4),
		avoidClasses(clPlayerTerritory, 4));

	log("Creating small water spots at the map border to ensure separation of players...");
	for (let coord of [coord1, coord2])
		createArea(
			new ClumpPlacer(diskArea(scaleByMapSize(5, 10)), 0.95, 0.6, 10, coord),
			new SmoothElevationPainter(ELEVATION_SET, waterHeight, 2),
			avoidClasses(clPlayerTerritory, 8));

	if (!g_AllowNaval || randBool())
	{
		log("Creating the shallows of the main river...");
		for (let i = 0; i <= randIntInclusive(1, scaleByMapSize(4, 8)); ++i)
		{
			let location = fractionToTiles(randFloat(0.15, 0.85));
			createPassage({
				"start": new Vector2D(location, mapBounds.top).rotateAround(startAngle, mapCenter),
				"end": new Vector2D(location, mapBounds.bottom).rotateAround(startAngle, mapCenter),
				"startWidth": scaleByMapSize(8, 12),
				"endWidth": scaleByMapSize(8, 12),
				"smoothWidth": 2,
				"startHeight": heightShallow,
				"endHeight": heightShallow,
				"maxHeight": heightShallow,
				"tileClass": clShallow
			});
		}
	}

	if (randBool(2/3))
		createTributaryRivers(
			startAngle,
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
	createArea(
		new MapBoundsPlacer(),
		new ElevationPainter(heightLand));

	let startAngle;
	if (!isNomad())
	{
		let playerAngle;
		[playerIDs, playerPosition, playerAngle, startAngle] = playerPlacementCircle(fractionToTiles(0.35));
		markPlayerArea("small");
	}

	let lake = randBool(3/4);
	if (lake)
	{
		log("Creating lake...");
		createArea(
			new ClumpPlacer(diskArea(fractionToTiles(0.17)), 0.7, 0.1, 10, mapCenter),
			[
				new SmoothElevationPainter(ELEVATION_SET, waterHeight, 4),
				new TileClassPainter(clWater)
			]);

		createShoreJaggedness(waterHeight, clWater, 3);
	}

	// Don't do this on nomad because the imbalances on the different islands are too drastic
	if (!isNomad() && (!lake || randBool(1/3)))
	{
		log("Creating small rivers separating players...");
		for (let river of distributePointsOnCircle(numPlayers, startAngle + Math.PI / numPlayers, fractionToTiles(0.5), mapCenter)[0])
		{
			createArea(
				new PathPlacer(mapCenter, river, scaleByMapSize(14, 24), 0.4, 3 * scaleByMapSize(1, 3), 0.2, 0.05),
				[
					new SmoothElevationPainter(ELEVATION_SET, waterHeight, 4),
					new TileClassPainter(clWater)
				],
				avoidClasses(clPlayer, 5));

			createArea(
				new ClumpPlacer(diskArea(scaleByMapSize(4, 22)), 0.95, 0.6, 10, river),
				[
					new SmoothElevationPainter(ELEVATION_SET, waterHeight, 0),
					new TileClassPainter(clWater)
				],
				avoidClasses(clPlayer, 5));
		}

		log("Creating lake...");
		createArea(
			new ClumpPlacer(diskArea(fractionToTiles(0.04)), 0.7, 0.1, 10, mapCenter),
			[
				new SmoothElevationPainter(ELEVATION_SET, waterHeight, 4),
				new TileClassPainter(clWater)
			]);
	}

	if (!isNomad && lake && randBool())
	{
		log("Creating small central island...");
		createArea(
			new ClumpPlacer(diskArea(fractionToTiles(0.05)), 0.7, 0.1, 10, mapCenter),
			[
				landElevationPainter,
				new TileClassPainter(clWater)
			]);
	}
}

/**
 * Align players on a land strip with seas bordering on one or both sides that can hold islands.
 */
function unknownEdgeSeas()
{
	let waterHeight = -4;

	createArea(
		new MapBoundsPlacer(),
		new ElevationPainter(heightLand));

	let startAngle = randomAngle();
	if (!isNomad())
	{
		playerIDs = sortAllPlayers();
		playerPosition = playerPlacementLine(startAngle + Math.PI / 2, mapCenter, fractionToTiles(0.2));
		// Don't place the shoreline inside the CC, but possibly into the players territory
		markPlayerArea("small");
	}

	for (let side of pickRandom([[0], [Math.PI], [0, Math.PI]]))
		paintRiver({
			"parallel": true,
			"start": new Vector2D(mapBounds.left, mapBounds.top).rotateAround(side + startAngle, mapCenter),
			"end": new Vector2D(mapBounds.left, mapBounds.bottom).rotateAround(side + startAngle, mapCenter),
			"width": scaleByMapSize(80, randFloat(270, 320)),
			"fadeDist": scaleByMapSize(2, 8),
			"deviation": 0,
			"heightRiverbed": waterHeight,
			"heightLand": heightLand,
			"meanderShort": 20,
			"meanderLong": 0
		});

	createExtensionsOrIslands();
	paintTileClassBasedOnHeight(0, heightCliff, 1, clLand);
	createShoreJaggedness(waterHeight, clLand, 7, false);
}

/**
 * Land shaped like a concrescent moon around a central lake.
 */
function unknownGulf()
{
	let waterHeight = -3;

	createArea(
		new MapBoundsPlacer(),
		new ElevationPainter(heightLand));

	let startAngle = randomAngle();
	if (!isNomad())
	{
		log("Determining player locations...");

		playerPosition = playerPlacementCustomAngle(
			fractionToTiles(0.35),
			mapCenter,
			i => startAngle + 2/3 * Math.PI * (-1 + (numPlayers == 1 ? 1 : 2 * i / (numPlayers - 1))))[0];

		markPlayerArea("large");
	}

	let gulfParts = [
		{ "radius": fractionToTiles(0.16), "distance": fractionToTiles(0) },
		{ "radius": fractionToTiles(0.2), "distance": fractionToTiles(0.2) },
		{ "radius": fractionToTiles(0.22), "distance": fractionToTiles(0.49) }
	];

	for (let gulfPart of gulfParts)
	{
		let position = Vector2D.sub(mapCenter, new Vector2D(gulfPart.distance, 0).rotate(-startAngle)).round();
		createArea(
			new ClumpPlacer(diskArea(gulfPart.radius), 0.7, 0.05, 10, position),
			[
				new SmoothElevationPainter(ELEVATION_SET, waterHeight, 4),
				new TileClassPainter(clWater)
			],
			avoidClasses(clPlayerTerritory, defaultPlayerBaseRadius()));
	}
}

/**
 * Mainland style with some small random lakes.
 */
function unknownLakes()
{
	let waterHeight = -5;

	createArea(
		new MapBoundsPlacer(),
		new ElevationPainter(heightLand));

	if (!isNomad())
	{
		[playerIDs, playerPosition] = playerPlacementCircle(fractionToTiles(0.35));
		markPlayerArea("large");
	}

	log("Creating lakes...");
	createAreas(
		new ClumpPlacer(scaleByMapSize(160, 700), 0.2, 0.1, 1),
		[
			new SmoothElevationPainter(ELEVATION_SET, waterHeight, 5),
			new TileClassPainter(clWater)
		],
		[avoidClasses(clPlayerTerritory, 12), randBool() ? avoidClasses(clWater, 8) : new NullConstraint()],
		scaleByMapSize(5, 16));
}

/**
 * A large hill leaving players only a small passage to each of the the two neighboring players.
 */
function unknownPasses()
{
	let heightMountain = 24;
	let waterHeight = -4;

	createArea(
		new MapBoundsPlacer(),
		new ElevationPainter(heightLand));

	let playerAngle;
	let startAngle;
	if (!isNomad())
	{
		[playerIDs, playerPosition, playerAngle, startAngle] = playerPlacementCircle(fractionToTiles(0.35));
		markPlayerArea("small");
	}
	else
		startAngle = randomAngle();

	for (let mountain of distributePointsOnCircle(numPlayers, startAngle + Math.PI / numPlayers, fractionToTiles(0.5), mapCenter)[0])
	{
		log("Creating a mountain range between neighboring players...");
		createArea(
			new PathPlacer(mapCenter, mountain, scaleByMapSize(14, 24), 0.4, 3 * scaleByMapSize(1, 3), 0.2, 0.05),
			[
				// More smoothing than this often results in the mountainrange becoming passable to one player.
				new SmoothElevationPainter(ELEVATION_SET, heightMountain, 1),
				new TileClassPainter(clWater)
			],
			avoidClasses(clPlayer, 5));

		log("Creating small mountain at the map border between the players to ensure separation of players...");
		createArea(
			new ClumpPlacer(diskArea(scaleByMapSize(4, 22)), 0.95, 0.6, 10, mountain),
			new SmoothElevationPainter(ELEVATION_SET, heightMountain, 0),
			avoidClasses(clPlayer, 5));
	}

	let passes = distributePointsOnCircle(numPlayers * 2, startAngle, fractionToTiles(0.35), mapCenter)[0];
	for (let i = 0; i < numPlayers; ++i)
	{
		log("Create passages between neighboring players...");
		createArea(
			new PathPlacer(
				passes[2 * i],
				passes[2 * ((i + 1) % numPlayers)],
				scaleByMapSize(14, 24),
				0.4,
				3 * scaleByMapSize(1, 3),
				0.2,
				0.05),
			new SmoothElevationPainter(ELEVATION_SET, heightLand, 2));
	}

	if (randBool(2/5))
	{
		log("Create central lake...");
		createArea(
			new ClumpPlacer(diskArea(fractionToTiles(0.1)), 0.7, 0.1, 10, mapCenter),
			[
				new SmoothElevationPainter(ELEVATION_SET, waterHeight, 3),
				new TileClassPainter(clWater)
			]);
	}
	else
	{
		log("Fill area between the paths...");
		createArea(
			new ClumpPlacer(diskArea(fractionToTiles(0.05)), 0.7, 0.1, 10, mapCenter),
			[
				new SmoothElevationPainter(ELEVATION_SET, heightMountain, 4),
				new TileClassPainter(clWater)
			]);
	}
}

/**
 * Land enclosed by a hill that leaves small areas for civic centers and large central place.
 */
function unknownLowlands()
{
	let heightMountain = 30;

	log("Creating mountain that is going to separate players...");
	createArea(
		new MapBoundsPlacer(),
		new ElevationPainter(heightMountain));

	let playerAngle;
	let startAngle;
	if (!isNomad())
	{
		[playerIDs, playerPosition, playerAngle, startAngle] = playerPlacementCircle(fractionToTiles(0.35));
		markPlayerArea("small");
	}
	else
		startAngle = randomAngle();

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
			new ClumpPlacer(diskArea(scaleByMapSize(18, 32)), 0.65, 0.1, 10, valley),
			[
				new SmoothElevationPainter(ELEVATION_SET, heightLand, 2),
				new TileClassPainter(clLand)
			]);

		log("Creating passes from player areas to the center...");
		createArea(
			new PathPlacer(mapCenter, valley, scaleByMapSize(14, 24), 0.4, 3 * scaleByMapSize(1, 3), 0.2, 0.05),
			[
				landElevationPainter,
				new TileClassPainter(clWater)
			]);
	}

	log("Creating the big central area...");
	createArea(
		new ClumpPlacer(diskArea(fractionToTiles(0.18)), 0.7, 0.1, 10, mapCenter),
		[
			landElevationPainter,
			new TileClassPainter(clWater)
		]);
}

/**
 * No water, no hills.
 */
function unknownMainland()
{
	createArea(
		new MapBoundsPlacer(),
		new ElevationPainter(3));

	if (!isNomad())
	{
		[playerIDs, playerPosition] = playerPlacementCircle(fractionToTiles(0.35));
		markPlayerArea("small");
	}
}

function centralRiverCoordinates(angle)
{
	return [
		new Vector2D(mapBounds.left + 1, mapCenter.y),
		new Vector2D(mapBounds.right - 1, mapCenter.y)
	].map(v => v.rotateAround(angle, mapCenter));
}

function createShoreJaggedness(waterHeight, borderClass, shoreDist, inwards = true)
{
	log("Creating shore jaggedness...");
	for (let i = 0; i < 2; ++i)
		if (i || inwards)
			createAreas(
				new ChainPlacer(2, Math.floor(scaleByMapSize(4, 6)), 15, 1),
				[
						new SmoothElevationPainter(ELEVATION_SET, i ? heightLand : waterHeight, 4),
						i ? new TileClassPainter(clLand) : new TileClassUnPainter(clLand)
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
				new TileClassPainter(clLand)
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
				new TileClassPainter(clLand)
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
		addCivicCenterAreaToClass(playerPosition[i], clPlayer);

		if (size == "large")
			createArea(
				new ClumpPlacer(diskArea(scaleByMapSize(17, 29) / 3), 0.6, 0.3, 10, playerPosition[i]),
				new TileClassPainter(clPlayerTerritory));
	}
}

function paintUnknownMapBasedOnHeight()
{
	paintTerrainBasedOnHeight(heightCliff, 40, 1, tCliff);
	paintTerrainBasedOnHeight(3, heightCliff, 1, tMainTerrain);
	paintTerrainBasedOnHeight(1, 3, 1, tShore);
	paintTerrainBasedOnHeight(-8, 1, 2, tWater);

	unPaintTileClassBasedOnHeight(0, heightCliff, 1, clWater);
	unPaintTileClassBasedOnHeight(-6, 0, 1, clLand);

	paintTileClassBasedOnHeight(-6, 0, 1, clWater);
	paintTileClassBasedOnHeight(0, heightCliff, 1, clLand);
	paintTileClassBasedOnHeight(heightCliff, 40, 1, clHill);
}

/**
 * Place resources and decoratives after the player territory was marked.
 */
function createUnknownObjects()
{
	log("Creating bumps...");
	createAreas(
		new ClumpPlacer(scaleByMapSize(20, 50), 0.3, 0.06, 1),
		new SmoothElevationPainter(ELEVATION_MODIFY, heightOffsetBump, 2),
		[avoidClasses(clWater, 2, clPlayer, 10), stayClasses(clLand, 3)],
		randIntInclusive(0, scaleByMapSize(1, 2) * 200));

	log("Creating hills...");
	createAreas(
		new ClumpPlacer(scaleByMapSize(20, 150), 0.2, 0.1, 1),
		[
			new LayeredPainter([tCliff, tHill], [2]),
			new SmoothElevationPainter(ELEVATION_SET, heightHill, 2),
			new TileClassPainter(clHill)
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
				new TileClassPainter(clForest)
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
				new TileClassPainter(clDirt)
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
	setSunRotation(randomAngle());
	setSunElevation(Math.PI * randFloat(1/5, 1/3));
}

function createUnknownPlayerBases()
{
	placePlayerBases({
		"PlayerPlacement": [playerIDs, playerPosition],
		"BaseResourceClass": clBaseResource,
		"Walls": g_StartingWalls,
		"CityPatch": {
			"outerTerrain": tRoadWild,
			"innerTerrain": tRoad,
			"painters": [
				new TileClassPainter(clPlayer)
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
