RMS.LoadLibrary("rmgen");

const tCity = "desert_city_tile_plaza";
const tSand = ["desert_dirt_persia_1", "desert_dirt_persia_2"];
const tDunes = ["desert_lakebed_dry_b", "desert_dirt_persia_1", "desert_dirt_persia_2", "desert_lakebed_dry"];
const tDunes2 = ["desert_lakebed_dry_b", "desert_dirt_persia_1", "desert_dirt_persia_2", "desert_lakebed_dry", "desert_dirt_persia_2", "desert_dirt_persia_1"];
const tFineSand = "desert_pebbles_rough";
const tCliff = ["desert_cliff_persia_1", "desert_cliff_persia_2"];
const tForestFloor = "medit_grass_field_dry";
const tRocky = "desert_dirt_persia_rocky";
const tRocks = "desert_dirt_persia_rocks";
const tDirt = "desert_dirt_rough";
const tHill = "desert_cliff_persia_base";

// gaia entities
const oBerryBush = "gaia/flora_bush_grapes";
const oChicken = "gaia/fauna_chicken";
const oCamel = "gaia/fauna_camel";
const oFish = "gaia/fauna_fish";
const oSheep = "gaia/fauna_sheep";
const oGoat = "gaia/fauna_goat";
const oLion = "gaia/fauna_lioness";
const oStoneLarge = "gaia/geology_stonemine_desert_badlands_quarry";
const oStoneSmall = "gaia/geology_stone_desert_small";
const oMetalLarge = "gaia/geology_metal_desert_slabs";
const oDead = "gaia/flora_tree_dead";
const oOak = "gaia/flora_tree_oak";

// decorative props
const aBush1 = "actor|props/flora/bush_desert_a.xml";
const aBush2 = "actor|props/flora/bush_desert_dry_a.xml";
const aBush3 = "actor|props/flora/bush_dry_a.xml";
const aBush4 = "actor|props/flora/plant_desert_a.xml";
const aBushes = [aBush1, aBush2, aBush3, aBush4];
const aDecorativeRock = "actor|geology/stone_desert_med.xml";

// terrain + entity (for painting)
const pForestO = [tForestFloor + TERRAIN_SEPARATOR + oOak, tForestFloor + TERRAIN_SEPARATOR + oOak, tForestFloor];
const pForestD = [tForestFloor + TERRAIN_SEPARATOR + oDead, tForestFloor + TERRAIN_SEPARATOR + oDead, tForestFloor];

const BUILDING_ANGlE = 0.75*PI;

log("Initializing map...");

InitMap();

var numPlayers = getNumPlayers();
var mapSize = getMapSize();
var mapArea = mapSize*mapSize;

// create tile classes

var clPlayer = createTileClass();
var clHill = createTileClass();
var clForest = createTileClass();
var clWater = createTileClass();
var clPatch = createTileClass();
var clRock = createTileClass();
var clMetal = createTileClass();
var clFood = createTileClass();
var clBaseResource = createTileClass();
var clCP = createTileClass();


