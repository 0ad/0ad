RMS.LoadLibrary("rmgen");

TILE_CENTERED_HEIGHT_MAP = true;

//set up the random terrain
var random_terrain = randInt(1,2);

//late spring
if (random_terrain == 1)
{
	var tPrimary = ["alpine_dirt_grass_50"];
	var tForestFloor = "alpine_forrestfloor";
	var tCliff = ["alpine_cliff_a", "alpine_cliff_b", "alpine_cliff_c"];
	var tSecondary = "alpine_grass_rocky";
	var tHalfSnow = ["alpine_grass_snow_50", "alpine_dirt_snow"];
	var tSnowLimited = ["alpine_snow_rocky"];
	var tDirt = "alpine_dirt";
	var tRoad = "new_alpine_citytile";
	var tRoadWild = "new_alpine_citytile";
	var tShore = "alpine_shore_rocks_grass_50";
	var tWater = "alpine_shore_rocks";

	// gaia entities
	var oPine = "gaia/flora_tree_pine";
	var oBerryBush = "gaia/flora_bush_berry";
	var oChicken = "gaia/fauna_chicken";
	var oDeer = "gaia/fauna_deer";
	var oRabbit = "gaia/fauna_rabbit";
	var oStoneLarge = "gaia/geology_stonemine_alpine_quarry";
	var oStoneSmall = "gaia/geology_stone_alpine_a";
	var oMetalLarge = "gaia/geology_metal_alpine_slabs";
	
	// decorative props
	var aGrass = "actor|props/flora/grass_soft_small_tall.xml";
	var aGrassShort = "actor|props/flora/grass_soft_large.xml";
	var aRockLarge = "actor|geology/stone_granite_med.xml";
	var aRockMedium = "actor|geology/stone_granite_med.xml";
	var aBushMedium = "actor|props/flora/bush_medit_me.xml";
	var aBushSmall = "actor|props/flora/bush_medit_sm.xml";
}
else
//winter
{
	var tPrimary = ["alpine_snow_a", "alpine_snow_b"];
	var tForestFloor = "alpine_forrestfloor_snow";
	var tCliff = ["alpine_cliff_snow"];
	var tSecondary = "alpine_grass_snow_50";
	var tHalfSnow = ["alpine_grass_snow_50", "alpine_dirt_snow"];
	var tSnowLimited = ["alpine_snow_a", "alpine_snow_b"];
	var tDirt = "alpine_dirt";
	var tRoad = "new_alpine_citytile";
	var tRoadWild = "new_alpine_citytile";
	var tShore = "alpine_shore_rocks_icy";
	var tWater = "alpine_shore_rocks";

	// gaia entities
	var oPine = "gaia/flora_tree_pine_w";
	var oBerryBush = "gaia/flora_bush_berry";
	var oChicken = "gaia/fauna_chicken";
	var oDeer = "gaia/fauna_deer";
	var oRabbit = "gaia/fauna_rabbit";
	var oStoneLarge = "gaia/geology_stonemine_alpine_quarry";
	var oStoneSmall = "gaia/geology_stone_alpine_a";
	var oMetalLarge = "gaia/geology_metal_alpine_slabs";
	
	// decorative props
	var aGrass = "actor|props/flora/grass_soft_dry_small_tall.xml";
	var aGrassShort = "actor|props/flora/grass_soft_dry_large.xml";
	var aRockLarge = "actor|geology/stone_granite_med.xml";
	var aRockMedium = "actor|geology/stone_granite_med.xml";
	var aBushMedium = "actor|props/flora/bush_medit_me_dry.xml";
	var aBushSmall = "actor|props/flora/bush_medit_sm_dry.xml";
}

//other constants
const pForest = [tForestFloor + TERRAIN_SEPARATOR + oPine, tForestFloor];
const BUILDING_ANGlE = -PI/4;

// initialize map
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
var clDirt = createTileClass();
var clRock = createTileClass();
var clMetal = createTileClass();
var clFood = createTileClass();
var clBaseResource = createTileClass();
var clSettlement = createTileClass();

