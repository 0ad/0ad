RMS.LoadLibrary("rmgen");

TILE_CENTERED_HEIGHT_MAP = true;
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
const tShore = rBiomeT14();
const tWater = rBiomeT15();

// gaia entities
const oTree1 = rBiomeE1();
const oTree2 = rBiomeE2();
const oTree3 = rBiomeE3();
const oTree4 = rBiomeE4();
const oTree5 = rBiomeE5();
const oFruitBush = rBiomeE6();
const oMainHuntableAnimal = rBiomeE8();
const oFish = rBiomeE9();
const oSecondaryHuntableAnimal = rBiomeE10();
const oStoneLarge = rBiomeE11();
const oStoneSmall = rBiomeE12();
const oMetalLarge = rBiomeE13();
const oWood = "gaia/special_treasure_wood";

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
var clShallow = createTileClass();

for (var ix = 0; ix < mapSize; ix++)
{
	for (var iz = 0; iz < mapSize; iz++)
	{
		var x = ix / (mapSize + 1.0);
		var z = iz / (mapSize + 1.0);
			placeTerrain(ix, iz, tWater);
	}
}

var iberianTowers = false;
var md = randInt(1,13);
var needsAdditionalWood = false;
//*****************************************************************************************************************************
if (md == 1) //archipelago and island
{
	needsAdditionalWood = true;
	iberianTowers = true;

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

	var mdd1 = randInt(1,3);

	for (var i = 0; i < numPlayers; ++i)
	{
		var radius = scaleByMapSize(17, 29);
		var shoreRadius = 4;
		var elevation = 3;

		var hillSize = PI * radius * radius;
		// get the x and z in tiles
		var fx = fractionToTiles(playerX[i]);
		var fz = fractionToTiles(playerZ[i]);
		var ix = round(fx);
		var iz = round(fz);
		// create a player island
		var placer = new ClumpPlacer(hillSize, 0.80, 0.1, 10, ix, iz);
		var terrainPainter = new LayeredPainter(
			[tMainTerrain , tMainTerrain, tMainTerrain],		// terrains
			[1, shoreRadius]		// widths
		);
		var elevationPainter = new SmoothElevationPainter(
			ELEVATION_SET,			// type
			elevation,				// elevation
			shoreRadius				// blend radius
		);
		createArea(placer, [terrainPainter, elevationPainter, paintClass(clPlayer)], null);
	}
	if (mdd1 == 1) //archipelago
	{
		// create islands
		log("Creating islands...");
		placer = new ClumpPlacer(floor(hillSize*randFloat(0.8,1.2)), 0.80, 0.1, 10);
		terrainPainter = new LayeredPainter(
			[tMainTerrain, tMainTerrain],		// terrains
			[2]								// widths
		);
		elevationPainter = new SmoothElevationPainter(ELEVATION_SET, 3, 4);
		createAreas(
			placer,
			[terrainPainter, elevationPainter, paintClass(clLand)],
			null,
			scaleByMapSize(2, 5)*randInt(8,14)
		);

		// create shore jaggedness
		log("Creating shore jaggedness...");
		placer = new ClumpPlacer(scaleByMapSize(15, 80), 0.2, 0.1, 1);
		terrainPainter = new LayeredPainter(
			[tCliff, tHill],		// terrains
			[2]								// widths
		);
		elevationPainter = new SmoothElevationPainter(ELEVATION_SET, 3, 4);
		createAreas(
			placer,
			[terrainPainter, elevationPainter, paintClass(clLand)],
			borderClasses(clLand, 6, 3),
			scaleByMapSize(12, 130) * 2, 150
		);
	}
	else if (mdd1 == 2) //islands
	{
		// create islands
		log("Creating islands...");
		placer = new ClumpPlacer(floor(hillSize*randFloat(0.6,1.4)), 0.80, 0.1, randFloat(0.0, 0.2));
		terrainPainter = new LayeredPainter(
			[tMainTerrain, tMainTerrain],		// terrains
			[2]								// widths
		);
		elevationPainter = new SmoothElevationPainter(ELEVATION_SET, 3, 4);
		createAreas(
			placer,
			[terrainPainter, elevationPainter, paintClass(clLand)],
			avoidClasses(clLand, 3, clPlayer, 3),
			scaleByMapSize(6, 10)*randInt(8,14)
		);

		// create small islands
		log("Creating small islands...");
		placer = new ClumpPlacer(floor(hillSize*randFloat(0.3,0.7)), 0.80, 0.1, 0.07);
		terrainPainter = new LayeredPainter(
			[tMainTerrain, tMainTerrain],		// terrains
			[2]								// widths
		);
		elevationPainter = new SmoothElevationPainter(ELEVATION_SET, 3, 6);
		createAreas(
			placer,
			[terrainPainter, elevationPainter, paintClass(clLand)],
			avoidClasses(clLand, 3, clPlayer, 3),
			scaleByMapSize(2, 6)*randInt(6,15), 25
		);
	}
	else if (mdd1 == 3) // tight islands
	{
		// create islands
		log("Creating islands...");
		placer = new ClumpPlacer(floor(hillSize*randFloat(0.8,1.2)), 0.80, 0.1, 10);
		terrainPainter = new LayeredPainter(
			[tMainTerrain, tMainTerrain],		// terrains
			[2]								// widths
		);
		elevationPainter = new SmoothElevationPainter(ELEVATION_SET, 3, 4);
		createAreas(
			placer,
			[terrainPainter, elevationPainter, paintClass(clLand)],
			avoidClasses(clLand, randInt(8, 16), clPlayer, 3),
			scaleByMapSize(2, 5)*randInt(8,14)
		);
	}

}
//********************************************************************************************************
else if (md == 2) //continent
{

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
		playerX[i] = 0.5 + 0.25*cos(playerAngle[i]);
		playerZ[i] = 0.5 + 0.25*sin(playerAngle[i]);

		var fx = fractionToTiles(playerX[i]);
		var fz = fractionToTiles(playerZ[i]);
		var ix = round(fx);
		var iz = round(fz);
		addToClass(ix, iz, clPlayer);
		addToClass(ix+5, iz, clPlayer);
		addToClass(ix, iz+5, clPlayer);
		addToClass(ix-5, iz, clPlayer);
		addToClass(ix, iz-5, clPlayer);
	}

	var fx = fractionToTiles(0.5);
	var fz = fractionToTiles(0.5);
	ix = round(fx);
	iz = round(fz);

	var placer = new ClumpPlacer(mapArea * 0.45, 0.9, 0.09, 10, ix, iz);
	var terrainPainter = new LayeredPainter(
		[tWater, tShore, tMainTerrain],		// terrains
		[4, 2]		// widths
	);
	var elevationPainter = new SmoothElevationPainter(
		ELEVATION_SET,			// type
		3,				// elevation
		4				// blend radius
	);
	createArea(placer, [terrainPainter, elevationPainter, paintClass(clLand)], null);

	var clPeninsulaSteam = createTileClass();

	if (randInt(1,3)==1) // peninsula
	{
		var angle = randFloat(0, TWO_PI);

		var fx = fractionToTiles(0.5 + 0.25*cos(angle));
		var fz = fractionToTiles(0.5 + 0.25*sin(angle));
		ix = round(fx);
		iz = round(fz);

		var placer = new ClumpPlacer(mapArea * 0.45, 0.9, 0.09, 10, ix, iz);
		var terrainPainter = new LayeredPainter(
			[tWater, tShore, tMainTerrain],		// terrains
			[4, 2]		// widths
		);
		var elevationPainter = new SmoothElevationPainter(
			ELEVATION_SET,			// type
			3,				// elevation
			4				// blend radius
		);
		createArea(placer, [terrainPainter, elevationPainter, paintClass(clLand)], null);

		var fx = fractionToTiles(0.5 + 0.35*cos(angle));
		var fz = fractionToTiles(0.5 + 0.35*sin(angle));
		ix = round(fx);
		iz = round(fz);

		var placer = new ClumpPlacer(mapArea * 0.3, 0.9, 0.01, 10, ix, iz);
		createArea(placer, [paintClass(clPeninsulaSteam)], null);
	}

	// create shore jaggedness
	log("Creating shore jaggedness...");
	placer = new ClumpPlacer(scaleByMapSize(20, 150), 0.2, 0.1, 1);
	terrainPainter = new LayeredPainter(
		[tCliff, tHill],		// terrains
		[2]								// widths
	);
	elevationPainter = new SmoothElevationPainter(ELEVATION_SET, -5, 4);
	createAreas(
		placer,
		[terrainPainter, elevationPainter, unPaintClass(clLand)],
		[avoidClasses(clPlayer, 20, clPeninsulaSteam, 20), borderClasses(clLand, 7, 7)],
		scaleByMapSize(7, 130) * 2, 150
	);

	// create outward shore jaggedness
	log("Creating shore jaggedness...");
	placer = new ClumpPlacer(scaleByMapSize(20, 150), 0.2, 0.1, 1);
	terrainPainter = new LayeredPainter(
		[tCliff, tHill],		// terrains
		[2]								// widths
	);
	elevationPainter = new SmoothElevationPainter(ELEVATION_SET, 3, 4);
	createAreas(
		placer,
		[terrainPainter, elevationPainter, paintClass(clLand)],
		[avoidClasses(clPlayer, 20), borderClasses(clLand, 7, 7)],
		scaleByMapSize(7, 130) * 2, 150
	);
}
//********************************************************************************************************
else if (md == 3) //central sea
{
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
	var playerPos = new Array(numPlayers);
	var iop = 0;

	var mdd1 = randInt(1,2);
	if (mdd1 == 1) //vertical
	{
		for (var i = 0; i < numPlayers; i++)
		{
			iop = i - 1;
			if (!(numPlayers%2)){
				playerPos[i] = ((iop + abs(iop%2))/2 + 1) / ((numPlayers / 2) + 1);
			}
			else
			{
				if (iop%2)
				{
					playerPos[i] = ((iop + abs(iop%2))/2 + 1) / (((numPlayers + 1) / 2) + 1);
				}
				else
				{
					playerPos[i] = ((iop)/2 + 1) / ((((numPlayers - 1)) / 2) + 1);
				}
			}
			playerZ[i] = playerPos[i];
			playerX[i] = 0.2 + 0.6*(i%2);
		}
	}
	else //horizontal
	{
		for (var i = 0; i < numPlayers; i++)
		{
			iop = i - 1;
			if (!(numPlayers%2)){
				playerPos[i] = ((iop + abs(iop%2))/2 + 1) / ((numPlayers / 2) + 1);
			}
			else
			{
				if (iop%2)
				{
					playerPos[i] = ((iop + abs(iop%2))/2 + 1) / (((numPlayers + 1) / 2) + 1);
				}
				else
				{
					playerPos[i] = ((iop)/2 + 1) / ((((numPlayers - 1)) / 2) + 1);
				}
			}
			playerZ[i] = 0.2 + 0.6*(i%2);
			playerX[i] = playerPos[i];
		}
	}


	var WATER_WIDTH = randFloat(0.22,0.3)+scaleByMapSize(1,4)/20;
	log("Creating sea");
	var theta = randFloat(0, 1);
	var theta2 = randFloat(0, 1);
	var seed = randFloat(2,3);
	var seed2 = randFloat(2,3);
	for (var ix = 0; ix < mapSize; ix++)
	{
		for (var iz = 0; iz < mapSize; iz++)
		{
			var x = ix / (mapSize + 1.0);
			var z = iz / (mapSize + 1.0);

			// add the rough shape of the water
			var km = 20/scaleByMapSize(35, 160);

			var fadeDist = 0.05;

			if (mdd1 == 1) //vertical
			{
				var cu = km*rndRiver(theta+z*0.5*(mapSize/64),seed);
				var cu2 = km*rndRiver(theta2+z*0.5*(mapSize/64),seed2);

				if ((x > cu + 0.5 - WATER_WIDTH/2) && (x < cu2 + 0.5 + WATER_WIDTH/2))
				{
					var h;
					if (x < (cu + 0.5 + fadeDist - WATER_WIDTH/2))
					{
						h = 3 - 6 * (1 - ((cu + 0.5 + fadeDist - WATER_WIDTH/2) - x)/fadeDist);
					}
					else if (x > (cu2 + 0.5 - fadeDist + WATER_WIDTH/2))
					{
						h = 3 - 6 * (1 - (x - (cu2 + 0.5 - fadeDist + WATER_WIDTH/2))/fadeDist);
					}
					else
					{
						h = -3.0;
					}

					if (h < -1.5)
					{
						placeTerrain(ix, iz, tWater);
					}
					else
					{
						placeTerrain(ix, iz, tShore);
					}

					setHeight(ix, iz, h);
					if (h < 0){
						addToClass(ix, iz, clWater);
					}
				}
				else
				{
					setHeight(ix, iz, 3.1);
					addToClass(ix, iz, clLand);
				}
			}
			else //horizontal
			{
				var cu = km*rndRiver(theta+x*0.5*(mapSize/64),seed);
				var cu2 = km*rndRiver(theta2+x*0.5*(mapSize/64),seed2);

				if ((z > cu + 0.5 - WATER_WIDTH/2) && (z < cu2 + 0.5 + WATER_WIDTH/2))
				{
					var h;
					if (z < (cu + 0.5 + fadeDist - WATER_WIDTH/2))
					{
						h = 3 - 6 * (1 - ((cu + 0.5 + fadeDist - WATER_WIDTH/2) - z)/fadeDist);
					}
					else if (z > (cu2 + 0.5 - fadeDist + WATER_WIDTH/2))
					{
						h = 3 - 6 * (1 - (z - (cu2 + 0.5 - fadeDist + WATER_WIDTH/2))/fadeDist);
					}
					else
					{
						h = -3.0;
					}

					if (h < -1.5)
					{
						placeTerrain(ix, iz, tWater);
					}
					else
					{
						placeTerrain(ix, iz, tShore);
					}

					setHeight(ix, iz, h);
					if (h < 0){
						addToClass(ix, iz, clWater);
					}
				}
				else
				{
					setHeight(ix, iz, 3.1);
					addToClass(ix, iz, clLand);
				}
			}
		}
	}

	if (!randInt(3))
	{
		// linked
		if (mdd1 == 1) //vertical
		{
			var placer = new PathPlacer(1, fractionToTiles(0.5), fractionToTiles(0.99), fractionToTiles(0.5), scaleByMapSize(randInt(16,24),randInt(100,140)), 0.5, 3*(scaleByMapSize(1,4)), 0.1, 0.01);
		}
		else
		{
			var placer = new PathPlacer(fractionToTiles(0.5), 1, fractionToTiles(0.5), fractionToTiles(0.99), scaleByMapSize(randInt(16,24),randInt(100,140)), 0.5, 3*(scaleByMapSize(1,4)), 0.1, 0.01);
		}
		var terrainPainter = new LayeredPainter(
			[tMainTerrain, tMainTerrain, tMainTerrain],		// terrains
			[1, 3]								// widths
		);
		var elevationPainter = new SmoothElevationPainter(
			ELEVATION_SET,			// type
			3.1,				// elevation
			4				// blend radius
		);
		createArea(placer, [terrainPainter, elevationPainter, unPaintClass(clWater)], null);
	}
	var mdd2 = randInt(1,7);
	if (mdd2 == 1)
	{
		// create islands
		log("Creating islands...");
		placer = new ClumpPlacer(randInt(scaleByMapSize(8,15),scaleByMapSize(15,23))*randInt(scaleByMapSize(8,15),scaleByMapSize(15,23)), 0.80, 0.1, randFloat(0.0, 0.2));
		terrainPainter = new LayeredPainter(
			[tMainTerrain, tMainTerrain],		// terrains
			[2]								// widths
		);
		elevationPainter = new SmoothElevationPainter(ELEVATION_SET, 3.1, 4);
		createAreas(
			placer,
			[terrainPainter, elevationPainter, paintClass(clLand)],
			avoidClasses(clLand, 3, clPlayer, 3),
			scaleByMapSize(2, 5)*randInt(8,14)
		);
	}
	else if (mdd2 == 2)
	{
		// create extentions
		log("Creating extentions...");
		placer = new ClumpPlacer(randInt(scaleByMapSize(13,24),scaleByMapSize(24,45))*randInt(scaleByMapSize(13,24),scaleByMapSize(24,45)), 0.80, 0.1, 10);
		terrainPainter = new LayeredPainter(
			[tMainTerrain, tMainTerrain],		// terrains
			[2]								// widths
		);
		elevationPainter = new SmoothElevationPainter(ELEVATION_SET, 3.1, 4);
		createAreas(
			placer,
			[terrainPainter, elevationPainter, paintClass(clLand)],
			null,
			scaleByMapSize(2, 5)*randInt(8,14)
		);
	}
}
//********************************************************************************************************
else if (md == 4) //central river
{

	for (var ix = 0; ix < mapSize; ix++)
	{
		for (var iz = 0; iz < mapSize; iz++)
		{
			var x = ix / (mapSize + 1.0);
			var z = iz / (mapSize + 1.0);
				setHeight(ix, iz, 3);
		}
	}

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
	var playerPos = new Array(numPlayers);
	var iop = 0;
	var mdd1 = randInt(1,2);
	if (mdd1 == 1) //horizontal
	{
		for (var i = 0; i < numPlayers; i++)
		{
			iop = i - 1;
			if (!(numPlayers%2)){
				playerPos[i] = ((iop + abs(iop%2))/2 + 1) / ((numPlayers / 2) + 1);
			}
			else
			{
				if (iop%2)
				{
					playerPos[i] = ((iop + abs(iop%2))/2 + 1) / (((numPlayers + 1) / 2) + 1);
				}
				else
				{
					playerPos[i] = ((iop)/2 + 1) / ((((numPlayers - 1)) / 2) + 1);
				}
			}

			playerZ[i] = 0.25 + 0.5*(i%2);
			playerX[i] = playerPos[i];
		}
	}
	else //vertical
	{
		for (var i = 0; i < numPlayers; i++)
		{
			iop = i - 1;
			if (!(numPlayers%2)){
				playerPos[i] = ((iop + abs(iop%2))/2 + 1) / ((numPlayers / 2) + 1);
			}
			else
			{
				if (iop%2)
				{
					playerPos[i] = ((iop + abs(iop%2))/2 + 1) / (((numPlayers + 1) / 2) + 1);
				}
				else
				{
					playerPos[i] = ((iop)/2 + 1) / ((((numPlayers - 1)) / 2) + 1);
				}
			}

			playerZ[i] = playerPos[i];
			playerX[i] = 0.25 + 0.5*(i%2);
		}
	}

	// create the main river
	log("Creating the main river");

	if (mdd1 == 2)
		var placer = new PathPlacer(fractionToTiles(0.5), 1, fractionToTiles(0.5) , fractionToTiles(0.99), scaleByMapSize(14,24), 0.5, 3*(scaleByMapSize(1,4)), 0.1, 0.01);
	else
		var placer = new PathPlacer(1, fractionToTiles(0.5), fractionToTiles(0.99), fractionToTiles(0.5), scaleByMapSize(14,24), 0.5, 3*(scaleByMapSize(1,4)), 0.1, 0.01);

	var terrainPainter = new LayeredPainter(
		[tShore, tWater, tWater],		// terrains
		[1, 3]								// widths
	);
	var elevationPainter = new SmoothElevationPainter(
		ELEVATION_SET,			// type
		-4,				// elevation
		4				// blend radius
	);
	createArea(placer, [terrainPainter, elevationPainter], avoidClasses(clPlayer, 4));

	if (mdd1 == 1)
		placer = new ClumpPlacer(floor(PI*scaleByMapSize(10,20)*scaleByMapSize(10,20)/4), 0.95, 0.6, 10, 1, fractionToTiles(0.5));
	else
		placer = new ClumpPlacer(floor(PI*scaleByMapSize(10,20)*scaleByMapSize(10,20)/4), 0.95, 0.6, 10, fractionToTiles(0.5), 1);

	var painter = new LayeredPainter([tWater, tWater], [1]);
	var elevationPainter = new SmoothElevationPainter(ELEVATION_SET, -4, 2);
	createArea(placer, [painter, elevationPainter], avoidClasses(clPlayer, 8));

	if (mdd1 == 1)
		placer = new ClumpPlacer(floor(PI*scaleByMapSize(10,20)*scaleByMapSize(10,20)/4), 0.95, 0.6, 10, fractionToTiles(0.99), fractionToTiles(0.5));
	else
		placer = new ClumpPlacer(floor(PI*scaleByMapSize(10,20)*scaleByMapSize(10,20)/4), 0.95, 0.6, 10, fractionToTiles(0.5), fractionToTiles(0.99));

	var painter = new LayeredPainter([tWater, tWater], [1]);
	var elevationPainter = new SmoothElevationPainter(ELEVATION_SET, -4, 2);
	createArea(placer, [painter, elevationPainter], avoidClasses(clPlayer, 8));

	var mdd2 = randInt(1,2);
	if (mdd2 == 1)
	{
		// create the shallows of the main river
		log("Creating the shallows of the main river");

		for (var i = 0; i <= randInt(1, scaleByMapSize(4,8)); i++)
		{
			var cLocation = randFloat(0.15,0.85);
			if (mdd1 == 1)
				passageMaker(floor(fractionToTiles(cLocation)), floor(fractionToTiles(0.35)), floor(fractionToTiles(cLocation)), floor(fractionToTiles(0.65)), scaleByMapSize(4,8), -2, -2, 2, clShallow, undefined, -4);
			else
				passageMaker(floor(fractionToTiles(0.35)), floor(fractionToTiles(cLocation)), floor(fractionToTiles(0.65)), floor(fractionToTiles(cLocation)), scaleByMapSize(4,8), -2, -2, 2, clShallow, undefined, -4);
		}
	}

	if (randInt(1,2) == 1)
	{
		for (var i = 0; i < numPlayers; i++)
		{
			var fx = fractionToTiles(playerX[i]);
			var fz = fractionToTiles(playerZ[i]);
			var ix = round(fx);
			var iz = round(fz);
			// create the city patch
			var cityRadius = scaleByMapSize(17,29)/3;
			placer = new ClumpPlacer(PI*cityRadius*cityRadius, 0.6, 0.3, 10, ix, iz);
			createArea(placer, paintClass(clPlayer), null);
		}

		// create tributaries
		log("Creating tributaries");

		for (var i = 0; i <= randInt(8, (scaleByMapSize(12,20))); i++)
		{
			var cLocation = randFloat(0.05,0.95);
			var tang = randFloat(PI*0.2, PI*0.8)*((randInt(2)-0.5)*2);
			if (tang > 0)
			{
				var cDistance = 0.05;
			}
			else
			{
				var cDistance = -0.05;
			}
			if (mdd1 == 1)
				var point = getTIPIADBON([fractionToTiles(cLocation), fractionToTiles(0.5 + cDistance)], [fractionToTiles(cLocation), fractionToTiles(0.5 - cDistance)], [-6, -1.5], 0.5, 5, 0.01);
			else
				var point = getTIPIADBON([fractionToTiles(0.5 + cDistance), fractionToTiles(cLocation)], [fractionToTiles(0.5 - cDistance), fractionToTiles(cLocation)], [-6, -1.5], 0.5, 5, 0.01);

			if (point !== undefined)
			{
				if (mdd1 == 1)
					var placer = new PathPlacer(floor(point[0]), floor(point[1]), floor(fractionToTiles(0.5 + 0.49*cos(tang))), floor(fractionToTiles(0.5 + 0.49*sin(tang))), scaleByMapSize(10,20), 0.4, 3*(scaleByMapSize(1,4)), 0.1, 0.05);
				else
					var placer = new PathPlacer(floor(point[0]), floor(point[1]), floor(fractionToTiles(0.5 + 0.49*sin(tang))), floor(fractionToTiles(0.5 + 0.49*cos(tang))), scaleByMapSize(10,20), 0.4, 3*(scaleByMapSize(1,4)), 0.1, 0.05);


				var terrainPainter = new LayeredPainter(
					[tShore, tWater, tWater],		// terrains
					[1, 3]								// widths
				);
				var elevationPainter = new SmoothElevationPainter(
					ELEVATION_SET,			// type
					-4,				// elevation
					4				// blend radius
				);
				var success = createArea(placer, [terrainPainter, elevationPainter, paintClass(clWater)], avoidClasses(clPlayer, 3, clWater, 3, clShallow, 2));
				if (success !== undefined)
				{
					if (mdd1 == 1)
						placer = new ClumpPlacer(floor(PI*scaleByMapSize(10,20)*scaleByMapSize(10,20)/4), 0.95, 0.6, 10, fractionToTiles(0.5 + 0.49*cos(tang)), fractionToTiles(0.5 + 0.49*sin(tang)));
					else
						placer = new ClumpPlacer(floor(PI*scaleByMapSize(10,20)*scaleByMapSize(10,20)/4), 0.95, 0.6, 10, fractionToTiles(0.5 + 0.49*sin(tang)), fractionToTiles(0.5 + 0.49*cos(tang)));

					var painter = new LayeredPainter([tWater, tWater], [1]);
					var elevationPainter = new SmoothElevationPainter(ELEVATION_SET, -4, 2);
					createArea(placer, [painter, elevationPainter], avoidClasses(clPlayer, 3));
				}
			}
		}
	}
}
//********************************************************************************************************
else if (md == 5) //rivers and lake
{

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

		var fx = fractionToTiles(playerX[i]);
		var fz = fractionToTiles(playerZ[i]);
		var ix = round(fx);
		var iz = round(fz);
		addToClass(ix, iz, clPlayer);
		addToClass(ix+5, iz, clPlayer);
		addToClass(ix, iz+5, clPlayer);
		addToClass(ix-5, iz, clPlayer);
		addToClass(ix, iz-5, clPlayer);
	}



	for (var ix = 0; ix < mapSize; ix++)
	{
		for (var iz = 0; iz < mapSize; iz++)
		{
			var x = ix / (mapSize + 1.0);
			var z = iz / (mapSize + 1.0);
				setHeight(ix, iz, 3);
		}
	}
	var mdd1 = randInt(1,2);
	if (mdd1 == 1) //lake
	{
		var fx = fractionToTiles(0.5);
		var fz = fractionToTiles(0.5);
		ix = round(fx);
		iz = round(fz);

		var lSize = sqrt(sqrt(sqrt(scaleByMapSize(1, 6))));

		var placer = new ClumpPlacer(mapArea * 0.09 * lSize, 0.7, 0.1, 10, ix, iz);
		var terrainPainter = new LayeredPainter(
			[tShore, tWater, tWater, tWater],		// terrains
			[1, 4, 2]		// widths
		);
		var elevationPainter = new SmoothElevationPainter(
			ELEVATION_SET,			// type
			-4,				// elevation
			4				// blend radius
		);
		createArea(placer, [terrainPainter, elevationPainter, paintClass(clWater)], null);

		// create shore jaggedness
		log("Creating shore jaggedness...");
		placer = new ClumpPlacer(scaleByMapSize(20, 150), 0.2, 0.1, 1);
		terrainPainter = new LayeredPainter(
			[tCliff, tHill],		// terrains
			[2]								// widths
		);
		elevationPainter = new SmoothElevationPainter(ELEVATION_SET, -5, 4);
		createAreas(
			placer,
			[terrainPainter, elevationPainter, paintClass(clWater)],
			[avoidClasses(clPlayer, 20), borderClasses(clWater, 6, 4)],
			scaleByMapSize(7, 130) * 2, 150
		);

		placer = new ClumpPlacer(scaleByMapSize(15, 80), 0.2, 0.1, 1);
		terrainPainter = new LayeredPainter(
			[tCliff, tHill],		// terrains
			[2]								// widths
		);
		elevationPainter = new SmoothElevationPainter(ELEVATION_SET, 3, 4);
		createAreas(
			placer,
			[terrainPainter, elevationPainter, unPaintClass(clWater)],
			borderClasses(clWater, 4, 7),
			scaleByMapSize(12, 130) * 2, 150
		);
	}

	if (randInt(1,2) == 1) //rivers
	{
		//create rivers
		log ("Creating rivers...");
		for (var m = 0; m < numPlayers; m++)
		{
			var tang = startAngle + (m+0.5)*TWO_PI/(numPlayers);
			var placer = new PathPlacer(fractionToTiles(0.5), fractionToTiles(0.5), fractionToTiles(0.5 + 0.49*cos(tang)), fractionToTiles(0.5 + 0.49*sin(tang)), scaleByMapSize(14,24), 0.4, 3*(scaleByMapSize(1,3)), 0.2, 0.05);
			var terrainPainter = new LayeredPainter(
				[tShore, tWater, tWater],		// terrains
				[1, 3]								// widths
			);
			var elevationPainter = new SmoothElevationPainter(
				ELEVATION_SET,			// type
				-4,				// elevation
				4				// blend radius
			);
			createArea(placer, [terrainPainter, elevationPainter, paintClass(clWater)], avoidClasses(clPlayer, 5));
			placer = new ClumpPlacer(floor(PI*scaleByMapSize(10,50)*scaleByMapSize(10,50)/5), 0.95, 0.6, 10, fractionToTiles(0.5 + 0.49*cos(tang)), fractionToTiles(0.5 + 0.49*sin(tang)));
			var painter = new LayeredPainter([tWater, tWater], [1]);
			var elevationPainter = new SmoothElevationPainter(ELEVATION_SET, -4, 0);
			createArea(placer, [painter, elevationPainter, paintClass(clWater)], avoidClasses(clPlayer, 5));

		}

		var fx = fractionToTiles(0.5);
		var fz = fractionToTiles(0.5);
		ix = round(fx);
		iz = round(fz);

		var placer = new ClumpPlacer(mapArea * 0.005, 0.7, 0.1, 10, ix, iz);
		var terrainPainter = new LayeredPainter(
			[tShore, tWater, tWater, tWater],		// terrains
			[1, 4, 2]		// widths
		);
		var elevationPainter = new SmoothElevationPainter(
			ELEVATION_SET,			// type
			-4,				// elevation
			4				// blend radius
		);
		createArea(placer, [terrainPainter, elevationPainter, paintClass(clWater)], null);
	}

	if ((randInt(1,3) == 1)&&(mdd1 == 1))//island
	{
		var placer = new ClumpPlacer(mapArea * 0.006 * lSize, 0.7, 0.1, 10, ix, iz);
		var terrainPainter = new LayeredPainter(
			[tShore, tWater, tWater, tWater],		// terrains
			[1, 4, 2]		// widths
		);
		var elevationPainter = new SmoothElevationPainter(
			ELEVATION_SET,			// type
			3,				// elevation
			4				// blend radius
		);
		createArea(placer, [terrainPainter, elevationPainter, paintClass(clWater)], null);
	}
}
//********************************************************************************************************
else if (md == 6) //edge seas
{

	for (var ix = 0; ix < mapSize; ix++)
	{
		for (var iz = 0; iz < mapSize; iz++)
		{
			var x = ix / (mapSize + 1.0);
			var z = iz / (mapSize + 1.0);
				setHeight(ix, iz, 3);
		}
	}

	var mdd1 = randInt(1,2);

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
	var playerPos = new Array(numPlayers);

	for (var i = 0; i < numPlayers; i++)
	{
		playerPos[i] = (i + 1) / (numPlayers + 1);
		if (mdd1 == 1) //horizontal
		{
			playerX[i] = playerPos[i];
			playerZ[i] = 0.4 + 0.2*(i%2);
		}
		else //vertical
		{
			playerX[i] = 0.4 + 0.2*(i%2);
			playerZ[i] = playerPos[i];
		}

		var fx = fractionToTiles(playerX[i]);
		var fz = fractionToTiles(playerZ[i]);
		var ix = round(fx);
		var iz = round(fz);
		addToClass(ix, iz, clPlayer);
		addToClass(ix+5, iz, clPlayer);
		addToClass(ix, iz+5, clPlayer);
		addToClass(ix-5, iz, clPlayer);
		addToClass(ix, iz-5, clPlayer);
	}

	var mdd2 = randInt(1,3);
	var fadedistance = 7;

	if (mdd1 == 1)
	{
		if ((mdd2 == 1)||(mdd2 == 3))
		{
			var distance = randFloat(0., 0.1);
			for (var ix = 0; ix < mapSize; ix++)
			{
				for (var iz = 0; iz < mapSize; iz++)
				{
					if (iz > (0.69+distance) * mapSize)
					{
						if (iz < (0.69+distance) * mapSize + fadedistance)
						{
							setHeight(ix, iz, 3 - 7 * (iz - (0.69+distance) * mapSize) / fadedistance);
							if (3 - 7 * (iz - (0.69+distance) * mapSize) / fadedistance < 0.5)
								addToClass(ix, iz, clWater);
						}
						else
						{
							setHeight(ix, iz, -4);
							addToClass(ix, iz, clWater);
						}
					}
				}

			}

			for (var i = 0; i < scaleByMapSize(20,120); i++)
			{
				placer = new ClumpPlacer(scaleByMapSize(50, 70), 0.2, 0.1, 10, randFloat(0.1,0.9)*mapSize, randFloat(0.67+distance,0.74+distance)*mapSize);
				var terrainPainter = new LayeredPainter(
					[tMainTerrain, tMainTerrain],		// terrains
					[2]								// widths
				);
				var elevationPainter = new SmoothElevationPainter(ELEVATION_SET, 3, 3);
				createArea(
					placer,
					[terrainPainter, elevationPainter, unPaintClass(clWater)],
					null
				);
			}
		}
		if ((mdd2 == 2)||(mdd2 == 3))
		{
			var distance = randFloat(0., 0.1);
			for (var ix = 0; ix < mapSize; ix++)
			{
				for (var iz = 0; iz < mapSize; iz++)
				{
					if (iz < (0.31-distance) * mapSize)
					{
						if (iz > (0.31-distance) * mapSize - fadedistance)
						{
							setHeight(ix, iz, 3 - 7 * ((0.31-distance) * mapSize - iz) / fadedistance);
							if (3 - 7 * ((0.31-distance) * mapSize - iz) / fadedistance < 0.5)
								addToClass(ix, iz, clWater);
						}
						else
						{
							setHeight(ix, iz, -4);
							addToClass(ix, iz, clWater);
						}
					}
				}
			}

			for (var i = 0; i < scaleByMapSize(20,120); i++)
			{
				placer = new ClumpPlacer(scaleByMapSize(50, 70), 0.2, 0.1, 10, randFloat(0.1,0.9)*mapSize, randFloat(0.26-distance,0.34-distance)*mapSize);
				var terrainPainter = new LayeredPainter(
					[tMainTerrain, tMainTerrain],		// terrains
					[2]								// widths
				);
				var elevationPainter = new SmoothElevationPainter(ELEVATION_SET, 3, 3);
				createArea(
					placer,
					[terrainPainter, elevationPainter, unPaintClass(clWater)],
					null
				);
			}
		}
	}
	else //vertical
	{
		if ((mdd2 == 1)||(mdd2 == 3))
		{
			var distance = randFloat(0., 0.1);
			for (var ix = 0; ix < mapSize; ix++)
			{
				for (var iz = 0; iz < mapSize; iz++)
				{
					if (ix > (0.69+distance) * mapSize)
					{
						if (ix < (0.69+distance) * mapSize + fadedistance)
						{
							setHeight(ix, iz, 3 - 7 * (ix - (0.69+distance) * mapSize) / fadedistance);
							if (3 - 7 * (ix - (0.69+distance) * mapSize) / fadedistance < 0.5)
								addToClass(ix, iz, clWater);
						}
						else
						{
							setHeight(ix, iz, -4);
							addToClass(ix, iz, clWater);
						}
					}
				}
			}
		}
		if ((mdd2 == 2)||(mdd2 == 3))
		{
			var distance = randFloat(0., 0.1);
			for (var ix = 0; ix < mapSize; ix++)
			{
				for (var iz = 0; iz < mapSize; iz++)
				{
					if (ix < (0.31-distance) * mapSize)
					{
						if (ix > (0.31-distance) * mapSize - fadedistance)
						{
							setHeight(ix, iz, 3 - 7 * ((0.31-distance) * mapSize - ix) / fadedistance);
							if (3 - 7 * ((0.31-distance) * mapSize - ix) / fadedistance < 0.5)
								addToClass(ix, iz, clWater);
						}
						else
						{
							setHeight(ix, iz, -4);
							addToClass(ix, iz, clWater);
						}
					}
				}
			}
		}
	}

	// create shore jaggedness
	log("Creating shore jaggedness...");
	placer = new ClumpPlacer(scaleByMapSize(20, 150), 0.2, 0.1, 1);
	terrainPainter = new LayeredPainter(
		[tCliff, tHill],		// terrains
		[2]								// widths
	);
	elevationPainter = new SmoothElevationPainter(ELEVATION_SET, -5, 4);
	createAreas(
		placer,
		[terrainPainter, elevationPainter, paintClass(clWater)],
		[avoidClasses(clPlayer, 20), borderClasses(clWater, 6, 4)],
		scaleByMapSize(7, 130) * 2, 150
	);

	placer = new ClumpPlacer(scaleByMapSize(15, 80), 0.2, 0.1, 1);
	terrainPainter = new LayeredPainter(
		[tCliff, tHill],		// terrains
		[2]								// widths
	);
	elevationPainter = new SmoothElevationPainter(ELEVATION_SET, 3, 4);
	createAreas(
		placer,
		[terrainPainter, elevationPainter, unPaintClass(clWater)],
		borderClasses(clWater, 4, 7),
		scaleByMapSize(12, 130) * 2, 150
	);

	var mdd3 = randInt(1,3);
	if (mdd3 == 1)
	{
		// create islands
		log("Creating islands...");
		placer = new ClumpPlacer(randInt(scaleByMapSize(8,15),scaleByMapSize(15,23))*randInt(scaleByMapSize(8,15),scaleByMapSize(15,23)), 0.80, 0.1, randFloat(0.0, 0.2));
		terrainPainter = new LayeredPainter(
			[tMainTerrain, tMainTerrain],		// terrains
			[2]								// widths
		);
		elevationPainter = new SmoothElevationPainter(ELEVATION_SET, 3.1, 4);
		createAreas(
			placer,
			[terrainPainter, elevationPainter, paintClass(clLand)],
			avoidClasses(clLand, 3, clPlayer, 3),
			scaleByMapSize(2, 5)*randInt(8,14)
		);
	}
	else if (mdd3 == 2)
	{
		// create extentions
		log("Creating extentions...");
		placer = new ClumpPlacer(randInt(scaleByMapSize(13,24),scaleByMapSize(24,45))*randInt(scaleByMapSize(13,24),scaleByMapSize(24,45)), 0.80, 0.1, 10);
		terrainPainter = new LayeredPainter(
			[tMainTerrain, tMainTerrain],		// terrains
			[2]								// widths
		);
		elevationPainter = new SmoothElevationPainter(ELEVATION_SET, 3.1, 4);
		createAreas(
			placer,
			[terrainPainter, elevationPainter, paintClass(clLand)],
			null,
			scaleByMapSize(2, 5)*randInt(8,14)
		);
	}
}
//********************************************************************************************************
else if (md == 7) //gulf
{

	for (var ix = 0; ix < mapSize; ix++)
	{
		for (var iz = 0; iz < mapSize; iz++)
		{
			var x = ix / (mapSize + 1.0);
			var z = iz / (mapSize + 1.0);
				setHeight(ix, iz, 3);
		}
	}

	var mdd1 = randFloat(0,4);

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

	var startAngle = -PI/6 + (mdd1-1)*PI/2;
	for (var i = 0; i < numPlayers; i++)
	{
		playerAngle[i] = startAngle + i*TWO_PI/(numPlayers-1)*2/3;
		playerX[i] = 0.5 + 0.35*cos(playerAngle[i]);
		playerZ[i] = 0.5 + 0.35*sin(playerAngle[i]);
	}

	for (var i = 0; i < numPlayers; i++)
	{
		var fx = fractionToTiles(playerX[i]);
		var fz = fractionToTiles(playerZ[i]);
		var ix = round(fx);
		var iz = round(fz);
		// create the city patch
		var cityRadius = scaleByMapSize(17,29)/3;
		placer = new ClumpPlacer(PI*cityRadius*cityRadius, 0.6, 0.3, 10, ix, iz);
		createArea(placer, paintClass(clPlayer), null);
	}

	fx = fractionToTiles(0.5);
	fz = fractionToTiles(0.5);
	ix = round(fx);
	iz = round(fz);

	var lSize = 1;

	var placer = new ClumpPlacer(mapArea * 0.08 * lSize, 0.7, 0.05, 10, ix, iz);
	var terrainPainter = new LayeredPainter(
		[tMainTerrain, tMainTerrain, tMainTerrain, tMainTerrain],		// terrains
		[1, 4, 2]		// widths
	);

	var elevationPainter = new SmoothElevationPainter(
		ELEVATION_SET,			// type
		-3,				// elevation
		4				// blend radius
	);
	createArea(placer, [terrainPainter, elevationPainter, paintClass(clWater)], avoidClasses(clPlayer,floor(scaleByMapSize(15,25))));

	fx = fractionToTiles(0.5 - 0.2*cos(mdd1*PI/2));
	fz = fractionToTiles(0.5 - 0.2*sin(mdd1*PI/2));
	ix = round(fx);
	iz = round(fz);

	var lSize = sqrt(sqrt(sqrt(scaleByMapSize(1, 6))));

	var placer = new ClumpPlacer(mapArea * 0.13 * lSize, 0.7, 0.05, 10, ix, iz);
	var terrainPainter = new LayeredPainter(
		[tMainTerrain, tMainTerrain, tMainTerrain, tMainTerrain],		// terrains
		[1, 4, 2]		// widths
	);
	var elevationPainter = new SmoothElevationPainter(
		ELEVATION_SET,			// type
		-3,				// elevation
		4				// blend radius
	);
	createArea(placer, [terrainPainter, elevationPainter, paintClass(clWater)], avoidClasses(clPlayer,floor(scaleByMapSize(15,25))));

	fx = fractionToTiles(0.5 - 0.49*cos(mdd1*PI/2));
	fz = fractionToTiles(0.5 - 0.49*sin(mdd1*PI/2));
	ix = round(fx);
	iz = round(fz);

	var lSize = sqrt(sqrt(sqrt(scaleByMapSize(1, 6))));

	var placer = new ClumpPlacer(mapArea * 0.15 * lSize, 0.7, 0.05, 10, ix, iz);
	var terrainPainter = new LayeredPainter(
		[tMainTerrain, tMainTerrain, tMainTerrain, tMainTerrain],		// terrains
		[1, 4, 2]		// widths
	);
	var elevationPainter = new SmoothElevationPainter(
		ELEVATION_SET,			// type
		-3,				// elevation
		4				// blend radius
	);
	createArea(placer, [terrainPainter, elevationPainter, paintClass(clWater)], avoidClasses(clPlayer,floor(scaleByMapSize(15,25))));
}
//********************************************************************************************************
else if (md == 8) //lakes
{

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

	for (var ix = 0; ix < mapSize; ix++)
	{
		for (var iz = 0; iz < mapSize; iz++)
		{
			var x = ix / (mapSize + 1.0);
			var z = iz / (mapSize + 1.0);
				setHeight(ix, iz, 3);
		}
	}

	for (var i = 0; i < numPlayers; i++)
	{
		var fx = fractionToTiles(playerX[i]);
		var fz = fractionToTiles(playerZ[i]);
		var ix = round(fx);
		var iz = round(fz);
		// create the city patch
		var cityRadius = scaleByMapSize(17,29)/3;
		placer = new ClumpPlacer(PI*cityRadius*cityRadius, 0.6, 0.3, 10, ix, iz);
		createArea(placer, paintClass(clPlayer), null);
	}

	// create lakes
	log("Creating lakes...");
	placer = new ClumpPlacer(scaleByMapSize(160, 700), 0.2, 0.1, 1);
	terrainPainter = new LayeredPainter(
		[tShore, tWater, tWater],		// terrains
		[1, 3]								// widths
	);
	elevationPainter = new SmoothElevationPainter(ELEVATION_SET, -5, 5);
	if (randInt(1,2) == 1)
	{
		createAreas(
			placer,
			[terrainPainter, elevationPainter, paintClass(clWater)],
			avoidClasses(clPlayer, 12, clWater, 8),
			scaleByMapSize(5, 16)
		);
	}
	else
	{
		createAreas(
			placer,
			[terrainPainter, elevationPainter, paintClass(clWater)],
			avoidClasses(clPlayer, 12),
			scaleByMapSize(5, 16)
		);
	}
}
//********************************************************************************************************
else if (md == 9) //passes
{

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

	for (var ix = 0; ix < mapSize; ix++)
	{
		for (var iz = 0; iz < mapSize; iz++)
		{
			var x = ix / (mapSize + 1.0);
			var z = iz / (mapSize + 1.0);
				setHeight(ix, iz, 3);
		}
	}

	//create ranges
	log ("Creating ranges...");
	for (var m = 0; m < numPlayers; m++)
	{
		var tang = startAngle + (m+0.5)*TWO_PI/(numPlayers);
		var placer = new PathPlacer(fractionToTiles(0.5), fractionToTiles(0.5), fractionToTiles(0.5 + 0.49*cos(tang)), fractionToTiles(0.5 + 0.49*sin(tang)), scaleByMapSize(14,24), 0.4, 3*(scaleByMapSize(1,3)), 0.2, 0.05);
		var terrainPainter = new LayeredPainter(
			[tShore, tWater, tWater],		// terrains
			[1, 3]								// widths
		);
		var elevationPainter = new SmoothElevationPainter(
			ELEVATION_SET,			// type
			24,				// elevation
			3				// blend radius
		);
		createArea(placer, [terrainPainter, elevationPainter, paintClass(clWater)], avoidClasses(clPlayer, 5));
		placer = new ClumpPlacer(floor(PI*scaleByMapSize(10,50)*scaleByMapSize(10,50)/5), 0.95, 0.6, 10, fractionToTiles(0.5 + 0.49*cos(tang)), fractionToTiles(0.5 + 0.49*sin(tang)));
		var painter = new LayeredPainter([tWater, tWater], [1]);
		var elevationPainter = new SmoothElevationPainter(ELEVATION_SET, 24, 0);
		createArea(placer, [painter, elevationPainter], avoidClasses(clPlayer, 5));

		var placer = new PathPlacer(fractionToTiles(0.5 + 0.3*cos(tang) - 0.1 * cos(tang+PI/2)), fractionToTiles(0.5 + 0.3*sin(tang) - 0.1 * sin(tang+PI/2)), fractionToTiles(0.5 + 0.3*cos(tang) + 0.1 * cos(tang+PI/2)), fractionToTiles(0.5 + 0.3*sin(tang) + 0.1 * sin(tang+PI/2)), scaleByMapSize(14,24), 0.4, 3*(scaleByMapSize(1,3)), 0.2, 0.05);
		var painter = new LayeredPainter([tCliff, tCliff], [1]);
		var elevationPainter = new SmoothElevationPainter(ELEVATION_SET, 3, 2);
		createArea(placer, [painter, elevationPainter], null);
	}
	var mdd1 = randInt (1,3);
	if (mdd1 <= 2)
	{
		var fx = fractionToTiles(0.5);
		var fz = fractionToTiles(0.5);
		ix = round(fx);
		iz = round(fz);

		var placer = new ClumpPlacer(mapArea * 0.005, 0.7, 0.1, 10, ix, iz);
		var terrainPainter = new LayeredPainter(
			[tShore, tWater, tWater, tWater],		// terrains
			[1, 4, 2]		// widths
		);
		var elevationPainter = new SmoothElevationPainter(
			ELEVATION_SET,			// type
			24,				// elevation
			4				// blend radius
		);
		createArea(placer, [terrainPainter, elevationPainter, paintClass(clWater)], null);
	}
	else
	{
		var fx = fractionToTiles(0.5);
		var fz = fractionToTiles(0.5);
		ix = round(fx);
		iz = round(fz);

		var lSize = sqrt(sqrt(sqrt(scaleByMapSize(1, 6))));

		var placer = new ClumpPlacer(mapArea * 0.03 * lSize, 0.7, 0.1, 10, ix, iz);
		var terrainPainter = new LayeredPainter(
			[tShore, tWater, tWater, tWater],		// terrains
			[1, 4, 2]		// widths
		);
		var elevationPainter = new SmoothElevationPainter(
			ELEVATION_SET,			// type
			-4,				// elevation
			3				// blend radius
		);
		createArea(placer, [terrainPainter, elevationPainter, paintClass(clWater)], null);
	}
}
//********************************************************************************************************
else if (md == 10) //lowlands
{
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

	for (var ix = 0; ix < mapSize; ix++)
	{
		for (var iz = 0; iz < mapSize; iz++)
		{
			var x = ix / (mapSize + 1.0);
			var z = iz / (mapSize + 1.0);
				setHeight(ix, iz, 30);
		}
	}


	var radius = scaleByMapSize(18,32);
	var cliffRadius = 2;
	var elevation = 20;
	var hillSize = PI * radius * radius;

	var split = 1;
	if ((mapSize / 64 == 2)&&(numPlayers <= 2))
	{
		split = 2;
	}
	else if ((mapSize / 64 == 3)&&(numPlayers <= 3))
	{
		split = 2;
	}
	else if ((mapSize / 64 == 4)&&(numPlayers <= 4))
	{
		split = 2;
	}
	else if ((mapSize / 64 == 5)&&(numPlayers <= 4))
	{
		split = 2;
	}
	else if ((mapSize / 64 == 6)&&(numPlayers <= 5))
	{
		split = 2;
	}
	else if ((mapSize / 64 == 7)&&(numPlayers <= 6))
	{
		split = 2;
	}

	for (var i = 0; i < numPlayers*split; i++)
	{
		var tang = startAngle + (i)*TWO_PI/(numPlayers*split);
		var fx = fractionToTiles(0.5 + 0.35*cos(tang));
		var fz = fractionToTiles(0.5 + 0.35*sin(tang));
		var ix = round(fx);
		var iz = round(fz);
		// create the hill
		var placer = new ClumpPlacer(hillSize, 0.65, 0.1, 10, ix, iz);
		var terrainPainter = new LayeredPainter(
			[tMainTerrain, tMainTerrain],		// terrains
			[cliffRadius]		// widths
		);
		var elevationPainter = new SmoothElevationPainter(
			ELEVATION_SET,			// type
			3,				// elevation
			cliffRadius				// blend radius
		);
		createArea(placer, [terrainPainter, elevationPainter, paintClass(clLand)], null);
	}

	var fx = fractionToTiles(0.5);
	var fz = fractionToTiles(0.5);
	ix = round(fx);
	iz = round(fz);

	var lSize = sqrt(sqrt(sqrt(scaleByMapSize(1, 6))));

	var placer = new ClumpPlacer(mapArea * 0.091 * lSize, 0.7, 0.1, 10, ix, iz);
	var terrainPainter = new LayeredPainter(
		[tMainTerrain, tMainTerrain, tMainTerrain, tMainTerrain],		// terrains
		[1, 4, 2]		// widths
	);
	var elevationPainter = new SmoothElevationPainter(
		ELEVATION_SET,			// type
		3,				// elevation
		4				// blend radius
	);
	createArea(placer, [terrainPainter, elevationPainter, paintClass(clWater)], null);

	for (var m = 0; m < numPlayers*split; m++)
	{
		var tang = startAngle + m*TWO_PI/(numPlayers*split);
		var placer = new PathPlacer(fractionToTiles(0.5), fractionToTiles(0.5), fractionToTiles(0.5 + 0.35*cos(tang)), fractionToTiles(0.5 + 0.35*sin(tang)), scaleByMapSize(14,24), 0.4, 3*(scaleByMapSize(1,3)), 0.2, 0.05);
		var terrainPainter = new LayeredPainter(
			[tMainTerrain, tMainTerrain, tMainTerrain],		// terrains
			[1, 3]								// widths
		);
		var elevationPainter = new SmoothElevationPainter(
			ELEVATION_SET,			// type
			3,				// elevation
			4				// blend radius
		);
		createArea(placer, [terrainPainter, elevationPainter, paintClass(clWater)], null);
	}
}
//********************************************************************************************************
else //mainland
{

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

	for (var ix = 0; ix < mapSize; ix++)
	{
		for (var iz = 0; iz < mapSize; iz++)
		{
			var x = ix / (mapSize + 1.0);
			var z = iz / (mapSize + 1.0);
				setHeight(ix, iz, 3);
		}
	}

}

