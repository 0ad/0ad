RMS.LoadLibrary("rmgen");

// terrain textures
const tGrass = ["medit_grass_field_a", "medit_grass_field_b"];
const tGrassForest = "medit_grass_wild";
const tCliff = ["medit_cliff_italia", "medit_cliff_italia_grass"];
const tGrassDirt75 = "medit_dirt";
const tGrassDirt50 = "medit_dirt_b";
const tGrassDirt25 = "medit_dirt_c";
const tDirt = "medit_dirt_b";
const tGrassPatch = "medit_grass_wild";
const tShore = "medit_rocks";
const tShoreBlend = "medit_rocks_grass";
const tWater = "medit_rocks_wet";

// gaia entities
const oTree = "gaia/flora_tree_oak";
const oTreeLarge = "gaia/flora_tree_oak";
const oBerryBush = "gaia/flora_bush_berry";
const oSheep = "gaia/fauna_sheep";
const oDeer = "gaia/fauna_deer";
const oMine = "gaia/geology_stone_temperate";

// decorative props
const aGrass = "actor|props/flora/grass_temp_field.xml";
const aGrassShort = "actor|props/flora/grass_field_lush_short.xml";
const aReeds = "actor|props/flora/grass_temp_field_dry.xml";
const aRockLarge = "actor|geology/stone_granite_large.xml";
const aRockMedium = "actor|geology/stone_granite_med.xml";
const aBushMedium = "actor|props/flora/bush_tempe_me.xml";
const aBushSmall = "actor|props/flora/bush_tempe_sm.xml";

// terrain + entity (for painting)
const pForest = tGrassForest + TERRAIN_SEPARATOR + oTree;

// initialize map

log("Initializing map...");

InitMap();

var numPlayers = getNumPlayers();
var mapSize = getMapSize();

// create tile classes

var clPlayer = createTileClass();
var clHill = createTileClass();
var clForest = createTileClass();
var clWater = createTileClass();
var clDirt = createTileClass();
var clRock = createTileClass();
var clFood = createTileClass();
var clBaseResource = createTileClass();

// place players

var playerX = new Array(numPlayers+1);
var playerY = new Array(numPlayers+1);
var playerAngle = new Array(numPlayers+1);

var startAngle = randFloat() * 2 * PI;
for (var i=1; i<=numPlayers; i++)
{
	playerAngle[i] = startAngle + i*2*PI/numPlayers;
	playerX[i] = 0.5 + 0.35*cos(playerAngle[i]);
	playerY[i] = 0.5 + 0.35*sin(playerAngle[i]);
}

