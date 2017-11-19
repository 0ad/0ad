RMS.LoadLibrary("rmgen");

const tGrass = ["steppe_grass_a", "steppe_grass_b", "steppe_grass_c", "steppe_grass_d"];
const tForestFloor = "steppe_grass_c";
const tGrassA = "steppe_grass_b";
const tGrassB = "steppe_grass_c";
const tGrassC = ["steppe_grass_b", "steppe_grass_c", "steppe_grass_d"];
const tGrassD = "steppe_grass_a";
const tDirt = ["steppe_dirt_a", "steppe_dirt_b"];
const tRoad = "road_stones";
const tRoadWild = "road_stones";

const oPoplar = "gaia/flora_tree_poplar_lombardy";
const oBush = "gaia/flora_bush_temperate";
const oBerryBush = "gaia/flora_bush_berry";
const oRabbit = "gaia/fauna_rabbit";
const oSheep = "gaia/fauna_sheep";
const oStoneLarge = "gaia/geology_stonemine_medit_quarry";
const oStoneSmall = "gaia/geology_stone_mediterranean";
const oMetalLarge = "gaia/geology_metal_mediterranean_slabs";

const aGrass = "actor|props/flora/grass_soft_small_tall.xml";
const aGrassShort = "actor|props/flora/grass_soft_large.xml";
const aRockLarge = "actor|geology/stone_granite_med.xml";
const aRockMedium = "actor|geology/stone_granite_med.xml";
const aBushMedium = "actor|props/flora/bush_medit_me.xml";
const aBushSmall = "actor|props/flora/bush_medit_sm.xml";

const pForest = [tForestFloor + TERRAIN_SEPARATOR + oPoplar, tForestFloor];

InitMap();

var numPlayers = getNumPlayers();

var clPlayer = createTileClass();
var clHill = createTileClass();
var clForest = createTileClass();
var clDirt = createTileClass();
var clRock = createTileClass();
var clMetal = createTileClass();
var clFood = createTileClass();
var clBaseResource = createTileClass();

var [playerIDs, playerX, playerZ] = radialPlayerPlacement();

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
		[new SimpleObject(oPoplar, num, num, 0,5)],
		false, clBaseResource, tX, tZ
	);
	createObjectGroup(group, 0, avoidClasses(clBaseResource,2));

	placeDefaultDecoratives(fx, fz, aGrassShort, clBaseResource, radius);
}

RMS.SetProgress(20);

log("Creating bumps...");
createAreas(
	new ChainPlacer(1, Math.floor(scaleByMapSize(4, 6)), Math.floor(scaleByMapSize(2, 5)), 0.5),
	new SmoothElevationPainter(ELEVATION_MODIFY, 2, 2),
	avoidClasses(clPlayer, 13),
	scaleByMapSize(300, 800));

log("Creating forests...");
var [forestTrees, stragglerTrees] = getTreeCounts(220, 1000, 0.65);
var types = [[[tForestFloor, tGrass, pForest], [tForestFloor, pForest]]];
var size = forestTrees / (scaleByMapSize(2,8) * numPlayers);
var num = 4 * floor(size / types.length);
for (let type of types)
	createAreas(
		new ChainPlacer(1, Math.floor(scaleByMapSize(2, 3)), 4, 1),
		[
			new LayeredPainter(type, [2]),
			paintClass(clForest)
		],
		avoidClasses(clPlayer, 13, clForest, 20, clHill, 1),
		num);
RMS.SetProgress(50);

log("Creating grass patches...");
createLayeredPatches(
 [scaleByMapSize(5, 48), scaleByMapSize(6, 84), scaleByMapSize(8, 128)],
 [[tGrass,tGrassA,tGrassC],[tGrass,tGrassA,tGrassC], [tGrass,tGrassA,tGrassC]],
 [1,1],
 avoidClasses(clForest, 0, clHill, 0, clDirt, 2, clPlayer, 10),
 scaleByMapSize(50, 70),
 clDirt);

log("Creating dirt patches...");
createLayeredPatches(
 [scaleByMapSize(5, 32), scaleByMapSize(6, 48), scaleByMapSize(7, 80)],
 [tGrassD ,tDirt],
 [1],
 avoidClasses(clForest, 0, clHill, 0, clDirt, 2, clPlayer, 10),
 scaleByMapSize(50, 90),
 clDirt);

RMS.SetProgress(55);

