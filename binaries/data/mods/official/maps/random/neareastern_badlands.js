// constants

const SIZE = 208;
const NUM_PLAYERS = 4;

const tSand = "desert_rough";
const tDunes = "desert_wave";
const tFineSand = "desert_sahara";
const tCliff = "cliff_desert";
const tForest = "grass_sand_75|flora/trees/palm_b.xml";
const tGrassSand75 = "grass_sand_75";
const tGrassSand50 = "grass_sand_50";
const tGrassSand25 = "grass_sand_25_2";
const tDirt = "dirt_hard";
const tDirtCracks = "dirt_cracks";
const tShore = "sand";
const tWater = "water_2";
const tWaterDeep = "water_3";

const oTree = "flora/trees/palm_b.xml";
const oBerryBush = "flora_bush_berry";
const oBush = "props/flora/bush_dry_a.xml";
const oSheep = "fauna_sheep";
const oDeer = "fauna_deer";
const oMine = "geology_stone_light";
const oDecorativeRock = "geology/gray1.xml";

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
init(SIZE, tSand, 10);

// create tile classes

clPlayer = createTileClass();
clHill1 = createTileClass();
clHill2 = createTileClass();
clHill3 = createTileClass();
clForest = createTileClass();
clWater = createTileClass();
clPatch = createTileClass();
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
	playerX[i] = 0.5 + 0.39*cos(playerAngle[i]);
	playerY[i] = 0.5 + 0.39*sin(playerAngle[i]);
}

