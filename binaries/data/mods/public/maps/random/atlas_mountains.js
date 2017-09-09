RMS.LoadLibrary("rmgen");

const tGrass = ["medit_rocks_grass_shrubs", "medit_rocks_shrubs"];
const tForestFloor = "medit_grass_field_dry";
const tCliff = "medit_cliff_italia";
const tHill = ["medit_rocks_grass", "medit_rocks_grass_shrubs", "medit_rocks_shrubs"];
const tGrassDirt = "medit_rocks_grass";
const tDirt = "medit_dirt";
const tRoad = "medit_city_tile";
const tRoadWild = "medit_city_tile";
const tGrass2 = "medit_rocks_grass_shrubs";
const tGrassPatch = "medit_grass_wild";
const tShoreBlend = "medit_sand";
const tShore = "medit_sand";
const tWater = "medit_sand";

const oCarob = "gaia/flora_tree_carob";
const oAleppoPine = "gaia/flora_tree_aleppo_pine";
const oBerryBush = "gaia/flora_bush_berry";
const oDeer = "gaia/fauna_deer";
const oFish = "gaia/fauna_fish";
const oSheep = "gaia/fauna_sheep";
const oStoneLarge = "gaia/geology_stonemine_medit_quarry";
const oStoneSmall = "gaia/geology_stone_mediterranean";
const oMetalLarge = "gaia/geology_metal_mediterranean_slabs";
const oWood = "gaia/special_treasure_wood";
const oFood = "gaia/special_treasure_food_bin";

const aGrass = "actor|props/flora/grass_soft_large_tall.xml";
const aGrassShort = "actor|props/flora/grass_soft_large.xml";
const aRockLarge = "actor|geology/stone_granite_large.xml";
const aRockMedium = "actor|geology/stone_granite_med.xml";
const aBushMedium = "actor|props/flora/bush_medit_me.xml";
const aBushSmall = "actor|props/flora/bush_medit_sm.xml";
const aCarob = "actor|flora/trees/carob.xml";
const aAleppoPine = "actor|flora/trees/aleppo_pine.xml";

// terrain + entity (for painting)
const pForest1 = [tForestFloor + TERRAIN_SEPARATOR + oCarob, tForestFloor];
const pForest2 = [tForestFloor + TERRAIN_SEPARATOR + oAleppoPine, tForestFloor];

InitMap();

const numPlayers = getNumPlayers();
const mapSize = getMapSize();
const mapArea = mapSize*mapSize;

var clPlayer = createTileClass();
var clHill = createTileClass();
var clForest = createTileClass();
var clDirt = createTileClass();
var clRock = createTileClass();
var clMetal = createTileClass();
var clFood = createTileClass();
var clBaseResource = createTileClass();
var clTreasure = createTileClass();
var clGrass = createTileClass();

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
	addToClass(ix, iz, clPlayer);
	addToClass(ix+5, iz, clPlayer);
	addToClass(ix, iz+5, clPlayer);
	addToClass(ix-5, iz, clPlayer);
	addToClass(ix, iz-5, clPlayer);

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
	// create starting trees
	var num = 4;
	var tAngle = randFloat(-PI/3, 4*PI/3);
	var tDist = randFloat(11, 13);
	var tX = round(fx + tDist * cos(tAngle));
	var tZ = round(fz + tDist * sin(tAngle));
	group = new SimpleGroup(
		[new SimpleObject(oCarob, num, num, 0,5)],
		false, clBaseResource, tX, tZ
	);
	createObjectGroup(group, 0, avoidClasses(clBaseResource,2));

	placeDefaultDecoratives(fx, fz, aGrassShort, clBaseResource, radius);
}

RMS.SetProgress(10);

createBumps(avoidClasses(clPlayer, 9));

createMountains(tCliff, avoidClasses(clPlayer, 20, clHill, 8), clHill, scaleByMapSize(20, 120));

RMS.SetProgress(25);

createForests(
 [tGrass, tForestFloor, tForestFloor, pForest1, pForest2],
 avoidClasses(clPlayer, 20, clForest, 14, clHill, 1),
 clForest,
 0.6);

RMS.SetProgress(40);

log("Creating dirt patches...");
createLayeredPatches(
 [scaleByMapSize(3, 6), scaleByMapSize(5, 10), scaleByMapSize(8, 21)],
 [tGrassDirt,tDirt],
 [2],
 avoidClasses(clForest, 0, clHill, 0, clDirt, 3, clPlayer, 10)
);