//cover the ground with the primary terrain chosen in the beginning
for (var ix = 0; ix < mapSize; ix++)
{
	for (var iz = 0; iz < mapSize; iz++)
	{
		var x = ix / (mapSize + 1.0);
		var z = iz / (mapSize + 1.0);
			placeTerrain(ix, iz, tPrimary);
	}
}

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
	
	// some constants
	var radius = scaleByMapSize(15,25);
	var cliffRadius = 2;
	var elevation = 20;
	
	// get the x and z in tiles
	var fx = fractionToTiles(playerX[i]);
	var fz = fractionToTiles(playerZ[i]);
	ix = round(fx);
	iz = round(fz);
	addToClass(ix, iz, clPlayer);
	addToClass(ix+5, iz, clPlayer);
	addToClass(ix, iz+5, clPlayer);
	addToClass(ix-5, iz, clPlayer);
	addToClass(ix, iz-5, clPlayer);
	
	// create the city patch
	var cityRadius = radius/3;
	placer = new ClumpPlacer(PI*cityRadius*cityRadius, 0.6, 0.3, 10, ix, iz);
	var painter = new LayeredPainter([tRoadWild, tRoad], [1]);
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
	var mDist = 12;
	var mX = round(fx + mDist * cos(mAngle));
	var mZ = round(fz + mDist * sin(mAngle));
	group = new SimpleGroup(
		[new SimpleObject(oMetalLarge, 1,1, 0,0)],
		true, clBaseResource, mX, mZ
	);
	createObjectGroup(group, 0);
	
	// create stone mines
	mAngle += randFloat(PI/8, PI/4);
	mX = round(fx + mDist * cos(mAngle));
	mZ = round(fz + mDist * sin(mAngle));
	group = new SimpleGroup(
		[new SimpleObject(oStoneLarge, 1,1, 0,2)],
		true, clBaseResource, mX, mZ
	);
	createObjectGroup(group, 0);
	var hillSize = PI * radius * radius;
	// create starting trees
	var num = floor(hillSize / 100);
	var tAngle = randFloat(-PI/3, 4*PI/3);
	var tDist = randFloat(11, 13);
	var tX = round(fx + tDist * cos(tAngle));
	var tZ = round(fz + tDist * sin(tAngle));
	group = new SimpleGroup(
		[new SimpleObject(oPine, num, num, 0,5)],
		false, clBaseResource, tX, tZ
	);
	createObjectGroup(group, 0, avoidClasses(clBaseResource,2));
	
	// create grass tufts
	var num = hillSize / 250;
	for (var j = 0; j < num; j++)
	{
		var gAngle = randFloat(0, TWO_PI);
		var gDist = radius - (5 + randInt(7));
		var gX = round(fx + gDist * cos(gAngle));
		var gZ = round(fz + gDist * sin(gAngle));
		group = new SimpleGroup(
			[new SimpleObject(aGrassShort, 2,5, 0,1, -PI/8,PI/8)],
			false, clBaseResource, gX, gZ
		);
		createObjectGroup(group, 0);
	}
}

RMS.SetProgress(20);

//place the mountains
var points = [];

var edgesConncetedToPoints = [];

//we want the points near the start locations be the first ones. hence we use two "for" blocks
for (var i = 0; i < numPlayers; ++i)
{
	playerAngle[i] = startAngle + (i+0.5)*TWO_PI/numPlayers;
	points.push([round(fractionToTiles(0.5 + 0.49 * cos(playerAngle[i]))), round(fractionToTiles(0.5 + 0.49 * sin(playerAngle[i])))]);
}

//the order of the other points doesn't matter
for (var i = 0; i < numPlayers; ++i)
{
	playerAngle[i] = startAngle + (i+0.7)*TWO_PI/numPlayers;
	points.push([round(fractionToTiles(0.5 + 0.34 * cos(playerAngle[i]))), round(fractionToTiles(0.5 + 0.34 * sin(playerAngle[i])))]);
	playerAngle[i] = startAngle + (i+0.3)*TWO_PI/numPlayers;
	points.push([round(fractionToTiles(0.5 + 0.34 * cos(playerAngle[i]))), round(fractionToTiles(0.5 + 0.34 * sin(playerAngle[i])))]);
	playerAngle[i] = startAngle + (i+0.5)*TWO_PI/numPlayers;
	points.push([round(fractionToTiles(0.5 + 0.18 * cos(playerAngle[i]))), round(fractionToTiles(0.5 + 0.18 * sin(playerAngle[i])))]);
}

