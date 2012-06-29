RMS.LoadLibrary("rmgen");


//random terrain textures
var rt = randomizeBiome();

var tGrass = rBiomeT1();
var tGrassPForest = rBiomeT2();
var tGrassDForest = rBiomeT3();
var tCliff = rBiomeT4();
var tGrassA = rBiomeT5();
var tGrassB = rBiomeT6();
var tGrassC = rBiomeT7();
var tHill = rBiomeT8();
var tDirt = rBiomeT9();
var tRoad = rBiomeT10();
var tRoadWild = rBiomeT11();
var tGrassPatch = rBiomeT12();
var tShoreBlend = rBiomeT13();
var tShore = rBiomeT14();
var tWater = rBiomeT15();
if (rt == 2)
{
	tShore = "alpine_shore_rocks_icy";
	tWater = "alpine_shore_rocks";
}
else if (rt == 7)
{
	tShore = "tropic_dirt_b_plants";
	tWater = "tropic_dirt_b";
}
// gaia entities
var oOak = rBiomeE1();
var oOakLarge = rBiomeE2();
var oApple = rBiomeE3();
var oPine = rBiomeE4();
var oAleppoPine = rBiomeE5();
var oBerryBush = rBiomeE6();
var oChicken = rBiomeE7();
var oDeer = rBiomeE8();
var oFish = rBiomeE9();
var oSheep = rBiomeE10();
var oStoneLarge = rBiomeE11();
var oStoneSmall = rBiomeE12();
var oMetalLarge = rBiomeE13();

// decorative props
var aGrass = rBiomeA1();
var aGrassShort = rBiomeA2();
var aReeds = rBiomeA3();
var aLillies = rBiomeA4();
var aRockLarge = rBiomeA5();
var aRockMedium = rBiomeA6();
var aBushMedium = rBiomeA7();
var aBushSmall = rBiomeA8();

var pForestD = [tGrassDForest + TERRAIN_SEPARATOR + oOak, tGrassDForest + TERRAIN_SEPARATOR + oOakLarge, tGrassDForest];
var pForestP = [tGrassPForest + TERRAIN_SEPARATOR + oPine, tGrassPForest + TERRAIN_SEPARATOR + oAleppoPine, tGrassPForest];
const BUILDING_ANGlE = -PI/4;

// initialize map

log("Initializing map...");

InitMap();

var numPlayers = getNumPlayers();
var mapSize = getMapSize();
var mapArea = mapSize*mapSize;

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
			placeTerrain(ix, iz, tGrass);
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
			[new SimpleObject(oChicken, 5,5, 0,3)],
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
		for (m = 0; m < numPlayers+1; m++)
		{
			for (n = 0; n < numPlayers+1; n++)
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
log("Creating bumps...");
placer = new ClumpPlacer(scaleByMapSize(20, 50), 0.3, 0.06, 1);
painter = new SmoothElevationPainter(ELEVATION_MODIFY, 2, 2);
createAreas(
	placer,
	painter, 
	avoidClasses(clWater, 2, clPlayer, 10),
	scaleByMapSize(100, 200)
);
if ((rt == 7)||(rt == 5))
{
	// create river bumbs
	log("Creating river bumps...");
	placer = new ClumpPlacer(scaleByMapSize(4, 6), 0.3, 0.06, 1);
	painter = new SmoothElevationPainter(ELEVATION_MODIFY, 2, 2);
	createAreas(
		placer,
		painter, 
		stayClasses(clWater, 3),
		scaleByMapSize(500, 700)
	);
}
// create hills
log("Creating hills...");
placer = new ClumpPlacer(scaleByMapSize(20, 150), 0.2, 0.1, 1);
terrainPainter = new LayeredPainter(
	[tGrass, tCliff, tHill],		// terrains
	[1, 2]								// widths
);
elevationPainter = new SmoothElevationPainter(ELEVATION_SET, 18, 2);
createAreas(
	placer,
	[terrainPainter, elevationPainter, paintClass(clHill)], 
	avoidClasses(clPlayer, 12, clHill, 15, clWater, 2),
	scaleByMapSize(1, 4) * numPlayers
);


// calculate desired number of trees for map (based on size)
if (rt == 6)
{
var MIN_TREES = 200;
var MAX_TREES = 1250;
var P_FOREST = 0.02;
}
else if (rt == 7)
{
var MIN_TREES = 1000;
var MAX_TREES = 6000;
var P_FOREST = 0.6;
}
else
{
var MIN_TREES = 500;
var MAX_TREES = 3000;
var P_FOREST = 0.7;
}
var totalTrees = scaleByMapSize(MIN_TREES, MAX_TREES);
var numForest = totalTrees * P_FOREST;
var numStragglers = totalTrees * (1.0 - P_FOREST);

// create forests
log("Creating forests...");
var types = [
	[[tGrassDForest, tGrass, pForestD], [tGrassDForest, pForestD]],
	[[tGrassPForest, tGrass, pForestP], [tGrassPForest, pForestP]]
];	// some variation

if (rt == 6)
{
var size = numForest / (0.5 * scaleByMapSize(2,8) * numPlayers);
}
else
{
var size = numForest / (scaleByMapSize(2,8) * numPlayers);
}
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
		avoidClasses(clPlayer, 12, clForest, 10, clHill, 0, clWater, 2),
		num
	);
}