var playerIDs = [];
for (var i = 0; i < numPlayers; i++)
{
	playerIDs.push(i+1);
}
playerIDs = shuffleArray(playerIDs);

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
	
	// scale radius of player area by map size
	var radius = scaleByMapSize(15,25);
	
	// get the x and z in tiles
	var fx = fractionToTiles(playerX[i]);
	var fz = fractionToTiles(playerZ[i]);
	var ix = round(fx);
	var iz = round(fz);

	// calculate size based on the radius
	var size = PI * radius * radius;
	
	// create the player area
	var placer = new ClumpPlacer(size, 0.9, 0.5, 10, ix, iz);
	createArea(placer, paintClass(clPlayer), null);
	
	// create the city patch
	var cityRadius = 10;
	placer = new ClumpPlacer(PI*cityRadius*cityRadius, 0.6, 0.3, 10, ix, iz);
	var painter = new LayeredPainter([tCity, tCity], [3]);
	createArea(placer, painter, null);
	
	// get civ specific starting entities
	var civEntities = getStartingEntities(id-1);
	
	// create starting units
	createStartingPlayerEntities(fx, fz, id, civEntities, BUILDING_ANGlE)
	
	// create animals
	for (var j = 0; j < 2; ++j)
	{
		var aAngle = randFloat(0, TWO_PI);
		var aDist = 7;
		var aX = round(fx + aDist * cos(aAngle));
		var aZ = round(fz + aDist * sin(aAngle));
		var group = new SimpleGroup(
			[new SimpleObject(oChicken, 5,5, 0,3)],
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
	var mDist = 11;
	var mX = round(fx + mDist * cos(mAngle));
	var mZ = round(fz + mDist * sin(mAngle));
	group = new SimpleGroup(
		[new SimpleObject(oMetalLarge, 1,1, 0,0), new RandomObject(aBushes, 2,4, 0,2)],
		true, clBaseResource, mX, mZ
	);
	createObjectGroup(group, 0);
	
	// create stone mines
	mAngle += randFloat(PI/8, PI/4);
	mX = round(fx + mDist * cos(mAngle));
	mZ = round(fz + mDist * sin(mAngle));
	group = new SimpleGroup(
		[new SimpleObject(oStoneLarge, 1,1, 0,2), new RandomObject(aBushes, 2,4, 0,2)],
		true, clBaseResource, mX, mZ
	);
	createObjectGroup(group, 0);
	
	// create starting straggler trees
	group = new SimpleGroup(
		[new SimpleObject(oOak, 3, 4, 6,12), new SimpleObject(oOak, 3,4, 6,12)],
		true, null,	ix, iz
	);
	createObjectGroup(group, 0, avoidClasses(clBaseResource,1));
}

RMS.SetProgress(10);

// create patches
log("Creating rock patches...");
placer = new ClumpPlacer(scaleByMapSize(40, 150), 0.2, 0.1, 0);
painter = new TerrainPainter(tRocky);
createAreas(placer, [painter, paintClass(clPatch)], 
	avoidClasses(clPatch, 2, clPlayer, 0),
	scaleByMapSize(5, 20)
);

RMS.SetProgress(15);

var placer = new ClumpPlacer(scaleByMapSize(25, 100), 0.2, 0.1, 0);
var painter = new TerrainPainter([tRocky, tRocks]);
createAreas(placer, [painter, paintClass(clPatch)], 
	avoidClasses(clPatch, 2, clPlayer, 0),
	scaleByMapSize(15, 50)
);

RMS.SetProgress(20);

log("Creating dirt patches...");
placer = new ClumpPlacer(scaleByMapSize(25, 100), 0.2, 0.1, 0);
painter = new TerrainPainter([tDirt]);
createAreas(placer, [painter, paintClass(clPatch)], 
	avoidClasses(clPatch, 2, clPlayer, 0),
	scaleByMapSize(15, 50)
);

RMS.SetProgress(25);

// create centeral plateau
log("Creating centeral plateau...");
var oRadius = scaleByMapSize(18, 68);
placer = new ClumpPlacer(PI*oRadius*oRadius, 0.6, 0.15, 0, mapSize/2, mapSize/2);
painter = new LayeredPainter([tDunes2, tDunes], [8]);
var elevationPainter = new SmoothElevationPainter(ELEVATION_MODIFY, -10, 8);
createArea(placer, [painter, elevationPainter, paintClass(clCP)], null);


RMS.SetProgress(30);


// create hills
log("Creating hills...");
placer = new ClumpPlacer(scaleByMapSize(20, 150), 0.2, 0.1, 1);
var terrainPainter = new LayeredPainter(
	[tCliff, tHill],		// terrains
	[2]								// widths
);
var elevationPainter = new SmoothElevationPainter(ELEVATION_SET, 22, 2);
createAreas(
	placer,
	[terrainPainter, elevationPainter, paintClass(clHill)], 
	avoidClasses(clPlayer, 3, clCP, 5, clHill, 10),
	round(scaleByMapSize(1, 4) * numPlayers * 1.4)
);

RMS.SetProgress(35);

// calculate desired number of trees for map (based on size)
const MIN_TREES = 500;
const MAX_TREES = 2500;
const P_FOREST = 0.7;

var totalTrees = scaleByMapSize(MIN_TREES, MAX_TREES);
var numForest = totalTrees * P_FOREST;
var numStragglers = totalTrees * (1.0 - P_FOREST);

// create forests
log("Creating forests...");
var types = [
	[[tSand, tForestFloor, pForestO], [tForestFloor, pForestO]],
	[[tSand, tForestFloor, pForestO], [tForestFloor, pForestO]]
];	// some variation
var size = numForest / (scaleByMapSize(2,8) * numPlayers);
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
		avoidClasses(clPlayer, 1, clWater, 3, clForest, 10, clHill, 1, clCP, 1),
		num
	);
}

RMS.SetProgress(50);


log("Creating stone mines...");
// create large stone quarries
group = new SimpleGroup([new SimpleObject(oStoneSmall, 0,2, 0,4), new SimpleObject(oStoneLarge, 1,1, 0,4), new RandomObject(aBushes, 2,4, 0,2)], true, clRock);
createObjectGroups(group, 0,
	[avoidClasses(clForest, 1, clPlayer, 10, clRock, 10, clHill, 1, clCP, 1)],
	scaleByMapSize(2,8), 100
);

