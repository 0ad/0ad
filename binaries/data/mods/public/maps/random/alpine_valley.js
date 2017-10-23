RMS.LoadLibrary("rmgen");

TILE_CENTERED_HEIGHT_MAP = true;

// late spring
if (randBool())
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

	var oPine = "gaia/flora_tree_pine";
	var oBerryBush = "gaia/flora_bush_berry";
	var oDeer = "gaia/fauna_deer";
	var oRabbit = "gaia/fauna_rabbit";
	var oStoneLarge = "gaia/geology_stonemine_alpine_quarry";
	var oStoneSmall = "gaia/geology_stone_alpine_a";
	var oMetalLarge = "gaia/geology_metal_alpine_slabs";

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

	var oPine = "gaia/flora_tree_pine_w";
	var oBerryBush = "gaia/flora_bush_berry";
	var oDeer = "gaia/fauna_deer";
	var oRabbit = "gaia/fauna_rabbit";
	var oStoneLarge = "gaia/geology_stonemine_alpine_quarry";
	var oStoneSmall = "gaia/geology_stone_alpine_a";
	var oMetalLarge = "gaia/geology_metal_alpine_slabs";

	var aGrass = "actor|props/flora/grass_soft_dry_small_tall.xml";
	var aGrassShort = "actor|props/flora/grass_soft_dry_large.xml";
	var aRockLarge = "actor|geology/stone_granite_med.xml";
	var aRockMedium = "actor|geology/stone_granite_med.xml";
	var aBushMedium = "actor|props/flora/bush_medit_me_dry.xml";
	var aBushSmall = "actor|props/flora/bush_medit_sm_dry.xml";
}

const pForest = [tForestFloor + TERRAIN_SEPARATOR + oPine, tForestFloor];

InitMap();

const numPlayers = getNumPlayers();
const mapSize = getMapSize();
const mapArea = getMapArea();

var clPlayer = createTileClass();
var clHill = createTileClass();
var clForest = createTileClass();
var clDirt = createTileClass();
var clRock = createTileClass();
var clMetal = createTileClass();
var clFood = createTileClass();
var clBaseResource = createTileClass();

initTerrain(tPrimary);

var [playerIDs, playerX, playerZ, playerAngle, startAngle] = radialPlayerPlacement();

