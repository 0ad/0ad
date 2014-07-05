RMS.LoadLibrary("rmgen");

// terrain textures
const tCity = "desert_city_tile";
const tCityPlaza = "desert_city_tile_plaza";
const tSand = "desert_dirt_rough";
const tDunes = "desert_sand_dunes_100";
const tFineSand = "desert_sand_smooth";
const tCliff = ["desert_cliff_badlands", "desert_cliff_badlands_2"];
const tForestFloor = "desert_forestfloor_palms";
const tGrass = "desert_grass_a";
const tGrassSand50 = "desert_grass_a_sand";
const tGrassSand25 = "desert_grass_a_stones";
const tDirt = "desert_dirt_rough";
const tDirtCracks = "desert_dirt_cracks";
const tShore = "desert_shore_stones";
const tWaterDeep = "desert_shore_stones_wet";

// gaia entities
const oBerryBush = "gaia/flora_bush_grapes";
const oChicken = "gaia/fauna_chicken";
const oCamel = "gaia/fauna_camel";
const oFish = "gaia/fauna_fish";
const oGazelle = "gaia/fauna_gazelle";
const oGiraffe = "gaia/fauna_giraffe";
const oGoat = "gaia/fauna_goat";
const oWildebeest = "gaia/fauna_wildebeest";
const oStoneLarge = "gaia/geology_stonemine_desert_badlands_quarry";
const oStoneSmall = "gaia/geology_stone_desert_small";
const oMetalLarge = "gaia/geology_metal_desert_slabs";
const oDatePalm = "gaia/flora_tree_date_palm";
const oSDatePalm = "gaia/flora_tree_senegal_date_palm";

// decorative props
const aBush1 = "actor|props/flora/bush_desert_a.xml";
const aBush2 = "actor|props/flora/bush_desert_dry_a.xml";
const aBush3 = "actor|props/flora/bush_dry_a.xml";
const aBush4 = "actor|props/flora/plant_desert_a.xml";
const aBushes = [aBush1, aBush2, aBush3, aBush4];
const aDecorativeRock = "actor|geology/stone_desert_med.xml";

// terrain + entity (for painting)
const pForest = [tForestFloor + TERRAIN_SEPARATOR + oDatePalm, tForestFloor + TERRAIN_SEPARATOR + oSDatePalm, tForestFloor];
const pForestOasis = [tGrass + TERRAIN_SEPARATOR + oDatePalm, tGrass + TERRAIN_SEPARATOR + oSDatePalm, tGrass];

const BUILDING_ANGlE = -PI/4;

// initialize map

log("Initializing map...");

InitMap();

const numPlayers = getNumPlayers();
const mapSize = getMapSize();
const mapArea = mapSize*mapSize;

// create tile classes

var clPlayer = createTileClass();
var clHill1 = createTileClass();
var clHill2 = createTileClass();
var clHill3 = createTileClass();
var clForest = createTileClass();
var clWater = createTileClass();
var clPatch = createTileClass();
var clRock = createTileClass();
var clMetal = createTileClass();
var clFood = createTileClass();
var clBaseResource = createTileClass();

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
	var painter = new LayeredPainter([tCity, tCityPlaza], [3]);
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
	
	var hillSize = PI * radius * radius;
	// create starting trees
	var num = floor(hillSize / 100);
	var tAngle = randFloat(-PI/3, 4*PI/3);
	var tDist = randFloat(12, 14);
	var tX = round(fx + tDist * cos(tAngle));
	var tZ = round(fz + tDist * sin(tAngle));
	group = new SimpleGroup(
		[new SimpleObject(oDatePalm, num, num, 0,5)],
		false, clBaseResource, tX, tZ
	);
	createObjectGroup(group, 0, avoidClasses(clBaseResource,2));
}

RMS.SetProgress(10);

// create patches
log("Creating dune patches...");
placer = new ClumpPlacer(scaleByMapSize(40, 150), 0.2, 0.1, 0);
painter = new TerrainPainter(tDunes);
createAreas(placer, [painter, paintClass(clPatch)], 
	avoidClasses(clPatch, 2, clPlayer, 0),
	scaleByMapSize(5, 20)
);

RMS.SetProgress(15);

log("Creating sand patches...");
var placer = new ClumpPlacer(scaleByMapSize(25, 100), 0.2, 0.1, 0);
var painter = new TerrainPainter([tSand, tFineSand]);
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

// create the oasis
log("Creating oasis...");
var oRadius = scaleByMapSize(14, 40);
placer = new ClumpPlacer(PI*oRadius*oRadius, 0.6, 0.15, 0, mapSize/2, mapSize/2);
painter = new LayeredPainter([[tSand, pForest], [tGrassSand25, pForestOasis], tGrassSand25, tShore, tWaterDeep], [2, 3, 1, 1]);
elevationPainter = new SmoothElevationPainter(ELEVATION_MODIFY, -11, 8);
createArea(placer, [painter, elevationPainter, paintClass(clForest)], null);

