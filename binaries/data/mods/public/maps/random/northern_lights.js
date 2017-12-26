Engine.LoadLibrary("rmgen");

const tSnowA = ["polar_snow_b"];
const tSnowB = "polar_ice_snow";
const tSnowC = "polar_ice";
const tSnowD = "polar_snow_a";
const tForestFloor = "polar_tundra_snow";
const tCliff = "polar_snow_rocks";
const tSnowE = ["polar_snow_glacial"];
const tRoad = "new_alpine_citytile";
const tRoadWild = "new_alpine_citytile";
const tShoreBlend = "alpine_shore_rocks_icy";
const tShore = "alpine_shore_rocks";
const tWater = "alpine_shore_rocks";

const oPine = "gaia/flora_tree_pine_w";
const oStoneLarge = "gaia/geology_stonemine_alpine_quarry";
const oStoneSmall = "gaia/geology_stone_alpine_a";
const oMetalLarge = "gaia/geology_metal_alpine_slabs";
const oFish = "gaia/fauna_fish";
const oWalrus = "gaia/fauna_walrus";
const oArcticWolf = "gaia/fauna_arctic_wolf";

const aIceberg = "actor|props/special/eyecandy/iceberg.xml";

const pForestD = [tForestFloor + TERRAIN_SEPARATOR + oPine, tForestFloor, tForestFloor];
const pForestS = [tForestFloor + TERRAIN_SEPARATOR + oPine, tForestFloor, tForestFloor, tForestFloor];

InitMap();

const numPlayers = getNumPlayers();
const mapSize = getMapSize();

var clPlayer = createTileClass();
var clHill = createTileClass();
var clForest = createTileClass();
var clWater = createTileClass();
var clDirt = createTileClass();
var clRock = createTileClass();
var clMetal = createTileClass();
var clFood = createTileClass();
var clBaseResource = createTileClass();

var [playerIDs, playerX, playerZ] = playerPlacementLine(true, 0.45, 0.2);

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
	var bbX = round(fx + bbDist * cos(bbAngle));
	var bbZ = round(fz + bbDist * sin(bbAngle));
	var mAngle = bbAngle;
	while(abs(mAngle - bbAngle) < PI/3)
	{
		mAngle = randFloat(0, TWO_PI);
	}
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
	var hillSize = PI * radius * radius;
	// create starting trees
	var num = floor(hillSize / 60);
	var tAngle = randFloat(-PI/3, 4*PI/3);
	var tDist = randFloat(12, 13);
	var tX = round(fx + tDist * cos(tAngle));
	var tZ = round(fz + tDist * sin(tAngle));
	group = new SimpleGroup(
		[new SimpleObject(oPine, num, num, 0,3)],
		false, clBaseResource, tX, tZ
	);
	createObjectGroup(group, 0, avoidClasses(clBaseResource,2));

}

Engine.SetProgress(15);

paintRiver({
	"parallel": true,
	"startX": 0,
	"startZ": 1,
	"endX": 1,
	"endZ": 1,
	"width": 0.62,
	"fadeDist": tilesToFraction(8),
	"deviation": 0,
	"waterHeight": -5,
	"landHeight": 3,
	"meanderShort": 0,
	"meanderLong": 0,
	"waterFunc": (ix, iz, height, riverFraction) => {
		addToClass(ix, iz, clWater);
	},
	"landFunc": (ix, iz, shoreDist1, shoreDist2) => {
		if (getHeight(ix, iz) < 0.5)
			addToClass(ix, iz, clWater);
	}
});

log("Creating shores...");
for (var i = 0; i < scaleByMapSize(20,120); i++)
	createArea(
		new ChainPlacer(
			1,
			Math.floor(scaleByMapSize(4, 6)),
			Math.floor(scaleByMapSize(16, 30)),
			1,
			randIntExclusive(0.1 * mapSize, 0.9 * mapSize),
			randIntExclusive(0.67 * mapSize, 0.74 * mapSize)),
		[
			new LayeredPainter([tSnowA, tSnowA], [2]),
			new SmoothElevationPainter(ELEVATION_SET, 3, 3), unPaintClass(clWater)
		],
		null);

log("Creating islands...");
createAreas(
	new ChainPlacer(1, Math.floor(scaleByMapSize(4, 6)), Math.floor(scaleByMapSize(16, 40)), 0.1),
	[
		new LayeredPainter([tSnowA, tSnowA], [3]),
		new SmoothElevationPainter(ELEVATION_SET, 3, 3),
		unPaintClass(clWater)
	],
	stayClasses(clWater, 7),
	scaleByMapSize(10, 80)
);

paintTerrainBasedOnHeight(-6, 1, 1, tWater);

log("Creating lakes...");
createAreas(
	new ChainPlacer(1, Math.floor(scaleByMapSize(5, 7)), Math.floor(scaleByMapSize(20, 50)), 0.1),
	[
		new LayeredPainter([tShoreBlend, tShore, tWater], [1, 1]),
		new SmoothElevationPainter(ELEVATION_SET, -4, 3),
		paintClass(clWater)
	],
	avoidClasses(clPlayer, 20, clWater, 20),
	Math.round(scaleByMapSize(1, 4) * numPlayers));

paintTerrainBasedOnHeight(1, 2.8, 1, tShoreBlend);
paintTileClassBasedOnHeight(-6, 0.5, 1, clWater);

Engine.SetProgress(45);

