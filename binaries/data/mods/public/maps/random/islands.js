Engine.LoadLibrary("rmgen");
Engine.LoadLibrary("rmbiome");

TILE_CENTERED_HEIGHT_MAP = true;

setSelectedBiome();

const tMainTerrain = g_Terrains.mainTerrain;
const tForestFloor1 = g_Terrains.forestFloor1;
const tForestFloor2 = g_Terrains.forestFloor2;
const tCliff = g_Terrains.cliff;
const tTier1Terrain = g_Terrains.tier1Terrain;
const tTier2Terrain = g_Terrains.tier2Terrain;
const tTier3Terrain = g_Terrains.tier3Terrain;
const tHill = g_Terrains.hill;
const tRoad = g_Terrains.road;
const tRoadWild = g_Terrains.roadWild;
const tTier4Terrain = g_Terrains.tier4Terrain;
const tShore = g_Terrains.shore;
const tWater = g_Terrains.water;

const oTree1 = g_Gaia.tree1;
const oTree2 = g_Gaia.tree2;
const oTree3 = g_Gaia.tree3;
const oTree4 = g_Gaia.tree4;
const oTree5 = g_Gaia.tree5;
const oFruitBush = g_Gaia.fruitBush;
const oMainHuntableAnimal = g_Gaia.mainHuntableAnimal;
const oFish = g_Gaia.fish;
const oSecondaryHuntableAnimal = g_Gaia.secondaryHuntableAnimal;
const oStoneLarge = g_Gaia.stoneLarge;
const oStoneSmall = g_Gaia.stoneSmall;
const oMetalLarge = g_Gaia.metalLarge;
const oWoodTreasure = "gaia/special_treasure_wood";
const oDock = "skirmish/structures/default_dock";

const aGrass = g_Decoratives.grass;
const aGrassShort = g_Decoratives.grassShort;
const aRockLarge = g_Decoratives.rockLarge;
const aRockMedium = g_Decoratives.rockMedium;
const aBushMedium = g_Decoratives.bushMedium;
const aBushSmall = g_Decoratives.bushSmall;

const pForest1 = [tForestFloor2 + TERRAIN_SEPARATOR + oTree1, tForestFloor2 + TERRAIN_SEPARATOR + oTree2, tForestFloor2];
const pForest2 = [tForestFloor1 + TERRAIN_SEPARATOR + oTree4, tForestFloor1 + TERRAIN_SEPARATOR + oTree5, tForestFloor1];

InitMap();

const numPlayers = getNumPlayers();
const mapSize = getMapSize();
const mapCenter = getMapCenter();

var clPlayer = createTileClass();
var clHill = createTileClass();
var clForest = createTileClass();
var clDirt = createTileClass();
var clRock = createTileClass();
var clMetal = createTileClass();
var clFood = createTileClass();
var clBaseResource = createTileClass();
var clLand = createTileClass();

var playerIslandRadius = scaleByMapSize(20, 29);

var [playerIDs, playerX, playerZ, playerAngle] = radialPlayerPlacement();

log("Creating player islands and docks...");
for (let i = 0; i < numPlayers; i++)
{
	let playerPos = new Vector2D(playerX[i], playerZ[i]).mult(mapSize);
	createArea(
		new ClumpPlacer(diskArea(playerIslandRadius), 0.8, 0.1, 10, playerPos.x, playerPos.y),
		[
			new LayeredPainter([tMainTerrain , tMainTerrain, tMainTerrain], [1, 6]),
			new SmoothElevationPainter(ELEVATION_SET, 3, 6),
			paintClass(clPlayer)
		]);

	let dockLocation = findLocationInDirectionBasedOnHeight(playerPos, mapCenter, -3 , 2.6, 3);
	placeObject(dockLocation.x, dockLocation.y, oDock, playerIDs[i], playerAngle[i] + Math.PI);
}

