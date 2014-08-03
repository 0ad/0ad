RMS.LoadLibrary("rmgen");

const tGrass = ["medit_grass_field_a", "medit_grass_field_b"];;
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

// gaia entities
const oPoplar = "gaia/flora_tree_poplar";
const oApple = "gaia/flora_tree_apple";
const oCarob = "gaia/flora_tree_carob";
const oBerryBush = "gaia/flora_bush_berry";
const oChicken = "gaia/fauna_chicken";
const oDeer = "gaia/fauna_deer";
const oFish = "gaia/fauna_fish";
const oSheep = "gaia/fauna_sheep";
const oStoneLarge = "gaia/geology_stonemine_medit_quarry";
const oStoneSmall = "gaia/geology_stone_mediterranean";
const oMetalLarge = "gaia/geology_metal_mediterranean_slabs";

// decorative props
const aGrass = "actor|props/flora/grass_soft_large_tall.xml";
const aGrassShort = "actor|props/flora/grass_soft_large.xml";
const aReeds = "actor|props/flora/reeds_pond_lush_a.xml";
const aLillies = "actor|props/flora/water_lillies.xml";
const aRockLarge = "actor|geology/stone_granite_large.xml";
const aRockMedium = "actor|geology/stone_granite_med.xml";
const aBushMedium = "actor|props/flora/bush_medit_me.xml";
const aBushSmall = "actor|props/flora/bush_medit_sm.xml";

// terrain + entity (for painting)

const pForestP = [tForestFloorP + TERRAIN_SEPARATOR + oPoplar, tForestFloorP];
const pForestC = [tForestFloorC + TERRAIN_SEPARATOR + oCarob, tForestFloorC];
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
var clLand = createTileClass();
var clUpperLand = createTileClass();
var clRiver = createTileClass();
var clShallow = createTileClass();

//Create the continent body
var fx = fractionToTiles(0.5);
var fz = fractionToTiles(0.7);
var ix = round(fx);
var iz = round(fz);

var placer = new RectPlacer(0, floor(mapSize * 0.70), mapSize - 1, mapSize - 1);
var terrainPainter = new LayeredPainter(
	[tWater, tShore, tGrass],		// terrains
	[4, 2]		// widths
);
var elevationPainter = new SmoothElevationPainter(
	ELEVATION_SET,			// type
	3,				// elevation
	4				// blend radius
);
createArea(placer, [terrainPainter, elevationPainter, paintClass(clUpperLand)], null);

var placer = new ChainPlacer(2, floor(scaleByMapSize(5, 12)), floor(scaleByMapSize(60, 700)), 1, ix, iz, 0, [floor(mapSize * 0.49)]);
var terrainPainter = new LayeredPainter(
	[tGrass, tGrass, tGrass],		// terrains
	[4, 2]		// widths
);
var elevationPainter = new SmoothElevationPainter(
	ELEVATION_SET,			// type
	3,				// elevation
	4				// blend radius
);
createArea(placer, [terrainPainter, elevationPainter, paintClass(clLand)], null);

// randomize player order
var playerIDs = [];
for (var i = 0; i < numPlayers; i++)
{
	playerIDs.push(i+1);
}
playerIDs = primeSortPlayers(sortPlayers(playerIDs));

// place players

