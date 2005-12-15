// constants

const SIZE = 208;
const NUM_PLAYERS = 4;


const tGrass = ["grass1_a", "grass_highlands_a", "grass1_a"];
const tGrassForest = "grass_forest_floor_oak";
const tCliff = ["cliff_greekb", "cliff_greekb", "cliff_greekb_moss"];
const tForest = "forrestfloor|gaia_flora_oak";
const tGrassDirt75 = "sand_grass_75";
const tGrassDirt50 = "sand_grass_50";
const tGrassDirt25 = "sand_grass_25";
const tDirt = "sand_path";
const tGrassPatch = "grass1_dense";
const tShore = "sand_path";
const tShoreBlend = "sand_grass_50";
const tWater = "sand_path";

const oTree = "gaia_flora_oak";
const oTreeLarge = "flora/trees/oak_large.xml";
const oBerryBush = "gaia_flora_bush_berry";
const oSheep = "gaia_fauna_sheep";
const oDeer = "gaia_fauna_deer";
const oMine = "gaia_rock_light";
const oGrass = "props/flora/grass_field_lush_tall.xml";
const oGrassShort = "props/flora/grass_field_lush_short.xml";
const oReeds = "props/flora/reeds_pond_dry.xml";
const oRockLarge = "geology/stone_granite_large.xml";
const oRockMedium = "geology/stone_granite_med.xml";
const oBushMedium = "props/flora/bush_tempe_me.xml";
const oBushSmall = "props/flora/bush_tempe_sm.xml";

// some utility functions to save typing

function paintClass(cl) {
	return new TileClassPainter(cl);
}

function avoidClasses(/*class1, dist1, class2, dist2, etc*/) {
	var ar = new Array(arguments.length/2);
	for(var i=0; i<arguments.length/2; i++) {
		ar[i] = new AvoidTileClassConstraint(arguments[2*i], arguments[2*i+1]);
	}
	return ar;
}

// initialize map

println("Initializing map...");
init(SIZE, tGrass, 8);

// create tile classes

clPlayer = createTileClass();
clHill = createTileClass();
clForest = createTileClass();
clWater = createTileClass();
clDirt = createTileClass();
clRock = createTileClass();
clFood = createTileClass();
clBaseResource = createTileClass();

// place players

playerX = new Array(NUM_PLAYERS+1);
playerY = new Array(NUM_PLAYERS+1);
playerAngle = new Array(NUM_PLAYERS+1);

startAngle = randFloat() * 2 * PI;
for(i=1; i<=NUM_PLAYERS; i++) {
	playerAngle[i] = startAngle + i*2*PI/NUM_PLAYERS;
	playerX[i] = 0.5 + 0.35*cos(playerAngle[i]);
	playerY[i] = 0.5 + 0.35*sin(playerAngle[i]);
}