for (var i=1; i<=numPlayers; i++)
{
	log("Creating base for player " + i + "...");
	
	// some constants
	var radius = 17;
	var cliffRadius = 2;
	var elevation = 20;
	
	// get the x and y in tiles
	var fx = fractionToTiles(playerX[i]);
	var fy = fractionToTiles(playerY[i]);
	var ix = round(fx);
	var iy = round(fy);

	// calculate size based on the radius
	var size = PI * radius * radius;
	
	// create the hill
	var placer = new ClumpPlacer(size, 0.95, 0.6, 0, ix, iy);
	var terrainPainter = new LayeredPainter(
		[tCliff, tGrass],		// terrains
		[cliffRadius]		// widths
	);
	var elevationPainter = new SmoothElevationPainter(
		ELEVATION_SET,			// type
		elevation,				// elevation
		cliffRadius				// blend radius
	);
	createArea(placer, [terrainPainter, elevationPainter, paintClass(clPlayer)], null);
	
	// create the ramp
	var rampAngle = playerAngle[i] + PI + (2*randFloat()-1)*PI/8;
	var rampDist = radius - 1;
	var rampX = round(fx + rampDist * cos(rampAngle));
	var rampY = round(fy + rampDist * sin(rampAngle));
	placer = new ClumpPlacer(100, 0.9, 0.5, 0, rampX, rampY);
	var painter = new SmoothElevationPainter(ELEVATION_SET, elevation-6, 5);
	createArea(placer, painter, null);
	placer = new ClumpPlacer(75, 0.9, 0.5, 0, rampX, rampY);
	painter = new TerrainPainter(tGrass);
	createArea(placer, painter, null);
	
	// create the central dirt patch
	placer = new ClumpPlacer(PI*3.5*3.5, 0.3, 0.1, 0, ix, iy);
	painter = new LayeredPainter(
		[tGrassDirt75, tGrassDirt50, tGrassDirt25, tDirt],		// terrains
		[1,1,1]									// widths
	);
	createArea(placer, painter, null);
	
	// create the TC and citizens
	var civ = getCivCode(i - 1);
	var group = new SimpleGroup(	// elements (type, min/max count, min/max distance)
		[new SimpleObject("structures/"+civ+"_civil_centre", 1,1, 0,0), new SimpleObject("units/"+civ+"_support_female_citizen", 3,3, 5,5)],
		true, null, ix, iy
	);
	createObjectGroup(group, i);
	
	// create berry bushes
	var bbAngle = randFloat()*2*PI;
	var bbDist = 9;
	var bbX = round(fx + bbDist * cos(bbAngle));
	var bbY = round(fy + bbDist * sin(bbAngle));
	group = new SimpleGroup(
		[new SimpleObject(oBerryBush, 5,5, 0,2)],
		true, clBaseResource, bbX, bbY
	);
	createObjectGroup(group, 0);
	
	// create mines
	var mAngle = bbAngle;
	while(abs(mAngle - bbAngle) < PI/3)
	{
		mAngle = randFloat()*2*PI;
	}
	var mDist = 9;
	var mX = round(fx + mDist * cos(mAngle));
	var mY = round(fy + mDist * sin(mAngle));
	group = new SimpleGroup(
		[new SimpleObject(oMine, 4,4, 0,2)],
		true, clBaseResource, mX, mY
	);
	createObjectGroup(group, 0);
	
	// create starting straggler trees
	group = new SimpleGroup(
		[new SimpleObject(oTree, 3,3, 8,12)],
		true, clBaseResource, ix, iy
	);
	createObjectGroup(group, 0, avoidClasses(clBaseResource,2));
	
	// create grass tufts
	for (var j=0; j < 10; j++)
	{
		var gAngle = randFloat()*2*PI;
		var gDist = 6 + randInt(9);
		var gX = round(fx + gDist * cos(gAngle));
		var gY = round(fy + gDist * sin(gAngle));
		group = new SimpleGroup(
			[new SimpleObject(aGrassShort, 3,6, 0,1, -PI/8,PI/8)],
			false, clBaseResource, gX, gY
		);
		createObjectGroup(group, undefined);
	}
}

// create lakes
log("Creating lakes...");
placer = new ClumpPlacer(140, 0.8, 0.1, 0);
terrainPainter = new LayeredPainter(
	[tShoreBlend, tShore, tWater],		// terrains
	[1,1]							// widths
);
elevationPainter = new SmoothElevationPainter(ELEVATION_SET, -7, 3);
createAreas(
	placer,
	[terrainPainter, elevationPainter, paintClass(clWater)], 
	avoidClasses(clPlayer, 2, clWater, 20),
	round(1.3 * numPlayers)
);

// create bumps
log("Creating bumps...");
placer = new ClumpPlacer(10, 0.3, 0.06, 0);
painter = new SmoothElevationPainter(ELEVATION_MODIFY, 2, 2);
createAreas(
	placer,
	painter, 
	avoidClasses(clWater, 2, clPlayer, 0),
	mapSize*mapSize/1000
);

// create hills
log("Creating hills...");
placer = new ClumpPlacer(30, 0.2, 0.1, 0);
terrainPainter = new LayeredPainter(
	[tCliff, [tGrass,tGrass,tGrassDirt75]],		// terrains
	[3]								// widths
);
elevationPainter = new SmoothElevationPainter(ELEVATION_SET, 12, 2);
createAreas(
	placer,
	[terrainPainter, elevationPainter, paintClass(clHill)], 
	avoidClasses(clPlayer, 2, clWater, 5, clHill, 15),
	2 * numPlayers
);

// create forests
log("Creating forests...");
placer = new ClumpPlacer(32, 0.1, 0.1, 0);
painter = new LayeredPainter(
	[[tGrassForest, tGrass, pForest], [tGrassForest, pForest]],		// terrains
	[2]											// widths
	);
createAreas(
	placer,
	[painter, paintClass(clForest)], 
	avoidClasses(clPlayer, 1, clWater, 3, clForest, 10, clHill, 0),
	6 * numPlayers
);

// create dirt patches
log("Creating dirt patches...");
var sizes = [8,14,20];
for (i=0; i<sizes.length; i++)
{
	placer = new ClumpPlacer(sizes[i], 0.3, 0.06, 0);
	painter = new LayeredPainter(
		[[tGrass,tGrassDirt75],[tGrassDirt75,tGrassDirt50], [tGrassDirt50,tGrassDirt25]], 		// terrains
		[1,1]															// widths
	);
	createAreas(
		placer,
		[painter, paintClass(clDirt)],
		avoidClasses(clWater, 1, clForest, 0, clHill, 0, clDirt, 5, clPlayer, 0),
		mapSize*mapSize/4000
	);
}

