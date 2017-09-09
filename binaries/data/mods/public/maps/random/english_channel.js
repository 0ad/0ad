RMS.LoadLibrary("rmgen");

const tGrass = ["temp_grass", "temp_grass", "temp_grass_d"];
const tGrassPForest = "temp_plants_bog";
const tGrassDForest = "temp_plants_bog";
const tGrassA = "temp_grass_plants";
const tGrassB = "temp_plants_bog";
const tGrassC = "temp_mud_a";
const tDirt = ["temp_plants_bog", "temp_mud_a"];
const tHill = ["temp_highlands", "temp_grass_long_b"];
const tCliff = ["temp_cliff_a", "temp_cliff_b"];
const tRoad = "temp_road";
const tRoadWild = "temp_road_overgrown";
const tGrassPatchBlend = "temp_grass_long_b";
const tGrassPatch = ["temp_grass_d", "temp_grass_clovers"];
const tShoreBlend = "temp_grass_plants";
const tShore = "temp_dirt_gravel";
const tWater = "temp_dirt_gravel_b";

const oBeech = "gaia/flora_tree_euro_beech";
const oPoplar = "gaia/flora_tree_poplar";
const oApple = "gaia/flora_tree_apple";
const oOak = "gaia/flora_tree_oak";
const oBerryBush = "gaia/flora_bush_berry";
const oDeer = "gaia/fauna_deer";
const oFish = "gaia/fauna_fish";
const oGoat = "gaia/fauna_goat";
const oBoar = "gaia/fauna_boar";
const oStoneLarge = "gaia/geology_stonemine_temperate_quarry";
const oStoneSmall = "gaia/geology_stone_temperate";
const oMetalLarge = "gaia/geology_metal_temperate_slabs";

const aGrass = "actor|props/flora/grass_soft_large_tall.xml";
const aGrassShort = "actor|props/flora/grass_soft_large.xml";
const aRockLarge = "actor|geology/stone_granite_large.xml";
const aRockMedium = "actor|geology/stone_granite_med.xml";
const aBushMedium = "actor|props/flora/bush_medit_me_lush.xml";
const aBushSmall = "actor|props/flora/bush_medit_sm_lush.xml";
const aReeds = "actor|props/flora/reeds_pond_lush_a.xml";
const aLillies = "actor|props/flora/water_lillies.xml";

// terrain + entity (for painting)
const pForestD = [tGrassDForest + TERRAIN_SEPARATOR + oBeech, tGrassDForest];
const pForestP = [tGrassPForest + TERRAIN_SEPARATOR + oOak, tGrassPForest];

const WATER_WIDTH = 0.25;

InitMap();

const numPlayers = getNumPlayers();
const mapSize = getMapSize();
const mapArea = mapSize*mapSize;

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
var clShallow = createTileClass();

var playerIDs = primeSortAllPlayers();

// place players
var playerX = [];
var playerZ = [];
var playerAngle = [];
var playerPos = [];
var iop = 0;

for (var i = 0; i < numPlayers; i++)
{
	iop = i - 1;
	if (numPlayers % 2 == 0)
		playerPos[i] = ((iop + abs(iop % 2)) / 2 + 1) / ((numPlayers / 2) + 1);
	else if (iop % 2 != 0)
		playerPos[i] = ((iop + abs(iop % 2)) / 2 + 1) / (((numPlayers + 1) / 2) + 1);
	else
		playerPos[i] = (iop / 2 + 1) / (((numPlayers - 1) / 2) + 1);

	playerZ[i] = 0.2 + 0.6*(i%2);
	playerX[i] = playerPos[i];
}

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
	// Setting tile class
	addToClass(ix, iz, clPlayer);
	addToClass(ix+5, iz, clPlayer);
	addToClass(ix, iz+5, clPlayer);
	addToClass(ix-5, iz, clPlayer);
	addToClass(ix, iz-5, clPlayer);

	// create the city patch
	var cityRadius = radius/3;
	var placer = new ClumpPlacer(PI*cityRadius*cityRadius, 0.6, 0.3, 10, ix, iz);
	var painter = new LayeredPainter([tRoadWild, tRoad], [1]);
	createArea(placer, [painter, paintClass(clPlayer)], null);

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

	// create starting trees
	var num = 2;
	var tAngle = randFloat(-PI/3, 4*PI/3);
	var tDist = randFloat(11, 13);
	var tX = round(fx + tDist * cos(tAngle));
	var tZ = round(fz + tDist * sin(tAngle));
	group = new SimpleGroup(
		[new SimpleObject(oOak, num, num, 0,5)],
		false, clBaseResource, tX, tZ
	);
	createObjectGroup(group, 0, avoidClasses(clBaseResource,2));

	placeDefaultDecoratives(fx, fz, aGrassShort, clBaseResource, radius);
}
RMS.SetProgress(10);