//add the center of the map
points.push([round(fractionToTiles(0.5)), round(fractionToTiles(0.5))]);

var numPoints = numPlayers * 4 + 1;

for (var i = 0; i < numPoints; ++i)
{
	edgesConncetedToPoints.push(0);
}

//we are making a planar graph where every edge is a straight line.
var possibleEdges = [];

//add all of the possible combinations
for (var i = 0; i < numPoints; ++i)
{
	for (var j = numPlayers; j < numPoints; ++j)
	{
		if (j > i)
			possibleEdges.push([i,j]);
	}
}

//we need a matrix so that we can prevent the mountain ranges from bocking a player
var matrix = []

for (var i = 0; i < numPoints; ++i)
{
	matrix.push([]);
	for (var j = 0; j < numPoints; ++j)
	{
		if (i < numPlayers && j < numPlayers && i != j && (i == j - 1 || i == j + 1) )
			matrix[i].push(true);
		else
			matrix[i].push(false);
	}
}

//find and place the edges
while (possibleEdges.length > 0)
{
	var index = randInt(0, possibleEdges.length - 1);
	
	//ensure that a point is connected to a maximum of 3 others
	if (edgesConncetedToPoints[possibleEdges[index][0]] > 2 || edgesConncetedToPoints[possibleEdges[index][1]] > 2)
	{
		possibleEdges.splice(index,1);
		continue;
	}
	
	//we don't want ranges that are longer than half of the map's size
	if ((((points[possibleEdges[index][0]][0] - points[possibleEdges[index][1]][0]) * 
		(points[possibleEdges[index][0]][0] - points[possibleEdges[index][1]][0])) + 
		((points[possibleEdges[index][0]][1] - points[possibleEdges[index][1]][1]) * 
		(points[possibleEdges[index][0]][1] - points[possibleEdges[index][1]][1]))) >
		mapArea)
	{
		possibleEdges.splice(index,1);
		continue;
	}
	
	//dfs
	var q = [possibleEdges[index][0]];

	matrix[possibleEdges[index][0]][possibleEdges[index][1]] = true;
	matrix[possibleEdges[index][1]][possibleEdges[index][0]] = true;
	var selected, accept = true, tree = [], backtree = [];
	while (q.length > 0)
	{
		selected = q.shift();
		if (tree.indexOf(selected) == -1)
		{
			tree.push(selected);
			backtree.push(-1);
		}

		for (var i = 0; i < numPoints; ++i)
		{
			if (matrix[selected][i])
			{
				if (i == backtree[tree.lastIndexOf(selected)]) 
				{
					continue;
				}
				else if (tree.indexOf(i) == -1)
				{
					tree.push(i);
					backtree.push(selected);
					q.unshift(i);
				}
				else
				{
					accept = false;
					matrix[possibleEdges[index][0]][possibleEdges[index][1]] = false;
					matrix[possibleEdges[index][1]][possibleEdges[index][0]] = false;
					break;
				}
			}
		}
	}
	
	if (!accept)
	{
		possibleEdges.splice(index,1);
		continue;
	}
	
	var ix = points[possibleEdges[index][0]][0];
	var iz = points[possibleEdges[index][0]][1];
	var ix2 = points[possibleEdges[index][1]][0];
	var iz2 = points[possibleEdges[index][1]][1];
	var placer = new PathPlacer(ix, iz, ix2, iz2, scaleByMapSize(9,15), 0.4, 3*(scaleByMapSize(1,4)), 0.1, 0.1, 0.1);
	var terrainPainter = new LayeredPainter(
		[tCliff, tPrimary],		// terrains
		[3]		// widths
	);
	var elevationPainter = new SmoothElevationPainter(
		ELEVATION_SET,			// type
		30,				// elevation
		2				// blend radius
	);
	accept = createArea(placer, [terrainPainter, elevationPainter, paintClass(clHill)], avoidClasses(clPlayer, 20));
	
	if (accept == null)
	{
		matrix[possibleEdges[index][0]][possibleEdges[index][1]] = false;
		matrix[possibleEdges[index][1]][possibleEdges[index][0]] = false;
		possibleEdges.splice(index,1);
		continue;
	}
	else
	{
		placer = new ClumpPlacer(floor(PI*scaleByMapSize(9,15)*scaleByMapSize(9,15)/4), 0.95, 0.6, 10, ix, iz);
		var terrainPainter = new LayeredPainter(
			[tCliff, tPrimary],		// terrains
			[3]		// widths
		);
		var elevationPainter = new SmoothElevationPainter(
			ELEVATION_SET,			// type
			30,				// elevation
			2				// blend radius
		);
		createArea(placer, [terrainPainter, elevationPainter, paintClass(clHill)], avoidClasses(clPlayer, 5));
		
		placer = new ClumpPlacer(floor(PI*scaleByMapSize(9,15)*scaleByMapSize(9,15)/4), 0.95, 0.6, 10, ix2, iz2);
		var terrainPainter = new LayeredPainter(
			[tCliff, tPrimary],		// terrains
			[3]		// widths
		);
		var elevationPainter = new SmoothElevationPainter(
			ELEVATION_SET,			// type
			30,				// elevation
			2				// blend radius
		);
		createArea(placer, [terrainPainter, elevationPainter, paintClass(clHill)], avoidClasses(clPlayer, 5));
	}
		
	for (var i = 0; i < possibleEdges.length; ++i)
	{
		if (possibleEdges[index][0] != possibleEdges[i][0] && possibleEdges[index][1] != possibleEdges[i][0] &&
			possibleEdges[index][0] != possibleEdges[i][1] && possibleEdges[index][1] != possibleEdges[i][1])
		{
			
			if (checkIfIntersect (points[possibleEdges[index][0]][0], points[possibleEdges[index][0]][1],
				points[possibleEdges[index][1]][0], points[possibleEdges[index][1]][1], points[possibleEdges[i][0]][0],
				points[possibleEdges[i][0]][1], points[possibleEdges[i][1]][0], points[possibleEdges[i][1]][1], scaleByMapSize(9,15) + scaleByMapSize(10,15)))
			{
				possibleEdges.splice(i,1);
				--i;
				if (index > i)
					--index;
			}
		}
		else if (((possibleEdges[index][0] == possibleEdges[i][0] && possibleEdges[index][1] != possibleEdges[i][1]) ||
			(possibleEdges[index][1] == possibleEdges[i][0] && possibleEdges[index][0] != possibleEdges[i][1])) && 
			distanceOfPointFromLine(points[possibleEdges[index][0]][0],points[possibleEdges[index][0]][1],
			points[possibleEdges[index][1]][0], points[possibleEdges[index][1]][1],points[possibleEdges[i][1]][0], points[possibleEdges[i][1]][1])
			< scaleByMapSize(9,15) + scaleByMapSize(10,15))
		{
			possibleEdges.splice(i,1);
			--i;
			if (index > i)
				--index;
		}
		else if (((possibleEdges[index][0] == possibleEdges[i][1] && possibleEdges[index][1] != possibleEdges[i][0]) ||
			(possibleEdges[index][1] == possibleEdges[i][1] && possibleEdges[index][0] != possibleEdges[i][0])) && 
			distanceOfPointFromLine(points[possibleEdges[index][0]][0],points[possibleEdges[index][0]][1],
			points[possibleEdges[index][1]][0], points[possibleEdges[index][1]][1],points[possibleEdges[i][0]][0], points[possibleEdges[i][0]][1])
			< scaleByMapSize(9,15) + scaleByMapSize(10,15))
		{
			possibleEdges.splice(i,1);
			--i;
			if (index > i)
				--index;
		}

	}
	
	edgesConncetedToPoints[possibleEdges[index][0]] += 1;
	edgesConncetedToPoints[possibleEdges[index][1]] += 1;
	
	possibleEdges.splice(index,1);
}

