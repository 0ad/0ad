// constants

const SIZE = 208;
const NUM_PLAYERS = 4;


const tGrass = "grass1_a";
const tCliff = "cliff_mountain";
const tForest = "grass_forest_floor_oak|wrld_flora_oak";
const tGrassDirt75 = "grass dirt 75";
const tGrassDirt50 = "grass dirt 50";
const tGrassDirt25 = "grass dirt 25";
const tDirt = "dirt_brown_b";
const tShore = "sand";
const tShoreBlend = "grass_sand_50";
const tWater = "water_2";
const tWaterDeep = "water_3";

const oTree = "wrld_flora_oak";
const oBerryBush = "wrld_flora_berrybush";
const oSheep = "wrld_fauna_sheep";
const oDeer = "wrld_fauna_deer";
const oMine = "wrld_rock_light";
const oGrass = "foliage/grass_tufts_a.xml";
const oReeds = "foliage/reeds_a.xml";
const oDecorativeRock = "geology/rock_gray1.xml";

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
init(SIZE, tGrass, 10);

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

playerX = new Array(NUM_PLAYERS);
playerY = new Array(NUM_PLAYERS);
playerAngle = new Array(NUM_PLAYERS);

startAngle = randFloat() * 2 * PI;
for(i=0; i<NUM_PLAYERS; i++) {
	playerAngle[i] = startAngle + i*2*PI/NUM_PLAYERS;
	playerX[i] = 0.5 + 0.35*cos(playerAngle[i]);
	playerY[i] = 0.5 + 0.35*sin(playerAngle[i]);
}