log("Creating sea");

var theta = randFloat(0, 1);
var theta2 = randFloat(0, 1);

var seed = randFloat(2,3);
var seed2 = randFloat(2,3);

for (var ix = 0; ix < mapSize; ix++)
	for (var iz = 0; iz < mapSize; iz++)
	{
		var x = ix / (mapSize + 1.0);
		var z = iz / (mapSize + 1.0);

		// add the rough shape of the water
		var km = 20/scaleByMapSize(35, 160);

		var fadeDist = 0.02;

		var cu = km*rndRiver(theta+x*0.5*(mapSize/64),seed);
		var cu2 = km*rndRiver(theta2+x*0.5*(mapSize/64),seed2);

		if (z > cu  + 0.5 - WATER_WIDTH/2 &&
		    z < cu2 + 0.5 + WATER_WIDTH/2)
		{
			var h;
			if (z < (cu + 0.5 + fadeDist - WATER_WIDTH/2))
				h = 3 - 7 * (1 - ((cu + 0.5 + fadeDist - WATER_WIDTH/2) - z)/fadeDist);
			else if (z > (cu2 + 0.5 - fadeDist + WATER_WIDTH/2))
				h = 3 - 7 * (1 - (z - (cu2 + 0.5 - fadeDist + WATER_WIDTH/2))/fadeDist);
			else
				h = -4.0;

			if (h < -1.5)
				placeTerrain(ix, iz, tWater);
			else
				placeTerrain(ix, iz, tShore);

			setHeight(ix, iz, h);
		}
		else
			setHeight(ix, iz, 3.1);
	}
RMS.SetProgress(20);

log("Creating rivers");
for (let i = 0; i <= randIntInclusive(8, scaleByMapSize(12, 20)); ++i)
{
	var cLocation = randFloat(0.05, 0.95);

	var sign = randBool() ? 1 : -1;
	var tang = sign * PI * randFloat(0.2, 0.8);
	var cDistance = sign * 0.05;

	var point = getTIPIADBON([fractionToTiles(cLocation), fractionToTiles(0.5 + cDistance)], [fractionToTiles(cLocation), fractionToTiles(0.5 - cDistance)], [-6, -1.5], 0.5, 4, 0.01);
	if (point !== undefined)
	{
		var placer = new PathPlacer(floor(point[0]), floor(point[1]), floor(fractionToTiles(0.5 + 0.49*cos(tang))), floor(fractionToTiles(0.5 + 0.49*sin(tang))), scaleByMapSize(10,20), 0.4, 3*(scaleByMapSize(1,4)), 0.1, 0.05);
		var terrainPainter = new LayeredPainter(
			[tShore, tWater, tWater],		// terrains
			[1, 3]								// widths
		);
		var elevationPainter = new SmoothElevationPainter(
			ELEVATION_SET,			// type
			-4,				// elevation
			4				// blend radius
		);
		var success = createArea(placer, [terrainPainter, elevationPainter, paintClass(clWater)], avoidClasses(clPlayer, 8, clWater, 3, clShallow, 2));
		if (success !== undefined)
		{
			placer = new ClumpPlacer(floor(PI*scaleByMapSize(10,20)*scaleByMapSize(10,20)/4), 0.95, 0.6, 10, fractionToTiles(0.5 + 0.49*cos(tang)), fractionToTiles(0.5 + 0.49*sin(tang)));
			var painter = new LayeredPainter(
				[tShore, tWater, tWater],		// terrains
				[1, 3]								// widths);
			);
			var elevationPainter = new SmoothElevationPainter(ELEVATION_SET, -3, 3);
			createArea(placer, [painter, elevationPainter], avoidClasses(clPlayer, 23));
		}
	}
}

passageMaker(round(fractionToTiles(0.2)), round(fractionToTiles(0.25)), round(fractionToTiles(0.8)), round(fractionToTiles(0.25)), scaleByMapSize(4,8), -2, -2, 2, clShallow, undefined, -4);
passageMaker(round(fractionToTiles(0.2)), round(fractionToTiles(0.75)), round(fractionToTiles(0.8)), round(fractionToTiles(0.75)), scaleByMapSize(4,8), -2, -2, 2, clShallow, undefined, -4);

