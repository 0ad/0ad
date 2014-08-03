RMS.LoadLibrary("rmgen");

//random terrain textures
const tSand = ["desert_sand_dunes_100", "desert_dirt_cracks","desert_sand_smooth", "desert_dirt_rough", "desert_dirt_rough_2", "desert_sand_smooth"];
const tDune = ["desert_sand_dunes_50"];
const tBigDune = ["desert_sand_dunes_50"];
const tForestFloor = "desert_forestfloor_palms";
const tHill = ["desert_dirt_rocks_1", "desert_dirt_rocks_2", "desert_dirt_rocks_3"];
const tDirt = ["desert_dirt_rough","desert_dirt_rough","desert_dirt_rough", "desert_dirt_rough_2", "desert_dirt_rocks_2"];
const tRoad = "desert_city_tile";;
const tRoadWild = "desert_city_tile";;
const tShoreBlend = "desert_sand_wet";
const tShore = "dirta";
const tWater = "desert_sand_wet";

// gaia entities
const ePalmShort = "gaia/flora_tree_cretan_date_palm_short";
const ePalmTall = "gaia/flora_tree_cretan_date_palm_tall";
const eBush = "gaia/flora_bush_grapes";
const eChicken = "gaia/fauna_chicken";
const eCamel = "gaia/fauna_camel";
const eGazelle = "gaia/fauna_gazelle";
const eLion = "gaia/fauna_lion";
const eLioness = "gaia/fauna_lioness";
const eStoneMine = "gaia/geology_stonemine_desert_quarry";
const eStoneMineSmall = "gaia/geology_stone_desert_small";
const eMetalMine = "gaia/geology_metal_desert_slabs";

// decorative props
const aFlower1 = "actor|props/flora/decals_flowers_daisies.xml";
const aWaterFlower = "actor|props/flora/water_lillies.xml";
const aReedsA = "actor|props/flora/reeds_pond_lush_a.xml";
const aReedsB = "actor|props/flora/reeds_pond_lush_b.xml";
const aRock = "actor|geology/stone_desert_med.xml";
const aBushA = "actor|props/flora/bush_desert_dry_a.xml";
const aBushB = "actor|props/flora/bush_desert_dry_a.xml";
const aSand = "actor|particle/blowing_sand.xml";

const pForestMain = [tForestFloor + TERRAIN_SEPARATOR + ePalmShort, tForestFloor + TERRAIN_SEPARATOR + ePalmTall, tForestFloor];
const pOasisForestLight = [tForestFloor + TERRAIN_SEPARATOR + ePalmShort, tForestFloor + TERRAIN_SEPARATOR + ePalmTall, tForestFloor,tForestFloor,tForestFloor
					,tForestFloor,tForestFloor,tForestFloor,tForestFloor];
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
var clPassage = createTileClass();
var clRock = createTileClass();
var clMetal = createTileClass();
var clFood = createTileClass();
var clBaseResource = createTileClass();
var clSettlement = createTileClass();
var clDune = createTileClass();

