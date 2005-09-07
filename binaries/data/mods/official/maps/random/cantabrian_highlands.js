const SIZE = 208;
const NUM_PLAYERS = 4;

// some utility stuff

const PI = Math.PI;

function fractionToTiles(f) {
	return SIZE * f;
}

function tilesToFraction(t) {
	return t / SIZE;
}

function fractionToSize(f) {
	return SIZE * SIZE * f;
}

function sizeToFraction(s) {
	return s / SIZE / SIZE;
}

function cos(x) {
	return Math.cos(x);
}

function sin(x) {
	return Math.sin(x);
}

function round(x) {
	return Math.round(x);
}

// environment constants

const oTree = "flora/wrld_flora_deci_1.xml";
const oGrass = "foliage/grass_tufts_a.xml";
const oBush = "foliage/bush_highlands.xml";

const tGrass = ["grass1_a", "grass2"];
const tCliff = ["cliff2", "cliff2_moss"];
const tForestFloor = "grass_forest_floor_oak";
const tForest = tForestFloor + "|" + oTree;
const tGrassDirt75 = "grass dirt 75";
const tGrassDirt50 = "grass dirt 50";
const tGrassDirt25 = "grass dirt 25";
const tDirt = "dirt_brown_a";
const tShore = "dirt_brown_rocks";
const tWater = "water_2";
const tWaterDeep = "water_3";

// initialize map

println("Initializing map...");
init(SIZE, tGrass, 0);

// tile classes

clPlayer = createTileClass();
clHill = createTileClass();
clForest = createTileClass();
clWater = createTileClass();
clDirt = createTileClass();

// player placement

playerX = new Array(NUM_PLAYERS);
playerY = new Array(NUM_PLAYERS);
playerAngle = new Array(NUM_PLAYERS);

startAngle = randFloat() * 2 * PI;
for(i=0; i<NUM_PLAYERS; i++) {
	playerAngle[i] = startAngle + i*2*PI/NUM_PLAYERS;
	playerX[i] = 0.5 + 0.35*cos(playerAngle[i]);
	playerY[i] = 0.5 + 0.35*sin(playerAngle[i]);
	println("Player " + i + " is at (" + playerX[i] + ", " + playerY[i] + ")");
}

// player areas

for(i=0; i<NUM_PLAYERS; i++) {
	println("Creating base for player " + i + "...");
	
	// some constants
	radius = 18;
	cliffRadius = 3;
	elevation = 14;
	
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
	createArea(placer, [terrainPainter, elevationPainter, new TileClassPainter(clPlayer)], null);
	
	// create the ramp
	rampAngle = playerAngle[i] + PI + (2*randFloat()-1)*PI/8;
	rampDist = radius - 1;
	rampX = round(fx + rampDist * cos(rampAngle));
	rampY = round(fy + rampDist * sin(rampAngle));
	placer = new ClumpPlacer(80, 0.9, 0.5, 0, rampX, rampY);
	painter = [new TerrainPainter(tGrass), new SmoothElevationPainter(ELEVATION_SET, elevation/3, 5)];
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
			new SimpleObject("hele_cc", 1, 0),
			new SimpleObject("hele_iar_b", 3, 5)
		],
		null,					// tile class
		ix, iy					// position
	);
	createObjectGroup(group, null);
	
	// maybe do other stuff, like sheep and villies?
}

// create lakes
println("Creating lakes...");
placer = new ClumpPlacer(170, 0.5, 0.1, 0);
painter = new LayeredPainter(
	[1,1,3],										// widths
	[tGrassDirt50, tShore, tWater, tWaterDeep]		// terrains
);
createAreas(placer, [painter, new TileClassPainter(clWater)], 
	[
		new AvoidTileClassConstraint(clPlayer, 2), 
		new AvoidTileClassConstraint(clWater, 13)
	],
	round(1.5 * NUM_PLAYERS)
);

// create bumps
println("Creating bumps...");
placer = new ClumpPlacer(10, 0.3, 0.06, 0);
painter = new SmoothElevationPainter(ELEVATION_MODIFY, 2.5, 2);
createAreas(placer, painter, 
	new AvoidTileClassConstraint(clWater, 2),
	SIZE*SIZE/300
);

// create forests
println("Creating forests...");
placer = new ClumpPlacer(30, 0.2, 0.06, 0);
painter = new LayeredPainter([2], [[tGrass, tForest], tForest]);
createAreas(placer, [painter, new TileClassPainter(clForest)], 
	[
		new AvoidTileClassConstraint(clPlayer, 2), 
		new AvoidTileClassConstraint(clWater, 5),
		new AvoidTileClassConstraint(clForest, 5),
	],
	8 * NUM_PLAYERS
);

// create forests
println("Creating hills...");
placer = new ClumpPlacer(60, 0.4, 0.1, 0);
terrainPainter = new LayeredPainter(
	[3],					// widths
	[tCliff, tGrass]		// terrains
);
elevationPainter = new SmoothElevationPainter(
	ELEVATION_SET,	// type
	9,				// elevation
	2				// blend radius
);
createAreas(placer, [terrainPainter, elevationPainter, new TileClassPainter(clHill)], 
	[
		new AvoidTileClassConstraint(clPlayer, 2), 
		new AvoidTileClassConstraint(clWater, 5),
		new AvoidTileClassConstraint(clForest, 5),
	],
	3 * NUM_PLAYERS
);

// create dirt patches
println("Creating dirt patches...");
var sizes = [25,45,70];
for(i=0; i<sizes.length; i++) {
	placer = new ClumpPlacer(sizes[i], 0.3, 0.06, 0);
	painter = new LayeredPainter([1,1], [tGrassDirt75,tGrassDirt50,tGrassDirt25]);
	createAreas(placer, [painter, new TileClassPainter(clDirt)],
		[
			new AvoidTileClassConstraint(clWater, 1),
			new AvoidTileClassConstraint(clForest, 0),
			new AvoidTileClassConstraint(clHill, 0),
			new AvoidTileClassConstraint(clDirt, 5),
			new AvoidTileClassConstraint(clPlayer, 0)
		],
		SIZE*SIZE/6000
	);
}

// create straggler trees
println("Creating straggler trees...");
group = new SimpleGroup([new SimpleObject(oTree, 1, 0)], null);
createObjectGroups(group, 
	[
		new AvoidTileClassConstraint(clForest, 1),
		new AvoidTileClassConstraint(clWater, 1),
		new AvoidTileClassConstraint(clHill, 0),
		new AvoidTileClassConstraint(clPlayer, 1)
	],
	SIZE*SIZE/7000
);

// create grass tufts
println("Creating grass tufts...");
group = new SimpleGroup(
	[
		new SimpleObject(oGrass, 2, 0.4),
		new SimpleObject(oGrass, 2, 1.1),
		new SimpleObject(oGrass, 3, 2.0),
	],
	null
);
createObjectGroups(group, 
	[
		new AvoidTileClassConstraint(clForest, 1),
		new AvoidTileClassConstraint(clWater, 1),
		new AvoidTileClassConstraint(clHill, 0),
		new AvoidTileClassConstraint(clPlayer, 1)
	],
	SIZE*SIZE/2500
);