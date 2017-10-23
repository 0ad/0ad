RMS.LoadLibrary("rmgen");

const tGrass = "savanna_grass_a";
const tForestFloor = "savanna_forestfloor_a";
const tCliff = "savanna_cliff_b";
const tDirtRocksA = "savanna_dirt_rocks_c";
const tDirtRocksB = "savanna_dirt_rocks_a";
const tDirtRocksC = "savanna_dirt_rocks_b";
const tHill = "savanna_cliff_a";
const tRoad = "savanna_tile_a_red";
const tRoadWild = "savanna_tile_a_red";
const tGrassPatch = "savanna_grass_b";
const tShore = "savanna_riparian_bank";
const tWater = "savanna_riparian_wet";

const oBaobab = "gaia/flora_tree_baobab";
const oFig = "gaia/flora_tree_fig";
const oBerryBush = "gaia/flora_bush_berry";
const oWildebeest = "gaia/fauna_wildebeest";
const oFish = "gaia/fauna_fish";
const oGazelle = "gaia/fauna_gazelle";
const oElephant = "gaia/fauna_elephant_african_bush";
const oGiraffe = "gaia/fauna_giraffe";
const oZebra = "gaia/fauna_zebra";
const oStoneLarge = "gaia/geology_stonemine_desert_quarry";
const oStoneSmall = "gaia/geology_stone_savanna_small";
const oMetalLarge = "gaia/geology_metal_savanna_slabs";

const aGrass = "actor|props/flora/grass_savanna.xml";
const aGrassShort = "actor|props/flora/grass_medit_field.xml";
const aRockLarge = "actor|geology/stone_savanna_med.xml";
const aRockMedium = "actor|geology/stone_savanna_med.xml";
const aBushMedium = "actor|props/flora/bush_desert_dry_a.xml";
const aBushSmall = "actor|props/flora/bush_dry_a.xml";

const pForest = [tForestFloor + TERRAIN_SEPARATOR + oBaobab, tForestFloor + TERRAIN_SEPARATOR + oBaobab, tForestFloor];

InitMap();

const numPlayers = getNumPlayers();

var clPlayer = createTileClass();
var clHill = createTileClass();
var clForest = createTileClass();
var clWater = createTileClass();
var clDirt = createTileClass();
var clRock = createTileClass();
var clMetal = createTileClass();
var clFood = createTileClass();
var clBaseResource = createTileClass();
var clShallows = createTileClass();

var [playerIDs, playerX, playerZ, playerAngle, startAngle] = radialPlayerPlacement();

for (var i = 0; i < numPlayers; i++)
{
	var id = playerIDs[i];
	log("Creating base for player " + id + "...");

	var radius = scaleByMapSize(15,25);
	var cliffRadius = 2;
	var elevation = 20;

	// get the x and z in tiles
	var fx = fractionToTiles(playerX[i]);
	var fz = fractionToTiles(playerZ[i]);
	var ix = round(fx);
	var iz = round(fz);
	addCivicCenterAreaToClass(ix, iz, clPlayer);

	// create the city patch
	var cityRadius = radius/3;
	var placer = new ClumpPlacer(PI*cityRadius*cityRadius, 0.6, 0.3, 10, ix, iz);
	var painter = new LayeredPainter([tRoadWild, tRoad], [1]);
	createArea(placer, painter, null);

	placeCivDefaultEntities(fx, fz, id);

	placeDefaultChicken(fx, fz, clBaseResource);

	// create berry bushes
	var bbAngle = randFloat(0, TWO_PI);
	var bbDist = 12;
	var bbX = round(fx + bbDist * cos(bbAngle));
	var bbZ = round(fz + bbDist * sin(bbAngle));
	var group = new SimpleGroup(
		[new SimpleObject(oBerryBush, 5,5, 0,3)],
		true, clBaseResource, bbX, bbZ
	);
	createObjectGroup(group, 0);

	// create metal mine
	var mAngle = bbAngle;
	while(abs(mAngle - bbAngle) < PI/3)
	{
		mAngle = randFloat(0, TWO_PI);
	}
	var mDist = 12;
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

	// create starting trees
	var num = 5;
	var tAngle = randFloat(-PI/3, 4*PI/3);
	var tDist = randFloat(12, 13);
	var tX = round(fx + tDist * cos(tAngle));
	var tZ = round(fz + tDist * sin(tAngle));
	group = new SimpleGroup(
		[new SimpleObject(oBaobab, num, num, 0,3)],
		false, clBaseResource, tX, tZ
	);
	createObjectGroup(group, 0, avoidClasses(clBaseResource,2));

	placeDefaultDecoratives(fx, fz, aGrassShort, clBaseResource, radius);
}