paintTerrainBasedOnHeight(-5, 1, 1, tWater);
paintTerrainBasedOnHeight(1, 3, 1, tShore);
paintTileClassBasedOnHeight(-6, 0.5, 1, clWater);
RMS.SetProgress(25);

createBumps(avoidClasses(clWater, 5, clPlayer, 20));
RMS.SetProgress(30);

createHills([tCliff, tCliff, tHill], avoidClasses(clPlayer, 20, clHill, 15, clWater, 5), clHill, scaleByMapSize(1, 4) * numPlayers);
RMS.SetProgress(50);

createForests(
 [tGrass, tGrassDForest, tGrassDForest, pForestD, pForestD],
 avoidClasses(clPlayer, 20, clForest, 17, clHill, 0, clWater, 6),
 clForest,
 1.0
);
RMS.SetProgress(70);

log("Creating dirt patches...");
createLayeredPatches(
 [scaleByMapSize(3, 6), scaleByMapSize(5, 10), scaleByMapSize(8, 21)],
 [[tGrass,tGrassA], tGrassB, [tGrassB,tGrassC]],
 [1,1],
 avoidClasses(clWater, 1, clForest, 0, clHill, 0, clDirt, 5, clPlayer, 6)
);

log("Creating grass patches...");
createPatches(
 [scaleByMapSize(2, 4), scaleByMapSize(3, 7), scaleByMapSize(5, 15)],
 [tGrassPatchBlend, tGrassPatch],
 [1],
 avoidClasses(clWater, 1, clForest, 0, clHill, 0, clDirt, 5, clPlayer, 6)
);
RMS.SetProgress(80);

log("Creating stone mines...");
createMines(
 [
  [new SimpleObject(oStoneSmall, 0,2, 0,4), new SimpleObject(oStoneLarge, 1,1, 0,4)],
  [new SimpleObject(oStoneSmall, 2,5, 1,3)]
 ],
 avoidClasses(clWater, 2, clForest, 1, clPlayer, 20, clRock, 10, clHill, 2)
);

log("Creating metal mines...");
createMines(
 [
  [new SimpleObject(oMetalLarge, 1,1, 0,4)]
 ],
 avoidClasses(clWater, 2, clForest, 1, clPlayer, 20, clMetal, 10, clRock, 5, clHill, 2),
 clMetal
);
RMS.SetProgress(85);

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
 avoidClasses(clWater, 1, clForest, 0, clPlayer, 0, clHill, 0)
);

// create water decoration in the shallow parts
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

createFood
(
 [
  [new SimpleObject(oDeer, 5,7, 0,4)],
  [new SimpleObject(oGoat, 2,3, 0,2)],
  [new SimpleObject(oBoar, 2,3, 0,2)]
 ],
 [
  3 * numPlayers,
  3 * numPlayers,
  3 * numPlayers
 ],
 avoidClasses(clWater, 1, clForest, 0, clPlayer, 20, clHill, 0, clFood, 15)
);

createFood
(
 [
  [new SimpleObject(oBerryBush, 5,7, 0,4)]
 ],
 [
  randIntInclusive(1, 4) * numPlayers + 2
 ],
 avoidClasses(clWater, 3, clForest, 0, clPlayer, 20, clHill, 1, clFood, 10)
);

createFood
(
 [
  [new SimpleObject(oFish, 2,3, 0,2)]
 ],
 [scaleByMapSize(3, 25) * numPlayers],
 [avoidClasses(clFood, 6), stayClasses(clWater, 4)]
);

log("Creating straggler trees...");
var types = [oBeech, oPoplar, oApple];	// some variation
createStragglerTrees(types, avoidClasses(clWater, 1, clForest, 1, clHill, 1, clPlayer, 8, clMetal, 6, clRock, 6));

setSkySet("cirrus");
setWaterColor(0.114, 0.192, 0.463);
setWaterTint(0.255, 0.361, 0.651);
setWaterWaviness(3.0);
setWaterType("ocean");
setWaterMurkiness(0.83);

setFogThickness(0.35);
setFogFactor(0.55);

setPPEffect("hdr");
setPPSaturation(0.62);
setPPContrast(0.62);
setPPBloom(0.37);

ExportMap();