for (var i = 0; i < numPlayers; i++)
{
	var id = playerIDs[i];
	log("Creating base for player " + id + "...");

	var radius = scaleByMapSize(15,25);
	var cliffRadius = 2;
	var elevation = 20;

	// get the x and z in tiles
	var fx = fractionToTiles(playerX[i]);
	var fz = fractionToTiles(playerZ[i]);
	ix = round(fx);
	iz = round(fz);
	addCivicCenterAreaToClass(ix, iz, clPlayer);

	// create the city patch
	var cityRadius = radius/3;
	var placer = new ClumpPlacer(PI*cityRadius*cityRadius, 0.6, 0.3, 10, ix, iz);
	var painter = new LayeredPainter([tRoadWild, tRoad], [1]);
	createArea(placer, painter, null);

	placeCivDefaultEntities(fx, fz, id);

	placeDefaultChicken(fx, fz, clBaseResource);

	// create berry bushes
	var bbAngle = randFloat(0, TWO_PI);
	var bbDist = 12;
	var bbX = round(fx + bbDist * cos(bbAngle));
	var bbZ = round(fz + bbDist * sin(bbAngle));
	var group = new SimpleGroup(
		[new SimpleObject(oBerryBush, 5,5, 0,3)],
		true, clBaseResource, bbX, bbZ
	);
	createObjectGroup(group, 0);

	// create metal mine
	var mAngle = bbAngle;
	while(abs(mAngle - bbAngle) < PI/3)
		mAngle = randFloat(0, TWO_PI);

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

	placeDefaultDecoratives(fx, fz, aGrassShort, clBaseResource, radius);
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
	edgesConncetedToPoints.push(0);

//we are making a planar graph where every edge is a straight line.
var possibleEdges = [];

//add all of the possible combinations
for (var i = 0; i < numPoints; ++i)
	for (var j = numPlayers; j < numPoints; ++j)
		if (j > i)
			possibleEdges.push([i,j]);

//we need a matrix so that we can prevent the mountain ranges from bocking a player
var matrix = [];
for (var i = 0; i < numPoints; ++i)
{
	matrix.push([]);
	for (var j = 0; j < numPoints; ++j)
		matrix[i].push(i < numPlayers && j < numPlayers && i != j && (i == j - 1 || i == j + 1))
}

//find and place the edges
while (possibleEdges.length)
{
	var index = randIntExclusive(0, possibleEdges.length);

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

	let accept = true;
	let tree = [];
	let backtree = [];

	while (q.length > 0)
	{
		let selected = q.shift();
		if (tree.indexOf(selected) == -1)
		{
			tree.push(selected);
			backtree.push(-1);
		}

		for (var i = 0; i < numPoints; ++i)
			if (matrix[selected][i])
			{
				if (i == backtree[tree.lastIndexOf(selected)])
					continue;

				if (tree.indexOf(i) == -1)
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

	if (!accept)
	{
		possibleEdges.splice(index,1);
		continue;
	}

	var ix = points[possibleEdges[index][0]][0];
	var iz = points[possibleEdges[index][0]][1];
	var ix2 = points[possibleEdges[index][1]][0];
	var iz2 = points[possibleEdges[index][1]][1];
	accept = createArea(
		new PathPlacer(ix, iz, ix2, iz2, scaleByMapSize(9,15), 0.4, 3 * scaleByMapSize(1, 4), 0.1, 0.1, 0.1),
		[
			new LayeredPainter([tCliff, tPrimary], [3]),
			new SmoothElevationPainter(ELEVATION_SET, 30, 2),
			paintClass(clHill)
		],
		avoidClasses(clPlayer, 20));

	if (accept == null)
	{
		matrix[possibleEdges[index][0]][possibleEdges[index][1]] = false;
		matrix[possibleEdges[index][1]][possibleEdges[index][0]] = false;
		possibleEdges.splice(index,1);
		continue;
	}

	for (let [x, z] of [[ix, iz], [ix2, iz2]])
		createArea(
			new ClumpPlacer(Math.floor(diskArea(scaleByMapSize(4.5, 7.5))), 0.95, 0.6, 10, x, z),
			[
				new LayeredPainter([tCliff, tPrimary], [3]),
				new SmoothElevationPainter(ELEVATION_SET, 30, 2),
				paintClass(clHill)
			],
			avoidClasses(clPlayer, 5));

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

log("Creating bumps...");
createAreas(
	new ClumpPlacer(scaleByMapSize(20, 50), 0.3, 0.06, 1),
	new SmoothElevationPainter(ELEVATION_MODIFY, 2, 2),
	avoidClasses(clPlayer, 10),
	scaleByMapSize(100, 200));
RMS.SetProgress(40);

log("Creating hills...");
createAreas(
	new ClumpPlacer(scaleByMapSize(40, 150), 0.2, 0.1, 1),
	[
		new LayeredPainter([tCliff, tSnowLimited], [2]),
		new SmoothElevationPainter(ELEVATION_SET, 30, 2),
		paintClass(clHill)
	],
	avoidClasses(clPlayer, 20, clHill, 14),
	scaleByMapSize(10, 80) * numPlayers
);
RMS.SetProgress(50);

// calculate desired number of trees for map (based on size)
var MIN_TREES = 500;
var MAX_TREES = 3000;
var P_FOREST = 0.7;

var totalTrees = scaleByMapSize(MIN_TREES, MAX_TREES);
var numForest = totalTrees * P_FOREST;
var numStragglers = totalTrees * (1.0 - P_FOREST);

log("Creating forests...");
var types = [
	[[tForestFloor, tPrimary, pForest], [tForestFloor, pForest]]
];

var size = numForest / (scaleByMapSize(2,8) * numPlayers);

var num = Math.floor(size / types.length);
for (let type of types)
	createAreas(
		new ClumpPlacer(numForest / num, 0.1, 0.1, 1),
		[
			new LayeredPainter(type, [2]),
			paintClass(clForest)
		],
		avoidClasses(clPlayer, 12, clForest, 10, clHill, 0),
		num);
RMS.SetProgress(60);

log("Creating dirt patches...");
for (let size of [scaleByMapSize(3, 48), scaleByMapSize(5, 84), scaleByMapSize(8, 128)])
	createAreas(
		new ClumpPlacer(size, 0.3, 0.06, 0.5),
		[
			new LayeredPainter([[tDirt, tHalfSnow], [tHalfSnow, tSnowLimited]], [2]),
			paintClass(clDirt)
		],
		avoidClasses(clForest, 0, clHill, 0, clDirt, 5, clPlayer, 12),
		scaleByMapSize(15, 45));

log("Creating grass patches...");
for (let size of [scaleByMapSize(2, 32), scaleByMapSize(3, 48), scaleByMapSize(5, 80)])
	createAreas(
		new ClumpPlacer(size, 0.3, 0.06, 0.5),
		new TerrainPainter(tSecondary),
		avoidClasses(clForest, 0, clHill, 0, clDirt, 5, clPlayer, 12),
		scaleByMapSize(15, 45));

RMS.SetProgress(65);

log("Creating stone mines...");
group = new SimpleGroup([new SimpleObject(oStoneSmall, 0,2, 0,4), new SimpleObject(oStoneLarge, 1,1, 0,4)], true, clRock);
createObjectGroupsDeprecated(group, 0,
	avoidClasses(clForest, 1, clPlayer, 20, clRock, 10, clHill, 1),
	scaleByMapSize(4,16), 100
);

log("Creating small stone mines...");
group = new SimpleGroup([new SimpleObject(oStoneSmall, 2,5, 1,3)], true, clRock);
createObjectGroupsDeprecated(group, 0,
	avoidClasses(clForest, 1, clPlayer, 20, clRock, 10, clHill, 1),
	scaleByMapSize(4,16), 100
);

log("Creating metal mines...");
group = new SimpleGroup([new SimpleObject(oMetalLarge, 1,1, 0,4)], true, clMetal);
createObjectGroupsDeprecated(group, 0,
	avoidClasses(clForest, 1, clPlayer, 20, clMetal, 10, clRock, 5, clHill, 1),
	scaleByMapSize(4,16), 100
);
RMS.SetProgress(70);

log("Creating small decorative rocks...");
group = new SimpleGroup(
	[new SimpleObject(aRockMedium, 1,3, 0,1)],
	true
);
createObjectGroupsDeprecated(
	group, 0,
	avoidClasses(clForest, 0, clPlayer, 0, clHill, 0),
	scaleByMapSize(16, 262), 50
);

log("Creating large decorative rocks...");
group = new SimpleGroup(
	[new SimpleObject(aRockLarge, 1,2, 0,1), new SimpleObject(aRockMedium, 1,3, 0,2)],
	true
);
createObjectGroupsDeprecated(
	group, 0,
	avoidClasses(clForest, 0, clPlayer, 0, clHill, 0),
	scaleByMapSize(8, 131), 50
);
RMS.SetProgress(75);

log("Creating deer...");
group = new SimpleGroup(
	[new SimpleObject(oDeer, 5,7, 0,4)],
	true, clFood
);
createObjectGroupsDeprecated(group, 0,
	avoidClasses(clForest, 0, clPlayer, 10, clHill, 1, clFood, 20),
	3 * numPlayers, 50
);

log("Creating berry bush...");
group = new SimpleGroup(
	[new SimpleObject(oBerryBush, 5,7, 0,4)],
	true, clFood
);
createObjectGroupsDeprecated(group, 0,
	avoidClasses(clForest, 0, clPlayer, 20, clHill, 1, clFood, 10),
	randIntInclusive(1, 4) * numPlayers + 2, 50
);

log("Creating rabbit...");
group = new SimpleGroup(
	[new SimpleObject(oRabbit, 2,3, 0,2)],
	true, clFood
);
createObjectGroupsDeprecated(group, 0,
	avoidClasses(clForest, 0, clPlayer, 10, clHill, 1, clFood, 20),
	3 * numPlayers, 50
);
RMS.SetProgress(85);

log("Creating straggler trees...");
var types = [oPine, oPine];
var num = floor(numStragglers / types.length);
for (let type of types)
	createObjectGroupsDeprecated(
		new SimpleGroup([new SimpleObject(type, 1,1, 0,3)], true, clForest),
		0,
		avoidClasses(clForest, 1, clHill, 1, clPlayer, 12, clMetal, 6, clRock, 6),
		num);

log("Creating small grass tufts...");
var planetm = 1;

group = new SimpleGroup(
	[new SimpleObject(aGrassShort, 1,2, 0,1, -PI/8,PI/8)]
);
createObjectGroupsDeprecated(group, 0,
	avoidClasses(clHill, 2, clPlayer, 2, clDirt, 0),
	planetm * scaleByMapSize(13, 200)
);
RMS.SetProgress(90);

log("Creating large grass tufts...");
group = new SimpleGroup(
	[new SimpleObject(aGrass, 2,4, 0,1.8, -PI/8,PI/8), new SimpleObject(aGrassShort, 3,6, 1.2,2.5, -PI/8,PI/8)]
);
createObjectGroupsDeprecated(group, 0,
	avoidClasses(clHill, 2, clPlayer, 2, clDirt, 1, clForest, 0),
	planetm * scaleByMapSize(13, 200)
);
RMS.SetProgress(95);

log("Creating bushes...");
group = new SimpleGroup(
	[new SimpleObject(aBushMedium, 1,2, 0,2), new SimpleObject(aBushSmall, 2,4, 0,2)]
);
createObjectGroupsDeprecated(group, 0,
	avoidClasses(clHill, 1, clPlayer, 1, clDirt, 1),
	planetm * scaleByMapSize(13, 200), 50
);

setSkySet(pickRandom(["cirrus", "cumulus", "sunny"]));
setSunRotation(randFloat(0, TWO_PI));
setSunElevation(randFloat(PI/ 5, PI / 3));
setWaterColor(0.0, 0.047, 0.286);				// dark majestic blue
setWaterTint(0.471, 0.776, 0.863);				// light blue
setWaterMurkiness(0.72);
setWaterWaviness(2.0);
setWaterType("lake");

ExportMap();
