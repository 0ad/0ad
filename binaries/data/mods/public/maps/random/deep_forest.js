RMS.LoadLibrary("rmgen");

InitMap();

var clPlayer = createTileClass();
var clPath = createTileClass();
var clHill = createTileClass();
var clForest = createTileClass();
var clBaseResource = createTileClass();

var templateStone = "gaia/geology_stone_temperate";
var templateStoneMine = "gaia/geology_stonemine_temperate_quarry";
var templateMetalMine = "gaia/geology_metal_temperate_slabs";
var startingResourcees = ["gaia/flora_tree_oak_large", "gaia/flora_bush_temperate", templateStoneMine,
	"gaia/flora_bush_grapes", "gaia/flora_tree_apple", "gaia/flora_bush_berry", templateMetalMine, "gaia/flora_bush_badlands"];

var terrainWood = ['temp_grass_mossy|gaia/flora_tree_oak', 'temp_forestfloor_pine|gaia/flora_tree_pine', 'temp_mud_plants|gaia/flora_tree_dead',
	'temp_plants_bog|gaia/flora_tree_oak_large', "temp_dirt_gravel_plants|gaia/flora_tree_aleppo_pine", 'temp_forestfloor_autumn|gaia/flora_tree_carob']; //'temp_forestfloor_autumn|gaia/flora_tree_fig'
var terrainWoodBorder = ['temp_grass_plants|gaia/flora_tree_euro_beech', 'temp_grass_mossy|gaia/flora_tree_poplar', 'temp_grass_mossy|gaia/flora_tree_poplar_lombardy',
	'temp_grass_long|gaia/flora_bush_temperate', 'temp_mud_plants|gaia/flora_bush_temperate', 'temp_mud_plants|gaia/flora_bush_badlands',
	'temp_grass_long|gaia/flora_tree_apple', 'temp_grass_clovers|gaia/flora_bush_berry', 'temp_grass_clovers_2|gaia/flora_bush_grapes',
	'temp_grass_plants|gaia/fauna_deer', "temp_grass_long_b|gaia/fauna_rabbit", "temp_grass_plants"];
var terrainBase = ['temp_dirt_gravel', 'temp_grass_b', 'temp_dirt_gravel', 'temp_grass_b', 'temp_dirt_gravel', 'temp_grass_b', 'temp_dirt_gravel', 'temp_grass_b',
	'temp_dirt_gravel', 'temp_grass_b', 'temp_dirt_gravel', 'temp_grass_b', 'temp_dirt_gravel', 'temp_grass_b', 'temp_dirt_gravel', 'temp_grass_b',
	'temp_dirt_gravel', 'temp_grass_b', 'temp_dirt_gravel', 'temp_grass_b', 'temp_dirt_gravel', 'temp_grass_b', 'temp_dirt_gravel', 'temp_grass_b',
	'temp_dirt_gravel', 'temp_grass_b', 'temp_dirt_gravel', 'temp_grass_b', 'temp_dirt_gravel', 'temp_grass_b', 'temp_dirt_gravel', 'temp_grass_b',
	'temp_grass_b|gaia/fauna_pig', 'temp_dirt_gravel|gaia/fauna_chicken'];
var terrainBaseBorder = ["temp_grass_b", "temp_grass_b", "temp_grass", "temp_grass_c", "temp_grass_mossy"];
var terrainBaseCenter = ['temp_dirt_gravel', 'temp_dirt_gravel', 'temp_grass_b'];
var terrainPath = ['temp_road', "temp_road_overgrown", 'temp_grass_b'];
var terrainHill = ["temp_highlands", "temp_highlands", "temp_highlands", "temp_dirt_gravel_b", "temp_cliff_a"];
var terrainHillBorder = ["temp_highlands", "temp_highlands", "temp_highlands", "temp_dirt_gravel_b", "temp_dirt_gravel_plants",
	"temp_highlands", "temp_highlands", "temp_highlands", "temp_dirt_gravel_b", "temp_dirt_gravel_plants",
	"temp_highlands", "temp_highlands", "temp_highlands", "temp_cliff_b", "temp_dirt_gravel_plants",
	"temp_highlands", "temp_highlands", "temp_highlands", "temp_cliff_b", "temp_dirt_gravel_plants",
	"temp_highlands|gaia/fauna_goat"];

var mapSize = getMapSize();
var mapRadius = mapSize/2;
var playableMapRadius = mapRadius - 5;
var mapCenterX = mapRadius;
var mapCenterZ = mapRadius;

