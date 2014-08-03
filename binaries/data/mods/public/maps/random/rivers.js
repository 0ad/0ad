RMS.LoadLibrary("rmgen");

//random terrain textures
var random_terrain = randomizeBiome();

const tMainTerrain = rBiomeT1();
const tForestFloor1 = rBiomeT2();
const tForestFloor2 = rBiomeT3();
const tCliff = rBiomeT4();
const tTier1Terrain = rBiomeT5();
const tTier2Terrain = rBiomeT6();
const tTier3Terrain = rBiomeT7();
const tHill = rBiomeT8();
const tDirt = rBiomeT9();
const tRoad = rBiomeT10();
const tRoadWild = rBiomeT11();
const tTier4Terrain = rBiomeT12();
const tShoreBlend = rBiomeT13();
var tShore = rBiomeT14();
var tWater = rBiomeT15();
if (random_terrain == 7)
{
	tShore = "tropic_dirt_b_plants";
	tWater = "tropic_dirt_b";
}
// gaia entities
const oTree1 = rBiomeE1();
const oTree2 = rBiomeE2();
const oTree3 = rBiomeE3();
const oTree4 = rBiomeE4();
const oTree5 = rBiomeE5();
const oFruitBush = rBiomeE6();
const oChicken = rBiomeE7();
const oMainHuntableAnimal = rBiomeE8();
const oFish = rBiomeE9();
const oSecondaryHuntableAnimal = rBiomeE10();
const oStoneLarge = rBiomeE11();
const oStoneSmall = rBiomeE12();
const oMetalLarge = rBiomeE13();

// decorative props
const aGrass = rBiomeA1();
const aGrassShort = rBiomeA2();
const aReeds = rBiomeA3();
const aLillies = rBiomeA4();
const aRockLarge = rBiomeA5();
const aRockMedium = rBiomeA6();
const aBushMedium = rBiomeA7();
const aBushSmall = rBiomeA8();

const pForest1 = [tForestFloor2 + TERRAIN_SEPARATOR + oTree1, tForestFloor2 + TERRAIN_SEPARATOR + oTree2, tForestFloor2];
const pForest2 = [tForestFloor1 + TERRAIN_SEPARATOR + oTree4, tForestFloor1 + TERRAIN_SEPARATOR + oTree5, tForestFloor1];
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
var clShallow = createTileClass();
for (var ix = 0; ix < mapSize; ix++)
{
	for (var iz = 0; iz < mapSize; iz++)
	{
		var x = ix / (mapSize + 1.0);
		var z = iz / (mapSize + 1.0);
			placeTerrain(ix, iz, tMainTerrain);
	}
}

var fx = fractionToTiles(0.5);
var fz = fractionToTiles(0.5);
ix = round(fx);
iz = round(fz);

var lSize = sqrt(sqrt(sqrt(scaleByMapSize(1, 6))));