paintTerrainBasedOnHeight(3.12, 40, 1, tCliff);
paintTerrainBasedOnHeight(3, 3.12, 1, tMainTerrain);
paintTerrainBasedOnHeight(1, 3, 1, tShore);
paintTerrainBasedOnHeight(-8, 1, 2, tWater);
unPaintTileClassBasedOnHeight(0, 3.12, 1, clWater);
unPaintTileClassBasedOnHeight(-6, 0, 1, clLand);
paintTileClassBasedOnHeight(-6, 0, 1, clWater);
paintTileClassBasedOnHeight(0, 3.12, 1, clLand);
paintTileClassBasedOnHeight(3.12, 40, 1, clHill);

for (var i = 0; i < numPlayers; i++)
{
	var id = playerIDs[i];
	log("Creating base for player " + id + "...");

	// some constants
	var radius = scaleByMapSize(17,29);
	var shoreRadius = 4;
	var elevation = 3;

	var hillSize = PI * radius * radius;
	// get the x and z in tiles
	var fx = fractionToTiles(playerX[i]);
	var fz = fractionToTiles(playerZ[i]);
	var ix = round(fx);
	var iz = round(fz);

	// create starting units
	if (iberianTowers)
		placeCivDefaultEntities(fx, fz, id, { 'iberWall': 'towers' });
	else
		placeCivDefaultEntities(fx, fz, id);

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
	if (needsAdditionalWood)
	{
		// create woods
		var bbAngle = randFloat(0, TWO_PI);
		var bbDist = 13;
		var bbX = round(fx + bbDist * cos(bbAngle));
		var bbZ = round(fz + bbDist * sin(bbAngle));
		group = new SimpleGroup(
			[new SimpleObject(oWood, 14,14, 0,3)],
			true, clBaseResource, bbX, bbZ
		);
		createObjectGroup(group, 0);
	}

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
		[new SimpleObject(oTree1, num, num, 0,5)],
		false, clBaseResource, tX, tZ
	);
	createObjectGroup(group, 0, avoidClasses(clBaseResource,2));

	placeDefaultDecoratives(fx, fz, aGrassShort, clBaseResource, radius);
}

