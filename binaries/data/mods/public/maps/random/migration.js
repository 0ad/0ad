RMS.LoadLibrary("rmgen");

//random terrain textures
var random_terrain = randomizeBiome();

const tMainTerrain = rBiomeT1();
const tForestFloor1 = rBiomeT2();
const tForestFloor2 = rBiomeT3();
const tCliff = rBiomeT4();
const tTier1Terrain = rBiomeT5();
const tTier2Terrain = rBiomeT6();
const tTier3Terrain = rBiomeT7();
const tHill = rBiomeT8();
const tDirt = rBiomeT9();
const tRoad = rBiomeT10();
const tRoadWild = rBiomeT11();
const tTier4Terrain = rBiomeT12();
const tShoreBlend = rBiomeT13();
const tShore = rBiomeT14();
const tWater = rBiomeT15();

// gaia entities
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
const oStoneSmall = rBiomeE12();
const oMetalLarge = rBiomeE13();
const oWood = "gaia/special_treasure_wood";

// decorative props
const aGrass = rBiomeA1();
const aGrassShort = rBiomeA2();
const aReeds = rBiomeA3();
const aLillies = rBiomeA4();
const aRockLarge = rBiomeA5();
const aRockMedium = rBiomeA6();
const aBushMedium = rBiomeA7();
const aBushSmall = rBiomeA8();

const pForest1 = [tForestFloor2 + TERRAIN_SEPARATOR + oTree1, tForestFloor2 + TERRAIN_SEPARATOR + oTree2, tForestFloor2];
const pForest2 = [tForestFloor1 + TERRAIN_SEPARATOR + oTree4, tForestFloor1 + TERRAIN_SEPARATOR + oTree5, tForestFloor1];

log("Initializing map...");

InitMap();

const numPlayers = getNumPlayers();
const mapSize = getMapSize();
const mapArea = mapSize*mapSize;

// create tile classes
var clPlayer = createTileClass();
var clHill = createTileClass();
var clForest = createTileClass();
var clDirt = createTileClass();
var clRock = createTileClass();
var clMetal = createTileClass();
var clFood = createTileClass();
var clBaseResource = createTileClass();
var clSettlement = createTileClass();
var clLand = createTileClass();

// randomize player order
var playerIDs = [];
for (var i = 0; i < numPlayers; i++)
	playerIDs.push(i+1);

playerIDs = sortPlayers(playerIDs);

// place players
var playerX = new Array(numPlayers);
var playerZ = new Array(numPlayers);
var playerAngle = new Array(numPlayers);

var startAngle = PI/2 + PI/14;
for (var i = 0; i < numPlayers; i++)
{
	playerAngle[i] = startAngle - (i+1)*(PI+ PI/7)/(numPlayers+1);
	playerX[i] = 0.5 + 0.35*cos(playerAngle[i]);
	playerZ[i] = 0.5 + 0.35*sin(playerAngle[i]);
}

