RMS.LoadLibrary("rmgen");

var tGrass = ["cliff volcanic light", "ocean_rock_a", "ocean_rock_b"];
var tGrassA = "cliff volcanic light";
var tGrassB = "ocean_rock_a";
var tGrassC = "ocean_rock_b";
var tCliff = ["cliff volcanic coarse", "cave_walls"];
var tRoad = "road1";
var tRoadWild = "road1";
var tLava1 = "LavaTest05";
var tLava2 = "LavaTest04";
var tLava3 = "LavaTest03";

var oTree = "gaia/flora_tree_dead";
var oStoneLarge = "gaia/geology_stonemine_alpine_quarry";
var oStoneSmall = "gaia/geology_stone_alpine_a";
var oMetalLarge = "gaia/geology_metal_alpine_slabs";

var aRockLarge = "actor|geology/stone_granite_med.xml";
var aRockMedium = "actor|geology/stone_granite_med.xml";

var pForestD = [tGrassC + TERRAIN_SEPARATOR + oTree, tGrassC];
var pForestP = [tGrassB + TERRAIN_SEPARATOR + oTree, tGrassB];

InitMap();

var numPlayers = getNumPlayers();

var clPlayer = createTileClass();
var clHill = createTileClass();
var clForest = createTileClass();
var clDirt = createTileClass();
var clRock = createTileClass();
var clMetal = createTileClass();
var clBaseResource = createTileClass();

var [playerIDs, playerX, playerZ] = radialPlayerPlacement();

for (var i = 0; i < numPlayers; i++)
{
	var id = playerIDs[i];
	log("Creating base for player " + id + "...");

	var radius = scaleByMapSize(15,25);

	// get the x and z in tiles
	var fx = fractionToTiles(playerX[i]);
	var fz = fractionToTiles(playerZ[i]);
	var ix = round(fx);
	var iz = round(fz);
	addCivicCenterAreaToClass(ix, iz, clPlayer);

	// create the city patch
	var cityRadius = radius/3;
	var placer = new ClumpPlacer(PI*cityRadius*cityRadius, 0.6, 0.3, 10, ix, iz);
	var painter = new LayeredPainter([tRoadWild, tRoad], [1]);
	createArea(placer, painter, null);

	placeCivDefaultEntities(fx, fz, id);

	// create metal mine
	var bbAngle = randFloat(0, TWO_PI);
	var bbDist = 12;
	var mAngle = bbAngle;
	while(abs(mAngle - bbAngle) < PI/3)
		mAngle = randFloat(0, TWO_PI);

	var mDist = 12;
	var mX = round(fx + mDist * cos(mAngle));
	var mZ = round(fz + mDist * sin(mAngle));
	var group = new SimpleGroup(
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

	// create starting trees
	var hillSize = PI * radius * radius;
	var num = floor(hillSize / 60);
	var tries = 10;
	for (var x = 0; x < tries; ++x)
	{
		var tAngle = randFloat(-PI/3, 4*PI/3);
		var tDist = randFloat(12, 13);
		var tX = round(fx + tDist * cos(tAngle));
		var tZ = round(fz + tDist * sin(tAngle));
		group = new SimpleGroup(
			[new SimpleObject(oTree, num, num, 0, 3)],
			false, clBaseResource, tX, tZ
		);
		if (createObjectGroup(group, 0, avoidClasses(clBaseResource, 2)))
			break;
	}
}
RMS.SetProgress(15);

createVolcano(0.5, 0.5, clHill, tCliff, [tLava1, tLava2, tLava3], true, ELEVATION_SET);
RMS.SetProgress(45);

log("Creating hills...");
createAreas(
	new ClumpPlacer(scaleByMapSize(20, 150), 0.2, 0.1, 1),
	[
		new LayeredPainter([tCliff, tGrass], [2]),
		new SmoothElevationPainter(ELEVATION_SET, 18, 2),
		paintClass(clHill)
	],
	avoidClasses(clPlayer, 12, clHill, 15, clBaseResource, 2),
	scaleByMapSize(2, 8) * numPlayers
);

// calculate desired number of trees for map (based on size)
var MIN_TREES = 200;
var MAX_TREES = 1250;
var P_FOREST = 0.7;

var totalTrees = scaleByMapSize(MIN_TREES, MAX_TREES);
var numForest = totalTrees * P_FOREST;
var numStragglers = totalTrees * (1.0 - P_FOREST);

log("Creating forests...");
var types = [
	[[tGrassB, tGrassA, pForestD], [tGrassB, pForestD]],
	[[tGrassB, tGrassA, pForestP], [tGrassB, pForestP]]
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
		avoidClasses(clPlayer, 12, clForest, 10, clHill, 0),
		num
	);
}