for (var i = 0; i < numPlayers; i++)
{
	var fx = fractionToTiles(playerX[i]);
	var fz = fractionToTiles(playerZ[i]);
	var ix = round(fx);
	var iz = round(fz);
	// create the city patch
	var cityRadius = radius/3;
	placer = new ClumpPlacer(PI*cityRadius*cityRadius, 0.6, 0.3, 10, ix, iz);
	var painter = new LayeredPainter([tRoadWild, tRoad], [1]);
	createArea(placer, [painter, paintClass(clPlayer)], null);
}

// create bumps
log("Creating bumps...");
placer = new ClumpPlacer(scaleByMapSize(20, 50), 0.3, 0.06, 1);
painter = new SmoothElevationPainter(ELEVATION_MODIFY, 2, 2);
createAreas(
	placer,
	painter,
	[avoidClasses(clWater, 2, clPlayer, 10), stayClasses(clLand, 3)],
	randInt(0,scaleByMapSize(200, 400))
);

// create hills
log("Creating hills...");
placer = new ClumpPlacer(scaleByMapSize(20, 150), 0.2, 0.1, 1);
terrainPainter = new LayeredPainter(
	[tCliff, tHill],		// terrains
	[2]								// widths
);
elevationPainter = new SmoothElevationPainter(ELEVATION_SET, 18, 2);
createAreas(
	placer,
	[terrainPainter, elevationPainter, paintClass(clHill)],
	[avoidClasses(clPlayer, 20, clHill, randInt(6, 18)), stayClasses(clLand, 0)],
	randInt(0, scaleByMapSize(4, 8))*randInt(1, scaleByMapSize(4, 9))
);