for (var i = 0; i < numPlayers; i++)
{
	var id = playerIDs[i];
	log("Creating base for player " + id + "...");

	// some constants
	var radius = scaleByMapSize(15,25);
	var shoreRadius = 4;
	var elevation = 3;

	var hillSize = PI * radius * radius;
	// get the x and z in tiles
	fx = fractionToTiles(playerX[i]);
	fz = fractionToTiles(playerZ[i]);
	ix = round(fx);
	iz = round(fz);
	// create the hill
	var placer = new ClumpPlacer(hillSize, 0.80, 0.1, 10, ix, iz);
	var terrainPainter = new LayeredPainter(
		[tWater , tShore, tMainTerrain],		// terrains
		[1, shoreRadius]		// widths
	);
	var elevationPainter = new SmoothElevationPainter(
		ELEVATION_SET,			// type
		elevation,				// elevation
		shoreRadius				// blend radius
	);
	createArea(placer, [terrainPainter, elevationPainter, paintClass(clPlayer)], null);

	// create the city patch
	var cityRadius = radius/3;
	placer = new ClumpPlacer(PI*cityRadius*cityRadius, 0.6, 0.3, 10, ix, iz);
	var painter = new LayeredPainter([tRoadWild, tRoad], [1]);
	createArea(placer, painter, null);

	// create starting units
	placeCivDefaultEntities(fx, fz, id, { 'iberWall': false });

	placeDefaultChicken(fx, fz, clBaseResource);

	// create berry bushes
	var bbAngle = randFloat(0, TWO_PI);
	var bbDist = 12;
	var bbX = round(fx + bbDist * cos(bbAngle));
	var bbZ = round(fz + bbDist * sin(bbAngle));
	var group = new SimpleGroup(
		[new SimpleObject(oFruitBush, 5,5, 0,3)],
		true, clBaseResource, bbX, bbZ
	);
	createObjectGroup(group, 0);

	// create woods
	var bbAngle = randFloat(0, TWO_PI);
	var bbDist = 13;
	var bbX = round(fx + bbDist * cos(bbAngle));
	var bbZ = round(fz + bbDist * sin(bbAngle));
	group = new SimpleGroup(
		[new SimpleObject(oWood, 14,14, 0,3)],
		true, clBaseResource, bbX, bbZ
	);
	createObjectGroup(group, 0);

	// create metal mine
	var mAngle = bbAngle;
	while(abs(mAngle - bbAngle) < PI/3)
		mAngle = randFloat(0, TWO_PI);

	var mDist = radius - 4;
	var mX = round(fx + mDist * cos(mAngle));
	var mZ = round(fz + mDist * sin(mAngle));
	group = new SimpleGroup(
		[new SimpleObject(oMetalLarge, 1,1, 0,0)],
		true, clBaseResource, mX, mZ
	);
	createObjectGroup(group, 0);

	// create stone mines
	mAngle += randFloat(PI/8, PI/4);
	mX = round(fx + mDist * cos(mAngle));
	mZ = round(fz + mDist * sin(mAngle));
	group = new SimpleGroup(
		[new SimpleObject(oStoneLarge, 1,1, 0,2)],
		true, clBaseResource, mX, mZ
	);
	createObjectGroup(group, 0);
	var hillSize = PI * radius * radius;
	// create starting trees
	var num = floor(hillSize / 60);
	var tAngle = randFloat(-PI/3, 4*PI/3);
	var tDist = 11;
	var tX = round(fx + tDist * cos(tAngle));
	var tZ = round(fz + tDist * sin(tAngle));
	group = new SimpleGroup(
		[new SimpleObject(oTree1, num, num, 0,4)],
		false, clBaseResource, tX, tZ
	);
	createObjectGroup(group, 0, avoidClasses(clBaseResource,2));

	placeDefaultDecoratives(fx, fz, aGrassShort, clBaseResource, radius);

	//create docks
	var dockLocation = getTIPIADBON([ix, iz], [mapSize / 2, mapSize / 2], [-3 , 2.6], 0.5, 3);
	if (dockLocation !== undefined)
		placeObject(dockLocation[0], dockLocation[1], "structures/" + getCivCode(id-1) + "_dock", id, playerAngle[i] + PI);
}
RMS.SetProgress(15);

//Create the continent body
var fx = fractionToTiles(0.12);
var fz = fractionToTiles(0.5);
var ix = round(fx);
var iz = round(fz);

var placer = new ClumpPlacer(mapArea * 0.50, 0.80, 0.08, 10, ix, iz);
var terrainPainter = new LayeredPainter(
	[tWater, tShore, tMainTerrain],		// terrains
	[4, 2]		// widths
);
var elevationPainter = new SmoothElevationPainter(
	ELEVATION_SET,			// type
	3,				// elevation
	4				// blend radius
);
createArea(placer, [terrainPainter, elevationPainter, paintClass(clLand)], avoidClasses(clPlayer, 8));
RMS.SetProgress(20);

log("Creating shore jaggedness...");
placer = new ClumpPlacer(scaleByMapSize(15, 80), 0.2, 0.1, 1);
terrainPainter = new LayeredPainter(
	[tMainTerrain, tMainTerrain],		// terrains
	[2]								// widths
);
elevationPainter = new SmoothElevationPainter(ELEVATION_SET, 3, 4);
createAreas(
	placer,
	[terrainPainter, elevationPainter, paintClass(clLand)],
	[borderClasses(clLand, 6, 3), avoidClasses(clPlayer, 8)],
	scaleByMapSize(20, 150) * 2, 150
);

paintTerrainBasedOnHeight(1, 3, 0, tShore);
paintTerrainBasedOnHeight(-8, 1, 2, tWater);
RMS.SetProgress(25);

log("Creating bumps...");
placer = new ClumpPlacer(scaleByMapSize(20, 50), 0.3, 0.06, 1);
painter = new SmoothElevationPainter(ELEVATION_MODIFY, 2, 2);
createAreas(
	placer,
	painter,
	[avoidClasses(clPlayer, 10), stayClasses(clLand, 3)],
	scaleByMapSize(100, 200)
);
RMS.SetProgress(30);

log("Creating hills...");
placer = new ClumpPlacer(scaleByMapSize(20, 150), 0.2, 0.1, 1);
terrainPainter = new LayeredPainter(
	[tCliff, tHill],		// terrains
	[2]								// widths
);
elevationPainter = new SmoothElevationPainter(ELEVATION_SET, 18, 2);
createAreas(
	placer,
	[terrainPainter, elevationPainter, paintClass(clHill)],
	[avoidClasses(clPlayer, 10, clHill, 15), stayClasses(clLand, 7)],
	scaleByMapSize(1, 4) * numPlayers
);
RMS.SetProgress(34);