var placer = new ClumpPlacer(mapArea * 0.01 * lSize, 0.7, 0.1, 10, ix, iz);
var terrainPainter = new LayeredPainter(
	[tShore, tWater, tWater, tWater],		// terrains
	[1, 4, 2]		// widths
);
var elevationPainter = new SmoothElevationPainter(
	ELEVATION_SET,			// type
	-3,				// elevation
	4				// blend radius
);
createArea(placer, [terrainPainter, elevationPainter, paintClass(clWater)], null);

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
	placeCivDefaultEntities(fx, fz, id, BUILDING_ANGlE);
	
	// create animals
	for (var j = 0; j < 2; ++j)
	{
		var aAngle = randFloat(0, TWO_PI);
		var aDist = 7;
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
	var bbDist = 12;
	var bbX = round(fx + bbDist * cos(bbAngle));
	var bbZ = round(fz + bbDist * sin(bbAngle));
	group = new SimpleGroup(
		[new SimpleObject(oFruitBush, 5,5, 0,3)],
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
	var num = 2;
	var tAngle = randFloat(-PI/3, 4*PI/3);
	var tDist = randFloat(11, 13);
	var tX = round(fx + tDist * cos(tAngle));
	var tZ = round(fz + tDist * sin(tAngle));
	group = new SimpleGroup(
		[new SimpleObject(oTree1, num, num, 0,5)],
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

//init rivers
var PX = new Array(numPlayers+1);
var PZ = new Array(numPlayers+1);
//isRiver actually tells us if two points must be joined by river
var isRiver = new Array(numPlayers+1);
for (var q=0; q <numPlayers+1; q++)
{
	isRiver[q]=new Array(numPlayers+1);
}

//At first nothing is joined
for (var m = 0; m < numPlayers+1; m++){
	for (var n = 0; n < numPlayers+1; n++){
		isRiver[m][n] = 0;
	}
}

//creating the first point in the center. all others are
//connected to this one so all of our rivers join together
//in the middle of the map
var fx = fractionToTiles(0.5);
var fz = fractionToTiles(0.5);
var ix = round(fx);
var iz = round(fz);
PX[numPlayers]= fx;
PZ[numPlayers]= fz;
var riverAngle = new Array(numPlayers);
for (var c = 0 ; c < numPlayers ; c++)
{
	//creating other points of the river and making them
	// join the point in the center of the map
	riverAngle[c] = startAngle + (((2 * c + 1) / (numPlayers * 2)) * TWO_PI );
	PX[c] = round(fractionToTiles(0.5 + 0.5 * cos(riverAngle[c])));
	PZ[c] = round(fractionToTiles(0.5 + 0.5 * sin(riverAngle[c])));	
	//log (playerIDs[c], ",,," ,playerIDs[0]);
	//isRiver[c][numPlayers]=1;
	if ((c == numPlayers-1)&&(!areAllies(playerIDs[c]-1, playerIDs[0]-1)))
	{
		isRiver[c][numPlayers]=1;
	}
	else if ((c < numPlayers-1)&&(!areAllies(playerIDs[c]-1, playerIDs[c+1]-1)))
	{
		isRiver[c][numPlayers]=1;
	}
}

//theta is the start value for rndRiver function. seed implies 
//the randomness. we must have one of these for each river we create.
//shallowpoint and shallow length define the place and size of the shallow part
var theta = new Array(numPlayers);
var seed = new Array(numPlayers);
var shallowpoint = new Array(numPlayers);
var shallowlength = new Array(numPlayers);
for (var q=0; q <numPlayers+1; q++)
{
	theta[q]=randFloat(0, 1);
	seed[q]=randFloat(2,3);
	shallowpoint[q]=randFloat(0.2,0.7);
	shallowlength[q]=randFloat(0.12,0.21);
}
//create rivers
//checking all the tiles
for (var ix = 0; ix < mapSize; ix++)
{
	for (var iz = 0; iz < mapSize; iz++)
	{
		for (var m = 0; m < numPlayers+1; m++)
		{
			for (var n = 0; n < numPlayers+1; n++)
			{
				//checking if there is a river between those points
				if(isRiver[m][n] == 1)
				{
					//very important calculations. don't change anything. results 
					//"dis" which is the distance to the riverline and "y" and "xm" which are 
					//the coordinations for the point it's image is in.
					var a = PZ[m]-PZ[n];
					var b = PX[n]-PX[m];
					var c = (PZ[m]*(PX[m]-PX[n]))-(PX[m]*(PZ[m]-PZ[n]));
					var dis = abs(a*ix + b*iz + c)/sqrt(a*a + b*b);
					if (abs(a*ix + b*iz + c) != 0)
					{
						var alamat = (a*ix + b*iz + c)/abs(a*ix + b*iz + c);
					}
					else
					{
						var alamat = 1;
					}
					var k = (a*ix + b*iz + c)/(a*a + b*b);
					var y = iz-(b*k);
					var xm = ix-(a*k);
					//this calculates which "part" of the river are we in now.
					//used for the function rndRiver.
					var sit = sqrt((PZ[n]-y)*(PZ[n]-y)+(PX[n]-xm)*(PX[n]-xm))/sqrt((PZ[n]-PZ[m])*(PZ[n]-PZ[m])+(PX[n]-PX[m])*(PX[n]-PX[m]));
					var sbms = scaleByMapSize(5,15) + alamat * ( scaleByMapSize(20, 60) * rndRiver( theta[m] + sit * 0.5 * (mapSize/64) , seed[m]) );
					if((dis < sbms)&&(y <= Math.max(PZ[m],PZ[n]))&&(y >= Math.min(PZ[m],PZ[n])))
					{
						//create the deep part of the river
						if (dis <= sbms-5){
							if ((sit > shallowpoint[m])&&(sit < shallowpoint[m]+shallowlength[m]))
							{
								//create the shallow part
								var h = -1;
								addToClass(ix, iz, clShallow);
							}
							else
							{
								var h = -3;
							}
							var t = tWater;
							addToClass(ix, iz, clWater);
						}
						//creating the rough edges
						else if (dis <= sbms)
						{
							if ((sit > shallowpoint[m])&&(sit < shallowpoint[m]+shallowlength[m]))
							{
								if (2-(sbms-dis)<-1)
								{
									//checking if there is shallow water here 
									var h = -1;
									addToClass(ix, iz, clShallow);
								}
								else
								{
									var h = 2-(sbms-dis);
								}
							}
							else
							{
								var h = 2-(sbms-dis);
							}
							//we must create shore lines for more beautiful terrain
							if (sbms-dis<=2)
							{
								var t = tShore;
							}
							else
							{
								var t = tWater;
							}
							addToClass(ix, iz, clWater);
						}
						//we don't want to cause problems when river joins sea
						if (getHeight(ix, iz)>h)
						{
							placeTerrain(ix, iz, t);
							setHeight(ix, iz, h);
						}
					}
				}
			}
		}
	}
}

RMS.SetProgress(40);

// create bumps
createBumps(avoidClasses(clWater, 2, clPlayer, 20));

// create hills
if (randInt(1,2) == 1)
	createHills([tMainTerrain, tCliff, tHill], avoidClasses(clPlayer, 20, clHill, 15, clWater, 2), clHill, scaleByMapSize(3, 15));
else
	createMountains(tCliff, avoidClasses(clPlayer, 20, clHill, 15, clWater, 2), clHill, scaleByMapSize(3, 15));

// create forests
createForests(
 [tMainTerrain, tForestFloor1, tForestFloor2, pForest1, pForest2],
 avoidClasses(clPlayer, 20, clForest, 17, clHill, 0, clWater, 2), 
 clForest,
 1.0,
 random_terrain
);

RMS.SetProgress(50);

// create dirt patches
log("Creating dirt patches...");
createLayeredPatches(
 [scaleByMapSize(3, 6), scaleByMapSize(5, 10), scaleByMapSize(8, 21)],
 [[tMainTerrain,tTier1Terrain],[tTier1Terrain,tTier2Terrain], [tTier2Terrain,tTier3Terrain]],
 [1,1],
 avoidClasses(clWater, 3, clForest, 0, clHill, 0, clDirt, 5, clPlayer, 12)
);

// create grass patches
log("Creating grass patches...");
createPatches(
 [scaleByMapSize(2, 4), scaleByMapSize(3, 7), scaleByMapSize(5, 15)],
 tTier4Terrain,
 avoidClasses(clWater, 3, clForest, 0, clHill, 0, clDirt, 5, clPlayer, 12)
);

RMS.SetProgress(55);

log("Creating stone mines...");
// create stone quarries
createMines(
 [
  [new SimpleObject(oStoneSmall, 0,2, 0,4), new SimpleObject(oStoneLarge, 1,1, 0,4)],
  [new SimpleObject(oStoneSmall, 2,5, 1,3)]
 ],
 avoidClasses(clWater, 3, clForest, 1, clPlayer, 20, clRock, 10, clHill, 1)
)

log("Creating metal mines...");
// create large metal quarries
createMines(
 [
  [new SimpleObject(oMetalLarge, 1,1, 0,4)]
 ],
 avoidClasses(clWater, 3, clForest, 1, clPlayer, 20, clMetal, 10, clRock, 5, clHill, 1),
 clMetal
)

RMS.SetProgress(65);

// create decoration
var planetm = 1;

if (random_terrain==7)
	planetm = 8;

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
  planetm * scaleByMapSize(13, 200),
  planetm * scaleByMapSize(13, 200),
  planetm * scaleByMapSize(13, 200)
 ],
 avoidClasses(clWater, 0, clForest, 0, clPlayer, 0, clHill, 0)
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
  [new SimpleObject(oMainHuntableAnimal, 5,7, 0,4)],
  [new SimpleObject(oSecondaryHuntableAnimal, 2,3, 0,2)]
 ], 
 [
  3 * numPlayers,
  3 * numPlayers
 ],
 avoidClasses(clWater, 3, clForest, 0, clPlayer, 20, clHill, 1, clFood, 20)
);

// create fruits
createFood
(
 [
  [new SimpleObject(oFruitBush, 5,7, 0,4)]
 ], 
 [
  3 * numPlayers
 ],
 avoidClasses(clWater, 3, clForest, 0, clPlayer, 20, clHill, 1, clFood, 10)
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
 [avoidClasses(clFood, 20), stayClasses(clWater, 6)]
);

RMS.SetProgress(85);

// create straggler trees
var types = [oTree1, oTree2, oTree4, oTree3];	// some variation
createStragglerTrees(types, avoidClasses(clWater, 5, clForest, 7, clHill, 1, clPlayer, 12, clMetal, 1, clRock, 1));

setWaterWaviness(3.0);
setWaterType("lake");
// Export map data

ExportMap();