paintTerrainBasedOnHeight(3.1, 29, 0, tCliff);
paintTerrainBasedOnHeight(29, 30, 3, tSnowLimited);

// create bumps
log("Creating bumps...");
placer = new ClumpPlacer(scaleByMapSize(20, 50), 0.3, 0.06, 1);
painter = new SmoothElevationPainter(ELEVATION_MODIFY, 2, 2);
createAreas(
	placer,
	painter, 
	avoidClasses(clWater, 2, clPlayer, 10),
	scaleByMapSize(100, 200)
);

// create hills
log("Creating hills...");
placer = new ClumpPlacer(scaleByMapSize(40, 150), 0.2, 0.1, 1);
var terrainPainter = new LayeredPainter(
	[tCliff, tSnowLimited],		// terrains
	[2]								// widths
);
var elevationPainter = new SmoothElevationPainter(ELEVATION_SET, 30, 2);
createAreas(
	placer,
	[terrainPainter, elevationPainter, paintClass(clHill)], 
	avoidClasses(clPlayer, 20, clHill, 14),
	scaleByMapSize(10, 80) * numPlayers
);

// calculate desired number of trees for map (based on size)
var MIN_TREES = 500;
var MAX_TREES = 3000;
var P_FOREST = 0.7;

var totalTrees = scaleByMapSize(MIN_TREES, MAX_TREES);
var numForest = totalTrees * P_FOREST;
var numStragglers = totalTrees * (1.0 - P_FOREST);

