RMS.LoadLibrary("rmgen");

TILE_CENTERED_HEIGHT_MAP = true;
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

// gaia entities
var oOak = rBiomeE1();
var oOakLarge = rBiomeE2();
var oApple = rBiomeE3();
var oPine = rBiomeE4();
var oAleppoPine = rBiomeE5();
var oBerryBush = rBiomeE6();
var oDeer = rBiomeE8();
var oFish = rBiomeE9();
var oSheep = rBiomeE10();
var oStoneLarge = rBiomeE11();
var oStoneSmall = rBiomeE12();
var oMetalLarge = rBiomeE13();
var oWood = "gaia/special_treasure_wood";

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

var startAngle = randFloat(0, TWO_PI);
var md = randInt(1,13);
var needsAdditionalWood = false;
//*****************************************************************************************************************************
if (md == 1) //archipelago and island
{
	needsAdditionalWood = true;
	
	// randomize player order
	var playerIDs = [];
	for (var i = 0; i < numPlayers; i++)
	{
		playerIDs.push(i+1);
	}
	playerIDs = sortPlayers(playerIDs);

	var radius = scaleByMapSize(17, 29);
	var hillSize = PI * radius * radius;

	var mdd1 = randInt(1,3);
	
	if (mdd1 == 1) //archipelago
	{
		// create islands
		log("Creating islands...");
		placer = new ClumpPlacer(floor(hillSize*randFloat(0.8,1.2)), 0.80, 0.1, 10);
		terrainPainter = new LayeredPainter(
			[tGrass, tGrass],		// terrains
			[2]								// widths
		);
		elevationPainter = new SmoothElevationPainter(ELEVATION_SET, 3, 4);
		createAreas(
			placer,
			[terrainPainter, elevationPainter, paintClass(clLand)], 
			null,
			scaleByMapSize(2, 5)*randInt(8,14)
		);
	}
	else if (mdd1 == 2) //islands
	{
		// create islands
		log("Creating islands...");
		placer = new ClumpPlacer(floor(hillSize*randFloat(0.6,1.4)), 0.80, 0.1, randFloat(0.0, 0.2));
		terrainPainter = new LayeredPainter(
			[tGrass, tGrass],		// terrains
			[2]								// widths
		);
		elevationPainter = new SmoothElevationPainter(ELEVATION_SET, 3, 4);
		createAreas(
			placer,
			[terrainPainter, elevationPainter, paintClass(clLand)], 
			avoidClasses(clLand, 3, clPlayer, 3),
			scaleByMapSize(6, 10)*randInt(8,14)
		);
	}
	else if (mdd1 == 3) // tight islands
	{
		// create islands
		log("Creating islands...");
		placer = new ClumpPlacer(floor(hillSize*randFloat(0.8,1.2)), 0.80, 0.1, 10);
		terrainPainter = new LayeredPainter(
			[tGrass, tGrass],		// terrains
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

	var fx = fractionToTiles(0.5);
	var fz = fractionToTiles(0.5);
	ix = round(fx);
	iz = round(fz);

	var placer = new ClumpPlacer(mapArea * 0.45, 0.9, 0.09, 10, ix, iz);
	var terrainPainter = new LayeredPainter(
		[tWater, tShore, tGrass],		// terrains
		[4, 2]		// widths
	);
	var elevationPainter = new SmoothElevationPainter(
		ELEVATION_SET,			// type
		3,				// elevation
		4				// blend radius
	);
	createArea(placer, [terrainPainter, elevationPainter, paintClass(clLand)], null);
	
	if (randInt(1,4)==1) // peninsula
	{
		var angle = randFloat(0, TWO_PI);
	
		var fx = fractionToTiles(0.5 + 0.25*cos(angle));
		var fz = fractionToTiles(0.5 + 0.25*sin(angle));
		ix = round(fx);
		iz = round(fz);

		var placer = new ClumpPlacer(mapArea * 0.45, 0.9, 0.09, 10, ix, iz);
		var terrainPainter = new LayeredPainter(
			[tWater, tShore, tGrass],		// terrains
			[4, 2]		// widths
		);
		var elevationPainter = new SmoothElevationPainter(
			ELEVATION_SET,			// type
			3,				// elevation
			4				// blend radius
		);
		createArea(placer, [terrainPainter, elevationPainter, paintClass(clLand)], null);
	}
	
	var mdd1 = randInt(1,3);
	if (mdd1 == 1)
	{
		// create islands
		log("Creating islands...");
		placer = new ClumpPlacer(randInt(scaleByMapSize(8,15),scaleByMapSize(15,23))*randInt(scaleByMapSize(8,15),scaleByMapSize(15,23)), 0.80, 0.1, randFloat(0.0, 0.2));
		terrainPainter = new LayeredPainter(
			[tGrass, tGrass],		// terrains
			[2]								// widths
		);
		elevationPainter = new SmoothElevationPainter(ELEVATION_SET, 3, 4);
		createAreas(
			placer,
			[terrainPainter, elevationPainter, paintClass(clLand)], 
			avoidClasses(clLand, 3, clPlayer, 3),
			scaleByMapSize(2, 5)*randInt(8,14)
		);
	}
	else if (mdd1 == 2)
	{
		// create extentions
		log("Creating extentions...");
		placer = new ClumpPlacer(randInt(scaleByMapSize(13,24),scaleByMapSize(24,45))*randInt(scaleByMapSize(13,24),scaleByMapSize(24,45)), 0.80, 0.1, 10);
		terrainPainter = new LayeredPainter(
			[tGrass, tGrass],		// terrains
			[2]								// widths
		);
		elevationPainter = new SmoothElevationPainter(ELEVATION_SET, 3, 4);
		createAreas(
			placer,
			[terrainPainter, elevationPainter, paintClass(clLand)], 
			null,
			scaleByMapSize(2, 5)*randInt(8,14)
		);
	}
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
				
				if ((x > cu + 0.5 - WATER_WIDTH/2) && (x < cu + 0.5 + WATER_WIDTH/2))
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
				
				if ((z > cu + 0.5 - WATER_WIDTH/2) && (z < cu + 0.5 + WATER_WIDTH/2))
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
		if (mdd1 == 1) //vertical
		{
			var placer = new PathPlacer(1, fractionToTiles(0.5), fractionToTiles(0.99), fractionToTiles(0.5), scaleByMapSize(randInt(16,24),randInt(100,140)), 0.5, 3*(scaleByMapSize(1,4)), 0.1, 0.01);
		}
		else
		{
			var placer = new PathPlacer(fractionToTiles(0.5), 1, fractionToTiles(0.5), fractionToTiles(0.99), scaleByMapSize(randInt(16,24),randInt(100,140)), 0.5, 3*(scaleByMapSize(1,4)), 0.1, 0.01);
		}
		var terrainPainter = new LayeredPainter(
			[tGrass, tGrass, tGrass],		// terrains
			[1, 3]								// widths
		);
		var elevationPainter = new SmoothElevationPainter(
			ELEVATION_SET,			// type
			3.1,				// elevation
			4				// blend radius
		);
		createArea(placer, [terrainPainter, elevationPainter, unPaintClass(clWater)], null);
	}
	var mdd2 = randInt(1,3);
	if (mdd2 == 1)
	{
		// create islands
		log("Creating islands...");
		placer = new ClumpPlacer(randInt(scaleByMapSize(8,15),scaleByMapSize(15,23))*randInt(scaleByMapSize(8,15),scaleByMapSize(15,23)), 0.80, 0.1, randFloat(0.0, 0.2));
		terrainPainter = new LayeredPainter(
			[tGrass, tGrass],		// terrains
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
			[tGrass, tGrass],		// terrains
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
	
	var mdd2 = randInt(1,2)
	if (mdd2 == 1)
	{
		// create the shallows of the main river
		log("Creating the shallows of the main river");

		for (var i = 0; i <= randInt(1, scaleByMapSize(4,8)); i++)
		{
			var cLocation = randFloat(0.15,0.85);
			if (mdd1 == 1)
				passageMaker(fractionToTiles(cLocation), fractionToTiles(0.35), fractionToTiles(cLocation), fractionToTiles(0.65), scaleByMapSize(4,8), -2, -2, 2, clShallow, undefined, -4);
			else
				passageMaker(fractionToTiles(0.35), fractionToTiles(cLocation), fractionToTiles(0.65), fractionToTiles(cLocation), scaleByMapSize(4,8), -2, -2, 2, clShallow, undefined, -4);
		}
	}
	
	if (randInt(1,2) == 1)
	{
	
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
				var point = getTIPIADBON([fractionToTiles(cLocation), fractionToTiles(0.5 + cDistance)], [fractionToTiles(cLocation), fractionToTiles(0.5 - cDistance)], [-6, -1.5], 0.5, 4, 0.01);
			else
				var point = getTIPIADBON([fractionToTiles(0.5 + cDistance), fractionToTiles(cLocation)], [fractionToTiles(0.5 - cDistance), fractionToTiles(cLocation)], [-6, -1.5], 0.5, 4, 0.01);
			
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
				var success = createArea(placer, [terrainPainter, elevationPainter, paintClass(clWater)], avoidClasses(clPlayer, 2, clWater, 3, clShallow, 2));
				if (success !== undefined)
				{
					if (mdd1 == 1)
						placer = new ClumpPlacer(floor(PI*scaleByMapSize(10,20)*scaleByMapSize(10,20)/4), 0.95, 0.6, 10, fractionToTiles(0.5 + 0.49*cos(tang)), fractionToTiles(0.5 + 0.49*sin(tang)));
					else
						placer = new ClumpPlacer(0.95, floor(PI*scaleByMapSize(10,20)*scaleByMapSize(10,20)/4), 0.6, 10, fractionToTiles(0.5 + 0.49*cos(tang)), fractionToTiles(0.5 + 0.49*sin(tang)));
						
					var painter = new LayeredPainter([tWater, tWater], [1]);
					var elevationPainter = new SmoothElevationPainter(ELEVATION_SET, -4, 2);
					createArea(placer, [painter, elevationPainter], avoidClasses(clPlayer, 15));
				}
			}
		}
	if (mdd2 == 1)
	{
		if (mdd1 == 1)
		{
			passageMaker(fractionToTiles(0.2), fractionToTiles(0.25), fractionToTiles(0.8), fractionToTiles(0.25), scaleByMapSize(4,8), -2, -2, 2, clShallow, undefined, -4);
			passageMaker(fractionToTiles(0.2), fractionToTiles(0.75), fractionToTiles(0.8), fractionToTiles(0.75), scaleByMapSize(4,8), -2, -2, 2, clShallow, undefined, -4);
		}
			passageMaker(fractionToTiles(0.25), fractionToTiles(0.2), fractionToTiles(0.25), fractionToTiles(0.8), scaleByMapSize(4,8), -2, -2, 2, clShallow, undefined, -4);
			passageMaker(fractionToTiles(0.75), fractionToTiles(0.2), fractionToTiles(0.75), fractionToTiles(0.8), scaleByMapSize(4,8), -2, -2, 2, clShallow, undefined, -4);
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

	for (var ix = 0; ix < mapSize; ix++)
	{
		for (var iz = 0; iz < mapSize; iz++)
		{
			var x = ix / (mapSize + 1.0);
			var z = iz / (mapSize + 1.0);
				setHeight(ix, iz, 3);
		}
	}

	var mdd1 = randInt(1,3);

	if (mdd1 < 3) //lake
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
	}

	if (mdd1 > 1) //rivers
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
	
	if (randInt(1,3) == 1 && mdd1 < 3)//island
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
					[tGrass, tGrass],		// terrains
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
					[tGrass, tGrass],		// terrains
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
			
			for (var i = 0; i < scaleByMapSize(20,120); i++)
			{
				placer = new ClumpPlacer(scaleByMapSize(50, 70), 0.2, 0.1, 10, randFloat(0.67+distance,0.74+distance)*mapSize, randFloat(0.1,0.9)*mapSize);
				var terrainPainter = new LayeredPainter(
					[tGrass, tGrass],		// terrains
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

			for (var i = 0; i < scaleByMapSize(20,120); i++)
			{
				placer = new ClumpPlacer(scaleByMapSize(50, 70), 0.2, 0.1, 10, randFloat(0.26-distance,0.34-distance)*mapSize, randFloat(0.1,0.9)*mapSize);
				var terrainPainter = new LayeredPainter(
					[tGrass, tGrass],		// terrains
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
	
	var mdd3 = randInt(1,3);
	if (mdd3 == 1)
	{
		// create islands
		log("Creating islands...");
		placer = new ClumpPlacer(randInt(scaleByMapSize(8,15),scaleByMapSize(15,23))*randInt(scaleByMapSize(8,15),scaleByMapSize(15,23)), 0.80, 0.1, randFloat(0.0, 0.2));
		terrainPainter = new LayeredPainter(
			[tGrass, tGrass],		// terrains
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
			[tGrass, tGrass],		// terrains
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
	
	var mdd1 = randInt(1,4);
	
	// randomize player order
	var playerIDs = [];
	for (var i = 0; i < numPlayers; i++)
	{
		playerIDs.push(i+1);
	}
	playerIDs = sortPlayers(playerIDs);
	
	var fx = fractionToTiles(0.5);
	var fz = fractionToTiles(0.5);
	ix = round(fx);
	iz = round(fz);

	var lSize = 1;

	var placer = new ClumpPlacer(mapArea * 0.08 * lSize, 0.7, 0.05, 10, ix, iz);
	var terrainPainter = new LayeredPainter(
		[tGrass, tGrass, tGrass, tGrass],		// terrains
		[1, 4, 2]		// widths
	);

	var elevationPainter = new SmoothElevationPainter(
		ELEVATION_SET,			// type
		-3,				// elevation
		4				// blend radius
	);
	createArea(placer, [terrainPainter, elevationPainter, paintClass(clWater)], avoidClasses(clPlayer,scaleByMapSize(15,25)));

	var fx = fractionToTiles(0.5 - 0.2*cos(mdd1*PI/2));
	var fz = fractionToTiles(0.5 - 0.2*sin(mdd1*PI/2));
	ix = round(fx);
	iz = round(fz);

	var lSize = sqrt(sqrt(sqrt(scaleByMapSize(1, 6))));

	var placer = new ClumpPlacer(mapArea * 0.13 * lSize, 0.7, 0.05, 10, ix, iz);
	var terrainPainter = new LayeredPainter(
		[tGrass, tGrass, tGrass, tGrass],		// terrains
		[1, 4, 2]		// widths
	);
	var elevationPainter = new SmoothElevationPainter(
		ELEVATION_SET,			// type
		-3,				// elevation
		4				// blend radius
	);
	createArea(placer, [terrainPainter, elevationPainter, paintClass(clWater)], avoidClasses(clPlayer,scaleByMapSize(15,25)));

	var fx = fractionToTiles(0.5 - 0.49*cos(mdd1*PI/2));
	var fz = fractionToTiles(0.5 - 0.49*sin(mdd1*PI/2));
	ix = round(fx);
	iz = round(fz);

	var lSize = sqrt(sqrt(sqrt(scaleByMapSize(1, 6))));

	var placer = new ClumpPlacer(mapArea * 0.15 * lSize, 0.7, 0.05, 10, ix, iz);
	var terrainPainter = new LayeredPainter(
		[tGrass, tGrass, tGrass, tGrass],		// terrains
		[1, 4, 2]		// widths
	);
	var elevationPainter = new SmoothElevationPainter(
		ELEVATION_SET,			// type
		-3,				// elevation
		4				// blend radius
	);
	createArea(placer, [terrainPainter, elevationPainter, paintClass(clWater)], avoidClasses(clPlayer,scaleByMapSize(15,25)));	
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

	for (var ix = 0; ix < mapSize; ix++)
	{
		for (var iz = 0; iz < mapSize; iz++)
		{
			var x = ix / (mapSize + 1.0);
			var z = iz / (mapSize + 1.0);
				setHeight(ix, iz, 3);
		}
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
			[tGrass, tGrass],		// terrains
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
		[tGrass, tGrass, tGrass, tGrass],		// terrains
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
			[tGrass, tGrass, tGrass],		// terrains
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
paintTerrainBasedOnHeight(3, 3.12, 1, tGrass);
paintTerrainBasedOnHeight(1, 3, 1, tShore);
paintTerrainBasedOnHeight(-8, 1, 2, tWater);
unPaintTileClassBasedOnHeight(0, 3.12, 1, clWater);
unPaintTileClassBasedOnHeight(-6, 0, 1, clLand);
paintTileClassBasedOnHeight(-6, 0, 1, clWater);
paintTileClassBasedOnHeight(0, 3.12, 1, clLand);
paintTileClassBasedOnHeight(3.12, 40, 1, clHill);

// place players

var playerX = new Array(numPlayers);
var playerZ = new Array(numPlayers);
var distmin = scaleByMapSize(60,240);
distmin *= distmin;

for (var i = 0; i < numPlayers; i++)
{
	var placableArea = [];
	for (var mx = 0; mx < mapSize; mx++)
	{
		for (var mz = 0; mz < mapSize; mz++)
		{
			if (!g_Map.validT(mx, mz, 6))
				continue;

			var placable = true;
			for (var c = 0; c < i; c++)
				if ((playerX[c] - mx)*(playerX[c] - mx) + (playerZ[c] - mz)*(playerZ[c] - mz) < distmin)
					placable = false;
			if (!placable)
				continue;

			if (g_Map.getHeight(mx, mz) >= 3 && g_Map.getHeight(mx, mz) <= 3.12)
				placableArea.push([mx, mz]);
		}
	}

	if (!placableArea.length)
	{
		for (var mx = 0; mx < mapSize; ++mx)
		{
			for (var mz = 0; mz < mapSize; mz++)
			{
				if (!g_Map.validT(mx, mz, 6))
					continue;

				var placable = true;
				for (var c = 0; c < i; c++)
					if ((playerX[c] - mx)*(playerX[c] - mx) + (playerZ[c] - mz)*(playerZ[c] - mz) < distmin/4)
						placable = false;
				if (!placable)
					continue;

				if (g_Map.getHeight(mx, mz) >= 3 && g_Map.getHeight(mx, mz) <= 3.12)
					placableArea.push([mx, mz]);
			}
		}
	}

	if (!placableArea.length)
	{
		for (var mx = 0; mx < mapSize; ++mx)
			for (var mz = 0; mz < mapSize; ++mz)
				if (g_Map.getHeight(mx, mz) >= 3 && g_Map.getHeight(mx, mz) <= 3.12)
					placableArea.push([mx, mz]);
	}
	var chosen = floor(Math.random()*placableArea.length);
	playerX[i] = placableArea[chosen][0];
	playerZ[i] = placableArea[chosen][1];
}


for (var i = 0; i < numPlayers; ++i)
{
	var id = playerIDs[i];
	log("Creating units for player " + id + "...");

	// get the x and z in tiles
	var ix = playerX[i];
	var iz = playerZ[i];
	var civEntities = getStartingEntities(id-1);
	var angle = randFloat(0, TWO_PI);
	for (var j = 0; j < civEntities.length; ++j)
	{
		// TODO: Make an rmlib function to get only non-structure starting entities and loop over those 
		if (!civEntities[j].Template.startsWith("units/"))
			continue;
		
		var count = civEntities[j].Count || 1;
		var jx = ix + 2 * cos(angle);
		var jz = iz + 2 * sin(angle);
		var kAngle = randFloat(0, TWO_PI);
		for (var k = 0; k < count; ++k)
			placeObject(jx + cos(kAngle + k*TWO_PI/count), jz + sin(kAngle + k*TWO_PI/count), civEntities[j].Template, id, randFloat(0, TWO_PI));
		angle += TWO_PI / 3;
	}

	if (md > 9)  // maps without water, so we must have enough resources to build a cc
	{
		if (g_MapSettings.StartingResources < 500)
		{
			var loop = (g_MapSettings.StartingResources < 200) ? 2 : 1;
			for (let l = 0; l < loop; ++l)
			{
				var angle = randFloat(0, TWO_PI);
				var rad = randFloat(3, 5);
				var jx = ix + rad * cos(angle);
				var jz = iz + rad * sin(angle);
				placeObject(jx, jz, "gaia/special_treasure_wood", 0, randFloat(0, TWO_PI));
				var angle = randFloat(0, TWO_PI);
				var rad = randFloat(3, 5);
				var jx = ix + rad * cos(angle);
				var jz = iz + rad * sin(angle);
				placeObject(jx, jz, "gaia/special_treasure_stone", 0, randFloat(0, TWO_PI));
				var angle = randFloat(0, TWO_PI);
				var rad = randFloat(3, 5);
				var jx = ix + rad * cos(angle);
				var jz = iz + rad * sin(angle);
				placeObject(jx, jz, "gaia/special_treasure_metal", 0, randFloat(0, TWO_PI));
			}
		}
	}
	else    // we must have enough resources to build a dock
	{
		if (g_MapSettings.StartingResources < 200)
		{
			var angle = randFloat(0, TWO_PI);
			var rad = randFloat(3, 5);
			var jx = ix + rad * cos(angle);
			var jz = iz + rad * sin(angle);
			placeObject(jx, jz, "gaia/special_treasure_wood", 0, randFloat(0, TWO_PI));
		}
	}
}

for (var i = 0; i < numPlayers; i++)
{
	var radius = scaleByMapSize(18,32);
	var ix = playerX[i];
	var iz = playerZ[i];
	var cityRadius = radius/3;
	placer = new ClumpPlacer(PI*cityRadius*cityRadius, 0.6, 0.3, 10, ix, iz);
	createArea(placer, paintClass(clPlayer), null);
}

// create bumps
log("Creating bumps...");
placer = new ClumpPlacer(scaleByMapSize(20, 50), 0.3, 0.06, 1);
var painter = new SmoothElevationPainter(ELEVATION_MODIFY, 2, 2);
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
	[avoidClasses(clPlayer, 15, clHill, randInt(6, 18)), stayClasses(clLand, 0)],
	randInt(0, scaleByMapSize(4, 8))*randInt(1, scaleByMapSize(4, 9))
);

var multiplier = sqrt(randFloat(0.5,1.2)*randFloat(0.5,1.2));
// calculate desired number of trees for map (based on size)
if (rt == 6)
{
var MIN_TREES = floor(200*multiplier);
var MAX_TREES = floor(1250*multiplier);
var P_FOREST = randFloat(0.02, 0.05);
}
else if (rt == 7)
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
		[avoidClasses(clPlayer, 17, clForest, randInt(5, 15), clHill, 0), stayClasses(clLand, 4)],
		num
	);
}

RMS.SetProgress(50);
// create dirt patches
log("Creating dirt patches...");
var sizes = [scaleByMapSize(3, 48), scaleByMapSize(5, 84), scaleByMapSize(8, 128)];
var numb = 1;
if (rt == 6)
	numb = 3
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
		[avoidClasses(clForest, 0, clHill, 0, clDirt, 5, clPlayer, 0), stayClasses(clLand, 4)],
		numb*scaleByMapSize(15, 45)
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
		[avoidClasses(clForest, 0, clHill, 0, clDirt, 5, clPlayer, 0), stayClasses(clLand, 4)],
		numb*scaleByMapSize(15, 45)
	);
}
RMS.SetProgress(55);


log("Creating stone mines...");
// create large stone quarries
var group = new SimpleGroup([new SimpleObject(oStoneSmall, 0,2, 0,4), new SimpleObject(oStoneLarge, 1,1, 0,4)], true, clRock);
createObjectGroups(group, 0,
	[avoidClasses(clForest, 1, clPlayer, 10, clRock, 10, clHill, 1), stayClasses(clLand, 3)],
	randInt(scaleByMapSize(2,9),scaleByMapSize(9,40)), 100
);

// create small stone quarries
group = new SimpleGroup([new SimpleObject(oStoneSmall, 2,5, 1,3)], true, clRock);
createObjectGroups(group, 0,
	[avoidClasses(clForest, 1, clPlayer, 10, clRock, 10, clHill, 1), stayClasses(clLand, 3)],
	randInt(scaleByMapSize(2,9),scaleByMapSize(9,40)), 100
);

log("Creating metal mines...");
// create large metal quarries
group = new SimpleGroup([new SimpleObject(oMetalLarge, 1,1, 0,4)], true, clMetal);
createObjectGroups(group, 0,
	[avoidClasses(clForest, 1, clPlayer, 10, clMetal, 10, clRock, 5, clHill, 1), stayClasses(clLand, 3)],
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
	[avoidClasses(clWater, 0, clForest, 0, clPlayer, 0, clHill, 0), stayClasses(clLand, 3)],
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
	[avoidClasses(clWater, 0, clForest, 0, clPlayer, 0, clHill, 0), stayClasses(clLand, 3)],
	scaleByMapSize(8, 131), 50
);

RMS.SetProgress(70);

// create deer
log("Creating deer...");
group = new SimpleGroup(
	[new SimpleObject(oDeer, 5,7, 0,4)],
	true, clFood
);
createObjectGroups(group, 0,
	[avoidClasses(clWater, 0, clForest, 0, clPlayer, 8, clHill, 1, clFood, 20), stayClasses(clLand, 2)],
	randInt(numPlayers+3, 5*numPlayers+4), 50
);

// create berry bush
log("Creating berry bush...");
group = new SimpleGroup(
	[new SimpleObject(oBerryBush, 5,7, 0,4)],
	true, clFood
);
createObjectGroups(group, 0,
	[avoidClasses(clWater, 0, clForest, 0, clPlayer, 8, clHill, 1, clFood, 20), stayClasses(clLand, 2)],
	randInt(1, 4) * numPlayers + 2, 50
);

RMS.SetProgress(75);

// create sheep
log("Creating sheep...");
group = new SimpleGroup(
	[new SimpleObject(oSheep, 2,3, 0,2)],
	true, clFood
);
createObjectGroups(group, 0,
	[avoidClasses(clWater, 0, clForest, 0, clPlayer, 8, clHill, 1, clFood, 20), stayClasses(clLand, 2)],
	randInt(numPlayers+3, 5*numPlayers+4), 50
);

// create fish
log("Creating fish...");
group = new SimpleGroup(
	[new SimpleObject(oFish, 2,3, 0,2)],
	true, clFood
);
createObjectGroups(group, 0,
	avoidClasses(clLand, 4, clForest, 0, clPlayer, 0, clHill, 0, clFood, 20),
	randInt(15, 40) * numPlayers, 60
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
		[avoidClasses(clWater, 1, clForest, 1, clHill, 1, clPlayer, 0, clMetal, 1, clRock, 1), stayClasses(clLand, 4)],
		num
	);
}

var planetm = 1;
if (rt==7)
{
	planetm = 8;
}
//create small grass tufts
log("Creating small grass tufts...");
group = new SimpleGroup(
	[new SimpleObject(aGrassShort, 1,2, 0,1, -PI/8,PI/8)]
);
createObjectGroups(group, 0,
	[avoidClasses(clWater, 2, clHill, 2, clPlayer, 2, clDirt, 0), stayClasses(clLand, 3)],
	planetm * scaleByMapSize(13, 200)
);

RMS.SetProgress(90);

// create large grass tufts
log("Creating large grass tufts...");
group = new SimpleGroup(
	[new SimpleObject(aGrass, 2,4, 0,1.8, -PI/8,PI/8), new SimpleObject(aGrassShort, 3,6, 1.2,2.5, -PI/8,PI/8)]
);
createObjectGroups(group, 0,
	[avoidClasses(clWater, 3, clHill, 2, clPlayer, 2, clDirt, 1, clForest, 0), stayClasses(clLand, 3)],
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

rt = randInt(1,6)
if (rt==1){
setSkySet("cirrus");
}
else if (rt ==2){
setSkySet("cumulus");
}
else if (rt ==3){
setSkySet("sunny");
}
else if (rt ==4){
setSkySet("sunny 1");
}
else if (rt ==5){
setSkySet("mountainous");
}
else if (rt ==6){
setSkySet("stratus");
}
setSunRotation(randFloat(0, TWO_PI));
setSunElevation(randFloat(PI/ 5, PI / 3));

// Export map data
ExportMap();