for(i=1; i<=NUM_PLAYERS; i++) {
	println("Creating base for player " + i + "...");
	
	// some constants
	radius = 17;
	cliffRadius = 2;
	elevation = 20;
	
	// get the x and y in tiles
	fx = fractionToTiles(playerX[i]);
	fy = fractionToTiles(playerY[i]);
	ix = round(fx);
	iy = round(fy);

	// calculate size based on the radius
	size = PI * radius * radius;
	
	// create the hill
	placer = new ClumpPlacer(size, 0.95, 0.6, 0, ix, iy);
	terrainPainter = new LayeredPainter(
		[cliffRadius],		// widths
		[tCliff, tGrass]		// terrains
	);
	elevationPainter = new SmoothElevationPainter(
		ELEVATION_SET,			// type
		elevation,				// elevation
		cliffRadius				// blend radius
	);
	createArea(placer, [terrainPainter, elevationPainter, paintClass(clPlayer)], null);
	
	// create the ramp
	rampAngle = playerAngle[i] + PI + (2*randFloat()-1)*PI/8;
	rampDist = radius - 1;
	rampX = round(fx + rampDist * cos(rampAngle));
	rampY = round(fy + rampDist * sin(rampAngle));
	placer = new ClumpPlacer(100, 0.9, 0.5, 0, rampX, rampY);
	painter = new SmoothElevationPainter(ELEVATION_SET, elevation-6, 5);
	createArea(placer, painter, null);
	placer = new ClumpPlacer(75, 0.9, 0.5, 0, rampX, rampY);
	painter = new TerrainPainter(tGrass);
	createArea(placer, painter, null);
	
	// create the central dirt patch
	placer = new ClumpPlacer(PI*3.5*3.5, 0.3, 0.1, 0, ix, iy);
	painter = new LayeredPainter(
		[1,1,1],												// widths
		[tGrassDirt75, tGrassDirt50, tGrassDirt25, tDirt]		// terrains
	);
	createArea(placer, painter, null);
	
	// create the TC and the "villies"
	group = new SimpleGroup(
		[							// elements (type, count, distance)
			new SimpleObject("hele_civil_centre", 1,1, 0,0),
			new SimpleObject("hele_infantry_javelinist_b", 3,3, 5,5)
		],
		true, null,	ix, iy
	);
	createObjectGroup(group, i);
	
	// create berry bushes
	bbAngle = randFloat()*2*PI;
	bbDist = 9;
	bbX = round(fx + bbDist * cos(bbAngle));
	bbY = round(fy + bbDist * sin(bbAngle));
	group = new SimpleGroup(
		[new SimpleObject(oBerryBush, 5,5, 0,2)],
		true, clBaseResource, bbX, bbY
	);
	createObjectGroup(group, 0);
	
	// create mines
	mAngle = bbAngle;
	while(abs(mAngle - bbAngle) < PI/3) {
		mAngle = randFloat()*2*PI;
	}
	mDist = 9;
	mX = round(fx + mDist * cos(mAngle));
	mY = round(fy + mDist * sin(mAngle));
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
	for(j=0; j<10; j++) {
		gAngle = randFloat()*2*PI;
		gDist = 6 + randInt(9);
		gX = round(fx + gDist * cos(gAngle));
		gY = round(fy + gDist * sin(gAngle));
		group = new SimpleGroup([new SimpleObject(oGrassShort, 3,6, 0,1, -PI/8,PI/8)],
			false, clBaseResource, gX, gY);
		createObjectGroup(group, 0);
	}
}

// create lakes
println("Creating lakes...");
placer = new ClumpPlacer(140, 0.8, 0.1, 0);
terrainPainter = new LayeredPainter(
	[1,1],									// widths
	[tShoreBlend, tShore, tWater]		// terrains
);
elevationPainter = new SmoothElevationPainter(ELEVATION_SET, -2, 3);
createAreas(placer, [terrainPainter, elevationPainter, paintClass(clWater)], 
	avoidClasses(clPlayer, 2, clWater, 20),
	round(1.3 * NUM_PLAYERS)
);

// create bumps
println("Creating bumps...");
placer = new ClumpPlacer(10, 0.3, 0.06, 0);
painter = new SmoothElevationPainter(ELEVATION_MODIFY, 2, 2);
createAreas(placer, painter, 
	avoidClasses(clWater, 2, clPlayer, 0),
	SIZE*SIZE/100
);

// create hills
println("Creating hills...");
placer = new ClumpPlacer(30, 0.2, 0.1, 0);
terrainPainter = new LayeredPainter(
	[3],					// widths
	[tCliff, [tGrass,tGrass,tGrassDirt75]]		// terrains
);
elevationPainter = new SmoothElevationPainter(ELEVATION_SET, 17, 2);
createAreas(placer, [terrainPainter, elevationPainter, paintClass(clHill)], 
	avoidClasses(clPlayer, 2, clWater, 5, clHill, 15),
	2 * NUM_PLAYERS
);

// create forests
println("Creating forests...");
placer = new ClumpPlacer(32, 0.1, 0.1, 0);
painter = new LayeredPainter([2], [[tGrassForest, tGrass, tForest], 
	[tGrassForest, tForest]]);
createAreas(placer, [painter, paintClass(clForest)], 
	avoidClasses(clPlayer, 1, clWater, 3, clForest, 10, clHill, 0),
	6 * NUM_PLAYERS
);

// create dirt patches
println("Creating dirt patches...");
var sizes = [8,14,20];
for(i=0; i<sizes.length; i++) {
	placer = new ClumpPlacer(sizes[i], 0.3, 0.06, 0);
	painter = new LayeredPainter([1,1], [
		[tGrass,tGrassDirt75],[tGrassDirt75,tGrassDirt50],
		[tGrassDirt50,tGrassDirt25]]);
	createAreas(placer, [painter, paintClass(clDirt)],
		avoidClasses(clWater, 1, clForest, 0, clHill, 0, clDirt, 5, clPlayer, 0),
		SIZE*SIZE/4000
	);
}

