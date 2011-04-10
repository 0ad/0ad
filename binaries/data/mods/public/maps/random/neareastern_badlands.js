RMS.LoadLibrary("rmgen");

// terrain textures
const tSand = "desert_dirt_rough";
const tDunes = "desert_sand_dunes_100";
const tFineSand = "desert_sand_smooth";
const tCliff = "desert_cliff_badlands";
const tGrassSand75 = "desert_grass_a";
const tGrassSand50 = "desert_grass_a_sand";
const tGrassSand25 = "desert_grass_a_stones";
const tDirt = "desert_dirt_rough";
const tDirtCracks = "desert_dirt_cracks";
const tShore = "desert_sand_wet";
const tWater = "desert_shore_stones";
const tWaterDeep = "desert_shore_stones_wet";

// gaia entities
const oBerryBush = "gaia/flora_bush_berry";
const oSheep = "gaia/fauna_sheep";
const oDeer = "gaia/fauna_deer";
const oStone = "gaia/geology_stone_greek";
const oMetal = "gaia/geology_metal_greek";
const oTree = "gaia/flora_tree_medit_fan_palm";

// decorative props
const aBush = "actor|props/flora/bush_dry_a.xml";
const aDecorativeRock = "actor|geology/gray1.xml";

// terrain + entity (for painting)
var pForest = tGrassSand75 + TERRAIN_SEPARATOR + oTree;

// initialize map

log("Initializing map...");

InitMap();

var numPlayers = getNumPlayers();
var mapSize = getMapSize();
var mapArea = mapSize*mapSize;

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

// place players

var playerX = new Array(numPlayers);
var playerY = new Array(numPlayers);
var playerAngle = new Array(numPlayers);

var startAngle = randFloat() * 2 * PI;
for (var i=0; i < numPlayers; i++)
{
	playerAngle[i] = startAngle + i*2*PI/numPlayers;
	playerX[i] = 0.5 + 0.39*cos(playerAngle[i]);
	playerY[i] = 0.5 + 0.39*sin(playerAngle[i]);
}

for (var i=0; i < numPlayers; i++)
{
	log("Creating base for player " + (i + 1) + "...");
	
	// scale radius of player area by map size
	var radius = mapSize * 0.026 + 7;
	
	// get the x and y in tiles
	var fx = fractionToTiles(playerX[i]);
	var fy = fractionToTiles(playerY[i]);
	var ix = round(fx);
	var iy = round(fy);

	// calculate size based on the radius
	var size = PI * radius * radius;
	
	// create the player area
	var placer = new ClumpPlacer(size, 0.9, 0.5, 0, ix, iy);
	createArea(placer, paintClass(clPlayer), null);
	
	// create the central road patch
	placer = new ClumpPlacer(PI*2*2, 0.6, 0.3, 0.5, ix, iy);
	var painter = new TerrainPainter(tDirt);
	createArea(placer, painter, null);
	
	// create the TC and citizens
	var civ = getCivCode(i);
	var group = new SimpleGroup(
		[							// elements (type, count, distance)
			new SimpleObject("structures/"+civ+"_civil_centre", 1,1, 0,0),
			new SimpleObject("units/"+civ+"_support_female_citizen", 3,3, 5,5)
		],
		true, null,	ix, iy
	);
	createObjectGroup(group, i+1);
	
	// create berry bushes
	var bbAngle = randFloat()*2*PI;
	var bbDist = 10;
	var bbX = round(fx + bbDist * cos(bbAngle));
	var bbY = round(fy + bbDist * sin(bbAngle));
	group = new SimpleGroup(
		[new SimpleObject(oSheep, 5,5, 0,2)],
		true, clBaseResource,	bbX, bbY
	);
	createObjectGroup(group, 0);
	
	// create mines
	var mAngle = bbAngle;
	while(abs(mAngle - bbAngle) < PI/3) {
		mAngle = randFloat()*2*PI;
	}
	var mDist = 12;
	var mX = round(fx + mDist * cos(mAngle));
	var mY = round(fy + mDist * sin(mAngle));
	group = new SimpleGroup(
		[new SimpleObject(oStone, 2,2, 0,3),
		new SimpleObject(oMetal, 2,2, 0,3)],
		true, clBaseResource,	mX, mY
	);
	createObjectGroup(group, 0);
	
	// create starting straggler trees
	group = new SimpleGroup(
		[new SimpleObject(oTree, 2,2, 6,12)],
		true, null,	ix, iy
	);
	createObjectGroup(group, 0, avoidClasses(clBaseResource,1));
}

RMS.SetProgress(5);

// create patches
log("Creating sand patches...");
var placer = new ClumpPlacer(30, 0.2, 0.1, 0);
var painter = new LayeredPainter([[tSand, tFineSand], tFineSand], [1]);
createAreas(placer, [painter, paintClass(clPatch)], 
	avoidClasses(clPatch, 5),
	mapArea/600
);

RMS.SetProgress(24);

log("Creating dirt patches...");
placer = new ClumpPlacer(10, 0.2, 0.1, 0);
painter = new TerrainPainter([tSand, tDirt]);
createAreas(placer, [painter, paintClass(clPatch)], 
	avoidClasses(clPatch, 5),
	mapArea/600
);

RMS.SetProgress(32);

// create the oasis (roughly 4% of map area)
log("Creating oasis...");
placer = new ClumpPlacer(mapArea * 0.04, 0.6, 0.1, 0, mapSize/2, mapSize/2);
painter = new LayeredPainter([[tSand, pForest], tShore, tWaterDeep], [6,1]);
elevationPainter = new SmoothElevationPainter(ELEVATION_MODIFY, -11, 5);
createArea(placer, [painter, elevationPainter, paintClass(clForest)], null);