log("Creating hills...");
createAreas(
	new ChainPlacer(1, Math.floor(scaleByMapSize(4, 6)), Math.floor(scaleByMapSize(16, 40)), 0.1),
	[
		new LayeredPainter([tCliff, tSnowA], [3]),
		new SmoothElevationPainter(ELEVATION_SET, 25, 3),
		paintClass(clHill)
	],
	avoidClasses(clPlayer, 20, clHill, 15, clWater, 2, clBaseResource, 2),
	scaleByMapSize(1, 4) * numPlayers
);

log("Creating forests...");
var [forestTrees, stragglerTrees] = getTreeCounts(100, 625, 0.7);
var types = [
	[[tSnowA, tSnowA, tSnowA, tSnowA, pForestD], [tSnowA, tSnowA, tSnowA, pForestD]],
	[[tSnowA, tSnowA, tSnowA, tSnowA, pForestS], [tSnowA, tSnowA, tSnowA, pForestS]]
];

var size = forestTrees / (scaleByMapSize(3,6) * numPlayers);

var num = floor(size / types.length);
for (let type of types)
	createAreas(
		new ChainPlacer(1, Math.floor(scaleByMapSize(3, 5)), forestTrees / (num * Math.floor(scaleByMapSize(2, 4))), 1),
		[
			new LayeredPainter(type, [2]),
			paintClass(clForest)
		],
		avoidClasses(clPlayer, 20, clForest, 20, clHill, 0, clWater, 8),
		num);

log("Creating iceberg...");
createObjectGroupsDeprecated(
	new SimpleGroup([new SimpleObject(aIceberg, 0, 2, 0, 4)], true, clRock),
	0,
	[avoidClasses(clRock, 6), stayClasses(clWater, 4)],
	scaleByMapSize(4, 16),
	100);
Engine.SetProgress(70);

log("Creating dirt patches...");
for (let size of [scaleByMapSize(3, 6), scaleByMapSize(5, 10), scaleByMapSize(8, 21)])
	createAreas(
		new ChainPlacer(1, Math.floor(scaleByMapSize(3, 5)), size, 0.5),
		[
			new LayeredPainter([tSnowD, tSnowB, tSnowC], [2, 1]),
			paintClass(clDirt)
		],
		avoidClasses(
			clWater, 8,
			clForest, 0,
			clHill, 0,
			clPlayer, 20,
			clDirt, 16),
		scaleByMapSize(20, 80));

for (let size of [scaleByMapSize(2, 4), scaleByMapSize(3, 7), scaleByMapSize(5, 15)])
	createAreas(
		new ChainPlacer(1, Math.floor(scaleByMapSize(3, 5)), size, 0.5),
		[
			new LayeredPainter([tSnowE, tSnowE], [1]),
			paintClass(clDirt)
		],
		avoidClasses(
			clWater, 8,
			clForest, 0,
			clHill, 0,
			clPlayer, 20,
			clDirt, 16),
		scaleByMapSize(20, 80));

log("Creating stone mines...");
var group = new SimpleGroup([new SimpleObject(oStoneSmall, 0, 2, 0, 4), new SimpleObject(oStoneLarge, 1, 1, 0, 4)], true, clRock);
createObjectGroupsDeprecated(group, 0,
	avoidClasses(clWater, 3, clForest, 1, clPlayer, 20, clRock, 10, clHill, 1),
	scaleByMapSize(8,32), 100
);

log("Creating small stone quarries...");
group = new SimpleGroup([new SimpleObject(oStoneSmall, 2,5, 1,3)], true, clRock);
createObjectGroupsDeprecated(group, 0,
	avoidClasses(clWater, 3, clForest, 1, clPlayer, 20, clRock, 10, clHill, 1),
	scaleByMapSize(8,32), 100
);

log("Creating metal mines...");
group = new SimpleGroup([new SimpleObject(oMetalLarge, 1,1, 0,4)], true, clMetal);
createObjectGroupsDeprecated(group, 0,
	avoidClasses(clWater, 3, clForest, 1, clPlayer, 20, clMetal, 10, clRock, 5, clHill, 1),
	scaleByMapSize(8,32), 100
);
Engine.SetProgress(95);

createStragglerTrees(
	[oPine],
	avoidClasses(clWater, 5, clForest, 1, clHill, 1, clPlayer, 12, clMetal, 6, clRock, 6),
	clForest,
	stragglerTrees);

log("Creating deer...");
group = new SimpleGroup(
	[new SimpleObject(oWalrus, 5,7, 0,4)],
	true, clFood
);
createObjectGroupsDeprecated(group, 0,
	avoidClasses(clWater, 3, clForest, 0, clPlayer, 20, clHill, 1, clFood, 20),
	3 * numPlayers, 50
);

Engine.SetProgress(75);

log("Creating sheep...");
group = new SimpleGroup(
	[new SimpleObject(oArcticWolf, 2,3, 0,2)],
	true, clFood
);
createObjectGroupsDeprecated(group, 0,
	avoidClasses(clWater, 3, clForest, 0, clPlayer, 20, clHill, 1, clFood, 20),
	3 * numPlayers, 50
);

log("Creating fish...");
group = new SimpleGroup(
	[new SimpleObject(oFish, 2,3, 0,2)],
	true, clFood
);
createObjectGroupsDeprecated(group, 0,
	[avoidClasses(clFood, 20), stayClasses(clWater, 6)],
	25 * numPlayers, 60
);

setSunColor(0.6, 0.6, 0.6);
setSunElevation(PI/ 6);

setWaterColor(0.02, 0.17, 0.52);
setWaterTint(0.494, 0.682, 0.808);
setWaterMurkiness(0.82);
setWaterWaviness(0.5);
setWaterType("ocean");

setFogFactor(0.95);
setFogThickness(0.09);
setPPSaturation(0.28);
setPPEffect("hdr");

setSkySet("fog");
ExportMap();
