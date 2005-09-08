// constants

const SIZE = 208;
const NUM_PLAYERS = 4;


const tGrass = ["grass1_a", "grass2"];
const tCliff = ["cliff2", "cliff2_moss"];
const tForest = "grass_forest_floor_oak|flora/wrld_flora_oak.xml";
const tGrassDirt75 = "grass dirt 75";
const tGrassDirt50 = "grass dirt 50";
const tGrassDirt25 = "grass dirt 25";
const tDirt = "dirt_brown_a";
const tShore = "dirt_brown_rocks";
const tWater = "water_2";
const tWaterDeep = "water_3";

const oGrass = "foliage/grass_tufts_a.xml";
const oTree = "flora/wrld_flora_oak.xml";

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
			new SimpleObject("hele_cc", 1, 0),
			new SimpleObject("hele_isp_b", 3, 5)
		],
		true,					// avoid self
		null,					// tile class
		ix, iy					// position
	);
	createObjectGroup(group, i);
	
	// maybe do other stuff, like sheep and villies?
}

// create lakes
println("Creating lakes...");
placer = new ClumpPlacer(170, 0.6, 0.1, 0);
terrainPainter = new LayeredPainter(
	[1,1],									// widths
	[tGrassDirt50, tShore, tWaterDeep]		// terrains
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
	SIZE*SIZE/200
);

// create forests
println("Creating forests...");
placer = new ClumpPlacer(30, 0.2, 0.06, 0);
painter = new LayeredPainter([2], [[tGrass, tForest], tForest]);
createAreas(placer, [painter, paintClass(clForest)], 
	avoidClasses(clPlayer, 2, clWater, 5, clForest, 5),
	8 * NUM_PLAYERS
);

// create forests
println("Creating hills...");
placer = new ClumpPlacer(60, 0.2, 0.1, 0);
terrainPainter = new LayeredPainter(
	[3],					// widths
	[tCliff, tGrass]		// terrains
);
elevationPainter = new SmoothElevationPainter(ELEVATION_SET, 32, 2);
createAreas(placer, [terrainPainter, elevationPainter, paintClass(clHill)], 
	avoidClasses(clPlayer, 2, clWater, 5, clForest, 5),
	3 * NUM_PLAYERS
);

// create dirt patches
println("Creating dirt patches...");
var sizes = [25,45,70];
for(i=0; i<sizes.length; i++) {
	placer = new ClumpPlacer(sizes[i], 0.3, 0.06, 0);
	painter = new LayeredPainter([1,1], [tGrassDirt75,tGrassDirt50,tGrassDirt25]);
	createAreas(placer, [painter, paintClass(clDirt)],
		avoidClasses(clWater, 1, clForest, 0, clHill, 0, clDirt, 5, clPlayer, 0),
		SIZE*SIZE/6000
	);
}

// create straggler trees
println("Creating straggler trees...");
group = new SimpleGroup([new SimpleObject(oTree, 1, 0)]);
createObjectGroups(group, 0,
	avoidClasses(clWater, 1, clForest, 0, clHill, 0, clPlayer, 0),
	SIZE*SIZE/7000
);

// create grass tufts
println("Creating grass tufts...");
group = new SimpleGroup([
	new SimpleObject(oGrass, 2, 0.4),
	new SimpleObject(oGrass, 2, 1.1),
	new SimpleObject(oGrass, 3, 2.0)
]);
createObjectGroups(group, 0,
	avoidClasses(clWater, 1, clForest, 0, clHill, 0, clPlayer, 0),
	SIZE*SIZE/2500
);