// calculate desired number of trees for map (based on size)
if (random_terrain == g_BiomeSavanna)
{
	var MIN_TREES = 200;
	var MAX_TREES = 1250;
	var P_FOREST = 0.02;
}
else if (random_terrain == g_BiomeTropic)
{
	var MIN_TREES = 1000;
	var MAX_TREES = 6000;
	var P_FOREST = 0.6;
}
else
{
	var MIN_TREES = 500;
	var MAX_TREES = 3000;
	var P_FOREST = 0.7;
}

var totalTrees = scaleByMapSize(MIN_TREES, MAX_TREES);
var numForest = totalTrees * P_FOREST;
var numStragglers = totalTrees * (1.0 - P_FOREST);

log("Creating forests...");
var types = [
	[[tForestFloor2, tMainTerrain, pForest1], [tForestFloor2, pForest1]],
	[[tForestFloor1, tMainTerrain, pForest2], [tForestFloor1, pForest2]]
];	// some variation

var size = numForest / (scaleByMapSize(2,8) * numPlayers) *
	(random_terrain == g_BiomeSavanna ? 2 : 1);

var num = floor(size / types.length);
for (var i = 0; i < types.length; ++i)
{
	placer = new ClumpPlacer(numForest / num, 0.1, 0.1, 1);
	painter = new LayeredPainter(
		types[i],		// terrains
		[2]											// widths
		);
	createAreas(
		placer,
		[painter, paintClass(clForest)],
		[avoidClasses(clPlayer, 6, clForest, 10, clHill, 0), stayClasses(clLand, 7)],
		num
	);
}
RMS.SetProgress(38);

log("Creating dirt patches...");
var sizes = [scaleByMapSize(3, 48), scaleByMapSize(5, 84), scaleByMapSize(8, 128)];
for (var i = 0; i < sizes.length; i++)
{
	placer = new ClumpPlacer(sizes[i], 0.3, 0.06, 0.5);
	painter = new LayeredPainter(
		[[tMainTerrain,tTier1Terrain],[tTier1Terrain,tTier2Terrain], [tTier2Terrain,tTier3Terrain]], 		// terrains
		[1,1]															// widths
	);
	createAreas(
		placer,
		[painter, paintClass(clDirt)],
		[avoidClasses(clForest, 0, clHill, 0, clDirt, 5, clPlayer, 0), stayClasses(clLand, 7)],
		scaleByMapSize(15, 45)
	);
}
RMS.SetProgress(42);

log("Creating grass patches...");
var sizes = [scaleByMapSize(2, 32), scaleByMapSize(3, 48), scaleByMapSize(5, 80)];
for (var i = 0; i < sizes.length; i++)
{
	placer = new ClumpPlacer(sizes[i], 0.3, 0.06, 0.5);
	painter = new TerrainPainter(tTier4Terrain);
	createAreas(
		placer,
		painter,
		[avoidClasses(clForest, 0, clHill, 0, clDirt, 5, clPlayer, 0), stayClasses(clLand, 7)],
		scaleByMapSize(15, 45)
	);
}
RMS.SetProgress(46);

log("Creating stone mines...");
group = new SimpleGroup([new SimpleObject(oStoneSmall, 0,2, 0,4), new SimpleObject(oStoneLarge, 1,1, 0,4)], true, clRock);
createObjectGroups(group, 0,
	[avoidClasses(clForest, 1, clPlayer, 10, clRock, 10, clHill, 1), stayClasses(clLand, 7)],
	scaleByMapSize(4,16), 100
);
RMS.SetProgress(50);

log("Creating small stone quarries...");
group = new SimpleGroup([new SimpleObject(oStoneSmall, 2,5, 1,3)], true, clRock);
createObjectGroups(group, 0,
	[avoidClasses(clForest, 1, clPlayer, 10, clRock, 10, clHill, 1), stayClasses(clLand, 7)],
	scaleByMapSize(4,16), 100
);
RMS.SetProgress(54);

log("Creating metal mines...");
// create large metal quarries
group = new SimpleGroup([new SimpleObject(oMetalLarge, 1,1, 0,4)], true, clMetal);
createObjectGroups(group, 0,
	[avoidClasses(clForest, 1, clPlayer, 10, clMetal, 10, clRock, 5, clHill, 1), stayClasses(clLand, 7)],
	scaleByMapSize(4,16), 100
);
RMS.SetProgress(58);

