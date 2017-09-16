RMS.LoadLibrary("rmgen");

const tGrass = ["medit_grass_field_a", "medit_grass_field_b"];
const tForestFloorC = "medit_plants_dirt";
const tForestFloorP = "medit_grass_shrubs";
const tCliff = ["medit_cliff_grass", "medit_cliff_greek", "medit_cliff_greek_2", "medit_cliff_aegean", "medit_cliff_italia", "medit_cliff_italia_grass"];
const tGrassA = "medit_grass_field_b";
const tGrassB = "medit_grass_field_brown";
const tGrassC = "medit_grass_field_dry";
const tHill = ["medit_rocks_grass_shrubs", "medit_rocks_shrubs"];
const tDirt = ["medit_dirt", "medit_dirt_b"];
const tRoad = "medit_city_tile";
const tRoadWild = "medit_city_tile";
const tGrassPatch = "medit_grass_shrubs";
const tShoreBlend = "medit_sand";
const tShore = "sand_grass_25";
const tWater = "medit_sand_wet";

const oPoplar = "gaia/flora_tree_poplar";
const oApple = "gaia/flora_tree_apple";
const oCarob = "gaia/flora_tree_carob";
const oBerryBush = "gaia/flora_bush_berry";
const oDeer = "gaia/fauna_deer";
const oFish = "gaia/fauna_fish";
const oSheep = "gaia/fauna_sheep";
const oStoneLarge = "gaia/geology_stonemine_medit_quarry";
const oStoneSmall = "gaia/geology_stone_mediterranean";
const oMetalLarge = "gaia/geology_metal_mediterranean_slabs";

const aGrass = "actor|props/flora/grass_soft_large_tall.xml";
const aGrassShort = "actor|props/flora/grass_soft_large.xml";
const aReeds = "actor|props/flora/reeds_pond_lush_a.xml";
const aLillies = "actor|props/flora/water_lillies.xml";
const aRockLarge = "actor|geology/stone_granite_large.xml";
const aRockMedium = "actor|geology/stone_granite_med.xml";
const aBushMedium = "actor|props/flora/bush_medit_me.xml";
const aBushSmall = "actor|props/flora/bush_medit_sm.xml";

const pForestP = [tForestFloorP + TERRAIN_SEPARATOR + oPoplar, tForestFloorP];
const pForestC = [tForestFloorC + TERRAIN_SEPARATOR + oCarob, tForestFloorC];

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
var clSettlement = createTileClass();
var clLand = createTileClass();
var clRiver = createTileClass();
var clShallow = createTileClass();

log("Create the continent body");
createArea(
	new ChainPlacer(
		2,
		Math.floor(scaleByMapSize(5, 12)),
		Math.floor(scaleByMapSize(60, 700)),
		1,
		Math.floor(fractionToTiles(0.5)),
		Math.floor(fractionToTiles(0.7)),
		0,
		[Math.floor(mapSize * 0.49)]),
	[
		new LayeredPainter([tGrass, tGrass, tGrass], [4, 2]),
		new SmoothElevationPainter(ELEVATION_SET, 3, 4),
		paintClass(clLand)
	],
	null);

var playerIDs = primeSortAllPlayers();

// place players
var playerX = [];
var playerZ = [];
var playerAngle = [];

var startAngle = 0;
for (var i = 0; i < numPlayers; i++)
{
	playerAngle[i] = startAngle - 0.23*(i+(i%2))*TWO_PI/numPlayers - (i%2)*PI/2;
	playerX[i] = 0.5 + 0.35*cos(playerAngle[i]);
	playerZ[i] = 0.7 + 0.35*sin(playerAngle[i]);
}