log("Creating grass patches...");
createLayeredPatches(
 [scaleByMapSize(2, 4), scaleByMapSize(3, 7), scaleByMapSize(5, 15)],
 [tGrass2,tGrassPatch],
 [1],
 avoidClasses(clForest, 0, clHill, 0, clDirt, 3, clPlayer, 10, clGrass, 15)
);

RMS.SetProgress(50);

log("Creating stone mines...");
createMines(
 [
  [new SimpleObject(oStoneSmall, 0,2, 0,4), new SimpleObject(oStoneLarge, 1,1, 0,4)],
  [new SimpleObject(oStoneSmall, 2,5, 1,3)]
 ],
 avoidClasses(clForest, 1, clPlayer, 20, clMetal, 10, clRock, 5, clHill, 2)
);

log("Creating metal mines...");
createMines(
 [
  [new SimpleObject(oMetalLarge, 1,1, 0,4)]
 ],
 avoidClasses(clForest, 1, clPlayer, 20, clMetal, 10, clRock, 5, clHill, 2),
 clMetal
);

RMS.SetProgress(60);

createDecoration
(
 [[new SimpleObject(aRockMedium, 1,3, 0,1)],
  [new SimpleObject(aRockLarge, 1,2, 0,1), new SimpleObject(aRockMedium, 1,3, 0,2)],
  [new SimpleObject(aGrassShort, 1,2, 0,1, -PI/8,PI/8)],
  [new SimpleObject(aGrass, 2,4, 0,1.8, -PI/8,PI/8), new SimpleObject(aGrassShort, 3,6, 1.2,2.5, -PI/8,PI/8)],
  [new SimpleObject(aBushMedium, 1,2, 0,2), new SimpleObject(aBushSmall, 2,4, 0,2)]
 ],
 [
  scaleByMapSize(16, 262),
  scaleByMapSize(8, 131),
  scaleByMapSize(13, 200),
  scaleByMapSize(13, 200),
  scaleByMapSize(13, 200)
 ],
 avoidClasses(clForest, 0, clPlayer, 0, clHill, 0)
);

RMS.SetProgress(75);

createFood
(
 [
  [new SimpleObject(oSheep, 5,7, 0,4)],
  [new SimpleObject(oDeer, 2,3, 0,2)]
 ],
 [
  3 * numPlayers,
  3 * numPlayers
 ]
);

createFood
(
 [
  [new SimpleObject(oBerryBush, 5,7, 0,4)]
 ],
 [
  randIntInclusive(3, 12) * numPlayers + 2
 ],
 avoidClasses(clForest, 0, clPlayer, 20, clHill, 1, clFood, 10)
);

log("Creating food treasures...");
group = new SimpleGroup(
	[new SimpleObject(oFood, 2,3, 0,2)],
	true, clTreasure
);
createObjectGroupsDeprecated(group, 0,
	avoidClasses(clForest, 0, clPlayer, 18, clHill, 1, clFood, 5),
	3 * numPlayers, 50
);

log("Creating food treasures...");
group = new SimpleGroup(
	[new SimpleObject(oWood, 2,3, 0,2)],
	true, clTreasure
);
createObjectGroupsDeprecated(group, 0,
	avoidClasses(clForest, 0, clPlayer, 18, clHill, 1),
	3 * numPlayers, 50
);

RMS.SetProgress(80);

var types = [oCarob, oAleppoPine];	// some variation
createStragglerTrees(types, avoidClasses(clForest, 1, clHill, 1, clPlayer, 10, clMetal, 6, clRock, 6, clTreasure, 4));

log("Creating hill trees...");
var types = [aCarob, aAleppoPine];	// some variation
var num = floor(0.2 * g_numStragglerTrees / types.length);
for (var i = 0; i < types.length; ++i)
{
	group = new SimpleGroup(
		[new SimpleObject(types[i], 1,1, 0,3)],
		true, clForest
	);
	createObjectGroupsDeprecated(group, 0,
		stayClasses(clHill, 2),
		num
	);
}

setFogFactor(0.2);
setFogThickness(0.14);

setPPEffect("hdr");
setPPContrast(0.45);
setPPSaturation(0.56);
setPPBloom(0.1);

ExportMap();
