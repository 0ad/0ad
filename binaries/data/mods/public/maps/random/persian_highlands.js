RMS.LoadLibrary("rmgen");

const tCity = "desert_city_tile_pers_dirt";
var random_season = randInt(0,1);
if (random_season) //summer
{
	var tDirtMain = ["desert_dirt_persia_1", "desert_dirt_persia_2", "grass_field_dry"];
	var tLakebed1 = ["desert_lakebed_dry_b", "desert_lakebed_dry"];
	var tLakebed2 = ["desert_lakebed_dry_b", "desert_lakebed_dry", "desert_shore_stones", "desert_shore_stones"];
	var tPebbles = "desert_pebbles_rough";
	var tCliff = ["desert_cliff_persia_1", "desert_cliff_persia_crumbling"];
	var tForestFloor = "medit_grass_field_dry";
	var tRocky = "desert_dirt_persia_rocky";
	var tRocks = "desert_dirt_persia_rocks";
	var tGrass = "grass_field_dry";
	var tHill = "desert_cliff_persia_base";
}
else //spring
{
	var tDirtMain = ["desert_grass_a", "desert_grass_a", "desert_grass_a", "desert_plants_a"];
	var tLakebed1 = ["desert_lakebed_dry_b", "desert_lakebed_dry"];
	var tLakebed2 = "desert_grass_a_sand";
	var tPebbles = "desert_pebbles_rough";
	var tCliff = ["desert_cliff_persia_1", "desert_cliff_persia_crumbling"];
	var tForestFloor = "desert_plants_b_persia";
	var tRocky = "desert_plants_b_persia";
	var tRocks = "desert_plants_a";
	var tGrass = "desert_dirt_persia_rocky";
	var tHill = "desert_cliff_persia_base";
}

// gaia entities
const oGrapesBush = "gaia/flora_bush_grapes";
const oChicken = "gaia/fauna_chicken";
const oCamel = "gaia/fauna_camel";
const oFish = "gaia/fauna_fish";
const oSheep = "gaia/fauna_sheep";
const oGoat = "gaia/fauna_goat";
const oLion = "gaia/fauna_lioness";
const oStoneLarge = "gaia/geology_stonemine_desert_badlands_quarry";
const oStoneSmall = "gaia/geology_stone_desert_small";
const oMetalLarge = "gaia/geology_metal_desert_slabs";
const oTamarix = "gaia/flora_tree_tamarix";
const oOak = "gaia/flora_tree_oak";

// decorative props
const aBush1 = "actor|props/flora/bush_desert_a.xml";
const aBush2 = "actor|props/flora/bush_desert_dry_a.xml";
const aBush3 = "actor|props/flora/bush_dry_a.xml";
const aBush4 = "actor|props/flora/plant_desert_a.xml";
const aBushes = [aBush1, aBush2, aBush3, aBush4];
const aDecorativeRock = "actor|geology/stone_desert_med.xml";

// terrain + entity (for painting)
const pForestO = [tForestFloor + TERRAIN_SEPARATOR + oOak, tForestFloor + TERRAIN_SEPARATOR + oOak, tForestFloor, tDirtMain, tDirtMain];

const BUILDING_ANGlE = -PI/4;

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
var clPatch = createTileClass();
var clRock = createTileClass();
var clMetal = createTileClass();
var clFood = createTileClass();
var clBaseResource = createTileClass();
var clCP = createTileClass();

for (var ix = 0; ix < mapSize; ix++)
{
	for (var iz = 0; iz < mapSize; iz++)
	{
		var x = ix / (mapSize + 1.0);
		var z = iz / (mapSize + 1.0);
			placeTerrain(ix, iz, tDirtMain);
	}
}

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
		[new SimpleObject(oGrapesBush, 5,5, 0,3)],
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
	
	// create starting trees
	var num = 3;
	var tAngle = randFloat(-PI/3, 4*PI/3);
	var tDist = randFloat(11, 13);
	var tX = round(fx + tDist * cos(tAngle));
	var tZ = round(fz + tDist * sin(tAngle));
	group = new SimpleGroup(
		[new SimpleObject(oOak, num, num, 0,5)],
		false, clBaseResource, tX, tZ
	);
	createObjectGroup(group, 0, avoidClasses(clBaseResource,2));
}

RMS.SetProgress(10);

// create patches
log("Creating rock patches...");
placer = new ChainPlacer(1, floor(scaleByMapSize(3, 6)), floor(scaleByMapSize(20, 45)), 0);
painter = new TerrainPainter(tRocky);
createAreas(placer, [painter, paintClass(clPatch)], 
	avoidClasses(clPatch, 2, clPlayer, 0),
	scaleByMapSize(5, 20)
);

