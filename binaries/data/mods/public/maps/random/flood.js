RMS.LoadLibrary("rmgen");

let random_terrain = randomizeBiome([g_BiomeSavanna]);

const tMainTerrain = rBiomeT1();
const tForestFloor1 = rBiomeT2();
const tForestFloor2 = rBiomeT3();
const tCliff = rBiomeT4();
const tTier1Terrain = rBiomeT5();
const tTier2Terrain = rBiomeT6();
const tTier3Terrain = rBiomeT7();
const tRoad = rBiomeT10();
const tRoadWild = rBiomeT11();
const tTier4Terrain = rBiomeT12();
const tShore = rBiomeT14();
const tWater = rBiomeT15();
let tHill = rBiomeT8();
let tDirt = rBiomeT9();

if (random_terrain == g_BiomeTemperate)
{
	tDirt = ["medit_shrubs_a", "grass_field"];
	tHill = ["grass_field", "peat_temp"];
}

// Gaia entities
const oTree1 = rBiomeE1();
const oTree2 = rBiomeE2();
const oTree3 = rBiomeE3();
const oTree4 = rBiomeE4();
const oTree5 = rBiomeE5();
const oFruitBush = rBiomeE6();
const oMainHuntableAnimal = rBiomeE8();
const oFish = rBiomeE9();
const oSecondaryHuntableAnimal = rBiomeE10();
const oStoneLarge = rBiomeE11();
const oMetalLarge = rBiomeE13();

// Decorative props
const aGrass = rBiomeA1();
const aGrassShort = rBiomeA2();
const aRockLarge = rBiomeA5();
const aRockMedium = rBiomeA6();
const aBushMedium = rBiomeA7();
const aBushSmall = rBiomeA8();

const pForest1 = [tForestFloor2 + TERRAIN_SEPARATOR + oTree1, tForestFloor2 + TERRAIN_SEPARATOR + oTree2, tForestFloor2];
const pForest2 = [tForestFloor1 + TERRAIN_SEPARATOR + oTree4, tForestFloor1 + TERRAIN_SEPARATOR + oTree5, tForestFloor1];

log("Initializing map...");
InitMap();

const radius = scaleByMapSize(15, 25);
const elevation = 2;
const shoreRadius = 6;
const numPlayers = getNumPlayers();
const mapSize = getMapSize();
const mapArea = mapSize * mapSize;
const centerOfMap = mapSize / 2;

// Create tile classes
let clPlayer = createTileClass();
let clHill = createTileClass();
let clMountain = createTileClass();
let clForest = createTileClass();
let clWater = createTileClass();
let clDirt = createTileClass();
let clRock = createTileClass();
let clMetal = createTileClass();
let clFood = createTileClass();
let clBaseResource = createTileClass();

for (let ix = 0; ix < mapSize; ++ix)
	for (let iz = 0; iz < mapSize; ++iz)
		placeTerrain(ix, iz, tWater);

// Randomize player order
let playerIDs = [];
for (let i = 0; i < numPlayers; ++i)
	playerIDs.push(i+1);
playerIDs = sortPlayers(playerIDs);

// Place players
let playerX = [];
let playerZ = [];
let playerAngle = [];

let startAngle = randFloat(0, TWO_PI);
for (let i = 0; i < numPlayers; ++i)
{
	playerAngle[i] = startAngle + i * TWO_PI/numPlayers;
	playerX[i] = 0.5 + 0.38 * cos(playerAngle[i]);
	playerZ[i] = 0.5 + 0.38 * sin(playerAngle[i]);
}

let fx = fractionToTiles(0.5);
let fz = fractionToTiles(0.5);
let ix = round(fx);
let iz = round(fz);

// Create the water
let placer = new ClumpPlacer(mapArea * 1, 1, 1, 1, ix, iz);
let terrainPainter = new LayeredPainter(
    [tWater, tWater, tShore], // terrains
    [1, 4] // widths
);
let elevationPainter = new SmoothElevationPainter(
   ELEVATION_SET,      // type
   getMapBaseHeight(), // elevation
   2                   // blend radius
);
createArea(placer, [terrainPainter, elevationPainter, paintClass(clWater)], avoidClasses(clPlayer, 5));