RMS.SetProgress(20);

log ("Creating rivers...");
for (var m = 0; m < numPlayers; m++)
{
	var tang = startAngle + (m + 0.5) * TWO_PI / numPlayers;

	createArea(
		new ClumpPlacer(
			Math.floor(diskArea(scaleByMapSize(10, 50)) / 3),
			0.95,
			0.6,
			10,
			fractionToTiles(0.5 + 0.15 * Math.cos(tang)),
			fractionToTiles(0.5 + 0.15 * Math.sin(tang))),
		[
			new SmoothElevationPainter(ELEVATION_SET, -4, 4),
			paintClass(clWater)
		],
		avoidClasses(clPlayer, 5));

	createArea(
		new PathPlacer(
			fractionToTiles(0.5 + 0.15 * Math.cos(tang)),
			fractionToTiles(0.5 + 0.15 * Math.sin(tang)),
			fractionToTiles(0.5 + 0.49 * Math.cos(tang)),
			fractionToTiles(0.5 + 0.49 * Math.sin(tang)),
			scaleByMapSize(10, 50),
			0.2,
			3 * scaleByMapSize(1, 4),
			0.2,
			0.05),
		[
			new LayeredPainter([tShore, tWater, tWater], [1, 3]),
			new SmoothElevationPainter(ELEVATION_SET, -4, 4),
			paintClass(clWater)
		],
		avoidClasses(clPlayer, 5));

	createArea(
		new ClumpPlacer(
			Math.floor(diskArea(scaleByMapSize(10, 50)) / 5),
			0.95,
			0.6,
			10,
			fractionToTiles(0.5 + 0.49 * Math.cos(tang)),
			fractionToTiles(0.5 + 0.49 * Math.sin(tang))),
		[
			new SmoothElevationPainter(ELEVATION_SET, -4, 4),
			paintClass(clWater)
		],
		avoidClasses(clPlayer, 5));
}

for (var i = 0; i < numPlayers; i++)
{
	if (i+1 == numPlayers)
	{
		passageMaker(
			round(fractionToTiles(playerX[i])),
			round(fractionToTiles(playerZ[i])),
			round(fractionToTiles(playerX[0])),
			round(fractionToTiles(playerZ[0])),
			6, -2, -2, 4, clShallows, undefined, -4);

		log("Creating animals in shallows...");
		var group = new SimpleGroup(
			[new SimpleObject(oElephant, 2,3, 0,4)],
			true, clFood, round((fractionToTiles(playerX[i]) + fractionToTiles(playerX[0]))/2), round((fractionToTiles(playerZ[i]) + fractionToTiles(playerZ[0]))/2)
		);
		createObjectGroup(group, 0);

		var group = new SimpleGroup(
			[new SimpleObject(oWildebeest, 5,6, 0,4)],
			true, clFood, round((fractionToTiles(playerX[i]) + fractionToTiles(playerX[0]))/2), round((fractionToTiles(playerZ[i]) + fractionToTiles(playerZ[0]))/2)
		);
		createObjectGroup(group, 0);
	}
	else
	{
		passageMaker(
			fractionToTiles(playerX[i]),
			fractionToTiles(playerZ[i]),
			fractionToTiles(playerX[i+1]),
			fractionToTiles(playerZ[i+1]),
			6, -2, -2, 4, clShallows, undefined, -4);

		log("Creating animals in shallows...");
		var group = new SimpleGroup(
			[new SimpleObject(oElephant, 2,3, 0,4)],
			true, clFood, round((fractionToTiles(playerX[i]) + fractionToTiles(playerX[i+1]))/2), round((fractionToTiles(playerZ[i]) + fractionToTiles(playerZ[i+1]))/2)
		);
		createObjectGroup(group, 0);

		var group = new SimpleGroup(
			[new SimpleObject(oWildebeest, 5,6, 0,4)],
			true, clFood, round((fractionToTiles(playerX[i]) + fractionToTiles(playerX[i+1]))/2), round((fractionToTiles(playerZ[i]) + fractionToTiles(playerZ[i+1]))/2)
		);
		createObjectGroup(group, 0);

	}
}
paintTerrainBasedOnHeight(-6, 2, 1, tWater);

log("Creating bumps...");
createAreas(
	new ClumpPlacer(scaleByMapSize(20, 50), 0.3, 0.06, 1),
	new SmoothElevationPainter(ELEVATION_MODIFY, 2, 2),
	avoidClasses(clWater, 2, clPlayer, 20),
	scaleByMapSize(100, 200));