RMS.SetProgress(15);

var placer = new ChainPlacer(1, floor(scaleByMapSize(3, 5)), floor(scaleByMapSize(15, 40)), 0);
var painter = new TerrainPainter([tRocky, tRocks]);
createAreas(placer, [painter, paintClass(clPatch)], 
	avoidClasses(clPatch, 2, clPlayer, 4),
	scaleByMapSize(15, 50)
);

RMS.SetProgress(20);

log("Creating dirt patches...");
placer = new ChainPlacer(1, floor(scaleByMapSize(3, 5)), floor(scaleByMapSize(15, 40)), 0);
painter = new TerrainPainter([tGrass]);
createAreas(placer, [painter, paintClass(clPatch)], 
	avoidClasses(clPatch, 2, clPlayer, 4),
	scaleByMapSize(15, 50)
);

RMS.SetProgress(25);

// create centeral plateau
log("Creating centeral plateau...");
var halfSize = mapSize / 2;
var oRadius = scaleByMapSize(18, 68);
placer = new ChainPlacer(2, floor(scaleByMapSize(5, 13)), floor(scaleByMapSize(35, 200)), 1, halfSize, halfSize, 0, [floor(oRadius)]);
painter = new LayeredPainter([tLakebed2, tLakebed1], [6]);
var elevationPainter = new SmoothElevationPainter(ELEVATION_MODIFY, -10, 8);
createArea(placer, [painter, elevationPainter, paintClass(clCP)], avoidClasses(clPlayer, 18));


RMS.SetProgress(30);

// create hills
log("Creating hills...");
var numHills = scaleByMapSize(20, 80)
for (var i = 0; i < numHills; ++i)
{
	
	createMountain(
		floor(scaleByMapSize(40, 60)),
		floor(scaleByMapSize(3, 4)),
		floor(scaleByMapSize(6, 12)),
		floor(scaleByMapSize(4, 10)),
		avoidClasses(clPlayer, 7, clCP, 5, clHill, floor(scaleByMapSize(18, 25))),
		randInt(mapSize),
		randInt(mapSize),
		tCliff,
		clHill,
		14
	);
}

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
	[[tDirtMain, tForestFloor, pForestO], [tForestFloor, pForestO]],
	[[tDirtMain, tForestFloor, pForestO], [tForestFloor, pForestO]]
];	// some variation
var size = numForest / (scaleByMapSize(3,6) * numPlayers);
var num = floor(size / types.length);
for (var i = 0; i < types.length; ++i)
{
	placer = new ChainPlacer(floor(scaleByMapSize(1, 2)), floor(scaleByMapSize(2, 5)), floor(size / floor(scaleByMapSize(8, 3))), 1);
	painter = new LayeredPainter(
		types[i],		// terrains
		[2]											// widths
		);
	createAreas(
		placer,
		[painter, paintClass(clForest)], 
		avoidClasses(clPlayer, 6, clWater, 3, clForest, 10, clHill, 1, clCP, 1),
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
	stayClasses(clCP, 6),
	5*scaleByMapSize(5,30), 50
);

// create small stone quarries
group = new SimpleGroup([new SimpleObject(oStoneSmall, 2,5, 1,3), new RandomObject(aBushes, 2,4, 0,2)], true, clRock);
createObjectGroups(group, 0,
	stayClasses(clCP, 6),
	5*scaleByMapSize(5,30), 50
);

log("Creating centeral metal mines...");
// create large metal quarries
group = new SimpleGroup([new SimpleObject(oMetalLarge, 1,1, 0,4), new RandomObject(aBushes, 2,4, 0,2)], true, clMetal);
createObjectGroups(group, 0,
	stayClasses(clCP, 6),
	5*scaleByMapSize(5,30), 50
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

// create grape bush
log("Creating grape bush...");
group = new SimpleGroup(
	[new SimpleObject(oGrapesBush, 5,7, 0,4)],
	true, clFood
);
createObjectGroups(group, 0,
	avoidClasses(clForest, 0, clPlayer, 20, clHill, 1, clFood, 10, clCP, 2),
	randInt(1, 4) * numPlayers + 2, 50
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

if (!random_season)
	setTerrainAmbientColor(0.329412, 0.419608, 0.501961);

setSunColor(1.0, 0.796, 0.374);	
setSunElevation(PI / 6);
setSunRotation(-1.86532);

setFogFactor(0.2);
setFogThickness(0.0);
setFogColor(0.852, 0.746, 0.493);

setPPEffect("hdr");
setPPContrast(0.75);
setPPSaturation(0.45);
setPPBloom(0.3);

ExportMap();