// create forests
log("Creating forests...");
var types = [
	[[tForestFloor, tPrimary, pForest], [tForestFloor, pForest]]
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
		avoidClasses(clPlayer, 12, clForest, 10, clHill, 0, clWater, 2),
		num
	);
}

RMS.SetProgress(60);

// create dirt patches
log("Creating dirt patches...");
var sizes = [scaleByMapSize(3, 48), scaleByMapSize(5, 84), scaleByMapSize(8, 128)];
for (var i = 0; i < sizes.length; i++)
{
	placer = new ClumpPlacer(sizes[i], 0.3, 0.06, 0.5);
	painter = new LayeredPainter(
		[[tDirt,tHalfSnow], [tHalfSnow,tSnowLimited]], 		// terrains
		[2]															// widths
	);
	createAreas(
		placer,
		[painter, paintClass(clDirt)],
		avoidClasses(clWater, 3, clForest, 0, clHill, 0, clDirt, 5, clPlayer, 12),
		scaleByMapSize(15, 45)
	);
}

// create grass patches
log("Creating grass patches...");
var sizes = [scaleByMapSize(2, 32), scaleByMapSize(3, 48), scaleByMapSize(5, 80)];
for (var i = 0; i < sizes.length; i++)
{
	placer = new ClumpPlacer(sizes[i], 0.3, 0.06, 0.5);
	painter = new TerrainPainter(tSecondary);
	createAreas(
		placer,
		painter,
		avoidClasses(clWater, 3, clForest, 0, clHill, 0, clDirt, 5, clPlayer, 12),
		scaleByMapSize(15, 45)
	);
}
RMS.SetProgress(65);


log("Creating stone mines...");
// create large stone quarries
group = new SimpleGroup([new SimpleObject(oStoneSmall, 0,2, 0,4), new SimpleObject(oStoneLarge, 1,1, 0,4)], true, clRock);
createObjectGroups(group, 0,
	avoidClasses(clWater, 3, clForest, 1, clPlayer, 20, clRock, 10, clHill, 1),
	scaleByMapSize(4,16), 100
);

// create small stone quarries
group = new SimpleGroup([new SimpleObject(oStoneSmall, 2,5, 1,3)], true, clRock);
createObjectGroups(group, 0,
	avoidClasses(clWater, 3, clForest, 1, clPlayer, 20, clRock, 10, clHill, 1),
	scaleByMapSize(4,16), 100
);

log("Creating metal mines...");
// create large metal quarries
group = new SimpleGroup([new SimpleObject(oMetalLarge, 1,1, 0,4)], true, clMetal);
createObjectGroups(group, 0,
	avoidClasses(clWater, 3, clForest, 1, clPlayer, 20, clMetal, 10, clRock, 5, clHill, 1),
	scaleByMapSize(4,16), 100
);

RMS.SetProgress(70);

// create small decorative rocks
log("Creating small decorative rocks...");
group = new SimpleGroup(
	[new SimpleObject(aRockMedium, 1,3, 0,1)],
	true
);
createObjectGroups(
	group, 0,
	avoidClasses(clWater, 0, clForest, 0, clPlayer, 0, clHill, 0),
	scaleByMapSize(16, 262), 50
);