// create grass patches
println("Creating grass patches...");
var sizes = [5,9,13];
for(i=0; i<sizes.length; i++) {
	placer = new ClumpPlacer(sizes[i], 0.3, 0.06, 0);
	painter = new TerrainPainter(tGrassPatch);
	createAreas(placer, painter,
		avoidClasses(clWater, 1, clForest, 0, clHill, 0, clDirt, 5, clPlayer, 0),
		SIZE*SIZE/4000
	);
}

// create mines
println("Creating mines...");
group = new SimpleGroup([new SimpleObject(oMine, 4,6, 0,2)], true, clRock);
createObjectGroups(group, 0,
	[avoidClasses(clWater, 0, clForest, 0, clPlayer, 0, clRock, 13), 
	 new BorderTileClassConstraint(clHill, 0, 4)],
	3 * NUM_PLAYERS, 100
);

// create small decorative rocks
println("Creating large decorative rocks...");
group = new SimpleGroup([new SimpleObject(oRockMedium, 1,3, 0,1)], true);
createObjectGroups(group, 0,
	avoidClasses(clForest, 0, clPlayer, 0, clHill, 0),
	SIZE*SIZE/1000, 50
);

// create large decorative rocks
println("Creating large decorative rocks...");
group = new SimpleGroup([new SimpleObject(oRockLarge, 1,2, 0,1),
	new SimpleObject(oRockMedium, 1,3, 0,2)], true);
createObjectGroups(group, 0,
	avoidClasses(clForest, 0, clPlayer, 0, clHill, 0),
	SIZE*SIZE/2000, 50
);

// create deer
println("Creating deer...");
group = new SimpleGroup([new SimpleObject(oDeer, 5,7, 0,4)], true, clFood);
createObjectGroups(group, 0,
	avoidClasses(clWater, 0, clForest, 0, clPlayer, 0, clHill, 0, clFood, 20),
	3 * NUM_PLAYERS, 50
);

// create sheep
println("Creating sheep...");
group = new SimpleGroup([new SimpleObject(oSheep, 2,3, 0,2)], true, clFood);
createObjectGroups(group, 0,
	avoidClasses(clWater, 0, clForest, 0, clPlayer, 0, clHill, 0, clFood, 20),
	3 * NUM_PLAYERS, 50
);

// create straggler trees
println("Creating straggler trees...");
group = new SimpleGroup([new SimpleObject(oTree, 1,1, 0,0)], true);
createObjectGroups(group, 0,
	avoidClasses(clWater, 1, clForest, 1, clHill, 1, clPlayer, 1),
	SIZE*SIZE/1100
);

// create small grass tufts
println("Creating small grass tufts...");
group = new SimpleGroup([new SimpleObject(oGrassShort, 3,6, 0,1, -PI/8,PI/8)]);
createObjectGroups(group, 0,
	avoidClasses(clWater, 2, clHill, 2, clPlayer, 2, clDirt, 0),
	SIZE*SIZE/90
);

// create large grass tufts
println("Creating large grass tufts...");
group = new SimpleGroup([new SimpleObject(oGrass, 20,30, 0,1.8, -PI/8,PI/8),
	new SimpleObject(oGrassShort, 20,30, 1.2,2.5, -PI/8,PI/8)]);
createObjectGroups(group, 0,
	avoidClasses(clWater, 3, clHill, 2, clPlayer, 2, clDirt, 1, clForest, 0),
	SIZE*SIZE/900
);

// create bushes
println("Creating bushes...");
group = new SimpleGroup([new SimpleObject(oBushSmall, 2,4, 0,2)]);
createObjectGroups(group, 0,
	avoidClasses(clWater, 1, clHill, 1, clPlayer, 1, clDirt, 1),
	SIZE*SIZE/2000, 50
);

// create reeds
println("Creating reeds...");
group = new SimpleGroup([new SimpleObject(oReeds, 5,10, 0,1.5, -PI/8,PI/8)]);
createObjectGroups(group, 0,
	[new BorderTileClassConstraint(clWater, 3, 0), new StayInTileClassConstraint(clWater, 1)],
	10 * NUM_PLAYERS, 100
);

