Engine.LoadLibrary("rmgen");

InitMap();

var clPlayer = createTileClass();
var clPath = createTileClass();
var clHill = createTileClass();
var clForest = createTileClass();
var clBaseResource = createTileClass();

var templateStone = "gaia/geology_stone_temperate";
var templateStoneMine = "gaia/geology_stonemine_temperate_quarry";
var templateMetalMine = "gaia/geology_metal_temperate_slabs";

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
var mapArea = getMapArea();
var mapRadius = mapSize/2;
var mapCenterX = mapRadius;
var mapCenterZ = mapRadius;

var numPlayers = getNumPlayers();
var baseRadius = 20;
var minPlayerRadius = Math.min(mapRadius - 1.5 * baseRadius, 5/8 * mapRadius);
var maxPlayerRadius = Math.min(mapRadius - baseRadius, 3/4 * mapRadius);
var playerStartLocX = [];
var playerStartLocZ = [];
var playerAngle = [];
var playerAngleStart = randomAngle();
var playerAngleAddAvrg = 2 * Math.PI / numPlayers;
var playerAngleMaxOff = playerAngleAddAvrg/4;

// Setup eyecandy
var templateEC = "other/unfinished_greek_temple";
var radiusEC = Math.max(mapRadius/8, baseRadius/2);

// Setup paths
var pathSucsessRadius = baseRadius/2;
var pathAngleOff = Math.PI / 2;
var pathWidth = 5; // This is not really the path's sickness in tiles but the number of tiles in the clumbs of the path

// Setup additional resources
var resourceRadius = 2*mapRadius/3; // 3*mapRadius/8;
var resourcePerPlayer = [templateStone, templateMetalMine];

// Setup woods
// For large maps there are memory errors with too many trees.  A density of 256*192/mapArea works with 0 players.
// Around each player there is an area without trees so with more players the max density can increase a bit.
var maxTreeDensity = Math.min(256 * (192 + 8 * numPlayers) / mapArea, 1); // Has to be tweeked but works ok
var bushChance = 1/3; // 1 means 50% chance in deepest wood, 0.5 means 25% chance in deepest wood

var playerIDs = [];
for (var i=0; i < numPlayers; i++)
{
	playerIDs[i] = i+1;
	playerAngle[i] = (playerAngleStart + i * playerAngleAddAvrg + randFloat(0, playerAngleMaxOff)) % (2 * Math.PI);
	playerStartLocX[i] = mapCenterX + Math.round(randFloat(minPlayerRadius, maxPlayerRadius) * Math.cos(playerAngle[i]));
	playerStartLocZ[i] = mapCenterZ + Math.round(randFloat(minPlayerRadius, maxPlayerRadius) * Math.sin(playerAngle[i]));
}
Engine.SetProgress(10);

placePlayerBases({
	"PlayerPlacement": [playerIDs, playerStartLocX.map(tilesToFraction), playerStartLocZ.map(tilesToFraction)],
	"BaseResourceClass": clBaseResource,
	// player class painted below
	"CityPatch": {
		"radius": 0.8 * baseRadius,
		"smoothness": 1/8,
		"painters": [
			new LayeredPainter([terrainBaseBorder, terrainBase, terrainBaseCenter], [baseRadius/4, baseRadius/4]),
			paintClass(clPlayer)
		]
	},
	// Chicken already placed at the base terrain
	"Berries": {
		"template": "gaia/flora_bush_grapes",
		"minCount": 2,
		"maxCount": 2,
		"minDist": 10,
		"maxDist": 10
	},
	"Mines": {
		"types": [
			{ "template": templateMetalMine },
			{ "template": templateStoneMine }
		],
		"minAngle": Math.PI / 2,
		"maxAngle": Math.PI
	},
	"Trees": {
		"template": "gaia/flora_tree_oak_large",
		"count": 2
	}
});
Engine.SetProgress(10);

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
		x += Math.round(pathSucsessRadius * Math.cos(angle));
		z += Math.round(pathSucsessRadius * Math.sin(angle));
		var targetReached = false;
		var tries = 0;
		// Placing paths
		while (targetReached == false && tries < 2*mapSize)
		{
			createArea(
				new ClumpPlacer(pathWidth, 1, 1, 1, x, z),
				[
					new TerrainPainter(terrainPath),
					new ElevationPainter(randFloat(-1, 0)),
					paintClass(clPath)
				],
				avoidClasses(clHill, 0, clBaseResource, 4));

			// Set vars for next loop
			angle = getAngle(x, z, targetX, targetZ);
			if (doublePaths == true) // Bended paths
			{
				x += Math.round(Math.cos(angle + randFloat(-pathAngleOff/2, 3*pathAngleOff/2)));
				z += Math.round(Math.sin(angle + randFloat(-pathAngleOff/2, 3*pathAngleOff/2)));
			}
			else // Straight paths
			{
				x += Math.round(Math.cos(angle + randFloat(-pathAngleOff, pathAngleOff)));
				z += Math.round(Math.sin(angle + randFloat(-pathAngleOff, pathAngleOff)));
			}
			if (Math.euclidDistance2D(x, z, targetX, targetZ) < pathSucsessRadius)
				targetReached = true;
			tries++;

		}
	}
}