RMS.SetProgress(51);

// create hills
log("Creating level 1 hills...");
placer = new ClumpPlacer(150, 0.25, 0.1, 0.3);
var terrainPainter = new LayeredPainter(
	[tCliff, tSand],		// terrains
	[1]				// widths
);
var elevationPainter = new SmoothElevationPainter(ELEVATION_MODIFY, 16, 1);
createAreas(placer, [terrainPainter, elevationPainter, paintClass(clHill1)], 
	avoidClasses(clForest, 2, clPlayer, 0, clHill1, 16),
	mapArea/3800, 100
);

RMS.SetProgress(70);

log("Creating small level 1 hills...");
placer = new ClumpPlacer(60, 0.25, 0.1, 0.3);
terrainPainter = new LayeredPainter(
	[tCliff, tSand],		// terrains
	[1]				// widths
);
elevationPainter = new SmoothElevationPainter(ELEVATION_MODIFY, 16, 1);
createAreas(placer, [terrainPainter, elevationPainter, paintClass(clHill1)], 
	avoidClasses(clForest, 2, clPlayer, 0, clHill1, 3),
	mapArea/2800, 100
);

RMS.SetProgress(81);

log("Creating level 2 hills...");
placer = new ClumpPlacer(60, 0.2, 0.1, 0.9);
terrainPainter = new LayeredPainter(
	[tCliff, tSand],		// terrains
	[1]				// widths
);
elevationPainter = new SmoothElevationPainter(ELEVATION_MODIFY, 16, 1);
createAreas(placer, [terrainPainter, elevationPainter, paintClass(clHill2)], 
	[avoidClasses(clHill2, 1), stayClasses(clHill1, 0)],
	mapArea/2800, 200
);

RMS.SetProgress(91);

log("Creating level 3 hills...");
placer = new ClumpPlacer(25, 0.2, 0.1, 0.9);
terrainPainter = new LayeredPainter(
	[tCliff, tSand],		// terrains
	[1]				// widths
);
elevationPainter = new SmoothElevationPainter(ELEVATION_MODIFY, 16, 1);
createAreas(placer, [terrainPainter, elevationPainter, paintClass(clHill3)], 
	[avoidClasses(clHill3, 1), stayClasses(clHill2, 0)],
	mapArea/9000, 300
);

// create forests
log("Creating forests...");
placer = new ClumpPlacer(25, 0.15, 0.1, 0.3);
painter = new TerrainPainter([tSand, pForest]);
createAreas(placer, [painter, paintClass(clForest)], 
	avoidClasses(clWater, 0, clPlayer, 1, clForest, 20, clHill1, 0),
	mapArea/4000, 50
);

log("Placing stone mines...");
// create stone
group = new SimpleGroup([new SimpleObject(oStone, 2,3, 0,2)], true, clRock);
createObjectGroups(group, 0,
	[avoidClasses(clWater, 2, clForest, 2, clPlayer, 0, clRock, 13), 
	 new BorderTileClassConstraint(clHill1, 0, 4)],
	mapArea/4000, 100
);

log("Placing metal mines...");
// create metal
group = new SimpleGroup([new SimpleObject(oMetal, 2,3, 0,2)], true, clMetal);
createObjectGroups(group, 0,
	[avoidClasses(clWater, 2, clForest, 2, clPlayer, 0, clMetal, 13, clRock, 5), 
	 new BorderTileClassConstraint(clHill1, 0, 4)],
	mapArea/4000, 100
);

RMS.SetProgress(97);

// create decorative rocks for hills
log("Creating decorative rocks...");
group = new SimpleGroup([new SimpleObject(aDecorativeRock, 1,1, 0,0)], true);
createObjectGroups(group, undefined,
	new BorderTileClassConstraint(clHill1, 0, 3),
	mapArea/2000, 100
);

// create deer
log("Creating deer...");
group = new SimpleGroup([new SimpleObject(oDeer, 5,7, 0,4)], true, clFood);
createObjectGroups(group, 0,
	avoidClasses(clWater, 0, clForest, 0, clPlayer, 0, clHill1, 0, clFood, 25),
	mapArea/5000, 50
);

// create sheep
log("Creating sheep...");
group = new SimpleGroup([new SimpleObject(oSheep, 1,3, 0,2)], true, clFood);
createObjectGroups(group, 0,
	avoidClasses(clWater, 0, clForest, 0, clPlayer, 0, clHill1, 0, clFood, 15),
	mapArea/5000, 50
);

// create straggler trees
log("Creating straggler trees...");
group = new SimpleGroup([new SimpleObject(oTree, 1,1, 0,0)], true);
createObjectGroups(group, 0,
	avoidClasses(clWater, 0, clForest, 0, clHill1, 0, clPlayer, 0),
	mapArea/1500
);

// create bushes
log("Creating bushes...");
group = new SimpleGroup([new SimpleObject(aBush, 2,3, 0,2)]);
createObjectGroups(group, undefined,
	avoidClasses(clWater, 3, clHill1, 0, clPlayer, 0, clForest, 0),
	mapArea/1000
);

// create bushes
log("Creating more decorative rocks...");
group = new SimpleGroup([new SimpleObject(aDecorativeRock, 1,2, 0,2)]);
createObjectGroups(group, undefined,
	avoidClasses(clWater, 3, clHill1, 0, clPlayer, 0, clForest, 0),
	mapArea/1000
);

// Export map data
ExportMap();