RMS.SetProgress(70);

log("Creating dirt patches...");
var sizes = [scaleByMapSize(3, 48), scaleByMapSize(5, 84), scaleByMapSize(8, 128)];
for (var i = 0; i < sizes.length; i++)
{
	placer = new ClumpPlacer(sizes[i], 0.3, 0.06, 0.5);
	painter = new LayeredPainter(
		[tGrassA,tGrassA], 		// terrains
		[1]															// widths
	);
	createAreas(
		placer,
		[painter, paintClass(clDirt)],
		avoidClasses(clForest, 0, clHill, 0, clPlayer, 12),
		scaleByMapSize(20, 80)
	);
}
var sizes = [scaleByMapSize(3, 48), scaleByMapSize(5, 84), scaleByMapSize(8, 128)];
for (var i = 0; i < sizes.length; i++)
{
	placer = new ClumpPlacer(sizes[i], 0.3, 0.06, 0.5);
	painter = new LayeredPainter(
		[tGrassB,tGrassB], 		// terrains
		[1]															// widths
	);
	createAreas(
		placer,
		[painter, paintClass(clDirt)],
		avoidClasses(clForest, 0, clHill, 0, clPlayer, 12),
		scaleByMapSize(20, 80)
	);
}
var sizes = [scaleByMapSize(3, 48), scaleByMapSize(5, 84), scaleByMapSize(8, 128)];
for (var i = 0; i < sizes.length; i++)
{
	placer = new ClumpPlacer(sizes[i], 0.3, 0.06, 0.5);
	painter = new LayeredPainter(
		[tGrassC,tGrassC], 		// terrains
		[1]															// widths
	);
	createAreas(
		placer,
		[painter, paintClass(clDirt)],
		avoidClasses(clForest, 0, clHill, 0, clPlayer, 12),
		scaleByMapSize(20, 80)
	);
}

log("Creating stone mines...");
group = new SimpleGroup([new SimpleObject(oStoneSmall, 0,2, 0,4), new SimpleObject(oStoneLarge, 1,1, 0,4)], true, clRock);
createObjectGroupsDeprecated(group, 0,
	avoidClasses(clForest, 1, clPlayer, 10, clRock, 10, clHill, 1),
	scaleByMapSize(4,16), 100
);

log("Creating small stone mines...");
group = new SimpleGroup([new SimpleObject(oStoneSmall, 2,5, 1,3)], true, clRock);
createObjectGroupsDeprecated(group, 0,
	avoidClasses(clForest, 1, clPlayer, 10, clRock, 10, clHill, 1),
	scaleByMapSize(4,16), 100
);

log("Creating metal mines...");
group = new SimpleGroup([new SimpleObject(oMetalLarge, 1,1, 0,4)], true, clMetal);
createObjectGroupsDeprecated(group, 0,
	avoidClasses(clForest, 1, clPlayer, 10, clMetal, 10, clRock, 5, clHill, 1),
	scaleByMapSize(4,16), 100
);

RMS.SetProgress(90);

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

RMS.SetProgress(95);

log("Creating straggler trees...");
var types = [oTree];	// some variation
var num = floor(numStragglers / types.length);
for (var i = 0; i < types.length; ++i)
{
	group = new SimpleGroup(
		[new SimpleObject(types[i], 1,1, 0,3)],
		true, clForest
	);
	createObjectGroupsDeprecated(group, 0,
		avoidClasses(clForest, 1, clHill, 1, clPlayer, 12, clMetal, 6, clRock, 6),
		num
	);
}

ExportMap();