for (var i = 0; i < numPlayers; i++)
{
	var id = playerIDs[i];
	log("Creating base for player " + id + "...");

	var radius = scaleByMapSize(15,25);
	var cliffRadius = 2;
	var elevation = 20;

	// get the x and z in tiles
	let fx = fractionToTiles(playerX[i]);
	let fz = fractionToTiles(playerZ[i]);
	let ix = Math.round(fx);
	let iz = Math.round(fz);
	addCivicCenterAreaToClass(ix, iz, clPlayer);

	// create the city patch
	var cityRadius = radius/3;
	var placer = new ClumpPlacer(PI*cityRadius*cityRadius, 0.6, 0.3, 10, ix, iz);
	var painter = new LayeredPainter([tRoadWild, tRoad], [1]);
	createArea(placer, painter, null);

	placeCivDefaultEntities(fx, fz, id, { 'iberWall': false });

	placeDefaultChicken(fx, fz, clBaseResource);

	// create berry bushes
	var bbAngle = randFloat(0, TWO_PI);
	var bbDist = 8;
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
	var mDist = 10;
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
	var num = 2;
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

var shallowHeight = -1.5;
paintRiver({
	"horizontal": false,
	"parallel": true,
	"constraint": stayClasses(clLand, 0),
	"position": 0.5,
	"width": 0.07,
	"fadeDist": 0.025,
	"deviation": 0.005,
	"waterHeight": -3,
	"landHeight": 2,
	"meanderShort": 12,
	"meanderLong": 0,
	"waterFunc": (ix, iz, height) => {
		addToClass(ix, iz, clRiver);
		placeTerrain(ix, iz, tWater);

		let z = iz / (mapSize + 1.0);

		if (height < shallowHeight && (
				z > 0.3 && z < 0.4 ||
				z > 0.5 && z < 0.6 ||
				z > 0.7 && z < 0.8))
		{
			setHeight(ix, iz, shallowHeight);
			addToClass(ix, iz, clShallow);
		}
	}
});

paintTerrainBasedOnHeight(1, 3, 0, tShore);
paintTerrainBasedOnHeight(-8, 1, 2, tWater);

createBumps([avoidClasses(clPlayer, 20, clRiver, 1), stayClasses(clLand, 3)]);

createForests(
 [tGrass, tForestFloorP, tForestFloorC, pForestC, pForestP],
 [avoidClasses(clPlayer, 20, clForest, 17, clHill, 0, clRiver, 1), stayClasses(clLand, 7)],
 clForest,
 1.0);

RMS.SetProgress(50);

log("Creating dirt patches...");
createLayeredPatches(
 [scaleByMapSize(3, 6), scaleByMapSize(5, 10), scaleByMapSize(8, 21)],
 [[tGrass,tGrassA],[tGrassA,tGrassB], [tGrassB,tGrassC]],
 [1,1],
 [avoidClasses(clForest, 0, clHill, 0, clDirt, 3, clPlayer, 8, clRiver, 1), stayClasses(clLand, 7)]
);

log("Creating grass patches...");
createPatches(
 [scaleByMapSize(2, 4), scaleByMapSize(3, 7), scaleByMapSize(5, 15)],
 tGrassPatch,
 [avoidClasses(clForest, 0, clHill, 0, clDirt, 3, clPlayer, 8, clRiver, 1), stayClasses(clLand, 7)]
);

RMS.SetProgress(55);

log("Creating stone mines...");
createMines(
 [
  [new SimpleObject(oStoneSmall, 0,2, 0,4), new SimpleObject(oStoneLarge, 1,1, 0,4)],
  [new SimpleObject(oStoneSmall, 2,5, 1,3)]
 ],
 [avoidClasses(clForest, 1, clPlayer, 20, clRock, 10, clHill, 1, clRiver, 1), stayClasses(clLand, 5)]
);

log("Creating metal mines...");
createMines(
 [
  [new SimpleObject(oMetalLarge, 1,1, 0,4)]
 ],
 [avoidClasses(clForest, 1, clPlayer, 20, clMetal, 10, clRock, 5, clHill, 1, clRiver, 1), stayClasses(clLand, 5)],
 clMetal
);
RMS.SetProgress(65);

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
 [avoidClasses(clHill, 1, clPlayer, 1, clDirt, 1, clRiver, 1), stayClasses(clLand, 6)]
);

log("Create water decoration in the shallow parts");
createDecoration
(
 [[new SimpleObject(aReeds, 1,3, 0,1)],
  [new SimpleObject(aLillies, 1,2, 0,1)]
 ],
 [
  scaleByMapSize(800, 12800),
  scaleByMapSize(800, 12800)
 ],
 stayClasses(clShallow, 0)
);

RMS.SetProgress(70);

createFood
(
 [
  [new SimpleObject(oDeer, 5,7, 0,4)],
  [new SimpleObject(oSheep, 2,3, 0,2)]
 ],
 [
  3 * numPlayers,
  3 * numPlayers
 ],
 [avoidClasses(clForest, 0, clPlayer, 20, clHill, 1, clFood, 20, clRiver, 1), stayClasses(clLand, 3)]
);

createFood
(
 [
  [new SimpleObject(oBerryBush, 5,7, 0,4)]
 ],
 [
  randIntInclusive(1, 4) * numPlayers + 2
 ],
 [avoidClasses(clForest, 0, clPlayer, 20, clHill, 1, clFood, 10, clRiver, 1), stayClasses(clLand, 3)]
);

createFood
(
 [
  [new SimpleObject(oFish, 2,3, 0,2)]
 ],
 [
  25 * numPlayers
 ],
 avoidClasses(clLand, 2, clRiver, 1)
);

RMS.SetProgress(85);

log("Creating straggler trees...");
var types = [oPoplar, oCarob, oApple];	// some variation
createStragglerTrees(types, [avoidClasses(clForest, 1, clHill, 1, clPlayer, 9, clMetal, 6, clRock, 6, clRiver, 1), stayClasses(clLand, 7)]);

setSkySet("cumulus");
setWaterColor(0.2,0.312,0.522);
setWaterTint(0.1,0.1,0.8);
setWaterWaviness(4.0);
setWaterType("lake");
setWaterMurkiness(0.73);

setFogFactor(0.3);
setFogThickness(0.25);

setPPEffect("hdr");
setPPContrast(0.62);
setPPSaturation(0.51);
setPPBloom(0.12);

ExportMap();