Engine.SetProgress(50);

// Place expansion resources
for (var i=0; i < numPlayers; i++)
{
	for (var rIndex = 0; rIndex < resourcePerPlayer.length; rIndex++)
	{
		if (numPlayers > 1)
			var angleDist = (playerAngle[(i+1)%numPlayers] - playerAngle[i] + 2 * Math.PI) % (2 * Math.PI);
		else
			var angleDist = 2 * Math.PI;

		var placeX = Math.round(mapCenterX + resourceRadius * Math.cos(playerAngle[i] + (rIndex+1)*angleDist/(resourcePerPlayer.length+1)));
		var placeZ = Math.round(mapCenterX + resourceRadius * Math.sin(playerAngle[i] + (rIndex+1)*angleDist/(resourcePerPlayer.length+1)));

		placeObject(placeX, placeZ, resourcePerPlayer[rIndex], 0, randomAngle());

		createArea(
			new ClumpPlacer(40, 1/2, 1/8, 1, placeX, placeZ),
			[
				new LayeredPainter([terrainHillBorder, terrainHill], [1]),
				new ElevationPainter(randFloat(1, 2)),
				paintClass(clHill)
			]);
	}
}

Engine.SetProgress(60);

// Place eyecandy
placeObject(mapCenterX, mapCenterZ, templateEC, 0, randomAngle());
addToClass(mapCenterX, mapCenterZ, clBaseResource);
createArea(
	new ClumpPlacer(Math.square(radiusEC), 1/2, 1/8, 1, mapCenterX, mapCenterZ),
	[
		new LayeredPainter([terrainHillBorder, terrainHill], [radiusEC/4]),
		new ElevationPainter(randFloat(1, 2)),
		paintClass(clHill)
	]);

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
		var tDensFactSL = Math.max(Math.min((minDistToSL - baseRadius) / baseRadius, 1), 0);
		var tDensFactRad = Math.abs((resourceRadius - radius) / resourceRadius);
		var tDensFactEC = Math.max(Math.min((radius - radiusEC) / radiusEC, 1), 0);
		var tDensActual = maxTreeDensity * tDensFactSL * tDensFactRad * tDensFactEC;

		if (randBool(tDensActual) && g_Map.validT(x, z))
		{
			let border = tDensActual < randFloat(0, bushChance * maxTreeDensity);
			createArea(
				new ClumpPlacer(1, 1, 1, 1, x, z),
				[
					new TerrainPainter(border ? terrainWoodBorder : terrainWood),
					new ElevationPainter(randFloat(0, 1)),
					paintClass(clForest)
				],
				avoidClasses(clPath, 1, clHill, border ? 0 : 1));
		}

		// General hight map
		var hVarMiddleHill = mapSize / 64 * (1 + Math.cos(3/2 * Math.PI * radius / mapRadius));
		var hVarHills = 5 * (1 + Math.sin(x / 10) * Math.sin(z / 10));
		setHeight(x, z, getHeight(x, z) + hVarMiddleHill + hVarHills + 1);
	}
}
Engine.SetProgress(95);

placePlayersNomad(clPlayer, avoidClasses(clForest, 1, clBaseResource, 4));

ExportMap();