var multiplier = sqrt(randFloat(0.5,1.2)*randFloat(0.5,1.2));
// calculate desired number of trees for map (based on size)
if (random_terrain == g_BiomeSavanna)
{
	var MIN_TREES = floor(200*multiplier);
	var MAX_TREES = floor(1250*multiplier);
	var P_FOREST = randFloat(0.02, 0.05);
}
else if (random_terrain == g_BiomeTropic)
{
	var MIN_TREES = floor(1000*multiplier);
	var MAX_TREES = floor(6000*multiplier);
	var P_FOREST = randFloat(0.5, 0.7);
}
else
{
	var MIN_TREES = floor(500*multiplier);
	var MAX_TREES = floor(3000*multiplier);
	var P_FOREST = randFloat(0.5,0.8);
}
var totalTrees = scaleByMapSize(MIN_TREES, MAX_TREES);
var numForest = totalTrees * P_FOREST;
var numStragglers = totalTrees * (1.0 - P_FOREST);

// create forests
log("Creating forests...");
var types = [
	[[tForestFloor2, tMainTerrain, pForest1], [tForestFloor2, pForest1]],
	[[tForestFloor1, tMainTerrain, pForest2], [tForestFloor1, pForest2]]
];	// some variation

if (random_terrain == g_BiomeSavanna)
	var size = numForest / (0.5 * scaleByMapSize(2,8) * numPlayers);