for (let i = 0; i < numPlayers; ++i)
{
	let id = playerIDs[i];
	log("Creating base for player " + id + "...");

	// Get the x and z in tiles
	let fx = fractionToTiles(playerX[i]);
	let fz = fractionToTiles(playerZ[i]);
	let ix = round(fx);
	let iz = round(fz);

	let hillSize = PI * radius * radius * 2;

	// Create the hill
	let placer = new ClumpPlacer(hillSize, 0.80, 0.1, 10, ix, iz);
	let terrainPainter = new LayeredPainter(
		[tShore, tMainTerrain],	// terrains
		[shoreRadius] // widths
	);
	let elevationPainter = new SmoothElevationPainter(
		ELEVATION_SET,			// type
		elevation,				// elevation
		shoreRadius				// blend radius
	);
	createArea(placer, [terrainPainter, elevationPainter, paintClass(clHill)], null);

	// Mark a small area around the player's starting coördinates with the clPlayer class
	addToClass(ix, iz, clPlayer);
	addToClass(ix + 5, iz, clPlayer);
	addToClass(ix, iz + 5, clPlayer);
	addToClass(ix - 5, iz, clPlayer);
	addToClass(ix, iz - 5, clPlayer);

	placeCivDefaultEntities(fx, fz, id, { "iberWall": false });

	// Create the city patch
	let cityRadius = radius/3;
	placer = new ClumpPlacer(PI * cityRadius * cityRadius, 0.6, 0.3, 10, ix, iz);
	let painter = new LayeredPainter([tRoadWild, tRoad], [1]);
	createArea(placer, painter, null);

	placeDefaultChicken(fx, fz, clBaseResource);

	// Create berry bushes
	let bbAngle = randFloat(0, TWO_PI);
	let bbDist = 12;
	let bbX = round(fx + bbDist * cos(bbAngle));
	let bbZ = round(fz + bbDist * sin(bbAngle));
	let group = new SimpleGroup(
		[new SimpleObject(oFruitBush, 5, 5, 0, 3)],
		true, clBaseResource, bbX, bbZ
	);
	createObjectGroup(group, 0);

	// Create metal mine
	let mAngle = bbAngle;
	while (abs(mAngle - bbAngle) < PI/3)
		mAngle = randFloat(0, TWO_PI);

	let mDist = 12;
	let mX = round(fx + mDist * cos(mAngle));
	let mZ = round(fz + mDist * sin(mAngle));
	group = new SimpleGroup(
		[new SimpleObject(oMetalLarge, 1, 1, 0, 0)],
		true, clBaseResource, mX, mZ
	);
	createObjectGroup(group, 0);

	// Create stone mines
	mAngle += randFloat(PI/8, PI/4);
	mX = round(fx + mDist * cos(mAngle));
	mZ = round(fz + mDist * sin(mAngle));
	group = new SimpleGroup(
		[new SimpleObject(oStoneLarge, 1, 1, 0, 2)],
		true, clBaseResource, mX, mZ
	);
	createObjectGroup(group, 0);

	// Create starting trees, should avoid mines and bushes
	let tries = 50;
	let tDist = 16;
	let num = 50;
	for (let x = 0; x < tries; ++x)
	{
		let tAngle = randFloat(0, TWO_PI);
		let tX = round(fx + tDist * cos(tAngle));
		let tZ = round(fz + tDist * sin(tAngle));
		group = new SimpleGroup(
			[new SimpleObject(oTree2, num, num, 0, 7)],
			true, clBaseResource, tX, tZ
		);
		if (createObjectGroup(group, 0, avoidClasses(clBaseResource, 5)))
			break;
	}

	placeDefaultDecoratives(fx, fz, aGrassShort, clBaseResource, radius);
}

RMS.SetProgress(40);

// Create central island
placer = new ChainPlacer(floor(scaleByMapSize(6, 6)), floor(scaleByMapSize(10, 15)), floor(scaleByMapSize(200, 300)), 1, centerOfMap, centerOfMap, 0, [floor(mapSize * 0.01)]);
terrainPainter = new LayeredPainter(
	[tShore, tMainTerrain], // terrains
	[shoreRadius, 100]      // widths
);
elevationPainter = new SmoothElevationPainter(
	ELEVATION_SET, // type
	elevation,     // elevation
	shoreRadius    // blend radius
);
createArea(placer, [terrainPainter, elevationPainter, paintClass(clHill)], avoidClasses(clPlayer, 40));