for (var ix = 0; ix < mapSize; ix++)
{
	for (var iz = 0; iz < mapSize; iz++)
	{
		var x = ix / (mapSize + 1.0);
		var z = iz / (mapSize + 1.0);
			placeTerrain(ix, iz, tSand);
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
var placer = undefined;
var fx = 0; var fz = 0;
var ix =0; var iz = 0;
for (var i = 0; i < numPlayers; i++)
{
	var id = playerIDs[i];
	log("Creating base for player " + id + "...");
	
	// some constants
	var radius = scaleByMapSize(15,25);
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
	placeCivDefaultEntities(fx, fz, id, BUILDING_ANGlE);
		
	// create animals
	for (var j = 0; j < 2; ++j)
	{
		var aAngle = randFloat(0, TWO_PI);
		var aDist = 9;
		var aX = round(fx + aDist * cos(aAngle));
		var aZ = round(fz + aDist * sin(aAngle));
		var group = new SimpleGroup(
			[new SimpleObject(eChicken, 5,5, 0,2)],
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
		[new SimpleObject(eBush, 5,5, 0,3)],
		true, clBaseResource, bbX, bbZ
	);
	createObjectGroup(group, 0);
	
	// create metal mine
	var mAngle = bbAngle;
	while(abs(mAngle - bbAngle) < PI/3)
	{
		mAngle = randFloat(0, TWO_PI);
	}
	var mDist = radius*1.3;
	var mX = round(fx + mDist * cos(mAngle));
	var mZ = round(fz + mDist * sin(mAngle));
	group = new SimpleGroup(
		[new SimpleObject(eMetalMine, 1,1, 0,0),new SimpleObject(aBushB, 1,1, 2,2), new SimpleObject(aBushA, 0,2, 1,3),new SimpleObject(ePalmShort, 2,2, 2,3),new SimpleObject(ePalmTall, 1,1, 2,2)],
		true, clBaseResource, mX, mZ
	);
	createObjectGroup(group, 0);
	mX = round(fx + mDist*1.5 * cos(mAngle + PI/1.578));
	mZ = round(fz + mDist*1.5 * sin(mAngle + PI/1.578));
	group = new SimpleGroup(
							[new SimpleObject(eMetalMine, 1,1, 0,0),new SimpleObject(aBushB, 1,1, 2,2), new SimpleObject(aBushA, 0,2, 1,3),new SimpleObject(ePalmShort, 2,2, 2,3),new SimpleObject(ePalmTall, 1,1, 2,2)],
							true, clBaseResource, mX, mZ
							);
	createObjectGroup(group, 0);

	// create stone mines
	mAngle += randFloat(PI/8, PI/4);
	mX = round(fx + mDist * cos(mAngle));
	mZ = round(fz + mDist * sin(mAngle));
	group = new SimpleGroup(
		[new SimpleObject(eStoneMine, 1,1, 0,2),new SimpleObject(aBushB, 1,1, 2,2), new SimpleObject(aBushA, 0,2, 1,3),new SimpleObject(ePalmShort, 2,2, 2,3),new SimpleObject(ePalmTall, 1,1, 2,2)],
		true, clBaseResource, mX, mZ
	);
	createObjectGroup(group, 0);
	mX = round(fx + mDist * 1.4 * cos(mAngle - PI /2.46));
	mZ = round(fz + mDist * 1.4 * sin(mAngle - PI /2.46));
	group = new SimpleGroup(
							[new SimpleObject(eStoneMine, 1,1, 0,2),new SimpleObject(aBushB, 1,1, 2,2), new SimpleObject(aBushA, 0,2, 3,3),new SimpleObject(ePalmShort, 2,2, 3,3),new SimpleObject(ePalmTall, 1,1, 3,3)],
							true, clBaseResource, mX, mZ
							);
	createObjectGroup(group, 0);	
	// Create starting batches of wood
	var types = [tForestFloor, pForestMain];	// some variation
	var forestX = 0;
	var forestY = 0;
	var forestAngle = 0;
	do {
		forestAngle = mAngle + randFloat(PI/2, (2*PI)/3);
		var forestDist = radius * 1.2;
		forestX = round(fx + forestDist * cos(forestAngle));
		forestY = round(fz + forestDist * sin(forestAngle));
		placer = new ClumpPlacer(70, 1.0, 0.5, 10,forestX,forestY);
		painter = new LayeredPainter(types, [0] );
	} while (createArea( placer, [painter, paintClass(clBaseResource)], avoidClasses(clBaseResource, 0) ) === undefined);
	// creating the water patch explaining the forest
	do {
		var watAngle = forestAngle + randFloat((PI/3), (5*PI/3));
		var watX = round(forestX + 6 * cos(watAngle));
		var watY = round(forestY + 6 * sin(watAngle));	
		placer = new ClumpPlacer(60, 0.9, 0.4, 5,watX,watY);
		terrainPainter = new LayeredPainter( [tShore,tShoreBlend], [1] );
		painter = new SmoothElevationPainter(ELEVATION_MODIFY, -5, 3);
		group = new SimpleGroup( [new SimpleObject(aFlower1, 1,5, 0,3)], true, undefined, round(forestX + 3 * cos(watAngle)),round(forestY + 3 * sin(watAngle)) );
		createObjectGroup(group, 0);
		group = new SimpleGroup( [new SimpleObject(aReedsA, 1,3, 0,0)], true, undefined, round(forestX + 5 * cos(watAngle)),round(forestY + 5 * sin(watAngle)) );
		createObjectGroup(group, 0);
	} while (createArea( placer, [terrainPainter, painter],  avoidClasses(clBaseResource,0) ) === undefined);
	
	// TODO: add a few random trees here and there
}

RMS.SetProgress(20);

// create bumps
log("Creating bumps...");
placer = new ClumpPlacer(scaleByMapSize(20, 50), 0.3, 0.06, 1);
painter = new SmoothElevationPainter(ELEVATION_MODIFY, 4, 3);
createAreas( placer, painter, 
			avoidClasses(clWater, 5, clPlayer, 10, clBaseResource, 6),
			scaleByMapSize(30, 70)
			);
log("Creating dirt Patches...");
placer = new ClumpPlacer(80, 0.3, 0.06, 1);
var terrainPainter = new TerrainPainter(tDirt);
createAreas( placer, terrainPainter, avoidClasses(clWater, 10, clPlayer, 10, clBaseResource, 6), scaleByMapSize(15, 50) );

log("Creating Dunes...");
placer = new ClumpPlacer(120, 0.3, 0.06, 1);
var terrainPainter = new TerrainPainter(tDune);
painter = new SmoothElevationPainter(ELEVATION_MODIFY, 18, 30);
createAreas( placer, [terrainPainter, painter], 
			avoidClasses(clWater, 13, clPlayer, 10, clBaseResource, 6),
			scaleByMapSize(15, 50)
			);


log("Creating actual oasis...");
var size = mapSize * 0.2;
size *= size;
//var lSize = sqrt(sqrt(sqrt(scaleByMapSize(1, 6))));
fx = fractionToTiles(0.5);
fz = fractionToTiles(0.5);
ix = round(fx);
iz = round(fz);
placer = new ClumpPlacer(size*1.1, 0.8, 0.2, 10, ix, iz);
terrainPainter = new LayeredPainter( [pOasisForestLight,tShoreBlend, tWater, tWater, tWater], [scaleByMapSize(6,20),3, 5, 2] );
var elevationPainter = new SmoothElevationPainter(ELEVATION_SET,  -3,  15 );
createArea(placer, [terrainPainter, elevationPainter, paintClass(clWater)], null);
RMS.SetProgress(50);
if(mapSize > 150 && randInt(0,1)) {
	log ("creating path through");
	var pAngle = randFloat(0, TWO_PI);
	var px = round(fx) + round(fractionToTiles(0.13 * cos(pAngle)));
	var py = round(fz) + round(fractionToTiles(0.13 * sin(pAngle)));
	var pex = round(fx) + round(fractionToTiles(0.13 * -cos(pAngle)));
	var pey = round(fz) + round(fractionToTiles(0.13 * sin(pAngle + PI)));
	var path = new PathPlacer(px,py,pex,pey,scaleByMapSize(7,18), 0.4,1,0.2,0)
	terrainPainter = new TerrainPainter(tSand);
	elevationPainter = new SmoothElevationPainter(ELEVATION_MODIFY, 4, 5 );
	createArea(path, [terrainPainter, elevationPainter, paintClass(clPassage)], null);
}
log("Creating some straggler trees around the Passage...");
group = new SimpleGroup([new SimpleObject(ePalmTall, 1,1, 0,0),new SimpleObject(ePalmShort, 1,2, 1,2), new SimpleObject(aBushA, 0,2, 1,3)], true, clForest);
createObjectGroups(group, 0, stayClasses(clPassage,1), scaleByMapSize(60,250), 100  );

log("Creating stone mines...");
// create large stone quarries
group = new SimpleGroup([new SimpleObject(eStoneMine, 1,1, 0,0),new SimpleObject(ePalmShort, 1,2, 3,3),new SimpleObject(ePalmTall, 0,1, 3,3)
						 ,new SimpleObject(aBushB, 1,1, 2,2), new SimpleObject(aBushA, 0,2, 1,3)], true, clRock);
createObjectGroups(group, 0,
	avoidClasses(clWater, 10, clForest, 1, clPlayer, 30, clRock, 10,clBaseResource, 2, clHill, 1),
	scaleByMapSize(6,25), 100
);

log("Creating metal mines...");
// create large metal quarries
group = new SimpleGroup([new SimpleObject(eMetalMine, 1,1, 0,0),new SimpleObject(ePalmShort, 1,2, 2,3),new SimpleObject(ePalmTall, 0,1, 2,2)
						 ,new SimpleObject(aBushB, 1,1, 2,2), new SimpleObject(aBushA, 0,2, 1,3)], true, clMetal);
createObjectGroups(group, 0,
	avoidClasses(clWater, 10, clForest, 1, clPlayer, 30, clMetal, 10,clBaseResource, 2, clRock, 10, clHill, 1),
	scaleByMapSize(6,25), 100
);

RMS.SetProgress(65);

/*
// create small decorative rocks
log("Creating small decorative rocks...");
group = new SimpleGroup(
	[new SimpleObject(rba5, 1,3, 0,1)],
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
	[new SimpleObject(rba5, 1,2, 0,1), new SimpleObject(rba5, 1,3, 0,2)],
	true
);
createObjectGroups(
	group, 0,
	avoidClasses(clWater, 0, clForest, 0, clPlayer, 0, clHill, 0),
	scaleByMapSize(8, 131), 50
);

RMS.SetProgress(70);
*/
log("Creating small decorative rocks...");
group = new SimpleGroup( [new SimpleObject(aRock, 2,4, 0,2)], true, undefined );
createObjectGroups(group, 0, avoidClasses(clWater, 3, clForest, 0, clPlayer, 10, clHill, 1, clFood, 20), 30, scaleByMapSize(10,50) );

RMS.SetProgress(70);
// create deer
log("Creating Camels...");
group = new SimpleGroup(
	[new SimpleObject(eCamel, 1,2, 0,4)],
	true, clFood
);
createObjectGroups(group, 0,
	avoidClasses(clWater, 3, clForest, 0, clPlayer, 10, clHill, 1, clFood, 20),
	1 * numPlayers, 50
);

RMS.SetProgress(75);

// create sheep
log("Creating Gazelles...");
group = new SimpleGroup(
	[new SimpleObject(eGazelle, 2,4, 0,2)],
	true, clFood
);
createObjectGroups(group, 0,
	avoidClasses(clWater, 3, clForest, 0, clPlayer, 10, clHill, 1, clFood, 20),
	1 * numPlayers, 50
);

RMS.SetProgress(85);
// create lions
log("Creating Oasis Animals...");
for (var p = 0; p < scaleByMapSize(5,30); p++)
{
	fx = fractionToTiles(0.5);
	fz = fractionToTiles(0.5);
	var aAngle = randFloat(0, TWO_PI);
	var aDist = fractionToTiles(0.11);
	var animX = round(fx + aDist * cos(aAngle));
	var animY = round(fz + aDist * sin(aAngle));
	group = new RandomGroup(
		[new SimpleObject(eLion, 1,2, 0,4),new SimpleObject(eLioness, 1,2, 2,4),new SimpleObject(eGazelle, 4,6, 1,5),new SimpleObject(eCamel, 1,2, 1,5)], true, clFood, animX,animY);
	createObjectGroup(group, 0);
}
/*
var planetm = 8;
//create small grass tufts
log("Creating small grass tufts...");
group = new SimpleGroup(
	[new SimpleObject(rba1, 1,2, 0,1, -PI/8,PI/8)]
);
createObjectGroups(group, 0,
	avoidClasses(clWater, 2, clHill, 2, clPlayer, 2, clDirt, 0),
	planetm * scaleByMapSize(13, 200)
);
*/
RMS.SetProgress(90);

RMS.SetProgress(95);

// create bushes
log("Creating bushes...");
group = new SimpleGroup(
	[new SimpleObject(aBushB, 1,2, 0,2), new SimpleObject(aBushA, 2,4, 0,2)]
);
createObjectGroups(group, 0,
	avoidClasses(clWater, 2, clHill, 1, clPlayer, 1, clPassage, 1),
	scaleByMapSize(10, 40), 20
);
log ("Creating Sand blows and beautifications");
for (var sandx = 0; sandx < mapSize; sandx += 4)
{
	for (var sandz = 0; sandz < mapSize; sandz += 4)
	{
		if (getHeight(sandx,sandz) > 3.4)
		{
			if (Math.random()* 1.4 < getHeight(sandx,sandz) - 3.4)
			{
				group = new SimpleGroup( [new SimpleObject(aSand, 0,1, 0,2)], true, undefined, sandx,sandz );
				createObjectGroup(group, 0);
			}
		} else if (getHeight(sandx,sandz) > -2.5 && getHeight(sandx,sandz) < -1.0)
		{
			if (Math.random() < 0.4)
			{
				group = new SimpleGroup( [new SimpleObject(aWaterFlower, 1,4, 1,2)], true, undefined, sandx,sandz );
				createObjectGroup(group, 0);
			} else if (Math.random() > 0.3 && getHeight(sandx,sandz) < -1.9)
			{
				group = new SimpleGroup( [new SimpleObject(aReedsA, 5,12, 0,2),new SimpleObject(aReedsB, 5,12, 0,2)], true, undefined, sandx,sandz );
				createObjectGroup(group, 0);
			}
			if (getTileClass(clPassage).countInRadius(sandx,sandz,2,true) > 0) {
				if (Math.random() < 0.4)
				{
					group = new SimpleGroup( [new SimpleObject(aWaterFlower, 1,4, 1,2)], true, undefined, sandx,sandz );
					createObjectGroup(group, 0);
				} else if (Math.random() > 0.3 && getHeight(sandx,sandz) < -1.9)
				{
					group = new SimpleGroup( [new SimpleObject(aReedsA, 5,12, 0,2),new SimpleObject(aReedsB, 5,12, 0,2)], true, undefined, sandx,sandz );
					createObjectGroup(group, 0);
				}				
			}
		}
	}
}

setSkySet("sunny");
setSunColour(0.914,0.827,0.639);
setSunRotation(PI/3);
setSunElevation(0.5);
setWaterColour(0, 0.227, 0.843);
setWaterTint(0, 0.545, 0.859);
setWaterWaviness(1.0);
setWaterType("clap");
setWaterMurkiness(0.5);
setTerrainAmbientColour(0.45, 0.5, 0.6);
setUnitsAmbientColour(0.501961, 0.501961, 0.501961);

// Export map data

ExportMap();