RMS.SetProgress(60);

// create dirt patches
log("Creating dirt patches...");
var sizes = [scaleByMapSize(3, 48), scaleByMapSize(5, 84), scaleByMapSize(8, 128)];
for (var i = 0; i < sizes.length; i++)
{
	placer = new ClumpPlacer(sizes[i], 0.3, 0.06, 0.5);
	painter = new LayeredPainter(
		[[tGrass,tGrassA],[tGrassA,tGrassB], [tGrassB,tGrassC]], 		// terrains
		[1,1]															// widths
	);
	createAreas(
		placer,
		[painter, paintClass(clDirt)],
		avoidClasses(clWater, 3, clForest, 0, clHill, 0, clDirt, 5, clPlayer, 12),
		scaleByMapSize(15, 45)
	);
}

// create grass patches
log("Creating grass patches...");
var sizes = [scaleByMapSize(2, 32), scaleByMapSize(3, 48), scaleByMapSize(5, 80)];
for (var i = 0; i < sizes.length; i++)
{
	placer = new ClumpPlacer(sizes[i], 0.3, 0.06, 0.5);
	painter = new TerrainPainter(tGrassPatch);
	createAreas(
		placer,
		painter,
		avoidClasses(clWater, 3, clForest, 0, clHill, 0, clDirt, 5, clPlayer, 12),
		scaleByMapSize(15, 45)
	);
}
RMS.SetProgress(65);


log("Creating stone mines...");
// create large stone quarries
group = new SimpleGroup([new SimpleObject(oStoneSmall, 0,2, 0,4), new SimpleObject(oStoneLarge, 1,1, 0,4)], true, clRock);
createObjectGroups(group, 0,
	avoidClasses(clWater, 3, clForest, 1, clPlayer, 10, clRock, 10, clHill, 1),
	scaleByMapSize(4,16), 100
);

// create small stone quarries
group = new SimpleGroup([new SimpleObject(oStoneSmall, 2,5, 1,3)], true, clRock);
createObjectGroups(group, 0,
	avoidClasses(clWater, 3, clForest, 1, clPlayer, 10, clRock, 10, clHill, 1),
	scaleByMapSize(4,16), 100
);

log("Creating metal mines...");
// create large metal quarries
group = new SimpleGroup([new SimpleObject(oMetalLarge, 1,1, 0,4)], true, clMetal);
createObjectGroups(group, 0,
	avoidClasses(clWater, 3, clForest, 1, clPlayer, 10, clMetal, 10, clRock, 5, clHill, 1),
	scaleByMapSize(4,16), 100
);

RMS.SetProgress(70);

// create small decorative rocks
log("Creating small decorative rocks...");
group = new SimpleGroup(
	[new SimpleObject(aRockMedium, 1,3, 0,1)],
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
	[new SimpleObject(aRockLarge, 1,2, 0,1), new SimpleObject(aRockMedium, 1,3, 0,2)],
	true
);
createObjectGroups(
	group, 0,
	avoidClasses(clWater, 0, clForest, 0, clPlayer, 0, clHill, 0),
	scaleByMapSize(8, 131), 50
);