for (let m = 0; m < randIntInclusive(20, 34); ++m)
{
	let placer = new ChainPlacer(
		Math.floor(scaleByMapSize(7, 7)),
		Math.floor(scaleByMapSize(15, 15)),
		Math.floor(scaleByMapSize(15, 20)),
		1,
		randIntExclusive(0, mapSize),
		randIntExclusive(0, mapSize),
		0,
		[Math.floor(mapSize * 0.01)]);

	let elevRand = randIntInclusive(6, 20);
	let terrainPainter = new LayeredPainter(
		[tDirt, tHill],        // terrains
		[floor(elevRand / 3), 40]       // widths
	);
	let elevationPainter = new SmoothElevationPainter(
		ELEVATION_SET,      // type
		elevRand,           // elevation
		floor(elevRand / 3)	// blend radius
	);
	createArea(placer, [terrainPainter, elevationPainter, paintClass(clHill)], [avoidClasses(clBaseResource, 2, clPlayer, 40), stayClasses(clHill, 6)]);
}

for (let m = 0; m < randIntInclusive(8, 17); ++m)
{
	let placer = new ChainPlacer(
		Math.floor(scaleByMapSize(5, 5)),
		Math.floor(scaleByMapSize(8, 8)),
		Math.floor(scaleByMapSize(15, 20)),
		1,
		randIntExclusive(0, mapSize),
		randIntExclusive(0, mapSize),
		0,
		[Math.floor(mapSize * 0.01)]);

	let elevRand = randIntInclusive(15, 29);
	let terrainPainter = new LayeredPainter(
		[tCliff, tForestFloor2],        // terrains
		[floor(elevRand / 3), 40]       // widths
	);
	let elevationPainter = new SmoothElevationPainter(
		ELEVATION_MODIFY,   // type
		elevRand,           // elevation
		floor(elevRand / 3) // blend radius
	);
	createArea(placer, [terrainPainter, elevationPainter, paintClass(clMountain)], [avoidClasses(clBaseResource, 2, clPlayer, 40), stayClasses(clHill, 6)]);
}

// Create center bounty
let group = new SimpleGroup(
	[new SimpleObject(oMetalLarge, 3, 6, 25, floor(mapSize * 0.25))],
	true, clBaseResource, centerOfMap, centerOfMap
);
createObjectGroup(group, 0, [avoidClasses(clBaseResource, 20, clPlayer, 40, clMountain, 4), stayClasses(clHill, 10)]);
group = new SimpleGroup(
	[new SimpleObject(oStoneLarge, 3, 6, 25, floor(mapSize * 0.25))],
	true, clBaseResource, centerOfMap, centerOfMap
);
createObjectGroup(group, 0, [avoidClasses(clBaseResource, 20, clPlayer, 40, clMountain, 4), stayClasses(clHill, 10)]);
group = new SimpleGroup(
	[new SimpleObject(oMainHuntableAnimal, floor(6 * numPlayers), floor(6 * numPlayers), 2, floor(mapSize * 0.1))],
	true, clBaseResource, centerOfMap, centerOfMap
);
createObjectGroup(group, 0, [avoidClasses(clBaseResource, 2, clMountain, 4, clPlayer, 40, clWater, 2), stayClasses(clHill, 10)]);

log("Creating fish...");
group = new SimpleGroup(
	[new SimpleObject(oFish, 2, 3, 0, 2)],
	true, clFood
);
createObjectGroupsDeprecated(group, 0,
	avoidClasses(clHill, 10, clFood, 20),
	10 * numPlayers, 60
);

createForests(
	[tMainTerrain, tForestFloor1, tForestFloor2, pForest1, pForest2],
	[avoidClasses(clPlayer, 25, clForest, 10, clBaseResource, 3, clMetal, 6, clRock, 3, clMountain, 2), stayClasses(clHill, 6)],
	clForest,
	0.7,
	random_terrain
);

log("Creating straggeler trees...");
let types = [oTree1, oTree2, oTree4, oTree3];
createStragglerTrees(types, [avoidClasses(clBaseResource, 2, clMetal, 6, clRock, 3, clMountain, 2, clPlayer, 25), stayClasses(clHill, 6)]);

RMS.SetProgress(65);

log("Creating dirt patches...");
let sizes = [scaleByMapSize(3, 6), scaleByMapSize(5, 10), scaleByMapSize(8, 21)];
let numb = random_terrain == g_BiomeSavanna ? 3 : 1;

