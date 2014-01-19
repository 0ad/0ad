var g_numStragglerTrees = 0

function createBumps(constraint, count, minsize, maxsize, spread, failfraction, elevation)
{
	log("Creating bumps...");
	constraint = (constraint !== undefined ? constraint : avoidClasses(clPlayer, 20));
	minsize = (minsize !== undefined ? minsize : 1);
	maxsize = (maxsize !== undefined ? maxsize : floor(scaleByMapSize(4, 6)));
	spread = (spread !== undefined ? spread : floor(scaleByMapSize(2, 5)));
	failfraction = (failfraction !== undefined ? failfraction : 0);
	elevation = (elevation !== undefined ? elevation : 2);
	count = (count !== undefined ? count : scaleByMapSize(100, 200));
	
	var placer = new ChainPlacer(minsize, maxsize, spread, failfraction);
	var painter = new SmoothElevationPainter(ELEVATION_MODIFY, elevation, 2);
	createAreas(
		placer,
		painter, 
		constraint,
		count
	);
}

function createHills(terrainset, tileclass, constraint, count, minsize, maxsize, spread, failfraction, elevation, elevationsmooth)
{
	log("Creating hills...");
	
	tileclass = (tileclass !== undefined ? tileclass : clHill);
	constraint = (constraint !== undefined ? constraint : avoidClasses(clPlayer, 20, clHill, 15));
	count = (count !== undefined ? count : scaleByMapSize(1, 4) * getNumPlayers());
	minsize = (minsize !== undefined ? minsize : 1);
	maxsize = (maxsize !== undefined ? maxsize : floor(scaleByMapSize(4, 6)));
	spread = (spread !== undefined ? spread : floor(scaleByMapSize(16, 40)));
	failfraction = (failfraction !== undefined ? failfraction : 0.5);
	elevation = (elevation !== undefined ? elevation : 18);
	elevationsmooth = (elevationsmooth !== undefined ? elevationsmooth : 2);
	
	var placer = new ChainPlacer(minsize, maxsize, spread, failfraction);
	var terrainPainter = new LayeredPainter(
		terrainset,		// terrains
		[1, elevationsmooth]			// widths
	);
	var elevationPainter = new SmoothElevationPainter(ELEVATION_SET, elevation, elevationsmooth);
	createAreas(
		placer,
		[terrainPainter, elevationPainter, paintClass(clHill)], 
		constraint,
		count
	);
}

function createMountains(terrain, tileclass, constraint, count, maxHeight, minRadius, maxRadius, numCircles)
{
	log("Creating mountains...");
	
	tileclass = (tileclass !== undefined ? tileclass : clHill);
	constraint = (constraint !== undefined ? constraint : avoidClasses(clPlayer, 20, clHill, 15));
	count = (count !== undefined ? count : scaleByMapSize(1, 4) * getNumPlayers());
	maxHeight = (maxHeight !== undefined ? maxHeight : floor(scaleByMapSize(30, 50)));
	minRadius = (minRadius !== undefined ? minRadius : floor(scaleByMapSize(3, 4)));
	maxRadius = (maxRadius !== undefined ? maxRadius : floor(scaleByMapSize(6, 12)));
	numCircles = (numCircles !== undefined ? numCircles : floor(scaleByMapSize(4, 10)));
	
	var numHills = count
	for (var i = 0; i < numHills; ++i)
	{
		
		createMountain(
			maxHeight,
			minRadius,
			maxRadius,
			numCircles,
			constraint,
			randInt(mapSize),
			randInt(mapSize),
			terrain,
			tileclass,
			14
		);
	}
}

function createForests(terrainset, tileclass, constraint, numMultiplier, biomeID)
{
	log("Creating forests...");

	tileclass = (tileclass !== undefined ? tileclass : clForest);
	constraint = (constraint !== undefined ? constraint : avoidClasses(clPlayer, 20, clForest, 17, clHill, 0));
	numMultiplier = (numMultiplier !== undefined ? numMultiplier : 1.0);
	biomeID = (biomeID !== undefined ? biomeID : 0);
	
	var tM = terrainset[0]
	var tFF1 = terrainset[1]
	var tFF2 = terrainset[2]
	var tF1 = terrainset[3]
	var tF2 = terrainset[4]

	if (biomeID == 6)
	{
		var MIN_TREES = 200 * numMultiplier;
		var MAX_TREES = 1250 * numMultiplier;
		var P_FOREST = 0;
	}
	else if (biomeID == 7)
	{
		var MIN_TREES = 1000 * numMultiplier;
		var MAX_TREES = 6000 * numMultiplier;
		var P_FOREST = 0.52;
	}
	else
	{
		var MIN_TREES = 500 * numMultiplier;
		var MAX_TREES = 3000 * numMultiplier;
		var P_FOREST = 0.7;
	}
	var totalTrees = scaleByMapSize(MIN_TREES, MAX_TREES);
	var numForest = totalTrees * P_FOREST;
	g_numStragglerTrees = totalTrees * (1.0 - P_FOREST);

	// create forests
	log("Creating forests...");
	var types = [
		[[tFF2, tM, tF1], [tFF2, tF1]],
		[[tFF1, tM, tF2], [tFF1, tF2]]
	];	// some variation

	if (biomeID != 6)
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
				[painter, paintClass(tileclass)], 
				constraint,
				num
			);
		}
	}
}