for (var i = 0; i < numPlayers; i++)
{
	var id = playerIDs[i];
	log("Creating base for player " + id + "...");

	let radius = playerIslandRadius;

	// get the x and z in tiles
	var fx = fractionToTiles(playerX[i]);
	var fz = fractionToTiles(playerZ[i]);
	var ix = round(fx);
	var iz = round(fz);

	placeCivDefaultEntities(fx, fz, id, { 'iberWall': 'towers' });

	placeDefaultChicken(fx, fz, clBaseResource);

	// create berry bushes
	var bbAngle = randFloat(0, TWO_PI);
	var bbDist = 12;
	var bbX = round(fx + bbDist * cos(bbAngle));
	var bbZ = round(fz + bbDist * sin(bbAngle));
	var group = new SimpleGroup(
		[new SimpleObject(oFruitBush, 5,5, 0,3)],
		true, clBaseResource, bbX, bbZ
	);
	createObjectGroup(group, 0);

	// create woods
	var bbAngle = randFloat(0, TWO_PI);
	var bbDist = 13;
	var bbX = round(fx + bbDist * cos(bbAngle));
	var bbZ = round(fz + bbDist * sin(bbAngle));
	group = new SimpleGroup(
		[new SimpleObject(oWoodTreasure, 14,14, 0,3)],
		true, clBaseResource, bbX, bbZ
	);
	createObjectGroup(group, 0);

	// create metal mine
	var mAngle = bbAngle;
	while (Math.abs(mAngle - bbAngle) < Math.PI / 3)
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
	var num = 5;
	var tAngle = randFloat(-PI/3, 4*PI/3);
	var tDist = randFloat(12, 13);
	var tX = round(fx + tDist * cos(tAngle));
	var tZ = round(fz + tDist * sin(tAngle));
	group = new SimpleGroup(
		[new SimpleObject(oTree1, num, num, 0,3)],
		false, clBaseResource, tX, tZ
	);
	createObjectGroup(group, 0, avoidClasses(clBaseResource,2));

	placeDefaultDecoratives(fx, fz, aGrassShort, clBaseResource, radius);
}

var landAreas = [];
var playerConstraint = new AvoidTileClassConstraint(clPlayer, Math.floor(scaleByMapSize(12,16)));
var landConstraint = new AvoidTileClassConstraint(clLand, Math.floor(scaleByMapSize(12,16)));

for (var x = 0; x < mapSize; ++x)
	for (var z = 0; z < mapSize; ++z)
		if (playerConstraint.allows(x, z) && landConstraint.allows(x, z))
			landAreas.push([x, z]);

var chosenPoint;
var landAreaLen;

log("Creating big islands...");
for (let i = 0; i < scaleByMapSize(4, 14); ++i)
{
	landAreaLen = landAreas.length;
	if (!landAreaLen)
		break;

	chosenPoint = pickRandom(landAreas);

	var newIsland = createAreas(
		new ChainPlacer(
			Math.floor(scaleByMapSize(4, 8)),
			Math.floor(scaleByMapSize(8, 14)),
			Math.floor(scaleByMapSize(25, 60)),
			0.07,
			chosenPoint[0],
			chosenPoint[1],
			scaleByMapSize(30, 70)),
		[
			new LayeredPainter([tMainTerrain, tMainTerrain], [2]),
			new SmoothElevationPainter(ELEVATION_SET, 3, 6),
			paintClass(clLand)
		],
		avoidClasses(clLand, 3, clPlayer, 3),
		1,
		1);

	if (newIsland && newIsland.length)
	{
		var n = 0;
		for (var j = 0; j < landAreas.length; ++j)
		{
			let [x, z] = landAreas[j];
			if (playerConstraint.allows(x, z) && landConstraint.allows(x, z))
				landAreas[n++] = landAreas[j];
		}
		landAreas.length = n;
	}
}

playerConstraint = new AvoidTileClassConstraint(clPlayer, Math.floor(scaleByMapSize(9,12)));
landConstraint = new AvoidTileClassConstraint(clLand, Math.floor(scaleByMapSize(9,12)));

log("Creating small islands...");
for (let i = 0; i < 6 * Math.square(scaleByMapSize(1, 3)); ++i)
{
	landAreaLen = landAreas.length;
	if (!landAreaLen)
		break;

	chosenPoint = pickRandom(landAreas);

	createAreas(
		new ChainPlacer(
			Math.floor(scaleByMapSize(4, 7)),
			Math.floor(scaleByMapSize(7, 10)),
			Math.floor(scaleByMapSize(16, 40)),
			0.07,
			chosenPoint[0],
			chosenPoint[1],
			scaleByMapSize(22, 40)),
		[
			new LayeredPainter([tMainTerrain, tMainTerrain], [2]),
			new SmoothElevationPainter(ELEVATION_SET, 3, 6),
			paintClass(clLand)
		],
		avoidClasses(clLand, 3, clPlayer, 3),
		1,
		1);

	if (newIsland !== undefined)
	{
		var temp = [];
		for (var j = 0; j < landAreaLen; ++j)
		{
			let [x, z] = landAreas[j];
			if (playerConstraint.allows(x, z) && landConstraint.allows(x, z))
					temp.push([x, z]);
		}
		landAreas = temp;
	}
}
paintTerrainBasedOnHeight(1, 3, 0, tShore);
paintTerrainBasedOnHeight(-8, 1, 2, tWater);