for (let i = 0; i < sizes.length; ++i)
{
	placer = new ChainPlacer(1, floor(scaleByMapSize(3, 5)), sizes[i], 0.5);
	let painter = new LayeredPainter(
		[[tMainTerrain, tTier1Terrain], [tTier1Terrain, tTier2Terrain], [tTier2Terrain, tTier3Terrain]], // terrains
		[1, 1] // widths
	);
	createAreas(
		placer,
		[painter, paintClass(clDirt)],
		avoidClasses(clForest, 0, clMountain, 0, clDirt, 5, clPlayer, 10),
		numb * scaleByMapSize(15, 45)
	);
}

log("Painting shorelines...");
paintTerrainBasedOnHeight(1, 2, 0, tMainTerrain);
paintTerrainBasedOnHeight(getMapBaseHeight(), 1, 3, tTier1Terrain);

log("Creating grass patches...");
sizes = [scaleByMapSize(2, 4), scaleByMapSize(3, 7), scaleByMapSize(5, 15)];
for (let i = 0; i < sizes.length; ++i)
{
	placer = new ChainPlacer(1, floor(scaleByMapSize(3, 5)), sizes[i], 0.5);
	let painter = new TerrainPainter(tTier4Terrain);
	createAreas(
		placer,
		painter,
		avoidClasses(clForest, 0, clMountain, 0, clDirt, 5, clPlayer, 10),
		numb * scaleByMapSize(15, 45)
	);
}

log("Creating food...");
createFood(
	[
		[new SimpleObject(oMainHuntableAnimal, 5, 7, 0, 4)],
		[new SimpleObject(oSecondaryHuntableAnimal, 2, 3, 0, 2)]
	],
	[3 * numPlayers, 3 * numPlayers],
	[avoidClasses(clForest, 0, clPlayer, 20, clMountain, 1, clFood, 4, clRock, 6, clMetal, 6), stayClasses(clHill, 2)]
);

RMS.SetProgress(75);

createFood(
	[
		[new SimpleObject(oFruitBush, 5, 7, 0, 4)]
	],
	[3 * numPlayers],
	[avoidClasses(clForest, 0, clPlayer, 15, clMountain, 1, clFood, 4, clRock, 6, clMetal, 6), stayClasses(clHill, 2)]
);

RMS.SetProgress(85);

log("Creating more straggeler trees...");
createStragglerTrees(types, avoidClasses(clWater, 5, clForest, 7, clMountain, 1, clPlayer, 30, clMetal, 6, clRock, 3));

log("Creating decoration...");
let planetm = random_terrain == g_BiomeTropic ? 8 : 1;
createDecoration
(
	[
		[new SimpleObject(aRockMedium, 1, 3, 0, 1)],
		[new SimpleObject(aRockLarge, 1, 2, 0, 1), new SimpleObject(aRockMedium, 1, 3, 0, 2)],
		[new SimpleObject(aGrassShort, 2, 15, 0, 1, -PI/8, PI/8)],
		[new SimpleObject(aGrass, 2, 10, 0, 1.8, -PI/8, PI/8), new SimpleObject(aGrassShort, 3, 10, 1.2, 2.5, -PI/8, PI/8)],
		[new SimpleObject(aBushMedium, 1, 5, 0, 2), new SimpleObject(aBushSmall, 2, 4, 0, 2)]
	],
	[
		scaleByMapSize(16, 262),
		scaleByMapSize(8, 131),
		planetm * scaleByMapSize(13, 200),
		planetm * scaleByMapSize(13, 200),
		planetm * scaleByMapSize(13, 200)
	],
	avoidClasses(clForest, 2, clPlayer, 20, clMountain, 5, clFood, 1, clBaseResource, 2)
);

log("Creating water forests...");
createForests(
	[tMainTerrain, tForestFloor1, tForestFloor2, pForest1, pForest2],
	avoidClasses(clPlayer, 30, clHill, 10, clFood, 5),
	clForest,
	0.1,
	random_terrain
);

log("Creating small grass tufts...");
group = new SimpleGroup(
	[new SimpleObject(aGrassShort, 1, 2, 0, 1, -PI / 8, PI / 8)]
);
createObjectGroupsDeprecated(group, 0,
	[avoidClasses(clMountain, 2, clPlayer, 2, clDirt, 0), stayClasses(clHill, 8)],
	planetm * scaleByMapSize(13, 200)
);

setSkySet(pickRandom(["cloudless", "cumulus", "overcast"]));
setWaterMurkiness(0.4);

ExportMap();