RMS.SetProgress(30);

// create oasis wildlife
var num = round(PI * oRadius / 8);
var constraint = new AndConstraint([borderClasses(clForest, 0, 3), avoidClasses(clForest, 0)]);
var halfSize = mapSize/2;
for (var i = 0; i < num; ++i)
{
	var r = 0;
	var angle = TWO_PI / num * i;
	do {
		// Work outward until constraint met
		var gx = round(halfSize + r * cos(angle));
		var gz = round(halfSize + r * sin(angle));
		++r;
	} while (!constraint.allows(gx,gz) && r < halfSize);
	
	group = new RandomGroup(
		[	new SimpleObject(oGiraffe, 2,4, 0,3),		// select from these groups randomly
			new SimpleObject(oWildebeest, 3,5, 0,3),
			new SimpleObject(oGazelle, 5,7, 0,3)
		], true, clFood, gx, gz
	);
	createObjectGroup(group, 0);
}

constraint = new AndConstraint([borderClasses(clForest, 15, 0), avoidClasses(clFood, 5)]);
num = round(PI * oRadius / 16);
for (var i = 0; i < num; ++i)
{
	var r = 0;
	var angle = TWO_PI / num * i;
	do {
		// Work outward until constraint met 
		var gx = round(halfSize + r * cos(angle));
		var gz = round(halfSize + r * sin(angle));
		++r;
	} while (!constraint.allows(gx,gz) && r < halfSize);
	
	group = new SimpleGroup(
		[new SimpleObject(oFish, 1,1, 0,1)],
		true, clFood, gx, gz
	);
	createObjectGroup(group, 0);
}

RMS.SetProgress(35);

// create hills
log("Creating level 1 hills...");
placer = new ClumpPlacer(scaleByMapSize(50,300), 0.25, 0.1, 0.5);
var terrainPainter = new LayeredPainter(
	[tCliff, tSand],		// terrains
	[1]				// widths
);
var elevationPainter = new SmoothElevationPainter(ELEVATION_MODIFY, 16, 1);
var hillAreas = createAreas(placer, [terrainPainter, elevationPainter, paintClass(clHill1)], 
	avoidClasses(clForest, 3, clPlayer, 0, clHill1, 10),
	scaleByMapSize(10,20), 100
);

RMS.SetProgress(40);

log("Creating small level 1 hills...");
placer = new ClumpPlacer(scaleByMapSize(25,150), 0.25, 0.1, 0.5);
terrainPainter = new LayeredPainter(
	[tCliff, tSand],		// terrains
	[1]				// widths
);
elevationPainter = new SmoothElevationPainter(ELEVATION_MODIFY, 16, 1);
var tempAreas = createAreas(placer, [terrainPainter, elevationPainter, paintClass(clHill1)], 
	avoidClasses(clForest, 3, clPlayer, 0, clHill1, 3),
	scaleByMapSize(15,25), 100
);
for (var i = 0; i < tempAreas.length; ++i)
{
	hillAreas.push(tempAreas[i]);
}

RMS.SetProgress(45);

// create decorative rocks for hills
log("Creating decorative rocks...");
group = new SimpleGroup(
	[new RandomObject([aDecorativeRock, aBush2, aBush3], 3,8, 0,2)],
	true
);
createObjectGroupsByAreas(group, 0,
	borderClasses(clHill1, 0, 3),
	scaleByMapSize(40,200), 50,
	hillAreas
);

RMS.SetProgress(50);

log("Creating level 2 hills...");
placer = new ClumpPlacer(scaleByMapSize(25,150), 0.25, 0.1, 0);
terrainPainter = new LayeredPainter(
	[tCliff, tSand],		// terrains
	[1]				// widths
);
elevationPainter = new SmoothElevationPainter(ELEVATION_MODIFY, 16, 1);
createAreasInAreas(placer, [terrainPainter, elevationPainter], 
	[stayClasses(clHill1, 0)],
	scaleByMapSize(15,25), 50,
	hillAreas
);

RMS.SetProgress(55);

log("Creating level 3 hills...");
placer = new ClumpPlacer(scaleByMapSize(12, 75), 0.25, 0.1, 0);
terrainPainter = new LayeredPainter(
	[tCliff, tSand],		// terrains
	[1]				// widths
);
elevationPainter = new SmoothElevationPainter(ELEVATION_MODIFY, 16, 1);
createAreas(placer, [terrainPainter, elevationPainter], 
	[stayClasses(clHill1, 0)],
	scaleByMapSize(15,25), 50,
	hillAreas
);
hillAreas = [];

RMS.SetProgress(60);