log("Creating hills...");
createAreas(
	new ClumpPlacer(scaleByMapSize(20, 150), 0.2, 0.1, 1),
	[
		new LayeredPainter([tGrass, tCliff, tHill], [1, 2]),
		new SmoothElevationPainter(ELEVATION_SET, 35, 3),
		paintClass(clHill)
	],
	avoidClasses(clPlayer, 20, clHill, 15, clWater, 3),
	scaleByMapSize(1, 4) * numPlayers);

// calculate desired number of trees for map (based on size)
var MIN_TREES = 160;
var MAX_TREES = 900;
var P_FOREST = 0.02;

var totalTrees = scaleByMapSize(MIN_TREES, MAX_TREES);
var numForest = totalTrees * P_FOREST;
var numStragglers = totalTrees * (1.0 - P_FOREST);

log("Creating forests...");
var types = [
	[[tForestFloor, tGrass, pForest], [tForestFloor, pForest]]
];

var size = numForest / (0.5 * scaleByMapSize(2,8) * numPlayers);
var num = floor(size / types.length);
for (let type of types)
	createAreas(
		new ClumpPlacer(numForest / num, 0.1, 0.1, 1),
		[
			new LayeredPainter(type, [2]),
			paintClass(clForest)
		],
		avoidClasses(clPlayer, 20, clForest, 10, clHill, 0, clWater, 2),
		num
	);
RMS.SetProgress(50);

log("Creating dirt patches...");
for (let size of [scaleByMapSize(3, 48), scaleByMapSize(5, 84), scaleByMapSize(8, 128)])
	createAreas(
		new ClumpPlacer(size, 0.3, 0.06, 0.5),
		[
			new LayeredPainter(
				[[tGrass, tDirtRocksA], [tDirtRocksA, tDirtRocksB], [tDirtRocksB, tDirtRocksC]],
				[1, 1]),
			paintClass(clDirt)
		],
		avoidClasses(clWater, 3, clForest, 0, clHill, 0, clPlayer, 20),
		scaleByMapSize(15, 45));

log("Creating grass patches...");
for (let size of [scaleByMapSize(2, 32), scaleByMapSize(3, 48), scaleByMapSize(5, 80)])
	createAreas(
		new ClumpPlacer(size, 0.3, 0.06, 0.5),
		new TerrainPainter(tGrassPatch),
		avoidClasses(clWater, 3, clForest, 0, clHill, 0, clPlayer, 20),
		scaleByMapSize(15, 45));
RMS.SetProgress(55);

log("Creating stone mines...");
var group = new SimpleGroup([new SimpleObject(oStoneSmall, 0, 2, 0, 4), new SimpleObject(oStoneLarge, 1, 1, 0, 4)], true, clRock);
createObjectGroupsDeprecated(group, 0,
	avoidClasses(clWater, 3, clForest, 1, clPlayer, 20, clRock, 10, clHill, 1),
	scaleByMapSize(4,16), 100
);

log("Creating small stone quarries...");
group = new SimpleGroup([new SimpleObject(oStoneSmall, 2,5, 1,3)], true, clRock);
createObjectGroupsDeprecated(group, 0,
	avoidClasses(clWater, 3, clForest, 1, clPlayer, 20, clRock, 10, clHill, 1),
	scaleByMapSize(4,16), 100
);

log("Creating metal mines...");
group = new SimpleGroup([new SimpleObject(oMetalLarge, 1,1, 0,4)], true, clMetal);
createObjectGroupsDeprecated(group, 0,
	avoidClasses(clWater, 3, clForest, 1, clPlayer, 20, clMetal, 10, clRock, 5, clHill, 1),
	scaleByMapSize(4,16), 100
);

RMS.SetProgress(65);

log("Creating small decorative rocks...");
group = new SimpleGroup(
	[new SimpleObject(aRockMedium, 1,3, 0,1)],
	true
);
createObjectGroupsDeprecated(
	group, 0,
	avoidClasses(clWater, 0, clForest, 0, clPlayer, 0, clHill, 0),
	scaleByMapSize(16, 262), 50
);

log("Creating large decorative rocks...");
group = new SimpleGroup(
	[new SimpleObject(aRockLarge, 1,2, 0,1), new SimpleObject(aRockMedium, 1,3, 0,2)],
	true
);
createObjectGroupsDeprecated(
	group, 0,
	avoidClasses(clWater, 0, clForest, 0, clPlayer, 0, clHill, 0),
	scaleByMapSize(8, 131), 50
);

RMS.SetProgress(70);

