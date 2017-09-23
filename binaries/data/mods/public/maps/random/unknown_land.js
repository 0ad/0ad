RMS.LoadLibrary("rmgen");
RMS.LoadLibrary("rmbiome");

TILE_CENTERED_HEIGHT_MAP = true;

setSelectedBiome();

const tMainTerrain = g_Terrains.mainTerrain;
const tForestFloor1 = g_Terrains.forestFloor1;
const tForestFloor2 = g_Terrains.forestFloor2;
const tCliff = g_Terrains.cliff;
const tTier1Terrain = g_Terrains.tier1Terrain;
const tTier2Terrain = g_Terrains.tier2Terrain;
const tTier3Terrain = g_Terrains.tier3Terrain;
const tHill = g_Terrains.hill;
const tDirt = g_Terrains.dirt;
const tRoad = g_Terrains.road;
const tRoadWild = g_Terrains.roadWild;
const tTier4Terrain = g_Terrains.tier4Terrain;
const tShoreBlend = g_Terrains.shoreBlend;
const tShore = g_Terrains.shore;
const tWater = g_Terrains.water;

const oTree1 = g_Gaia.tree1;
const oTree2 = g_Gaia.tree2;
const oTree3 = g_Gaia.tree3;
const oTree4 = g_Gaia.tree4;
const oTree5 = g_Gaia.tree5;
const oFruitBush = g_Gaia.fruitBush;
const oMainHuntableAnimal = g_Gaia.mainHuntableAnimal;
const oFish = g_Gaia.fish;
const oSecondaryHuntableAnimal = g_Gaia.secondaryHuntableAnimal;
const oStoneLarge = g_Gaia.stoneLarge;
const oStoneSmall = g_Gaia.stoneSmall;
const oMetalLarge = g_Gaia.metalLarge;
const oWood = "gaia/special_treasure_wood";

const aGrass = g_Decoratives.grass;
const aGrassShort = g_Decoratives.grassShort;
const aReeds = g_Decoratives.reeds;
const aLillies = g_Decoratives.lillies;
const aRockLarge = g_Decoratives.rockLarge;
const aRockMedium = g_Decoratives.rockMedium;
const aBushMedium = g_Decoratives.bushMedium;
const aBushSmall = g_Decoratives.bushSmall;

const pForest1 = [tForestFloor2 + TERRAIN_SEPARATOR + oTree1, tForestFloor2 + TERRAIN_SEPARATOR + oTree2, tForestFloor2];
const pForest2 = [tForestFloor1 + TERRAIN_SEPARATOR + oTree4, tForestFloor1 + TERRAIN_SEPARATOR + oTree5, tForestFloor1];

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
var clLand = createTileClass();
var clShallow = createTileClass();

initTerrain(tWater);