else
	var size = numForest / (scaleByMapSize(2,8) * numPlayers);

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
		[avoidClasses(clPlayer, 20, clForest, randInt(5, 15), clHill, 0), stayClasses(clLand, 4)],
		num
	);
}

RMS.SetProgress(50);
// create dirt patches
log("Creating dirt patches...");
var sizes = [scaleByMapSize(3, 48), scaleByMapSize(5, 84), scaleByMapSize(8, 128)];
var numb = 1;
if (random_terrain == g_BiomeSavanna)
	numb = 3;
for (var i = 0; i < sizes.length; i++)
{
	placer = new ClumpPlacer(sizes[i], 0.3, 0.06, 0.5);
	painter = new LayeredPainter(
		[[tMainTerrain,tTier1Terrain],[tTier1Terrain,tTier2Terrain], [tTier2Terrain,tTier3Terrain]], 		// terrains
		[1,1]															// widths
	);
	createAreas(
		placer,
		[painter, paintClass(clDirt)],
		[avoidClasses(clForest, 0, clHill, 0, clDirt, 5, clPlayer, 7), stayClasses(clLand, 4)],
		numb*scaleByMapSize(15, 45)
	);
}

// create grass patches
log("Creating grass patches...");
var sizes = [scaleByMapSize(2, 32), scaleByMapSize(3, 48), scaleByMapSize(5, 80)];
for (var i = 0; i < sizes.length; i++)
{
	placer = new ClumpPlacer(sizes[i], 0.3, 0.06, 0.5);
	painter = new TerrainPainter(tTier4Terrain);
	createAreas(
		placer,
		painter,
		[avoidClasses(clForest, 0, clHill, 0, clDirt, 5, clPlayer, 7), stayClasses(clLand, 4)],
		numb*scaleByMapSize(15, 45)
	);
}
RMS.SetProgress(55);


