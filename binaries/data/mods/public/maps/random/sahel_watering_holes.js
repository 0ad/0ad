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
const tShoreBlend = "savanna_riparian";
const tShore = "savanna_riparian_bank";
const tWater = "savanna_riparian_wet";

// gaia entities
const oBaobab = "gaia/flora_tree_baobab";
const oFig = "gaia/flora_tree_fig";
const oBerryBush = "gaia/flora_bush_berry";
const oChicken = "gaia/fauna_chicken";
const oWildebeest = "gaia/fauna_wildebeest"
const oFish = "gaia/fauna_fish";
const oGazelle = "gaia/fauna_gazelle";
const oElephant = "gaia/fauna_elephant_african_bush";
const oGiraffe = "gaia/fauna_giraffe";
const oZebra = "gaia/fauna_zebra";
const oStoneLarge = "gaia/geology_stonemine_desert_quarry";
const oStoneSmall = "gaia/geology_stone_savanna_small";
const oMetalLarge = "gaia/geology_metal_savanna_slabs";

// decorative props
const aGrass = "actor|props/flora/grass_savanna.xml";
const aGrassShort = "actor|props/flora/grass_medit_field.xml";
const aReeds = "actor|props/flora/reeds_pond_lush_a.xml";
const aLillies = "actor|props/flora/reeds_pond_lush_b.xml";
const aRockLarge = "actor|geology/stone_savanna_med.xml";
const aRockMedium = "actor|geology/stone_savanna_med.xml";
const aBushMedium = "actor|props/flora/bush_desert_dry_a.xml";
const aBushSmall = "actor|props/flora/bush_dry_a.xml";

const pForest = [tForestFloor + TERRAIN_SEPARATOR + oBaobab, tForestFloor + TERRAIN_SEPARATOR + oBaobab, tForestFloor];
const BUILDING_ANGlE = -PI/4;

// initialize map

log("Initializing map...");

InitMap();

const numPlayers = getNumPlayers();
const mapSize = getMapSize();
const mapArea = mapSize*mapSize;

// create tile classes

var clPlayer = createTileClass();
var clHill = createTileClass();
var clForest = createTileClass();
var clWater = createTileClass();
var clDirt = createTileClass();
var clRock = createTileClass();
var clMetal = createTileClass();
var clFood = createTileClass();
var clBaseResource = createTileClass();
var clSettlement = createTileClass();
var clShallows = createTileClass();

// randomize player order
var playerIDs = [];
for (var i = 0; i < numPlayers; i++)
{
	playerIDs.push(i+1);
}
playerIDs = sortPlayers(playerIDs);

// place players

var playerX = new Array(numPlayers);
var playerZ = new Array(numPlayers);
var playerAngle = new Array(numPlayers);

var startAngle = randFloat(0, TWO_PI);
for (var i = 0; i < numPlayers; i++)
{
	playerAngle[i] = startAngle + i*TWO_PI/numPlayers;
	playerX[i] = 0.5 + 0.35*cos(playerAngle[i]);
	playerZ[i] = 0.5 + 0.35*sin(playerAngle[i]);
}