for(i=0; i<NUM_PLAYERS; i++) {
	println("Creating base for player " + i + "...");
	
	// some constants
	radius = 20;
	cliffRadius = 2;
	elevation = 30;
	
	// get the x and y in tiles
	fx = fractionToTiles(playerX[i]);
	fy = fractionToTiles(playerY[i]);
	ix = round(fx);
	iy = round(fy);

	// calculate size based on the radius
	size = PI * radius * radius;
	
	// create the hill
	placer = new ClumpPlacer(size, 0.9, 0.5, 0, ix, iy);
	createArea(placer, paintClass(clPlayer), null);
	
	// create the central road patch
	placer = new ClumpPlacer(PI*2*2, 0.6, 0.3, 0.5, ix, iy);
	painter = new TerrainPainter(tDirt);
	createArea(placer, painter, null);
	
	// create the TC and the villies
	group = new SimpleGroup(
		[							// elements (type, count, distance)
			new SimpleObject("hele_civil_centre", 1,1, 0,0),
			new SimpleObject("hele_infantry_spearman_b", 3,3, 5,5)
		],
		true, null,	ix, iy
	);
	createObjectGroup(group, i);
	
	// create berry bushes
	bbAngle = randFloat()*2*PI;
	bbDist = 10;
	bbX = round(fx + bbDist * cos(bbAngle));
	bbY = round(fy + bbDist * sin(bbAngle));
	group = new SimpleGroup(
		[new SimpleObject(oSheep, 5,5, 0,2)],
		true, clBaseResource,	bbX, bbY
	);
	createObjectGroup(group, 0);
	
	// create mines
	mAngle = bbAngle;
	while(abs(mAngle - bbAngle) < PI/3) {
		mAngle = randFloat()*2*PI;
	}
	mDist = 12;
	mX = round(fx + mDist * cos(mAngle));
	mY = round(fy + mDist * sin(mAngle));
	group = new SimpleGroup(
		[new SimpleObject(oMine, 3,3, 0,2)],
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

// create patches
println("Creating sand patches...");
placer = new ClumpPlacer(30, 0.2, 0.1, 0);
painter = new LayeredPainter([1], [[tSand, tFineSand], tFineSand]);
createAreas(placer, [painter, paintClass(clPatch)], 
	avoidClasses(clPatch, 5),
	(SIZE*SIZE)/600
);

println("Creating dirt patches...");
placer = new ClumpPlacer(10, 0.2, 0.1, 0);
painter = new TerrainPainter([tSand, tDirt]);
createAreas(placer, [painter, paintClass(clPatch)], 
	avoidClasses(clPatch, 5),
	(SIZE*SIZE)/600
);

// create the oasis
println("Creating water...");
placer = new ClumpPlacer(1200, 0.6, 0.1, 0, SIZE/2, SIZE/2);
painter = new LayeredPainter([6,1], [[tSand, tForest], tShore, tWaterDeep]);
elevationPainter = new SmoothElevationPainter(ELEVATION_MODIFY, -10, 5);
createArea(placer, [painter, elevationPainter, paintClass(clForest)], null);

// create hills
println("Creating level 1 hills...");
placer = new ClumpPlacer(150, 0.25, 0.1, 0.3);
terrainPainter = new LayeredPainter(
	[1],				// widths
	[tCliff, tSand]		// terrains
);
elevationPainter = new SmoothElevationPainter(ELEVATION_MODIFY, 16, 1);
createAreas(placer, [terrainPainter, elevationPainter, paintClass(clHill1)], 
	avoidClasses(clForest, 2, clPlayer, 0, clHill1, 16),
	(SIZE*SIZE)/3800, 100
);

println("Creating small level 1 hills...");
placer = new ClumpPlacer(60, 0.25, 0.1, 0.3);
terrainPainter = new LayeredPainter(
	[1],				// widths
	[tCliff, tSand]		// terrains
);
elevationPainter = new SmoothElevationPainter(ELEVATION_MODIFY, 16, 1);
createAreas(placer, [terrainPainter, elevationPainter, paintClass(clHill1)], 
	avoidClasses(clForest, 2, clPlayer, 0, clHill1, 3),
	(SIZE*SIZE)/2800, 100
);

println("Creating level 2 hills...");
placer = new ClumpPlacer(60, 0.2, 0.1, 0.9);
terrainPainter = new LayeredPainter(
	[1],				// widths
	[tCliff, tSand]		// terrains
);
elevationPainter = new SmoothElevationPainter(ELEVATION_MODIFY, 16, 1);
createAreas(placer, [terrainPainter, elevationPainter, paintClass(clHill2)], 
	[avoidClasses(clHill2, 1), new StayInTileClassConstraint(clHill1, 0)],
	(SIZE*SIZE)/2800, 200
);

println("Creating level 3 hills...");
placer = new ClumpPlacer(25, 0.2, 0.1, 0.9);
terrainPainter = new LayeredPainter(
	[1],				// widths
	[tCliff, tSand]		// terrains
);
elevationPainter = new SmoothElevationPainter(ELEVATION_MODIFY, 16, 1);
createAreas(placer, [terrainPainter, elevationPainter, paintClass(clHill3)], 
	[avoidClasses(clHill3, 1), new StayInTileClassConstraint(clHill2, 0)],
	(SIZE*SIZE)/9000, 300
);

// create forests
println("Creating forests...");
placer = new ClumpPlacer(25, 0.15, 0.1, 0.3);
painter = new TerrainPainter([tSand, tForest]);
createAreas(placer, [painter, paintClass(clForest)], 
	avoidClasses(clWater, 0, clPlayer, 1, clForest, 20, clHill1, 0),
	(SIZE*SIZE)/4000, 50
);

// create mines
println("Creating mines...");
group = new SimpleGroup([new SimpleObject(oMine, 4,6, 0,2)], true, clRock);
createObjectGroups(group, 0,
	[avoidClasses(clWater, 2, clForest, 2, clPlayer, 0, clRock, 13), 
	 new BorderTileClassConstraint(clHill1, 0, 4)],
	(SIZE*SIZE)/4000, 100
);

// create decorative rocks for hills
println("Creating decorative rocks...");
group = new SimpleGroup([new SimpleObject(oDecorativeRock, 1,1, 0,0)], true);
createObjectGroups(group, 0,
	new BorderTileClassConstraint(clHill1, 0, 3),
	(SIZE*SIZE)/2000, 100
);

// create deer
println("Creating deer...");
group = new SimpleGroup([new SimpleObject(oDeer, 5,7, 0,4)], true, clFood);
createObjectGroups(group, 0,
	avoidClasses(clWater, 0, clForest, 0, clPlayer, 0, clHill1, 0, clFood, 25),
	(SIZE*SIZE)/5000, 50
);

// create sheep
println("Creating sheep...");
group = new SimpleGroup([new SimpleObject(oSheep, 1,3, 0,2)], true, clFood);
createObjectGroups(group, 0,
	avoidClasses(clWater, 0, clForest, 0, clPlayer, 0, clHill1, 0, clFood, 15),
	(SIZE*SIZE)/5000, 50
);

// create straggler trees
println("Creating straggler trees...");
group = new SimpleGroup([new SimpleObject(oTree, 1,1, 0,0)], true);
createObjectGroups(group, 0,
	avoidClasses(clWater, 0, clForest, 0, clHill1, 0, clPlayer, 0),
	SIZE*SIZE/1500
);


// create bushes
println("Creating bushes...");
group = new SimpleGroup([new SimpleObject(oBush, 2,3, 0,2)]);
createObjectGroups(group, 0,
	avoidClasses(clWater, 3, clHill1, 0, clPlayer, 0, clForest, 0),
	SIZE*SIZE/1000
);

// create bushes
println("Creating more decorative rocks...");
group = new SimpleGroup([new SimpleObject(oDecorativeRock, 1,2, 0,2)]);
createObjectGroups(group, 0,
	avoidClasses(clWater, 3, clHill1, 0, clPlayer, 0, clForest, 0),
	SIZE*SIZE/1000
);