var numPlayers = getNumPlayers();
var baseRadius = 20;
var minPlayerRadius = min(mapRadius-1.5*baseRadius, 5*mapRadius/8);
var maxPlayerRadius = min(mapRadius-baseRadius, 3*mapRadius/4);
var playerStartLocX = [];
var playerStartLocZ = [];
var playerAngle = [];
var playerAngleStart = randFloat(0, 2*PI);
var playerAngleAddAvrg = 2*PI / numPlayers;
var playerAngleMaxOff = playerAngleAddAvrg/4;

// Setup eyecandy
var templateEC = "other/unfinished_greek_temple";
var radiusEC = max(mapRadius/8, baseRadius/2);

// Setup paths
var pathSucsessRadius = baseRadius/2;
var pathAngleOff = PI/2;
var pathWidth = 5; // This is not really the path's sickness in tiles but the number of tiles in the clumbs of the path

// Setup additional resources
var resourceRadius = 2*mapRadius/3; // 3*mapRadius/8;
var resourcePerPlayer = [templateStone, templateMetalMine];

// Setup woods
// For large maps there are memory errors with too many trees.  A density of 256*192/mapArea works with 0 players.
// Around each player there is an area without trees so with more players the max density can increase a bit.
var maxTreeDensity = min(256 * (192 + 8 * numPlayers) / (mapSize * mapSize), 1); // Has to be tweeked but works ok
var bushChance = 1/3; // 1 means 50% chance in deepest wood, 0.5 means 25% chance in deepest wood

RMS.SetProgress(2);

// Place bases
for (var i=0; i < numPlayers; i++)
{
	playerAngle[i] = (playerAngleStart + i*playerAngleAddAvrg + randFloat(0, playerAngleMaxOff))%(2*PI);
	var x = round(mapCenterX + randFloat(minPlayerRadius, maxPlayerRadius)*cos(playerAngle[i]));
	var z = round(mapCenterZ + randFloat(minPlayerRadius, maxPlayerRadius)*sin(playerAngle[i]));
	playerStartLocX[i] = x;
	playerStartLocZ[i] = z;

	placeCivDefaultEntities(x, z, i+1);

	// Place base texture
	var placer = new ClumpPlacer(2*baseRadius*baseRadius, 2/3, 1/8, 10, x, z);
	var painter = [new LayeredPainter([terrainBaseBorder, terrainBase, terrainBaseCenter], [baseRadius/4, baseRadius/4]), paintClass(clPlayer)];
	createArea(placer, painter);

	// Place starting resources
	var distToSL = 10;
	var resStartAngle = playerAngle[i] + PI;
	var resAddAngle = 2*PI / startingResourcees.length;
	for (var rIndex = 0; rIndex < startingResourcees.length; rIndex++)
	{
		var angleOff = randFloat(-resAddAngle/2, resAddAngle/2);
		var placeX = x + distToSL*cos(resStartAngle + rIndex*resAddAngle + angleOff);
		var placeZ = z + distToSL*sin(resStartAngle + rIndex*resAddAngle + angleOff);
		placeObject(placeX, placeZ, startingResourcees[rIndex], 0, randFloat(0, 2*PI));
		addToClass(round(placeX), round(placeZ), clBaseResource);
	}
}

RMS.SetProgress(10);

// Place paths
var doublePaths = true;
if (numPlayers > 4)
	doublePaths = false;
if (doublePaths == true)
	var maxI = numPlayers+1;
else
	var maxI = numPlayers;
for (var i = 0; i < maxI; i++)
{
	if (doublePaths == true)
		var minJ = 0;
	else
		var minJ = i+1;
	for (var j = minJ; j < numPlayers+1; j++)
	{
		// Setup start and target coordinates
		if (i < numPlayers)
		{
			var x = playerStartLocX[i];
			var z = playerStartLocZ[i];
		}
		else
		{
			var x = mapCenterX;
			var z = mapCenterZ;
		}

		if (j < numPlayers)
		{
			var targetX = playerStartLocX[j];
			var targetZ = playerStartLocZ[j];
		}
		else
		{
			var targetX = mapCenterX;
			var targetZ = mapCenterZ;
		}

		// Prepare path placement
		var angle = getAngle(x, z, targetX, targetZ);
		x += round(pathSucsessRadius*cos(angle));
		z += round(pathSucsessRadius*sin(angle));
		var targetReached = false;
		var tries = 0;
		// Placing paths
		while (targetReached == false && tries < 2*mapSize)
		{
			var placer = new ClumpPlacer(pathWidth, 1, 1, 1, x, z);
			var painter = [new TerrainPainter(terrainPath), new ElevationPainter(randFloat(-1, 0)), paintClass(clPath)];
			createArea(placer, painter, avoidClasses(clHill, 0, clBaseResource, 4));
			// addToClass(x, z, clPath); // Not needed...
			// Set vars for next loop
			angle = getAngle(x, z, targetX, targetZ);
			if (doublePaths == true) // Bended paths
			{
				x += round(cos(angle + randFloat(-pathAngleOff/2, 3*pathAngleOff/2)));
				z += round(sin(angle + randFloat(-pathAngleOff/2, 3*pathAngleOff/2)));
			}
			else // Straight paths
			{
				x += round(cos(angle + randFloat(-pathAngleOff, pathAngleOff)));
				z += round(sin(angle + randFloat(-pathAngleOff, pathAngleOff)));
			}
			if (Math.euclidDistance2D(x, z, targetX, targetZ) < pathSucsessRadius)
				targetReached = true;
			tries++;

		}
	}
}