// create small stone quarries
group = new SimpleGroup([new SimpleObject(oStoneSmall, 2,5, 1,3), new RandomObject(aBushes, 2,4, 0,2)], true, clRock);
createObjectGroups(group, 0,
	[avoidClasses(clForest, 1, clPlayer, 10, clRock, 10, clHill, 1, clCP, 1)],
	scaleByMapSize(2,8), 100
);

log("Creating metal mines...");
// create large metal quarries
group = new SimpleGroup([new SimpleObject(oMetalLarge, 1,1, 0,4), new RandomObject(aBushes, 2,4, 0,2)], true, clMetal);
createObjectGroups(group, 0,
	[avoidClasses(clForest, 1, clPlayer, 10, clMetal, 10, clRock, 5, clHill, 1, clCP, 1)],
	scaleByMapSize(2,8), 100
);


log("Creating centeral stone mines...");
// create large stone quarries
group = new SimpleGroup([new SimpleObject(oStoneSmall, 0,2, 0,4), new SimpleObject(oStoneLarge, 1,1, 0,4), new RandomObject(aBushes, 2,4, 0,2)], true, clRock);
createObjectGroups(group, 0,
	stayClasses(clCP, 3),
	5*scaleByMapSize(10,30), 100
);

// create small stone quarries
group = new SimpleGroup([new SimpleObject(oStoneSmall, 2,5, 1,3), new RandomObject(aBushes, 2,4, 0,2)], true, clRock);
createObjectGroups(group, 0,
	stayClasses(clCP, 3),
	5*scaleByMapSize(10,30), 100
);

log("Creating centeral metal mines...");
// create large metal quarries
group = new SimpleGroup([new SimpleObject(oMetalLarge, 1,1, 0,4), new RandomObject(aBushes, 2,4, 0,2)], true, clMetal);
createObjectGroups(group, 0,
	stayClasses(clCP, 3),
	5*scaleByMapSize(10,30), 100
);

RMS.SetProgress(60);

// create small decorative rocks
log("Creating small decorative rocks...");
group = new SimpleGroup(
	[new SimpleObject(aDecorativeRock, 1,3, 0,1)],
	true
);
createObjectGroups(
	group, 0,
	avoidClasses(clWater, 0, clForest, 0, clPlayer, 0, clHill, 0),
	scaleByMapSize(16, 262), 50
);

RMS.SetProgress(65);

//create bushes
log("Creating bushes...");
group = new SimpleGroup(
	[new SimpleObject(aBush2, 1,2, 0,1), new SimpleObject(aBush1, 1,3, 0,2)],
	true
);
createObjectGroups(
	group, 0,
	avoidClasses(clWater, 0, clForest, 0, clPlayer, 0, clHill, 0),
	scaleByMapSize(8, 131), 50
);

RMS.SetProgress(70);

// create goats
log("Creating goat...");
group = new SimpleGroup(
	[new SimpleObject(oGoat, 5,7, 0,4)],
	true, clFood
);
createObjectGroups(group, 0,
	avoidClasses(clForest, 0, clPlayer, 1, clHill, 1, clFood, 20, clCP, 2),
	3 * numPlayers, 50
);

// create sheep
log("Creating sheep...");
group = new SimpleGroup(
	[new SimpleObject(oSheep, 2,3, 0,2)],
	true, clFood
);
createObjectGroups(group, 0,
	avoidClasses(clForest, 0, clPlayer, 1, clHill, 1, clFood, 20, clCP, 2),
	3 * numPlayers, 50
);

// create camels
log("Creating camels...");
group = new SimpleGroup(
	[new SimpleObject(oCamel, 2,3, 0,2)],
	true, clFood
);
createObjectGroups(group, 0,
	stayClasses(clCP, 2),
	3 * numPlayers, 50
);

RMS.SetProgress(85);

// create dead trees
log("Creating dead trees...");
var types = [oDead];	// some variation
var num = floor(numStragglers / types.length);
for (var i = 0; i < types.length; ++i)
{
	group = new SimpleGroup(
		[new SimpleObject(types[i], 1,1, 0,3)],
		true, clForest
	);
	createObjectGroups(group, 0,
		[avoidClasses(clMetal, 1, clRock, 1), stayClasses(clCP, 2)],
		num
	);
}

RMS.SetProgress(90);

// create straggler trees
log("Creating straggler trees...");
var types = [oOak];	// some variation
var num = floor(numStragglers / types.length);
for (var i = 0; i < types.length; ++i)
{
	group = new SimpleGroup(
		[new SimpleObject(types[i], 1,1, 0,3)],
		true, clForest
	);
	createObjectGroups(group, 0,
		avoidClasses(clWater, 1, clForest, 1, clHill, 1, clPlayer, 1, clMetal, 1, clRock, 1, clCP, 2),
		num
	);
}


setSunColour(0.733, 0.746, 0.574);	
ExportMap();