log("Creating wildebeest...");
group = new SimpleGroup(
	[new SimpleObject(oWildebeest, 5,7, 0,4)],
	true, clFood
);
createObjectGroupsDeprecated(group, 0,
	avoidClasses(clWater, 3, clForest, 0, clPlayer, 20, clHill, 1, clFood, 5),
	3 * numPlayers, 50
);

RMS.SetProgress(75);

log("Creating gazelle...");
group = new SimpleGroup(
	[new SimpleObject(oGazelle, 2,3, 0,2)],
	true, clFood
);
createObjectGroupsDeprecated(group, 0,
	avoidClasses(clWater, 3, clForest, 0, clPlayer, 20, clHill, 1, clFood, 5),
	3 * numPlayers, 50
);

log("Creating elephant...");
group = new SimpleGroup(
	[new SimpleObject(oElephant, 2,3, 0,2)],
	true, clFood
);
createObjectGroupsDeprecated(group, 0,
	avoidClasses(clWater, 3, clForest, 0, clPlayer, 20, clHill, 1, clFood, 5),
	3 * numPlayers, 50
);

log("Creating giraffe...");
group = new SimpleGroup(
	[new SimpleObject(oGiraffe, 2,3, 0,2)],
	true, clFood
);
createObjectGroupsDeprecated(group, 0,
	avoidClasses(clWater, 3, clForest, 0, clPlayer, 20, clHill, 1, clFood, 5),
	3 * numPlayers, 50
);

log("Creating zebra...");
group = new SimpleGroup(
	[new SimpleObject(oZebra, 2,3, 0,2)],
	true, clFood
);
createObjectGroupsDeprecated(group, 0,
	avoidClasses(clWater, 3, clForest, 0, clPlayer, 20, clHill, 1, clFood, 5),
	3 * numPlayers, 50
);

log("Creating fish...");
group = new SimpleGroup(
	[new SimpleObject(oFish, 2,3, 0,2)],
	true, clFood
);
createObjectGroupsDeprecated(group, 0,
	[avoidClasses(clFood, 20), stayClasses(clWater, 6)],
	25 * numPlayers, 60
);

log("Creating berry bush...");
group = new SimpleGroup(
	[new SimpleObject(oBerryBush, 5,7, 0,4)],
	true, clFood
);
createObjectGroupsDeprecated(group, 0,
	avoidClasses(clWater, 3, clForest, 0, clPlayer, 20, clHill, 1, clFood, 10),
	randIntInclusive(1, 4) * numPlayers + 2, 50
);

RMS.SetProgress(85);

log("Creating straggler trees...");
var types = [oBaobab, oBaobab, oBaobab, oFig];
var num = floor(numStragglers / types.length);
for (let type of types)
	createObjectGroupsDeprecated(
		new SimpleGroup([new SimpleObject(type, 1,1, 0,3)], true, clForest),
		0,
		avoidClasses(clWater, 5, clForest, 1, clHill, 1, clPlayer, 12, clMetal, 6, clRock, 6),
		num);

var planetm = 4;
log("Creating small grass tufts...");
group = new SimpleGroup(
	[new SimpleObject(aGrassShort, 1,2, 0,1, -PI/8,PI/8)]
);
createObjectGroupsDeprecated(group, 0,
	avoidClasses(clWater, 2, clHill, 2, clPlayer, 2),
	planetm * scaleByMapSize(13, 200)
);

RMS.SetProgress(90);

log("Creating large grass tufts...");
group = new SimpleGroup(
	[new SimpleObject(aGrass, 2,4, 0,1.8, -PI/8,PI/8), new SimpleObject(aGrassShort, 3,6, 1.2,2.5, -PI/8,PI/8)]
);
createObjectGroupsDeprecated(group, 0,
	avoidClasses(clWater, 3, clHill, 2, clPlayer, 2, clForest, 0),
	planetm * scaleByMapSize(13, 200)
);

RMS.SetProgress(95);

log("Creating bushes...");
group = new SimpleGroup(
	[new SimpleObject(aBushMedium, 1,2, 0,2), new SimpleObject(aBushSmall, 2,4, 0,2)]
);
createObjectGroupsDeprecated(group, 0,
	avoidClasses(clWater, 2, clHill, 1, clPlayer, 1),
	planetm * scaleByMapSize(13, 200), 50
);

setSkySet("sunny");

setSunRotation(randFloat(0, TWO_PI));
setSunElevation(randFloat(PI/ 5, PI / 4));
setWaterColor(0.478,0.42,0.384);				// greyish
setWaterTint(0.58,0.22,0.067);				// reddish
setWaterMurkiness(0.87);
setWaterWaviness(0.5);
setWaterType("clap");

ExportMap();