var md = randIntInclusive(2,13);
var needsAdditionalWood = false;
//*****************************************************************************************************************************
if (md == 2) //continent
{
	var [playerIDs, playerX, playerZ, playerAngle] = radialPlayerPlacement(0.25);

	for (var i = 0; i < numPlayers; i++)
	{
		var fx = fractionToTiles(playerX[i]);
		var fz = fractionToTiles(playerZ[i]);
		var ix = round(fx);
		var iz = round(fz);

		addCivicCenterAreaToClass(ix, iz, clPlayer);

		var placer = new ChainPlacer(2, floor(scaleByMapSize(5, 9)), floor(scaleByMapSize(5, 20)), 1, ix, iz, 0, [floor(scaleByMapSize(23, 50))]);
		var elevationPainter = new SmoothElevationPainter(
			ELEVATION_SET,			// type
			3,				// elevation
			4				// blend radius
		);
		createArea(placer, [elevationPainter, paintClass(clLand)], null);
	}

	var fx = fractionToTiles(0.5);
	var fz = fractionToTiles(0.5);
	ix = round(fx);
	iz = round(fz);

	var placer = new ChainPlacer(2, floor(scaleByMapSize(5, 12)), floor(scaleByMapSize(60, 700)), 1, ix, iz, 0, [floor(mapSize * 0.33)]);
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

	if (randBool(1/3)) // peninsula
	{
		var angle = randFloat(0, TWO_PI);

		var fx = fractionToTiles(0.5 + 0.25*cos(angle));
		var fz = fractionToTiles(0.5 + 0.25*sin(angle));
		ix = round(fx);
		iz = round(fz);

		var placer = new ChainPlacer(2, floor(scaleByMapSize(5, 12)), floor(scaleByMapSize(60, 700)), 1, ix, iz, 0, [floor(mapSize * 0.33)]);
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
}
//********************************************************************************************************
else if (md == 3) //central sea
{
	var playerIDs = primeSortAllPlayers();
	var playerPos = placePlayersRiver();
	var playerX = [];
	var playerZ = [];
	var playerAngle = [];

	var mdd1 = randIntInclusive(1,2);
	if (mdd1 == 1) //vertical
		for (var i = 0; i < numPlayers; i++)
		{
			playerZ[i] = playerPos[i];
			playerX[i] = 0.2 + 0.6*(i%2);
		}
	else //horizontal
		for (var i = 0; i < numPlayers; i++)
		{
			playerZ[i] = 0.2 + 0.6*(i%2);
			playerX[i] = playerPos[i];
		}

	paintRiver({
		"horizontal": mdd1 != 1,
		"parallel": false,
		"position": 0.5,
		"width": randFloat(0.22, 0.3) + scaleByMapSize(1, 4) / 20,
		"fadeDist": 0.025,
		"deviation": 0,
		"waterHeight": -3,
		"landHeight": 3,
		"meanderShort": 20,
		"meanderLong": 0,
		"waterFunc": (ix, iz, height) => {
			placeTerrain(ix, iz, height < -1.5 ? tWater : tShore);
			if (height < 0)
				addToClass(ix, iz, clWater);
		},
		"landFunc": (ix, iz, shoreDist1, shoreDist2) => {
			setHeight(ix, iz, 3.1);
			addToClass(ix, iz, clLand);
		}
	});

	// linked
	if (mdd1 == 1) //vertical
	{
		var placer = new PathPlacer(1, fractionToTiles(0.5), fractionToTiles(0.99), fractionToTiles(0.5), scaleByMapSize(randIntInclusive(16,24),randIntInclusive(100,140)), 0.5, 3*(scaleByMapSize(1,4)), 0.1, 0.01);
	}
	else
	{
		var placer = new PathPlacer(fractionToTiles(0.5), 1, fractionToTiles(0.5), fractionToTiles(0.99), scaleByMapSize(randIntInclusive(16,24),randIntInclusive(100,140)), 0.5, 3*(scaleByMapSize(1,4)), 0.1, 0.01);
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

	var mdd2 = randIntInclusive(1,7);
	if (mdd2 == 1)
	{
		log("Creating islands...");
		placer = new ChainPlacer(floor(scaleByMapSize(4, 7)), floor(scaleByMapSize(7, 10)), floor(scaleByMapSize(16, 40)), 0.07);
		terrainPainter = new LayeredPainter(
			[tMainTerrain, tMainTerrain],		// terrains
			[2]								// widths
		);
		elevationPainter = new SmoothElevationPainter(ELEVATION_SET, 3.1, 4);
		createAreas(
			placer,
			[terrainPainter, elevationPainter, paintClass(clLand)],
			avoidClasses(clLand, 3, clPlayer, 3),
			scaleByMapSize(2, 5)*randIntInclusive(8,14)
		);
	}
	else if (mdd2 == 2)
	{
		log("Creating extentions...");
		placer = new ClumpPlacer(randIntInclusive(scaleByMapSize(13,24),scaleByMapSize(24,45))*randIntInclusive(scaleByMapSize(13,24),scaleByMapSize(24,45)), 0.80, 0.1, 10);
		terrainPainter = new LayeredPainter(
			[tMainTerrain, tMainTerrain],		// terrains
			[2]								// widths
		);
		elevationPainter = new SmoothElevationPainter(ELEVATION_SET, 3.1, 4);
		createAreas(
			placer,
			[terrainPainter, elevationPainter, paintClass(clLand)],
			null,
			scaleByMapSize(2, 5)*randIntInclusive(8,14)
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

	var playerIDs = primeSortAllPlayers();
	var playerPos = placePlayersRiver();

	var playerX = [];
	var playerZ = [];
	var playerAngle = [];

	var mdd1 = randIntInclusive(1,2);
	if (mdd1 == 1) //horizontal
		for (var i = 0; i < numPlayers; i++)
		{
			playerZ[i] = 0.25 + 0.5*(i%2);
			playerX[i] = playerPos[i];
		}
	else //vertical
		for (var i = 0; i < numPlayers; i++)
		{
			playerZ[i] = playerPos[i];
			playerX[i] = 0.25 + 0.5*(i%2);
		}

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

	log("Creating the shallows of the main river");

	for (var i = 0; i <= randIntInclusive(1, scaleByMapSize(4,8)); i++)
	{
		var cLocation = randFloat(0.15,0.85);
		if (mdd1 == 1)
			passageMaker(floor(fractionToTiles(cLocation)), floor(fractionToTiles(0.35)), floor(fractionToTiles(cLocation)), floor(fractionToTiles(0.65)), scaleByMapSize(4,8), -2, -2, 2, clShallow, undefined, -4);
		else
			passageMaker(floor(fractionToTiles(0.35)), floor(fractionToTiles(cLocation)), floor(fractionToTiles(0.65)), floor(fractionToTiles(cLocation)), scaleByMapSize(4,8), -2, -2, 2, clShallow, undefined, -4);
	}

	if (randBool())
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

		log("Creating tributaries");

		for (var i = 0; i <= randIntInclusive(8, (scaleByMapSize(12,20))); i++)
		{
			var cLocation = randFloat(0.05,0.95);
			var tang = randFloat(PI*0.2, PI*0.8)*((randIntInclusive(0, 1)-0.5)*2);
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
	var [playerIDs, playerX, playerZ] = radialPlayerPlacement();

	for (var i = 0; i < numPlayers; i++)
	{
		var fx = fractionToTiles(playerX[i]);
		var fz = fractionToTiles(playerZ[i]);
		var ix = round(fx);
		var iz = round(fz);

		addCivicCenterAreaToClass(ix, iz, clPlayer);
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
	var mdd1 = randIntInclusive(1,2);
	if (mdd1 == 1) //lake
	{
		var fx = fractionToTiles(0.5);
		var fz = fractionToTiles(0.5);
		ix = round(fx);
		iz = round(fz);

		var lSize = sqrt(sqrt(sqrt(scaleByMapSize(1, 6))));

		var placer = new ChainPlacer(2, floor(scaleByMapSize(5, 16)), floor(scaleByMapSize(35, 200)), 1, ix, iz, 0, [floor(mapSize * 0.17 * lSize)]);
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

		log("Creating shore jaggedness...");
		placer = new ChainPlacer(2, floor(scaleByMapSize(4, 6)), 3, 1);
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

	if (randBool(1/3) &&(mdd1 == 1))//island
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

	var mdd1 = randIntInclusive(1,2);

	var playerIDs = sortAllPlayers();

	// place players
	var playerX = [];
	var playerZ = [];
	var playerPos = [];

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

		addCivicCenterAreaToClass(ix, iz, clPlayer);
	}

	var mdd2 = randIntInclusive(1,3);
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

	log("Creating shore jaggedness...");
	placer = new ChainPlacer(2, floor(scaleByMapSize(4, 6)), 3, 1);
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

	placer = new ChainPlacer(2, floor(scaleByMapSize(4, 6)), 3, 1);
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

	var mdd3 = randIntInclusive(1,5);
	if (mdd3 == 1)
	{
		log("Creating islands...");
		placer = new ChainPlacer(floor(scaleByMapSize(4, 7)), floor(scaleByMapSize(7, 10)), floor(scaleByMapSize(16, 40)), 0.07);
		terrainPainter = new LayeredPainter(
			[tMainTerrain, tMainTerrain],		// terrains
			[2]								// widths
		);
		elevationPainter = new SmoothElevationPainter(ELEVATION_SET, 3.1, 4);
		createAreas(
			placer,
			[terrainPainter, elevationPainter, paintClass(clLand)],
			avoidClasses(clLand, 3, clPlayer, 3),
			scaleByMapSize(2, 5)*randIntInclusive(8,14)
		);
	}
	else if (mdd3 == 2)
	{
		log("Creating extentions...");
		placer = new ChainPlacer(floor(scaleByMapSize(4, 7)), floor(scaleByMapSize(7, 10)), floor(scaleByMapSize(16, 40)), 0.07);
		terrainPainter = new LayeredPainter(
			[tMainTerrain, tMainTerrain],		// terrains
			[2]								// widths
		);
		elevationPainter = new SmoothElevationPainter(ELEVATION_SET, 3.1, 4);
		createAreas(
			placer,
			[terrainPainter, elevationPainter, paintClass(clLand)],
			null,
			scaleByMapSize(2, 5)*randIntInclusive(8,14)
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

	var playerIDs = sortAllPlayers();

	// place players
	var playerX = [];
	var playerZ = [];
	var playerAngle = [];

	var startAngle = -PI/6 + (mdd1-1)*PI/2;
	for (var i = 0; i < numPlayers; i++)
	{
		playerAngle[i] = startAngle + Math.PI * 2/3 * (numPlayers == 1 ? 1 : 2 * i / (numPlayers - 1));
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

	var placer = new ChainPlacer(2, floor(scaleByMapSize(5, 16)), floor(scaleByMapSize(35, 200)), 1, ix, iz, 0, [floor(mapSize * 0.17 * lSize)]);
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

	var placer = new ChainPlacer(2, floor(scaleByMapSize(5, 16)), floor(scaleByMapSize(35, 120)), 1, ix, iz, 0, [floor(mapSize * 0.18 * lSize)]);
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

	var placer = new ChainPlacer(2, floor(scaleByMapSize(5, 16)), floor(scaleByMapSize(35, 100)), 1, ix, iz, 0, [floor(mapSize * 0.19 * lSize)]);
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
	var [playerIDs, playerX, playerZ, playerAngle] = radialPlayerPlacement();

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

	var lakeAreas = [];
	var playerConstraint = new AvoidTileClassConstraint(clPlayer, 20);
	var waterConstraint = new AvoidTileClassConstraint(clWater, 8);

	for (var x = 0; x < mapSize; ++x)
		for (var z = 0; z < mapSize; ++z)
			if (playerConstraint.allows(x, z) && waterConstraint.allows(x, z))
				lakeAreas.push([x, z]);

	var chosenPoint;
	var lakeAreaLen;

	log("Creating lakes...");
	var numLakes = scaleByMapSize(5, 16);
	for (var i = 0; i < numLakes; ++i)
	{
		lakeAreaLen = lakeAreas.length;
		if (!lakeAreaLen)
			break;

		chosenPoint = pickRandom(lakeAreas);

		placer = new ChainPlacer(1, floor(scaleByMapSize(4, 8)), floor(scaleByMapSize(40, 180)), 0.7, chosenPoint[0], chosenPoint[1]);
		terrainPainter = new LayeredPainter(
			[tShore, tWater, tWater],		// terrains
			[1, 3]								// widths
		);
		elevationPainter = new SmoothElevationPainter(ELEVATION_SET, -5, 5);
		var newLake = createAreas(
			placer,
			[terrainPainter, elevationPainter, paintClass(clWater)],
			avoidClasses(clPlayer, 20, clWater, 8),
			1, 1
		);

		if (newLake && newLake.length)
		{
			var n = 0;
			for (var j = 0; j < lakeAreaLen; ++j)
			{
				var x = lakeAreas[j][0], z = lakeAreas[j][1];
				if (playerConstraint.allows(x, z) && waterConstraint.allows(x, z))
					lakeAreas[n++] = lakeAreas[j];
			}
			lakeAreas.length = n;
		}
	}
}
//********************************************************************************************************
else if (md == 9) //passes
{
	var [playerIDs, playerX, playerZ, playerAngle, startAngle] = radialPlayerPlacement();

	for (var ix = 0; ix < mapSize; ix++)
	{
		for (var iz = 0; iz < mapSize; iz++)
		{
			var x = ix / (mapSize + 1.0);
			var z = iz / (mapSize + 1.0);
				setHeight(ix, iz, 3);
		}
	}

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
	var mdd1 = randIntInclusive (1,3);
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
	var [playerIDs, playerX, playerZ, playerAngle, startAngle] = radialPlayerPlacement();

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
	var [playerIDs, playerX, playerZ, playerAngle] = radialPlayerPlacement();

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

	var radius = scaleByMapSize(17,29);
	var shoreRadius = 4;
	var elevation = 3;

	var hillSize = PI * radius * radius;
	// get the x and z in tiles
	var fx = fractionToTiles(playerX[i]);
	var fz = fractionToTiles(playerZ[i]);
	var ix = round(fx);
	var iz = round(fz);

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

log("Creating bumps...");
placer = new ClumpPlacer(scaleByMapSize(20, 50), 0.3, 0.06, 1);
painter = new SmoothElevationPainter(ELEVATION_MODIFY, 2, 2);
createAreas(
	placer,
	painter,
	[avoidClasses(clWater, 2, clPlayer, 10), stayClasses(clLand, 3)],
	randIntInclusive(0,scaleByMapSize(200, 400))
);

log("Creating hills...");
placer = new ChainPlacer(1, floor(scaleByMapSize(4, 6)), floor(scaleByMapSize(16, 40)), 0.5);
terrainPainter = new LayeredPainter(
	[tCliff, tHill],		// terrains
	[2]								// widths
);
elevationPainter = new SmoothElevationPainter(ELEVATION_SET, 18, 2);
createAreas(
	placer,
	[terrainPainter, elevationPainter, paintClass(clHill)],
	[avoidClasses(clPlayer, 20, clHill, randIntInclusive(6, 18)), stayClasses(clLand, 0)],
	randIntInclusive(0, scaleByMapSize(4, 8))*randIntInclusive(1, scaleByMapSize(4, 9))
);

var multiplier = sqrt(randFloat(0.5,1.2)*randFloat(0.5,1.2));
// calculate desired number of trees for map (based on size)
if (currentBiome() == "savanna")
{
	var MIN_TREES = floor(200*multiplier);
	var MAX_TREES = floor(1250*multiplier);
	var P_FOREST = 0;
}
else if (currentBiome() == "tropic")
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

log("Creating forests...");
var types = [
	[[tForestFloor2, tMainTerrain, pForest1], [tForestFloor2, pForest1]],
	[[tForestFloor1, tMainTerrain, pForest2], [tForestFloor1, pForest2]]
];	// some variation

if (currentBiome() != "savanna")
{
	var size = numForest / (scaleByMapSize(3,6) * numPlayers);
	var num = floor(size / types.length);
	for (var i = 0; i < types.length; ++i)
	{
		placer = new ChainPlacer(1, floor(scaleByMapSize(3, 5)), numForest / num, 0.5);
		painter = new LayeredPainter(
			types[i],		// terrains
			[2]											// widths
			);
		createAreas(
			placer,
			[painter, paintClass(clForest)],
			[avoidClasses(clPlayer, 20, clForest, randIntInclusive(5, 15), clHill, 0), stayClasses(clLand, 4)],
			num
		);
	}
}

RMS.SetProgress(50);
log("Creating dirt patches...");
var sizes = [scaleByMapSize(3, 6), scaleByMapSize(5, 10), scaleByMapSize(8, 21)];
var numb = 1;
if (currentBiome() == "savanna")
	numb = 3;
for (var i = 0; i < sizes.length; i++)
{
	placer = new ChainPlacer(1, floor(scaleByMapSize(3, 5)), sizes[i], 0.5);
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

log("Creating grass patches...");
var sizes = [scaleByMapSize(2, 4), scaleByMapSize(3, 7), scaleByMapSize(5, 15)];
for (var i = 0; i < sizes.length; i++)
{
	placer = new ChainPlacer(1, floor(scaleByMapSize(3, 5)), sizes[i], 0.5);
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
group = new SimpleGroup([new SimpleObject(oStoneSmall, 0,2, 0,4), new SimpleObject(oStoneLarge, 1,1, 0,4)], true, clRock);
createObjectGroupsDeprecated(group, 0,
	[avoidClasses(clForest, 1, clPlayer, 20, clRock, 10, clHill, 1), stayClasses(clLand, 4)],
	randIntInclusive(scaleByMapSize(2,9),scaleByMapSize(9,40)), 100
);

log("Creating small stone quarries...");
group = new SimpleGroup([new SimpleObject(oStoneSmall, 2,5, 1,3)], true, clRock);
createObjectGroupsDeprecated(group, 0,
	[avoidClasses(clForest, 1, clPlayer, 20, clRock, 10, clHill, 1), stayClasses(clLand, 4)],
	randIntInclusive(scaleByMapSize(2,9),scaleByMapSize(9,40)), 100
);

log("Creating metal mines...");
group = new SimpleGroup([new SimpleObject(oMetalLarge, 1,1, 0,4)], true, clMetal);
createObjectGroupsDeprecated(group, 0,
	[avoidClasses(clForest, 1, clPlayer, 20, clMetal, 10, clRock, 5, clHill, 1), stayClasses(clLand, 4)],
	randIntInclusive(scaleByMapSize(2,9),scaleByMapSize(9,40)), 100
);

RMS.SetProgress(65);

log("Creating small decorative rocks...");
group = new SimpleGroup(
	[new SimpleObject(aRockMedium, 1,3, 0,1)],
	true
);
createObjectGroupsDeprecated(
	group, 0,
	[avoidClasses(clWater, 0, clForest, 0, clPlayer, 0, clHill, 0), stayClasses(clLand, 4)],
	scaleByMapSize(16, 262), 50
);

log("Creating large decorative rocks...");
group = new SimpleGroup(
	[new SimpleObject(aRockLarge, 1,2, 0,1), new SimpleObject(aRockMedium, 1,3, 0,2)],
	true
);
createObjectGroupsDeprecated(
	group, 0,
	[avoidClasses(clWater, 0, clForest, 0, clPlayer, 0, clHill, 0), stayClasses(clLand, 4)],
	scaleByMapSize(8, 131), 50
);
RMS.SetProgress(70);

log("Creating deer...");
group = new SimpleGroup(
	[new SimpleObject(oMainHuntableAnimal, 5,7, 0,4)],
	true, clFood
);
createObjectGroupsDeprecated(group, 0,
	[avoidClasses(clWater, 0, clForest, 0, clPlayer, 20, clHill, 1, clFood, 20), stayClasses(clLand, 4)],
	randIntInclusive(numPlayers+3, 5*numPlayers+4), 50
);

log("Creating berry bush...");
group = new SimpleGroup(
	[new SimpleObject(oFruitBush, 5,7, 0,4)],
	true, clFood
);
createObjectGroupsDeprecated(group, 0,
	[avoidClasses(clWater, 0, clForest, 0, clPlayer, 20, clHill, 1, clFood, 20), stayClasses(clLand, 4)],
	randIntInclusive(1, 4) * numPlayers + 2, 50
);
RMS.SetProgress(75);

log("Creating sheep...");
group = new SimpleGroup(
	[new SimpleObject(oSecondaryHuntableAnimal, 2,3, 0,2)],
	true, clFood
);
createObjectGroupsDeprecated(group, 0,
	[avoidClasses(clWater, 0, clForest, 0, clPlayer, 20, clHill, 1, clFood, 20), stayClasses(clLand, 4)],
	randIntInclusive(numPlayers+3, 5*numPlayers+4), 50
);

log("Creating fish...");
group = new SimpleGroup(
	[new SimpleObject(oFish, 2,3, 0,2)],
	true, clFood
);
createObjectGroupsDeprecated(group, 0,
	avoidClasses(clLand, 5, clForest, 0, clPlayer, 0, clHill, 0, clFood, 20),
	randIntInclusive(15, 40) * numPlayers, 60
);
RMS.SetProgress(85);

log("Creating straggler trees...");
var types = [oTree1, oTree2, oTree4, oTree3];	// some variation
var num = floor(numStragglers / types.length);
for (var i = 0; i < types.length; ++i)
{
	group = new SimpleGroup(
		[new SimpleObject(types[i], 1,1, 0,3)],
		true, clForest
	);
	createObjectGroupsDeprecated(group, 0,
		[avoidClasses(clWater, 1, clForest, 7, clHill, 1, clPlayer, 0, clMetal, 6, clRock, 6), stayClasses(clLand, 4)],
		num
	);
}

var planetm = 1;
if (currentBiome() == "tropic")
	planetm = 8;

log("Creating small grass tufts...");
group = new SimpleGroup(
	[new SimpleObject(aGrassShort, 1,2, 0,1, -PI/8,PI/8)]
);
createObjectGroupsDeprecated(group, 0,
	[avoidClasses(clWater, 2, clHill, 2, clPlayer, 2, clDirt, 0), stayClasses(clLand, 4)],
	planetm * scaleByMapSize(13, 200)
);
RMS.SetProgress(90);

log("Creating large grass tufts...");
group = new SimpleGroup(
	[new SimpleObject(aGrass, 2,4, 0,1.8, -PI/8,PI/8), new SimpleObject(aGrassShort, 3,6, 1.2,2.5, -PI/8,PI/8)]
);
createObjectGroupsDeprecated(group, 0,
	[avoidClasses(clWater, 3, clHill, 2, clPlayer, 2, clDirt, 1, clForest, 0), stayClasses(clLand, 4)],
	planetm * scaleByMapSize(13, 200)
);
RMS.SetProgress(95);

log("Creating shallow flora...");
group = new SimpleGroup(
	[new SimpleObject(aLillies, 1,2, 0,2), new SimpleObject(aReeds, 2,4, 0,2)]
);
createObjectGroupsDeprecated(group, 0,
	stayClasses(clShallow, 1),
	60 * scaleByMapSize(13, 200), 80
);

log("Creating bushes...");
group = new SimpleGroup(
	[new SimpleObject(aBushMedium, 1,2, 0,2), new SimpleObject(aBushSmall, 2,4, 0,2)]
);
createObjectGroupsDeprecated(group, 0,
	[avoidClasses(clWater, 1, clHill, 1, clPlayer, 1, clDirt, 1), stayClasses(clLand, 3)],
	planetm * scaleByMapSize(13, 200), 50
);

setSkySet(pickRandom(["cirrus", "cumulus", "sunny", "sunny 1", "mountainous", "stratus"]));
setSunRotation(randFloat(0, TWO_PI));
setSunElevation(randFloat(PI/ 5, PI / 3));

ExportMap();