log("Creating stone mines...");
// create large stone quarries
group = new SimpleGroup([new SimpleObject(oStoneSmall, 0,2, 0,4), new SimpleObject(oStoneLarge, 1,1, 0,4)], true, clRock);
createObjectGroups(group, 0,
	[avoidClasses(clForest, 1, clPlayer, 20, clRock, 10, clHill, 1), stayClasses(clLand, 4)],
	randInt(scaleByMapSize(2,9),scaleByMapSize(9,40)), 100
);

// create small stone quarries
group = new SimpleGroup([new SimpleObject(oStoneSmall, 2,5, 1,3)], true, clRock);
createObjectGroups(group, 0,
	[avoidClasses(clForest, 1, clPlayer, 20, clRock, 10, clHill, 1), stayClasses(clLand, 4)],
	randInt(scaleByMapSize(2,9),scaleByMapSize(9,40)), 100
);

log("Creating metal mines...");
// create large metal quarries
group = new SimpleGroup([new SimpleObject(oMetalLarge, 1,1, 0,4)], true, clMetal);
createObjectGroups(group, 0,
	[avoidClasses(clForest, 1, clPlayer, 20, clMetal, 10, clRock, 5, clHill, 1), stayClasses(clLand, 4)],
	randInt(scaleByMapSize(2,9),scaleByMapSize(9,40)), 100
);