for(i=0; i<NUM_PLAYERS; i++) {
	println("Creating base for player " + i + "...");
	
	// some constants
	radius = 18;
	cliffRadius = 3;
	elevation = 32;
	
	// get the x and y in tiles
	fx = fractionToTiles(playerX[i]);
	fy = fractionToTiles(playerY[i]);
	ix = round(fx);
	iy = round(fy);

	// calculate size based on the radius
	size = PI * radius * radius;
	
	// create the hill
	placer = new ClumpPlacer(size, 0.9, 0.5, 0, ix, iy);
	terrainPainter = new LayeredPainter(
		[cliffRadius+1],		// widths
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
	placer = new ClumpPlacer(80, 0.9, 0.5, 0, rampX, rampY);
	painter = [new TerrainPainter(tGrass), new SmoothElevationPainter(ELEVATION_SET, (elevation-10)/3+10, 5)];
	createArea(placer, painter, null);
	
	// create the central dirt patch
	placer = new ClumpPlacer(PI*4*4, 0.3, 0.1, 0, ix, iy);
	painter = new LayeredPainter(
		[1,1,1],												// widths
		[tGrassDirt75, tGrassDirt50, tGrassDirt25, tDirt]		// terrains
	);
	createArea(placer, painter, null);
	
	// create the TC and the villies
	group = new SimpleGroup(
		[							// elements (type, count, distance)
			new SimpleObject("hele_cc", 1,1, 0,0),
			new SimpleObject("hele_isp_b", 3,3, 5,5)
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
		true, clBaseResource,	bbX, bbY
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
		[new SimpleObject(oMine, 3,3, 0,2)],
		true, clBaseResource,	mX, mY
	);
	createObjectGroup(group, 0);
	
	// create starting straggler trees
	group = new SimpleGroup(
		[new SimpleObject(oTree, 3,3, 6,12)],
		true, null,	ix, iy
	);
	createObjectGroup(group, 0, avoidClasses(clBaseResource,1));
}

// create lakes
println("Creating lakes...");
placer = new ClumpPlacer(170, 0.6, 0.1, 0);
terrainPainter = new LayeredPainter(
	[1,1],									// widths
	[tShoreBlend, tShore, tWaterDeep]		// terrains
);
elevationPainter = new SmoothElevationPainter(ELEVATION_SET, 5, 2);
createAreas(placer, [terrainPainter, elevationPainter, paintClass(clWater)], 
	avoidClasses(clPlayer, 2, clWater, 13),
	round(1.5 * NUM_PLAYERS)
);

// create bumps
println("Creating bumps...");
placer = new ClumpPlacer(10, 0.3, 0.06, 0);
painter = new SmoothElevationPainter(ELEVATION_MODIFY, 3, 2);
createAreas(placer, painter, 
	avoidClasses(clWater, 2),
	SIZE*SIZE/150
);

// create hills
println("Creating hills...");
placer = new ClumpPlacer(30, 0.2, 0.1, 0);
terrainPainter = new LayeredPainter(
	[2],					// widths
	[tCliff, tGrass]		// terrains
);
elevationPainter = new SmoothElevationPainter(ELEVATION_SET, 25, 2);
createAreas(placer, [terrainPainter, elevationPainter, paintClass(clHill)], 
	avoidClasses(clPlayer, 2, clWater, 5, clHill, 15),
	2 * NUM_PLAYERS
);

// create forests
println("Creating forests...");
placer = new ClumpPlacer(30, 0.2, 0.06, 0);
painter = new LayeredPainter([2], [[tGrass, tForest], tForest]);
createAreas(placer, [painter, paintClass(clForest)], 
	avoidClasses(clPlayer, 1, clWater, 3, clForest, 10, clHill, 0),
	7 * NUM_PLAYERS
);

// create dirt patches
println("Creating dirt patches...");
var sizes = [20,40,60];
for(i=0; i<sizes.length; i++) {
	placer = new ClumpPlacer(sizes[i], 0.3, 0.06, 0);
	painter = new LayeredPainter([1,1], [tGrassDirt75,tGrassDirt50,tGrassDirt25]);
	createAreas(placer, [painter, paintClass(clDirt)],
		avoidClasses(clWater, 1, clForest, 0, clHill, 0, clDirt, 5, clPlayer, 0),
		SIZE*SIZE/6000
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

// create decorative rocks
println("Creating decorative rocks...");
group = new SimpleGroup([new SimpleObject(oDecorativeRock, 1,1, 0,0)], true);
createObjectGroups(group, 0,
	new BorderTileClassConstraint(clHill, 0, 2),
	5 * NUM_PLAYERS, 100
);

// create deer
println("Creating deer...");
group = new SimpleGroup([new SimpleObject(oDeer, 5,7, 0,4)], true, clFood);
createObjectGroups(group, 0,
	avoidClasses(clWater, 0, clForest, 0, clPlayer, 0, clHill, 0, clFood, 15),
	2 * NUM_PLAYERS, 50
);

// create sheep
println("Creating sheep...");
group = new SimpleGroup([new SimpleObject(oSheep, 2,3, 0,2)], true, clFood);
createObjectGroups(group, 0,
	avoidClasses(clWater, 0, clForest, 0, clPlayer, 0, clHill, 0, clFood, 15),
	3 * NUM_PLAYERS, 50
);

// create straggler trees
println("Creating straggler trees...");
group = new SimpleGroup([new SimpleObject(oTree, 1,1, 0,0)], true);
createObjectGroups(group, 0,
	avoidClasses(clWater, 1, clForest, 0, clHill, 0, clPlayer, 0),
	SIZE*SIZE/800
);

// create grass tufts
println("Creating grass tufts...");
group = new SimpleGroup([new SimpleObject(oGrass, 6,10, 0,2)]);
createObjectGroups(group, 0,
	avoidClasses(clWater, 3, clForest, 0, clHill, 0, clPlayer, 0, clDirt, 0),
	SIZE*SIZE/800
);

// create reeds
println("Creating reeds...");
group = new SimpleGroup([new SimpleObject(oReeds, 2,3, 0,2)]);
createObjectGroups(group, 0,
	new BorderTileClassConstraint(clWater, 1, 2),
	5 * NUM_PLAYERS, 50
);