log("Creating big patches...");
createLayeredPatches(
 [scaleByMapSize(10, 60), scaleByMapSize(15, 90), scaleByMapSize(20, 120)],
 [tGrassB ,tGrassA],
 [1],
 avoidClasses(clHill, 0, clPlayer, 8),
 scaleByMapSize(30, 90),
 clDirt);

RMS.SetProgress(55);

log("Creating stone mines...");
var group = new SimpleGroup([new SimpleObject(oStoneSmall, 0,2, 0,4), new SimpleObject(oStoneLarge, 1,1, 0,4)], true, clRock);
createObjectGroupsDeprecated(group, 0,
	avoidClasses(clForest, 1, clPlayer, 20, clRock, 10, clHill, 1),
	scaleByMapSize(1,4), 100
);

log("Creating small stone quarries...");
group = new SimpleGroup([new SimpleObject(oStoneSmall, 2,5, 1,3)], true, clRock);
createObjectGroupsDeprecated(group, 0,
	avoidClasses(clForest, 1, clPlayer, 20, clRock, 10, clHill, 1),
	scaleByMapSize(1,4), 100
);

log("Creating metal mines...");
group = new SimpleGroup([new SimpleObject(oMetalLarge, 1,1, 0,4)], true, clMetal);
createObjectGroupsDeprecated(group, 0,
	avoidClasses(clForest, 1, clPlayer, 20, clMetal, 10, clRock, 5, clHill, 1),
	scaleByMapSize(2,8), 100
);

RMS.SetProgress(65);

log("Creating small decorative rocks...");
group = new SimpleGroup(
	[new SimpleObject(aRockMedium, 1,3, 0,1)],
	true
);
createObjectGroupsDeprecated(
	group, 0,
	avoidClasses(clForest, 0, clPlayer, 10, clHill, 0),
	scaleByMapSize(16, 262), 50
);

log("Creating large decorative rocks...");
group = new SimpleGroup(
	[new SimpleObject(aRockLarge, 1,2, 0,1), new SimpleObject(aRockMedium, 1,3, 0,2)],
	true
);
createObjectGroupsDeprecated(
	group, 0,
	avoidClasses(clForest, 0, clPlayer, 10, clHill, 0),
	scaleByMapSize(8, 131), 50
);

RMS.SetProgress(70);

log("Creating rabbits...");
group = new SimpleGroup(
	[new SimpleObject(oRabbit, 5,7, 0,4)],
	true, clFood
);
createObjectGroupsDeprecated(group, 0,
	avoidClasses(clForest, 0, clPlayer, 10, clHill, 1, clFood, 20),
	6 * numPlayers, 50
);

log("Creating berry bush...");
group = new SimpleGroup(
	[new SimpleObject(oBerryBush, 5,7, 0,4)],
	true, clFood
);
createObjectGroupsDeprecated(group, 0,
	avoidClasses(clForest, 0, clPlayer, 20, clHill, 1, clFood, 20),
	randIntInclusive(1, 4) * numPlayers + 2, 50
);

RMS.SetProgress(75);

log("Creating sheep...");
group = new SimpleGroup(
	[new SimpleObject(oSheep, 2,3, 0,2)],
	true, clFood
);
createObjectGroupsDeprecated(group, 0,
	avoidClasses(clForest, 0, clPlayer, 10, clHill, 1, clFood, 20),
	3 * numPlayers, 50
);

RMS.SetProgress(85);

createStragglerTrees(
	[oBush, oPoplar],
	avoidClasses(clForest, 1, clHill, 1, clPlayer, 13, clMetal, 6, clRock, 6),
	clForest,
	stragglerTrees);

log("Creating large grass tufts...");
group = new SimpleGroup(
	[new SimpleObject(aGrass, 2,4, 0,1.8, -PI/8,PI/8), new SimpleObject(aGrassShort, 3,6, 1.2,2.5, -PI/8,PI/8)]
);
createObjectGroupsDeprecated(group, 0,
	avoidClasses(clHill, 2, clPlayer, 2, clDirt, 1, clForest, 0),
	scaleByMapSize(13, 200)
);

RMS.SetProgress(95);

log("Creating bushes...");
group = new SimpleGroup(
	[new SimpleObject(aBushMedium, 1,2, 0,2), new SimpleObject(aBushSmall, 2,4, 0,2)]
);
createObjectGroupsDeprecated(group, 0,
	avoidClasses(clHill, 1, clPlayer, 1, clDirt, 1),
	scaleByMapSize(13, 200), 50
);

setFogThickness(0.1);
setFogFactor(0.2);

setPPEffect("hdr");
setPPSaturation(0.45);
setPPContrast(0.62);
setPPBloom(0.2);

ExportMap();