RMS.SetProgress(75);

// create deer
log("Creating deer...");
group = new SimpleGroup(
	[new SimpleObject(oDeer, 5,7, 0,4)],
	true, clFood
);
createObjectGroups(group, 0,
	avoidClasses(clWater, 3, clForest, 0, clPlayer, 10, clHill, 1, clFood, 20),
	3 * numPlayers, 50
);

// create sheep
log("Creating sheep...");
group = new SimpleGroup(
	[new SimpleObject(oSheep, 2,3, 0,2)],
	true, clFood
);
createObjectGroups(group, 0,
	avoidClasses(clWater, 3, clForest, 0, clPlayer, 10, clHill, 1, clFood, 20),
	3 * numPlayers, 50
);

// create fish
log("Creating fish...");
group = new SimpleGroup(
	[new SimpleObject(oFish, 2,3, 0,2)],
	true, clFood
);
createObjectGroups(group, 0,
	[avoidClasses(clFood, 20), stayClasses(clWater, 6)],
	25 * numPlayers, 60
);

RMS.SetProgress(85);


// create straggler trees
log("Creating straggler trees...");
var types = [oOak, oOakLarge, oPine, oApple];	// some variation
var num = floor(numStragglers / types.length);
for (var i = 0; i < types.length; ++i)
{
	group = new SimpleGroup(
		[new SimpleObject(types[i], 1,1, 0,3)],
		true, clForest
	);
	createObjectGroups(group, 0,
		avoidClasses(clWater, 5, clForest, 1, clHill, 1, clPlayer, 12, clMetal, 1, clRock, 1),
		num
	);
}


//create small grass tufts
log("Creating small grass tufts...");
var planetm = 1;
if (rt==7)
{
	planetm = 8;
}
group = new SimpleGroup(
	[new SimpleObject(aGrassShort, 1,2, 0,1, -PI/8,PI/8)]
);
createObjectGroups(group, 0,
	avoidClasses(clWater, 2, clHill, 2, clPlayer, 2, clDirt, 0),
	planetm * scaleByMapSize(13, 200)
);

RMS.SetProgress(90);

// create large grass tufts
log("Creating large grass tufts...");
group = new SimpleGroup(
	[new SimpleObject(aGrass, 2,4, 0,1.8, -PI/8,PI/8), new SimpleObject(aGrassShort, 3,6, 1.2,2.5, -PI/8,PI/8)]
);
createObjectGroups(group, 0,
	avoidClasses(clWater, 3, clHill, 2, clPlayer, 2, clDirt, 1, clForest, 0),
	planetm * scaleByMapSize(13, 200)
);

RMS.SetProgress(95);

// create bushes
log("Creating bushes...");
group = new SimpleGroup(
	[new SimpleObject(aBushMedium, 1,2, 0,2), new SimpleObject(aBushSmall, 2,4, 0,2)]
);
createObjectGroups(group, 0,
	avoidClasses(clWater, 2, clHill, 1, clPlayer, 1, clDirt, 1),
	planetm * scaleByMapSize(13, 200), 50
);

// create shallow flora
log("Creating shallow flora...");
group = new SimpleGroup(
	[new SimpleObject(aLillies, 1,2, 0,2), new SimpleObject(aReeds, 2,4, 0,2)]
);
createObjectGroups(group, 0,
	stayClasses(clShallow, 1),
	60 * scaleByMapSize(13, 200), 80
);


rt = randInt(1,3)
if (rt==1){
setSkySet("cirrus");
}
else if (rt ==2){
setSkySet("cumulus");
}
else if (rt ==3){
setSkySet("sunny");
}
setSunRotation(randFloat(0, TWO_PI));
setSunElevation(randFloat(PI/ 5, PI / 3));
setWaterTint(0.447, 0.412, 0.322);				// muddy brown
setWaterReflectionTint(0.447, 0.412, 0.322);	// muddy brown
setWaterMurkiness(1.0);
setWaterReflectionTintStrength(0.677);

// Export map data

ExportMap();