RMS.SetProgress(50);

// Place expansion resources
for (var i=0; i < numPlayers; i++)
{
	for (var rIndex = 0; rIndex < resourcePerPlayer.length; rIndex++)
	{
		if (numPlayers > 1)
			var angleDist = (playerAngle[(i+1)%numPlayers] - playerAngle[i] + 2*PI)%(2*PI);
		else
			var angleDist = 2*PI;
		var placeX = round(mapCenterX + resourceRadius*cos(playerAngle[i] + (rIndex+1)*angleDist/(resourcePerPlayer.length+1)));
		var placeZ = round(mapCenterX + resourceRadius*sin(playerAngle[i] + (rIndex+1)*angleDist/(resourcePerPlayer.length+1)));
		placeObject(placeX, placeZ, resourcePerPlayer[rIndex], 0, randFloat(0, 2*PI));
		var placer = new ClumpPlacer(40, 1/2, 1/8, 1, placeX, placeZ);
		var painter = [new LayeredPainter([terrainHillBorder, terrainHill], [1]), new ElevationPainter(randFloat(1, 2)), paintClass(clHill)];
		createArea(placer, painter);
	}
}

RMS.SetProgress(60);

// Place eyecandy
placeObject(mapCenterX, mapCenterZ, templateEC, 0, randFloat(0, 2*PI));
var placer = new ClumpPlacer(radiusEC*radiusEC, 1/2, 1/8, 1, mapCenterX, mapCenterZ);
var painter = [new LayeredPainter([terrainHillBorder, terrainHill], [radiusEC/4]), new ElevationPainter(randFloat(1, 2)), paintClass(clHill)];
createArea(placer, painter);

// Woods and general hight map
for (var x = 0; x < mapSize; x++)
{
	for (var z = 0;z < mapSize;z++)
	{
		// The 0.5 is a correction for the entities placed on the center of tiles
		var radius = Math.euclidDistance2D(x + 0.5, z + 0.5, mapCenterX, mapCenterZ);
		var minDistToSL = mapSize;
		for (var i=0; i < numPlayers; i++)
			minDistToSL = Math.min(minDistToSL, Math.euclidDistance2D(x, z, playerStartLocX[i], playerStartLocZ[i]));

		// Woods tile based
		var tDensFactSL = max(min((minDistToSL - baseRadius) / baseRadius, 1), 0);
		var tDensFactRad = abs((resourceRadius - radius) / resourceRadius);
		var tDensFactEC = max(min((radius - radiusEC) / radiusEC, 1), 0);
		var tDensActual = maxTreeDensity * tDensFactSL * tDensFactRad * tDensFactEC;

		if (randBool(tDensActual) && radius < playableMapRadius)
		{
			if (tDensActual < randFloat(0, bushChance * maxTreeDensity))
			{
				var placer = new ClumpPlacer(1, 1.0, 1.0, 1, x, z);
				var painter = [new TerrainPainter(terrainWoodBorder), new ElevationPainter(randFloat(0, 1)), paintClass(clForest)];
				createArea(placer, painter, avoidClasses(clPath, 1, clHill, 0));
			}
			else
			{
				var placer = new ClumpPlacer(1, 1.0, 1.0, 1, x, z);
				var painter = [new TerrainPainter(terrainWood), new ElevationPainter(randFloat(0, 1)), paintClass(clForest)];
				createArea(placer, painter, avoidClasses(clPath, 2, clHill, 1));
			}
		}

		// General hight map
		var hVarMiddleHill = mapSize/64 * (1+cos(3*PI/2 * radius/mapRadius));
		var hVarHills = 5*(1+sin(x/10)*sin(z/10));
		setHeight(x, z, getHeight(x, z) + hVarMiddleHill + hVarHills + 1);
	}
}
RMS.SetProgress(95);

ExportMap();