// create small decorative rocks
log("Creating small decorative rocks...");
group = new SimpleGroup(
	[new SimpleObject(aRockMedium, 1,3, 0,1)],
	true
);
createObjectGroups(
	group, 0,
	[avoidClasses(clForest, 0, clPlayer, 0, clHill, 0), stayClasses(clLand, 6)],
	scaleByMapSize(16, 262), 50
);
RMS.SetProgress(62);

// create large decorative rocks
log("Creating large decorative rocks...");
group = new SimpleGroup(
	[new SimpleObject(aRockLarge, 1,2, 0,1), new SimpleObject(aRockMedium, 1,3, 0,2)],
	true
);
createObjectGroups(
	group, 0,
	[avoidClasses(clForest, 0, clPlayer, 0, clHill, 0), stayClasses(clLand, 6)],
	scaleByMapSize(8, 131), 50
);
RMS.SetProgress(66);

log("Creating deer...");
group = new SimpleGroup(
	[new SimpleObject(oMainHuntableAnimal, 5,7, 0,4)],
	true, clFood
);
createObjectGroups(group, 0,
	[avoidClasses(clForest, 0, clPlayer, 10, clHill, 1, clFood, 20), stayClasses(clLand, 7)],
	3 * numPlayers, 50
);
RMS.SetProgress(70);

log("Creating sheep...");
group = new SimpleGroup(
	[new SimpleObject(oSecondaryHuntableAnimal, 2,3, 0,2)],
	true, clFood
);
createObjectGroups(group, 0,
	[avoidClasses(clForest, 0, clPlayer, 10, clHill, 1, clFood, 20), stayClasses(clLand, 7)],
	3 * numPlayers, 50
);
RMS.SetProgress(74);

log("Creating fruit bush...");
group = new SimpleGroup(
	[new SimpleObject(oFruitBush, 5,7, 0,4)],
	true, clFood
);
createObjectGroups(group, 0,
	[avoidClasses(clForest, 0, clPlayer, 8, clHill, 1, clFood, 20), stayClasses(clLand, 7)],
	randInt(1, 4) * numPlayers + 2, 50
);
RMS.SetProgress(78);

log("Creating fish...");
createObjectGroups(
	new SimpleGroup([new SimpleObject(oFish, 2,3, 0,2)], true, clFood),
	0,
	avoidClasses(clLand, 2, clPlayer, 2, clHill, 0, clFood, 20),
	25 * numPlayers, 60
);
RMS.SetProgress(82);

log("Creating straggler trees...");
var types = [oTree1, oTree2, oTree4, oTree3];	// some variation
var num = floor(numStragglers / types.length);
for (var i = 0; i < types.length; ++i)
{
	group = new SimpleGroup(
		[new SimpleObject(types[i], 1,1, 0,3)],
		true, clForest
	);
	createObjectGroups(group, 0,
		[avoidClasses(clForest, 1, clHill, 1, clPlayer, 9, clMetal, 6, clRock, 6), stayClasses(clLand, 9)],
		num
	);
}
RMS.SetProgress(86);

var planetm = random_terrain == g_BiomeTropic ? 8 : 1;

log("Creating small grass tufts...");
group = new SimpleGroup(
	[new SimpleObject(aGrassShort, 1,2, 0,1, -PI/8,PI/8)]
);
createObjectGroups(group, 0,
	[avoidClasses(clHill, 2, clPlayer, 2, clDirt, 0), stayClasses(clLand, 6)],
	planetm * scaleByMapSize(13, 200)
);
RMS.SetProgress(90);

log("Creating large grass tufts...");
group = new SimpleGroup(
	[new SimpleObject(aGrass, 2,4, 0,1.8, -PI/8,PI/8), new SimpleObject(aGrassShort, 3,6, 1.2,2.5, -PI/8,PI/8)]
);
createObjectGroups(group, 0,
	[avoidClasses(clHill, 2, clPlayer, 2, clDirt, 1, clForest, 0), stayClasses(clLand, 6)],
	planetm * scaleByMapSize(13, 200)
);
RMS.SetProgress(94);

log("Creating bushes...");
group = new SimpleGroup(
	[new SimpleObject(aBushMedium, 1,2, 0,2), new SimpleObject(aBushSmall, 2,4, 0,2)]
);
createObjectGroups(group, 0,
	[avoidClasses(clHill, 1, clPlayer, 1, clDirt, 1), stayClasses(clLand, 6)],
	planetm * scaleByMapSize(13, 200), 50
);
RMS.SetProgress(98);

setSkySet(pickRandom(["cirrus", "cumulus", "sunny"]));
setSunRotation(randFloat(0, TWO_PI));
setSunElevation(randFloat(PI/ 5, PI / 3));
setWaterWaviness(2);

ExportMap();