var playerX = new Array(numPlayers);
var playerZ = new Array(numPlayers);
var playerAngle = new Array(numPlayers);

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
	
	// some constants
	var radius = scaleByMapSize(15,25);
	var cliffRadius = 2;
	var elevation = 20;
	
	// get the x and z in tiles
	fx = fractionToTiles(playerX[i]);
	fz = fractionToTiles(playerZ[i]);
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
	placeCivDefaultEntities(fx, fz, id, BUILDING_ANGlE, {'iberWall' : false});
	
	// create animals
	for (var j = 0; j < 2; ++j)
	{
		var aAngle = randFloat(0, TWO_PI);
		var aDist = 6;
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
	var bbDist = 8;
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
	var hillSize = PI * radius * radius;
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

const WATER_WIDTH = 0.07;
log("Creating river");
var theta = randFloat(0, 1);
var seed = randFloat(2,3);

for (var ix = 0; ix < mapSize; ix++)
{
	for (var iz = 0; iz < mapSize; iz++)
	{
		var x = ix / (mapSize + 1.0);
		var z = iz / (mapSize + 1.0);
		
		var h = 0;
		var distToWater = 0;
		
		h = 32 * (z - 0.5);
		
		// add the rough shape of the water
		var km = 12/scaleByMapSize(35, 160);
		var cu = km*rndRiver(theta+z*0.5*(mapSize/64),seed);
		var zk = z*randFloat(0.995,1.005);
		var xk = x*randFloat(0.995,1.005);
		if (-3.0 < getHeight(ix, iz)){
		if ((xk > cu+((1.0-WATER_WIDTH)/2))&&(xk < cu+((1.0+WATER_WIDTH)/2)))
		{
			if (xk < cu+((1.05-WATER_WIDTH)/2))
			{
				h = -3 + 200.0* abs(cu+((1.05-WATER_WIDTH)/2-xk));	
				if ((((zk>0.3)&&(zk<0.4))||((zk>0.5)&&(zk<0.6))||((zk>0.7)&&(zk<0.8)))&&(h<-1.5))
				{
					h=-1.5;
					addToClass(ix, iz, clShallow);
				}
			
			}
			else if (xk > (cu+(0.95+WATER_WIDTH)/2))
			{
				h = -3 + 200.0*(xk-(cu+((0.95+WATER_WIDTH)/2)));
				if ((((zk>0.3)&&(zk<0.4))||((zk>0.5)&&(zk<0.6))||((zk>0.7)&&(zk<0.8)))&&(h<-1.5))
				{
					h=-1.5;
					addToClass(ix, iz, clShallow);
				}
			}
			else
			{
				if (((zk>0.3)&&(zk<0.4))||((zk>0.5)&&(zk<0.6))||((zk>0.7)&&(zk<0.8))){
					h = -1.5;
					addToClass(ix, iz, clShallow);
				}
				else
				{
					h = -3.0;
				}
			}
			setHeight(ix, iz, h);
			addToClass(ix, iz, clRiver);
			placeTerrain(ix, iz, tWater);
		}
		}
	}
}

paintTerrainBasedOnHeight(1, 3, 0, tShore);
paintTerrainBasedOnHeight(-8, 1, 2, tWater);

// create bumps
createBumps([avoidClasses(clWater, 2, clPlayer, 20, clRiver, 1), stayClasses(clLand, 3)]);

// create forests
createForests(
 [tGrass, tForestFloorP, tForestFloorC, pForestC, pForestP],
 [avoidClasses(clPlayer, 20, clForest, 17, clHill, 0, clRiver, 1), stayClasses(clLand, 7)], 
 clForest,
 1.0,
 0
);

RMS.SetProgress(50);

// create dirt patches
log("Creating dirt patches...");
createLayeredPatches(
 [scaleByMapSize(3, 6), scaleByMapSize(5, 10), scaleByMapSize(8, 21)],
 [[tGrass,tGrassA],[tGrassA,tGrassB], [tGrassB,tGrassC]],
 [1,1],
 [avoidClasses(clForest, 0, clHill, 0, clDirt, 3, clPlayer, 8, clRiver, 1), stayClasses(clLand, 7)]
);

// create grass patches
log("Creating grass patches...");
createPatches(
 [scaleByMapSize(2, 4), scaleByMapSize(3, 7), scaleByMapSize(5, 15)],
 tGrassPatch,
 [avoidClasses(clForest, 0, clHill, 0, clDirt, 3, clPlayer, 8, clRiver, 1), stayClasses(clLand, 7)]
);

RMS.SetProgress(55);

log("Creating stone mines...");
// create stone quarries
createMines(
 [
  [new SimpleObject(oStoneSmall, 0,2, 0,4), new SimpleObject(oStoneLarge, 1,1, 0,4)],
  [new SimpleObject(oStoneSmall, 2,5, 1,3)]
 ],
 [avoidClasses(clForest, 1, clPlayer, 20, clRock, 10, clHill, 1, clRiver, 1), stayClasses(clLand, 5)]
)

log("Creating metal mines...");
// create large metal quarries
createMines(
 [
  [new SimpleObject(oMetalLarge, 1,1, 0,4)]
 ],
 [avoidClasses(clForest, 1, clPlayer, 20, clMetal, 10, clRock, 5, clHill, 1, clRiver, 1), stayClasses(clLand, 5)],
 clMetal
)

RMS.SetProgress(65);

// create decoration
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
 [avoidClasses(clWater, 1, clHill, 1, clPlayer, 1, clDirt, 1, clRiver, 1), stayClasses(clLand, 6)]
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

RMS.SetProgress(70);

// create animals
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
 [avoidClasses(clWater, 0, clForest, 0, clPlayer, 20, clHill, 1, clFood, 20, clRiver, 1), stayClasses(clLand, 3)]
);

// create fruits
createFood
(
 [
  [new SimpleObject(oBerryBush, 5,7, 0,4)]
 ], 
 [
  randInt(1, 4) * numPlayers + 2
 ],
 [avoidClasses(clWater, 0, clForest, 0, clPlayer, 20, clHill, 1, clFood, 10, clRiver, 1), stayClasses(clLand, 3)]
);

// create fish
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


// create straggler trees
log("Creating straggler trees...");
var types = [oPoplar, oCarob, oApple];	// some variation
createStragglerTrees(types, [avoidClasses(clWater, 1, clForest, 1, clHill, 1, clPlayer, 9, clMetal, 1, clRock, 1, clRiver, 1), stayClasses(clLand, 7)]);

setSkySet("cumulus");
setWaterColour(0.443,0.412,0.322);
setWaterTint(0.647,0.82,0.949);
setWaterWaviness(4.0);
setWaterType("lake");
setWaterMurkiness(0.70);

setFogFactor(0.3);
setFogThickness(0.25);

setPPEffect("hdr");
setPPContrast(0.62);
setPPSaturation(0.51);
setPPBloom(0.12);

// Export map data
ExportMap();