// create large decorative rocks
log("Creating large decorative rocks...");
group = new SimpleGroup(
	[new SimpleObject(aRockLarge, 1,2, 0,1), new SimpleObject(aRockMedium, 1,3, 0,2)],
	true
);
createObjectGroups(
	group, 0,
	avoidClasses(clWater, 0, clForest, 0, clPlayer, 0, clHill, 0),
	scaleByMapSize(8, 131), 50
);

RMS.SetProgress(75);

// create deer
log("Creating deer...");
group = new SimpleGroup(
	[new SimpleObject(oDeer, 5,7, 0,4)],
	true, clFood
);
createObjectGroups(group, 0,
	avoidClasses(clWater, 3, clForest, 0, clPlayer, 10, clHill, 1, clFood, 20),
	3 * numPlayers, 50
);

// create berry bush
log("Creating berry bush...");
group = new SimpleGroup(
	[new SimpleObject(oBerryBush, 5,7, 0,4)],
	true, clFood
);
createObjectGroups(group, 0,
	avoidClasses(clWater, 3, clForest, 0, clPlayer, 20, clHill, 1, clFood, 10),
	randInt(1, 4) * numPlayers + 2, 50
);

// create rabbit
log("Creating rabbit...");
group = new SimpleGroup(
	[new SimpleObject(oRabbit, 2,3, 0,2)],
	true, clFood
);
createObjectGroups(group, 0,
	avoidClasses(clWater, 3, clForest, 0, clPlayer, 10, clHill, 1, clFood, 20),
	3 * numPlayers, 50
);

RMS.SetProgress(85);


// create straggler trees
log("Creating straggler trees...");
var types = [oPine, oPine];	// some variation
var num = floor(numStragglers / types.length);
for (var i = 0; i < types.length; ++i)
{
	group = new SimpleGroup(
		[new SimpleObject(types[i], 1,1, 0,3)],
		true, clForest
	);
	createObjectGroups(group, 0,
		avoidClasses(clWater, 5, clForest, 1, clHill, 1, clPlayer, 12, clMetal, 1, clRock, 1),
		num
	);
}


//create small grass tufts
log("Creating small grass tufts...");
var planetm = 1;

group = new SimpleGroup(
	[new SimpleObject(aGrassShort, 1,2, 0,1, -PI/8,PI/8)]
);
createObjectGroups(group, 0,
	avoidClasses(clWater, 2, clHill, 2, clPlayer, 2, clDirt, 0),
	planetm * scaleByMapSize(13, 200)
);

RMS.SetProgress(90);

// create large grass tufts
log("Creating large grass tufts...");
group = new SimpleGroup(
	[new SimpleObject(aGrass, 2,4, 0,1.8, -PI/8,PI/8), new SimpleObject(aGrassShort, 3,6, 1.2,2.5, -PI/8,PI/8)]
);
createObjectGroups(group, 0,
	avoidClasses(clWater, 3, clHill, 2, clPlayer, 2, clDirt, 1, clForest, 0),
	planetm * scaleByMapSize(13, 200)
);

RMS.SetProgress(95);

// create bushes
log("Creating bushes...");
group = new SimpleGroup(
	[new SimpleObject(aBushMedium, 1,2, 0,2), new SimpleObject(aBushSmall, 2,4, 0,2)]
);
createObjectGroups(group, 0,
	avoidClasses(clWater, 2, clHill, 1, clPlayer, 1, clDirt, 1),
	planetm * scaleByMapSize(13, 200), 50
);

random_terrain = randInt(1,3)
if (random_terrain==1){
	setSkySet("cirrus");
}
else if (random_terrain ==2){
	setSkySet("cumulus");
}
else if (random_terrain ==3){
	setSkySet("sunny");
}
setSunRotation(randFloat(0, TWO_PI));
setSunElevation(randFloat(PI/ 5, PI / 3));
setWaterColour(0.0, 0.047, 0.286);				// dark majestic blue
setWaterTint(0.471, 0.776, 0.863);				// light blue
setWaterMurkiness(0.72);
setWaterWaviness(3);

// Export map data

ExportMap();