// create bumps
log("Creating bumps...");
placer = new ClumpPlacer(scaleByMapSize(20, 50), 0.3, 0.06, 0);
elevationPainter = new SmoothElevationPainter(ELEVATION_MODIFY, 2, 2);
createAreas(
	placer,
	elevationPainter, 
	avoidClasses(clForest, 0, clPlayer, 0, clHill1, 2),
	scaleByMapSize(100, 200)
);

RMS.SetProgress(65);

// calculate desired number of trees for map (based on size)
const MIN_TREES = 500;
const MAX_TREES = 2500;
const P_FOREST = 0.5;

var totalTrees = scaleByMapSize(MIN_TREES, MAX_TREES);
var numForest = totalTrees * P_FOREST;
var numStragglers = totalTrees * (1.0 - P_FOREST);

// create forests
log("Creating forests...");
var num = scaleByMapSize(10,30);
placer = new ClumpPlacer(numForest / num, 0.15, 0.1, 0.5);
painter = new TerrainPainter([tSand, pForest]);
createAreas(placer, [painter, paintClass(clForest)], 
	avoidClasses(clPlayer, 1, clForest, 10, clHill1, 1),
	num, 50
);

RMS.SetProgress(70);

log("Creating stone mines...");
// create large stone quarries
group = new SimpleGroup([new SimpleObject(oStoneSmall, 0,2, 0,4), new SimpleObject(oStoneLarge, 1,1, 0,4), new RandomObject(aBushes, 2,4, 0,2)], true, clRock);
createObjectGroups(group, 0,
	[avoidClasses(clForest, 1, clPlayer, 10, clRock, 10, clHill1, 1)],
	scaleByMapSize(4,16), 100
);

// create small stone quarries
group = new SimpleGroup([new SimpleObject(oStoneSmall, 2,5, 1,3), new RandomObject(aBushes, 2,4, 0,2)], true, clRock);
createObjectGroups(group, 0,
	[avoidClasses(clForest, 1, clPlayer, 10, clRock, 10, clHill1, 1)],
	scaleByMapSize(4,16), 100
);

log("Creating metal mines...");
// create large metal quarries
group = new SimpleGroup([new SimpleObject(oMetalLarge, 1,1, 0,4), new RandomObject(aBushes, 2,4, 0,2)], true, clMetal);
createObjectGroups(group, 0,
	[avoidClasses(clForest, 1, clPlayer, 10, clMetal, 10, clRock, 5, clHill1, 1)],
	scaleByMapSize(4,16), 100
);

RMS.SetProgress(80);

// create gazelles
log("Creating gazelles...");
group = new SimpleGroup([new SimpleObject(oGazelle, 5,7, 0,4)], true, clFood);
createObjectGroups(group, 0,
	avoidClasses(clForest, 0, clPlayer, 5, clHill1, 1, clFood, 10),
	scaleByMapSize(5,20), 50
);

// create goats
log("Creating goats...");
group = new SimpleGroup([new SimpleObject(oGoat, 2,4, 0,3)], true, clFood);
createObjectGroups(group, 0,
	avoidClasses(clForest, 0, clPlayer, 5, clHill1, 1, clFood, 10),
	scaleByMapSize(5,20), 50
);

// create camels
log("Creating camels...");
group = new SimpleGroup([new SimpleObject(oCamel, 2,4, 0,2)], true, clFood);
createObjectGroups(group, 0,
	avoidClasses(clForest, 0, clPlayer, 5, clHill1, 1, clFood, 10),
	scaleByMapSize(5,20), 50
);

RMS.SetProgress(85);

// create straggler trees
log("Creating straggler trees...");
var types = [oDatePalm, oSDatePalm];	// some variation
var num = floor(numStragglers / types.length);
for (var i = 0; i < types.length; ++i)
{
	group = new SimpleGroup([new SimpleObject(types[i], 1,1, 0,0)], true);
	createObjectGroups(group, 0,
		avoidClasses(clForest, 0, clHill1, 1, clPlayer, 4, clMetal, 1, clRock, 1),
		num
	);
}

RMS.SetProgress(90);

// create bushes
log("Creating bushes...");
group = new SimpleGroup([new RandomObject(aBushes, 2,3, 0,2)]);
createObjectGroups(group, 0,
	avoidClasses(clHill1, 1, clPlayer, 0, clForest, 0),
	scaleByMapSize(16, 262)
);

// create rocks
log("Creating more decorative rocks...");
group = new SimpleGroup([new SimpleObject(aDecorativeRock, 1,2, 0,2)]);
createObjectGroups(group, 0,
	avoidClasses(clHill1, 1, clPlayer, 0, clForest, 0),
	scaleByMapSize(16, 262)
);

setWaterColour(0, 0.227, 0.843);
setWaterTint(0, 0.545, 0.859);
setWaterWaviness(1);
setWaterMurkiness(0.75);

// Export map data
ExportMap();