for (var i = 0; i < numPlayers; i++)
{
	var fx = fractionToTiles(playerX[i]);
	var fz = fractionToTiles(playerZ[i]);
	var ix = round(fx);
	var iz = round(fz);
	// create the city patch
	var placer = new ClumpPlacer(diskArea(playerIslandRadius / 3), 0.6, 0.3, 10, ix, iz);
	var painter = new LayeredPainter([tRoadWild, tRoad], [1]);
	createArea(placer, painter, null);
}

log("Creating bumps...");
createAreas(
	new ClumpPlacer(scaleByMapSize(20, 50), 0.3, 0.06, 1),
	new SmoothElevationPainter(ELEVATION_MODIFY, 2, 2),
	[avoidClasses(clPlayer, 0), stayClasses(clLand, 3)],
	scaleByMapSize(20, 100));

log("Creating hills...");
createAreas(
	new ChainPlacer(1, Math.floor(scaleByMapSize(4, 6)), Math.floor(scaleByMapSize(16, 40)), 0.5),
	[
		new LayeredPainter([tCliff, tHill], [2]),
		new SmoothElevationPainter(ELEVATION_SET, 18, 2),
		paintClass(clHill)
	],
	[avoidClasses(clPlayer, 2, clHill, 15), stayClasses(clLand, 0)],
	scaleByMapSize(4, 13));

log("Creating forests...");
var [forestTrees, stragglerTrees] = getTreeCounts(...rBiomeTreeCount(1));
var types = [
	[[tForestFloor2, tMainTerrain, pForest1], [tForestFloor2, pForest1]],
	[[tForestFloor1, tMainTerrain, pForest2], [tForestFloor1, pForest2]]
];

if (currentBiome() != "savanna")
{
	var size = forestTrees / (scaleByMapSize(3,6) * numPlayers);
	var num = Math.floor(size / types.length);
	for (let type of types)
		createAreas(
			new ChainPlacer(1, Math.floor(scaleByMapSize(3, 5)), forestTrees / (num * Math.floor(scaleByMapSize(2, 5))), 0.5),
			[
				new LayeredPainter(type, [2]),
				paintClass(clForest)
			],
			[avoidClasses(clPlayer, 0, clForest, 10, clHill, 0), stayClasses(clLand, 6)],
			num);
}

Engine.SetProgress(50);
log("Creating dirt patches...");
var numberOfPatches = scaleByMapSize(15, 45) * (currentBiome() == "savanna" ? 3 : 1);
for (let size of [scaleByMapSize(3, 6), scaleByMapSize(5, 10), scaleByMapSize(8, 21)])
	createAreas(
		new ChainPlacer(1, Math.floor(scaleByMapSize(3, 5)), size, 0.5),
		[
			new LayeredPainter([[tMainTerrain, tTier1Terrain], [tTier1Terrain, tTier2Terrain], [tTier2Terrain, tTier3Terrain]], [1, 1]),
			paintClass(clDirt)
		],
		[avoidClasses(clForest, 0, clHill, 0, clDirt, 5, clPlayer, 0), stayClasses(clLand, 6)],
		numberOfPatches);

log("Creating grass patches...");
for (let size of [scaleByMapSize(2, 4), scaleByMapSize(3, 7), scaleByMapSize(5, 15)])
	createAreas(
		new ChainPlacer(1, Math.floor(scaleByMapSize(3, 5)), size, 0.5),
		new TerrainPainter(tTier4Terrain),
		[avoidClasses(clForest, 0, clHill, 0, clDirt, 5, clPlayer, 0), stayClasses(clLand, 6)],
		numberOfPatches);

Engine.SetProgress(55);

log("Creating stone mines...");
var group = new SimpleGroup([new SimpleObject(oStoneSmall, 0, 2, 0, 4), new SimpleObject(oStoneLarge, 1, 1, 0, 4)], true, clRock);
createObjectGroupsDeprecated(group, 0,
	[avoidClasses(clForest, 1, clPlayer, 0, clRock, 10, clHill, 1), stayClasses(clLand, 5)],
	scaleByMapSize(4,16), 100
);

log("Creating small stone quarries...");
group = new SimpleGroup([new SimpleObject(oStoneSmall, 2,5, 1,3)], true, clRock);
createObjectGroupsDeprecated(group, 0,
	[avoidClasses(clForest, 1, clPlayer, 0, clRock, 10, clHill, 1), stayClasses(clLand, 5)],
	scaleByMapSize(4,16), 100
);

log("Creating metal mines...");
group = new SimpleGroup([new SimpleObject(oMetalLarge, 1,1, 0,4)], true, clMetal);
createObjectGroupsDeprecated(group, 0,
	[avoidClasses(clForest, 1, clPlayer, 0, clMetal, 10, clRock, 5, clHill, 1), stayClasses(clLand, 5)],
	scaleByMapSize(4,16), 100
);