// create grass patches
log("Creating grass patches...");
var sizes = [5,9,13];
for (i=0; i<sizes.length; i++)
{
	placer = new ClumpPlacer(sizes[i], 0.3, 0.06, 0);
	painter = new TerrainPainter(tGrassPatch);
	createAreas(
		placer,
		painter,
		avoidClasses(clWater, 1, clForest, 0, clHill, 0, clDirt, 5, clPlayer, 0),
		mapSize*mapSize/4000
	);
}

// create mines
log("Creating mines...");
group = new SimpleGroup(
	[new SimpleObject(oMine, 4,6, 0,2)],
	true, clRock
);
createObjectGroups(
	group, 0,
	[avoidClasses(clWater, 0, clForest, 0, clPlayer, 0, clRock, 13), 
	 new BorderTileClassConstraint(clHill, 0, 4)],
	3 * numPlayers, 100
);

// create small decorative rocks
log("Creating large decorative rocks...");
group = new SimpleGroup(
	[new SimpleObject(aRockMedium, 1,3, 0,1)],
	true
);
createObjectGroups(
	group, undefined,
	avoidClasses(clForest, 0, clPlayer, 0, clHill, 0),
	mapSize*mapSize/1000, 50
);

// create large decorative rocks
log("Creating large decorative rocks...");
group = new SimpleGroup(
	[new SimpleObject(aRockLarge, 1,2, 0,1), new SimpleObject(aRockMedium, 1,3, 0,2)],
	true
);
createObjectGroups(
	group, undefined,
	avoidClasses(clForest, 0, clPlayer, 0, clHill, 0),
	mapSize*mapSize/2000, 50
);

// create deer
log("Creating deer...");
group = new SimpleGroup(
	[new SimpleObject(oDeer, 5,7, 0,4)],
	true, clFood
);
createObjectGroups(group, 0,
	avoidClasses(clWater, 0, clForest, 0, clPlayer, 0, clHill, 0, clFood, 20),
	3 * numPlayers, 50
);

// create sheep
log("Creating sheep...");
group = new SimpleGroup(
	[new SimpleObject(oSheep, 2,3, 0,2)],
	true, clFood
);
createObjectGroups(group, 0,
	avoidClasses(clWater, 0, clForest, 0, clPlayer, 0, clHill, 0, clFood, 20),
	3 * numPlayers, 50
);

// create straggler trees
log("Creating straggler trees...");
group = new SimpleGroup(
	[new SimpleObject(oTree, 1,1, 0,0)],
	true
);
createObjectGroups(group, 0,
	avoidClasses(clWater, 1, clForest, 1, clHill, 1, clPlayer, 1),
	mapSize*mapSize/1100
);

// create small grass tufts
log("Creating small grass tufts...");
group = new SimpleGroup(
	[new SimpleObject(aGrassShort, 3,6, 0,1, -PI/8,PI/8)]
);
createObjectGroups(group, undefined,
	avoidClasses(clWater, 2, clHill, 2, clPlayer, 2, clDirt, 0),
	mapSize*mapSize/90
);

// create large grass tufts
log("Creating large grass tufts...");
group = new SimpleGroup(
	[new SimpleObject(aGrass, 20,30, 0,1.8, -PI/8,PI/8), new SimpleObject(aGrassShort, 20,30, 1.2,2.5, -PI/8,PI/8)]
);
createObjectGroups(group, undefined,
	avoidClasses(clWater, 3, clHill, 2, clPlayer, 2, clDirt, 1, clForest, 0),
	mapSize*mapSize/900
);

// create bushes
log("Creating bushes...");
group = new SimpleGroup(
	[new SimpleObject(aBushSmall, 2,4, 0,2)]
);
createObjectGroups(group, undefined,
	avoidClasses(clWater, 1, clHill, 1, clPlayer, 1, clDirt, 1),
	mapSize*mapSize/2000, 50
);

// create reeds
log("Creating reeds...");
group = new SimpleGroup(
	[new SimpleObject(aReeds, 5,10, 0,1.5, -PI/8,PI/8)]
);
createObjectGroups(group, undefined,
	[new BorderTileClassConstraint(clWater, 3, 0), new StayInTileClassConstraint(clWater, 1)],
	10 * numPlayers, 100
);

// Export map data
ExportMap();