RMS.SetProgress(65);

// create small decorative rocks
log("Creating small decorative rocks...");
group = new SimpleGroup(
	[new SimpleObject(aRockMedium, 1,3, 0,1)],
	true
);
createObjectGroups(
	group, 0,
	[avoidClasses(clWater, 0, clForest, 0, clPlayer, 0, clHill, 0), stayClasses(clLand, 4)],
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
	[avoidClasses(clWater, 0, clForest, 0, clPlayer, 0, clHill, 0), stayClasses(clLand, 4)],
	scaleByMapSize(8, 131), 50
);

RMS.SetProgress(70);

// create deer
log("Creating deer...");
group = new SimpleGroup(
	[new SimpleObject(oMainHuntableAnimal, 5,7, 0,4)],
	true, clFood
);
createObjectGroups(group, 0,
	[avoidClasses(clWater, 0, clForest, 0, clPlayer, 20, clHill, 1, clFood, 20), stayClasses(clLand, 4)],
	randInt(numPlayers+3, 5*numPlayers+4), 50
);

// create berry bush
log("Creating berry bush...");
group = new SimpleGroup(
	[new SimpleObject(oFruitBush, 5,7, 0,4)],
	true, clFood
);
createObjectGroups(group, 0,
	[avoidClasses(clWater, 0, clForest, 0, clPlayer, 20, clHill, 1, clFood, 20), stayClasses(clLand, 4)],
	randInt(1, 4) * numPlayers + 2, 50
);