Engine.SetProgress(65);

log("Creating small decorative rocks...");
group = new SimpleGroup(
	[new SimpleObject(aRockMedium, 1,3, 0,1)],
	true
);
createObjectGroupsDeprecated(
	group, 0,
	[avoidClasses(clForest, 0, clPlayer, 0, clHill, 0), stayClasses(clLand, 5)],
	scaleByMapSize(16, 262), 50
);

log("Creating large decorative rocks...");
group = new SimpleGroup(
	[new SimpleObject(aRockLarge, 1,2, 0,1), new SimpleObject(aRockMedium, 1,3, 0,2)],
	true
);
createObjectGroupsDeprecated(
	group, 0,
	[avoidClasses(clForest, 0, clPlayer, 0, clHill, 0), stayClasses(clLand, 5)],
	scaleByMapSize(8, 131), 50
);

Engine.SetProgress(70);

log("Creating deer...");
group = new SimpleGroup(
	[new SimpleObject(oMainHuntableAnimal, 5,7, 0,4)],
	true, clFood
);
createObjectGroupsDeprecated(group, 0,
	[avoidClasses(clForest, 0, clPlayer, 0, clHill, 1, clFood, 20), stayClasses(clLand, 5)],
	3 * numPlayers, 50
);

Engine.SetProgress(75);

log("Creating sheep...");
group = new SimpleGroup(
	[new SimpleObject(oSecondaryHuntableAnimal, 2,3, 0,2)],
	true, clFood
);
createObjectGroupsDeprecated(group, 0,
	[avoidClasses(clForest, 0, clPlayer, 0, clHill, 1, clFood, 20), stayClasses(clLand, 5)],
	3 * numPlayers, 50
);

log("Creating fruit bush...");
group = new SimpleGroup(
	[new SimpleObject(oFruitBush, 5,7, 0,4)],
	true, clFood
);
createObjectGroupsDeprecated(group, 0,
	[avoidClasses(clForest, 0, clPlayer, 8, clHill, 1, clFood, 20), stayClasses(clLand, 5)],
	randIntInclusive(1, 4) * numPlayers + 2, 50
);

log("Creating fish...");
group = new SimpleGroup(
	[new SimpleObject(oFish, 2,3, 0,2)],
	true, clFood
);
createObjectGroupsDeprecated(group, 0,
	avoidClasses(clLand, 4, clForest, 2, clPlayer, 2, clHill, 2, clFood, 20),
	25 * numPlayers, 60
);

Engine.SetProgress(85);

createStragglerTrees(
	[oTree1, oTree2, oTree4, oTree3],
	[avoidClasses(clForest, 1, clHill, 1, clPlayer, 0, clMetal, 6, clRock, 6), stayClasses(clLand, 6)],
	clForest,
	stragglerTrees);

var planetm = 1;
if (currentBiome() == "tropic")
	planetm = 8;

log("Creating small grass tufts...");
group = new SimpleGroup(
	[new SimpleObject(aGrassShort, 1,2, 0,1, -PI/8,PI/8)]
);
createObjectGroupsDeprecated(group, 0,
	[avoidClasses(clHill, 2, clPlayer, 2, clDirt, 0), stayClasses(clLand, 6)],
	planetm * scaleByMapSize(13, 200)
);

Engine.SetProgress(90);

log("Creating large grass tufts...");
group = new SimpleGroup(
	[new SimpleObject(aGrass, 2,4, 0,1.8, -PI/8,PI/8), new SimpleObject(aGrassShort, 3,6, 1.2,2.5, -PI/8,PI/8)]
);
createObjectGroupsDeprecated(group, 0,
	[avoidClasses(clHill, 2, clPlayer, 2, clDirt, 1, clForest, 0), stayClasses(clLand, 5)],
	planetm * scaleByMapSize(13, 200)
);

Engine.SetProgress(95);

log("Creating bushes...");
group = new SimpleGroup(
	[new SimpleObject(aBushMedium, 1,2, 0,2), new SimpleObject(aBushSmall, 2,4, 0,2)]
);
createObjectGroupsDeprecated(group, 0,
	[avoidClasses(clHill, 1, clPlayer, 1, clDirt, 1), stayClasses(clLand, 6)],
	planetm * scaleByMapSize(13, 200), 50
);

setSkySet(pickRandom(["cirrus", "cumulus", "sunny"]));
setSunRotation(randFloat(0, TWO_PI));
setSunElevation(randFloat(PI/ 5, PI / 3));
setWaterWaviness(2);

ExportMap();