for (var i = 0; i < numPlayers; i++)
{
	var id = playerIDs[i];
	log("Creating base for player " + id + "...");
	
	// some constants
	var radius = scaleByMapSize(15,25);
	var cliffRadius = 2;
	var elevation = 20;
	
	// get the x and z in tiles
	var fx = fractionToTiles(playerX[i]);
	var fz = fractionToTiles(playerZ[i]);
	var ix = round(fx);
	var iz = round(fz);
	addToClass(ix, iz, clPlayer);
	addToClass(ix+5, iz, clPlayer);
	addToClass(ix, iz+5, clPlayer);
	addToClass(ix-5, iz, clPlayer);
	addToClass(ix, iz-5, clPlayer);
	
	// create the city patch
	var cityRadius = radius/3;
	var placer = new ClumpPlacer(PI*cityRadius*cityRadius, 0.6, 0.3, 10, ix, iz);
	var painter = new LayeredPainter([tRoadWild, tRoad], [1]);
	createArea(placer, painter, null);
	
	// create starting units
	placeCivDefaultEntities(fx, fz, id, BUILDING_ANGlE);
	
	// create animals
	for (var j = 0; j < 2; ++j)
	{
		var aAngle = randFloat(0, TWO_PI);
		var aDist = 7;
		var aX = round(fx + aDist * cos(aAngle));
		var aZ = round(fz + aDist * sin(aAngle));
		var group = new SimpleGroup(
			[new SimpleObject(oChicken, 5,5, 0,2)],
			true, clBaseResource, aX, aZ
		);
		createObjectGroup(group, 0);
	}
	
	// create berry bushes
	var bbAngle = randFloat(0, TWO_PI);
	var bbDist = 12;
	var bbX = round(fx + bbDist * cos(bbAngle));
	var bbZ = round(fz + bbDist * sin(bbAngle));
	group = new SimpleGroup(
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
	var hillSize = PI * radius * radius;
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
	
	// create grass tufts
	var num = hillSize / 250;
	for (var j = 0; j < num; j++)
	{
		var gAngle = randFloat(0, TWO_PI);
		var gDist = radius - (5 + randInt(7));
		var gX = round(fx + gDist * cos(gAngle));
		var gZ = round(fz + gDist * sin(gAngle));
		group = new SimpleGroup(
			[new SimpleObject(aGrassShort, 2,5, 0,1, -PI/8,PI/8)],
			false, clBaseResource, gX, gZ
		);
		createObjectGroup(group, 0);
	}
}

RMS.SetProgress(20);

//create rivers
log ("Creating rivers...");
for (var m = 0; m < numPlayers; m++)
{
	var tang = startAngle + (m+0.5)*TWO_PI/numPlayers
	placer = new ClumpPlacer(floor(PI*scaleByMapSize(10,50)*scaleByMapSize(10,50)/3), 0.95, 0.6, 10, fractionToTiles(0.5 + 0.15*cos(tang)), fractionToTiles(0.5 + 0.15*sin(tang)));
	var painter = new LayeredPainter([tShore, tWater, tWater], [1, 3]);
	var elevationPainter = new SmoothElevationPainter(ELEVATION_SET, -4, 4);
	createArea(placer, [painter, elevationPainter, paintClass(clWater)], avoidClasses(clPlayer, 5));
	var placer = new PathPlacer(fractionToTiles(0.5 + 0.15*cos(tang)), fractionToTiles(0.5 + 0.15*sin(tang)), fractionToTiles(0.5 + 0.49*cos(tang)), fractionToTiles(0.5 + 0.49*sin(tang)), scaleByMapSize(10,50), 0.2, 3*(scaleByMapSize(1,4)), 0.2, 0.05);
	var terrainPainter = new LayeredPainter(
		[tShore, tWater, tWater],		// terrains
		[1, 3]								// widths
	);
	var elevationPainter = new SmoothElevationPainter(
		ELEVATION_SET,			// type
		-4,				// elevation
		4				// blend radius
	);
	createArea(placer, [terrainPainter, elevationPainter, paintClass(clWater)], avoidClasses(clPlayer, 5));
	placer = new ClumpPlacer(floor(PI*scaleByMapSize(10,50)*scaleByMapSize(10,50)/5), 0.95, 0.6, 10, fractionToTiles(0.5 + 0.49*cos(tang)), fractionToTiles(0.5 + 0.49*sin(tang)));
	var painter = new LayeredPainter([tWater, tWater], [1]);
	var elevationPainter = new SmoothElevationPainter(ELEVATION_SET, -4, 4);
	createArea(placer, [painter, elevationPainter, paintClass(clWater)], avoidClasses(clPlayer, 5));
}


for (var i = 0; i < numPlayers; i++)
{
	if (i+1 == numPlayers)
	{
		passageMaker(round(fractionToTiles(playerX[i])), round(fractionToTiles(playerZ[i])), round(fractionToTiles(playerX[0])), round(fractionToTiles(playerZ[0])), 6, -2, -2, 4, clShallows, undefined, -4)
		
		// create animals in shallows
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
		passageMaker(fractionToTiles(playerX[i]), fractionToTiles(playerZ[i]), fractionToTiles(playerX[i+1]), fractionToTiles(playerZ[i+1]), 6, -2, -2, 4, clShallows, undefined, -4)

		// create animals in shallows
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


// create bumps
log("Creating bumps...");
placer = new ClumpPlacer(scaleByMapSize(20, 50), 0.3, 0.06, 1);
painter = new SmoothElevationPainter(ELEVATION_MODIFY, 2, 2);
createAreas(
	placer,
	painter, 
	avoidClasses(clWater, 2, clPlayer, 20),
	scaleByMapSize(100, 200)
);

// create hills
log("Creating hills...");
placer = new ClumpPlacer(scaleByMapSize(20, 150), 0.2, 0.1, 1);
terrainPainter = new LayeredPainter(
	[tGrass, tCliff, tHill],		// terrains
	[1, 2]								// widths
);
elevationPainter = new SmoothElevationPainter(ELEVATION_SET, 35, 3);
createAreas(
	placer,
	[terrainPainter, elevationPainter, paintClass(clHill)], 
	avoidClasses(clPlayer, 20, clHill, 15, clWater, 3),
	scaleByMapSize(1, 4) * numPlayers
);


// calculate desired number of trees for map (based on size)
var MIN_TREES = 160;
var MAX_TREES = 900;
var P_FOREST = 0.02;

var totalTrees = scaleByMapSize(MIN_TREES, MAX_TREES);
var numForest = totalTrees * P_FOREST;
var numStragglers = totalTrees * (1.0 - P_FOREST);

// create forests
log("Creating forests...");
var types = [
	[[tForestFloor, tGrass, pForest], [tForestFloor, pForest]]
];	// some variation


var size = numForest / (0.5 * scaleByMapSize(2,8) * numPlayers);
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
		avoidClasses(clPlayer, 20, clForest, 10, clHill, 0, clWater, 2),
		num
	);
}

RMS.SetProgress(50);

// create dirt patches
log("Creating dirt patches...");
var sizes = [scaleByMapSize(3, 48), scaleByMapSize(5, 84), scaleByMapSize(8, 128)];
for (var i = 0; i < sizes.length; i++)
{
	placer = new ClumpPlacer(sizes[i], 0.3, 0.06, 0.5);
	painter = new LayeredPainter(
		[[tGrass,tDirtRocksA],[tDirtRocksA,tDirtRocksB], [tDirtRocksB,tDirtRocksC]], 		// terrains
		[1,1]															// widths
	);
	createAreas(
		placer,
		[painter, paintClass(clDirt)],
		avoidClasses(clWater, 3, clForest, 0, clHill, 0, clPlayer, 20),
		scaleByMapSize(15, 45)
	);
}

// create grass patches
log("Creating grass patches...");
var sizes = [scaleByMapSize(2, 32), scaleByMapSize(3, 48), scaleByMapSize(5, 80)];
for (var i = 0; i < sizes.length; i++)
{
	placer = new ClumpPlacer(sizes[i], 0.3, 0.06, 0.5);
	painter = new TerrainPainter(tGrassPatch);
	createAreas(
		placer,
		painter,
		avoidClasses(clWater, 3, clForest, 0, clHill, 0, clPlayer, 20),
		scaleByMapSize(15, 45)
	);
}
RMS.SetProgress(55);


log("Creating stone mines...");
// create large stone quarries
group = new SimpleGroup([new SimpleObject(oStoneSmall, 0,2, 0,4), new SimpleObject(oStoneLarge, 1,1, 0,4)], true, clRock);
createObjectGroups(group, 0,
	avoidClasses(clWater, 3, clForest, 1, clPlayer, 20, clRock, 10, clHill, 1),
	scaleByMapSize(4,16), 100
);

// create small stone quarries
group = new SimpleGroup([new SimpleObject(oStoneSmall, 2,5, 1,3)], true, clRock);
createObjectGroups(group, 0,
	avoidClasses(clWater, 3, clForest, 1, clPlayer, 20, clRock, 10, clHill, 1),
	scaleByMapSize(4,16), 100
);

log("Creating metal mines...");
// create large metal quarries
group = new SimpleGroup([new SimpleObject(oMetalLarge, 1,1, 0,4)], true, clMetal);
createObjectGroups(group, 0,
	avoidClasses(clWater, 3, clForest, 1, clPlayer, 20, clMetal, 10, clRock, 5, clHill, 1),
	scaleByMapSize(4,16), 100
);

RMS.SetProgress(65);

// create small decorative rocks
log("Creating small decorative rocks...");
group = new SimpleGroup(
	[new SimpleObject(aRockMedium, 1,3, 0,1)],
	true
);
createObjectGroups(
	group, 0,
	avoidClasses(clWater, 0, clForest, 0, clPlayer, 0, clHill, 0),
	scaleByMapSize(16, 262), 50
);


// create large decorative rocks
log("Creating large decorative rocks...");
group = new SimpleGroup(
	[new SimpleObject(aRockLarge, 1,2, 0,1), new SimpleObject(aRockMedium, 1,3, 0,2)],
	true
);
createObjectGroups(
	group, 0,
	avoidClasses(clWater, 0, clForest, 0, clPlayer, 0, clHill, 0),
	scaleByMapSize(8, 131), 50
);

RMS.SetProgress(70);

// create wildebeest
log("Creating wildebeest...");
group = new SimpleGroup(
	[new SimpleObject(oWildebeest, 5,7, 0,4)],
	true, clFood
);
createObjectGroups(group, 0,
	avoidClasses(clWater, 3, clForest, 0, clPlayer, 20, clHill, 1, clFood, 5),
	3 * numPlayers, 50
);

RMS.SetProgress(75);

// create gazelle
log("Creating gazelle...");
group = new SimpleGroup(
	[new SimpleObject(oGazelle, 2,3, 0,2)],
	true, clFood
);
createObjectGroups(group, 0,
	avoidClasses(clWater, 3, clForest, 0, clPlayer, 20, clHill, 1, clFood, 5),
	3 * numPlayers, 50
);

// create elephant
log("Creating elephant...");
group = new SimpleGroup(
	[new SimpleObject(oElephant, 2,3, 0,2)],
	true, clFood
);
createObjectGroups(group, 0,
	avoidClasses(clWater, 3, clForest, 0, clPlayer, 20, clHill, 1, clFood, 5),
	3 * numPlayers, 50
);

// create giraffe
log("Creating giraffe...");
group = new SimpleGroup(
	[new SimpleObject(oGiraffe, 2,3, 0,2)],
	true, clFood
);
createObjectGroups(group, 0,
	avoidClasses(clWater, 3, clForest, 0, clPlayer, 20, clHill, 1, clFood, 5),
	3 * numPlayers, 50
);

// create zebra
log("Creating zebra...");
group = new SimpleGroup(
	[new SimpleObject(oZebra, 2,3, 0,2)],
	true, clFood
);
createObjectGroups(group, 0,
	avoidClasses(clWater, 3, clForest, 0, clPlayer, 20, clHill, 1, clFood, 5),
	3 * numPlayers, 50
);

// create fish
log("Creating fish...");
group = new SimpleGroup(
	[new SimpleObject(oFish, 2,3, 0,2)],
	true, clFood
);
createObjectGroups(group, 0,
	[avoidClasses(clFood, 20), stayClasses(clWater, 6)],
	25 * numPlayers, 60
);

// create berry bush
log("Creating berry bush...");
group = new SimpleGroup(
	[new SimpleObject(oBerryBush, 5,7, 0,4)],
	true, clFood
);
createObjectGroups(group, 0,
	avoidClasses(clWater, 3, clForest, 0, clPlayer, 20, clHill, 1, clFood, 10),
	randInt(1, 4) * numPlayers + 2, 50
);

RMS.SetProgress(85);

// create straggler trees
log("Creating straggler trees...");
var types = [oBaobab, oBaobab, oBaobab, oFig];	// some variation
var num = floor(numStragglers / types.length);
for (var i = 0; i < types.length; ++i)
{
	group = new SimpleGroup(
		[new SimpleObject(types[i], 1,1, 0,3)],
		true, clForest
	);
	createObjectGroups(group, 0,
		avoidClasses(clWater, 5, clForest, 1, clHill, 1, clPlayer, 12, clMetal, 1, clRock, 1),
		num
	);
}

var planetm = 4;
//create small grass tufts
log("Creating small grass tufts...");
group = new SimpleGroup(
	[new SimpleObject(aGrassShort, 1,2, 0,1, -PI/8,PI/8)]
);
createObjectGroups(group, 0,
	avoidClasses(clWater, 2, clHill, 2, clPlayer, 2),
	planetm * scaleByMapSize(13, 200)
);

RMS.SetProgress(90);

// create large grass tufts
log("Creating large grass tufts...");
group = new SimpleGroup(
	[new SimpleObject(aGrass, 2,4, 0,1.8, -PI/8,PI/8), new SimpleObject(aGrassShort, 3,6, 1.2,2.5, -PI/8,PI/8)]
);
createObjectGroups(group, 0,
	avoidClasses(clWater, 3, clHill, 2, clPlayer, 2, clForest, 0),
	planetm * scaleByMapSize(13, 200)
);

RMS.SetProgress(95);

// create bushes
log("Creating bushes...");
group = new SimpleGroup(
	[new SimpleObject(aBushMedium, 1,2, 0,2), new SimpleObject(aBushSmall, 2,4, 0,2)]
);
createObjectGroups(group, 0,
	avoidClasses(clWater, 2, clHill, 1, clPlayer, 1),
	planetm * scaleByMapSize(13, 200), 50
);


setSkySet("sunny");

setSunRotation(randFloat(0, TWO_PI));
setSunElevation(randFloat(PI/ 5, PI / 4));
setWaterColour(0.478,0.42,0.384);				// greyish
setWaterTint(0.58,0.22,0.067);				// reddish
setWaterMurkiness(0.87);
setWaterWaviness(0.5);

// Export map data

ExportMap();