RMS.SetProgress(75);

// create sheep
log("Creating sheep...");
group = new SimpleGroup(
	[new SimpleObject(oSecondaryHuntableAnimal, 2,3, 0,2)],
	true, clFood
);
createObjectGroups(group, 0,
	[avoidClasses(clWater, 0, clForest, 0, clPlayer, 20, clHill, 1, clFood, 20), stayClasses(clLand, 4)],
	randInt(numPlayers+3, 5*numPlayers+4), 50
);

// create fish
log("Creating fish...");
group = new SimpleGroup(
	[new SimpleObject(oFish, 2,3, 0,2)],
	true, clFood
);
createObjectGroups(group, 0,
	avoidClasses(clLand, 5, clForest, 0, clPlayer, 0, clHill, 0, clFood, 20),
	randInt(15, 40) * numPlayers, 60
);

RMS.SetProgress(85);


// create straggler trees
log("Creating straggler trees...");
var types = [oTree1, oTree2, oTree4, oTree3];	// some variation
var num = floor(numStragglers / types.length);
for (var i = 0; i < types.length; ++i)
{
	group = new SimpleGroup(
		[new SimpleObject(types[i], 1,1, 0,3)],
		true, clForest
	);
	createObjectGroups(group, 0,
		[avoidClasses(clWater, 1, clForest, 1, clHill, 1, clPlayer, 0, clMetal, 6, clRock, 6), stayClasses(clLand, 4)],
		num
	);
}

var planetm = 1;
if (random_terrain == g_BiomeTropic)
	planetm = 8;

//create small grass tufts
log("Creating small grass tufts...");
group = new SimpleGroup(
	[new SimpleObject(aGrassShort, 1,2, 0,1, -PI/8,PI/8)]
);
createObjectGroups(group, 0,
	[avoidClasses(clWater, 2, clHill, 2, clPlayer, 2, clDirt, 0), stayClasses(clLand, 4)],
	planetm * scaleByMapSize(13, 200)
);

RMS.SetProgress(90);

// create large grass tufts
log("Creating large grass tufts...");
group = new SimpleGroup(
	[new SimpleObject(aGrass, 2,4, 0,1.8, -PI/8,PI/8), new SimpleObject(aGrassShort, 3,6, 1.2,2.5, -PI/8,PI/8)]
);
createObjectGroups(group, 0,
	[avoidClasses(clWater, 3, clHill, 2, clPlayer, 2, clDirt, 1, clForest, 0), stayClasses(clLand, 4)],
	planetm * scaleByMapSize(13, 200)
);

RMS.SetProgress(95);

// create shallow flora
log("Creating shallow flora...");
group = new SimpleGroup(
	[new SimpleObject(aLillies, 1,2, 0,2), new SimpleObject(aReeds, 2,4, 0,2)]
);
createObjectGroups(group, 0,
	stayClasses(clShallow, 1),
	60 * scaleByMapSize(13, 200), 80
);

// create bushes
log("Creating bushes...");
group = new SimpleGroup(
	[new SimpleObject(aBushMedium, 1,2, 0,2), new SimpleObject(aBushSmall, 2,4, 0,2)]
);
createObjectGroups(group, 0,
	[avoidClasses(clWater, 1, clHill, 1, clPlayer, 1, clDirt, 1), stayClasses(clLand, 3)],
	planetm * scaleByMapSize(13, 200), 50
);

random_terrain = randInt(1,6);
if (random_terrain == 1)
	setSkySet("cirrus");
else if (random_terrain == 2)
	setSkySet("cumulus");
else if (random_terrain == 3)
	setSkySet("sunny");
else if (random_terrain == 4)
	setSkySet("sunny 1");
else if (random_terrain == 5)
	setSkySet("mountainous");
else if (random_terrain == 6)
	setSkySet("stratus");

setSunRotation(randFloat(0, TWO_PI));
setSunElevation(randFloat(PI/ 5, PI / 3));

// Export map data
ExportMap();
