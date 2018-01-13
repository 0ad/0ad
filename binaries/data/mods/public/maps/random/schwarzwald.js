Engine.LoadLibrary('rmgen');
Engine.LoadLibrary("heightmap");

log('Initializing map...');

InitMap();

setSkySet("fog");
setFogFactor(0.35);
setFogThickness(0.19);

setWaterColor(0.501961, 0.501961, 0.501961);
setWaterTint(0.25098, 0.501961, 0.501961);
setWaterWaviness(0.5);
setWaterType("clap");
setWaterMurkiness(0.75);

setPPSaturation(0.37);
setPPContrast(0.4);
setPPBrightness(0.4);
setPPEffect("hdr");
setPPBloom(0.4);

var clPlayer = createTileClass();
var clPath = createTileClass();
var clForest = createTileClass();
var clWater = createTileClass();
var clFood = createTileClass();
var clBaseResource = createTileClass();
var clOpen = createTileClass();

var templateStoneMine = 'gaia/geology_stonemine_alpine_quarry';
var templateMetalMine = 'gaia/geology_metal_alpine_slabs';
var aGrass = 'actor|props/flora/grass_soft_small_tall.xml';
var aGrassShort = 'actor|props/flora/grass_soft_large.xml';
var aRockLarge = 'actor|geology/stone_granite_med.xml';
var aRockMedium = 'actor|geology/stone_granite_med.xml';
var aBushMedium = 'actor|props/flora/bush_medit_me.xml';
var aBushSmall = 'actor|props/flora/bush_medit_sm.xml';
var aReeds = 'actor|props/flora/reeds_pond_lush_b.xml';
var oFish = "gaia/fauna_fish";

var terrainWood = ['alpine_forrestfloor|gaia/flora_tree_oak', 'alpine_forrestfloor|gaia/flora_tree_pine'];

var terrainWoodBorder = ['new_alpine_grass_mossy|gaia/flora_tree_oak', 'alpine_forrestfloor|gaia/flora_tree_pine',
	'temp_grass_long|gaia/flora_bush_temperate', 'temp_grass_clovers|gaia/flora_bush_berry', 'temp_grass_clovers_2|gaia/flora_bush_grapes',
	'temp_grass_plants|gaia/fauna_deer', 'temp_grass_plants|gaia/fauna_rabbit', 'new_alpine_grass_dirt_a'];

var terrainBase = ['temp_plants_bog', 'temp_grass_plants', 'temp_grass_d', 'temp_grass_plants',
	'temp_plants_bog', 'temp_grass_plants', 'temp_grass_plants',
	'temp_plants_bog', 'temp_grass_plants', 'temp_grass_plants',
	'temp_plants_bog', 'temp_grass_plants', 'temp_grass_plants',
	'temp_plants_bog', 'temp_grass_plants', 'temp_grass_plants',
	'temp_plants_bog', 'temp_grass_plants', 'temp_grass_plants',
	'temp_plants_bog', 'temp_grass_plants', 'temp_grass_d', 'temp_grass_plants',
	'temp_plants_bog', 'temp_grass_plants', 'temp_grass_d', 'temp_grass_plants',
	'temp_plants_bog', 'temp_grass_plants', 'temp_grass_d', 'temp_grass_plants',
	'temp_plants_bog', 'temp_grass_plants', 'temp_grass_d', 'temp_grass_plants',
	'temp_plants_bog', 'temp_grass_plants', 'temp_grass_d', 'temp_grass_plants',
	'temp_plants_bog', 'temp_grass_plants', 'temp_grass_plants', 'temp_grass_plants|gaia/fauna_sheep'];

var terrainBaseBorder = ['temp_plants_bog', 'temp_grass_plants', 'temp_grass_d', 'temp_grass_plants',
	'temp_plants_bog', 'temp_grass_plants', 'temp_grass_plants',
	'temp_plants_bog', 'temp_grass_plants', 'temp_grass_plants',
	'temp_plants_bog', 'temp_grass_plants', 'temp_grass_plants',
	'temp_plants_bog', 'temp_grass_plants', 'temp_grass_plants',
	'temp_plants_bog', 'temp_grass_plants', 'temp_grass_plants',
	'temp_plants_bog', 'temp_grass_plants', 'temp_grass_d', 'temp_grass_plants',
	'temp_plants_bog', 'temp_grass_plants', 'temp_grass_d', 'temp_grass_plants',
	'temp_plants_bog', 'temp_grass_plants', 'temp_grass_d', 'temp_grass_plants',
	'temp_plants_bog', 'temp_grass_plants', 'temp_grass_d', 'temp_grass_plants',
	'temp_plants_bog', 'temp_grass_plants', 'temp_grass_d', 'temp_grass_plants',
	'temp_plants_bog', 'temp_grass_plants', 'temp_grass_plants'];

var baseTex = ['temp_road', 'temp_road_overgrown'];

var terrainPath = ['temp_road', 'temp_road_overgrown'];

var tWater = ['dirt_brown_d'];
var tWaterBorder = ['dirt_brown_d'];

var mapSize = getMapSize();
var mapArea = getMapArea();
var mapRadius = mapSize/2;
var mapCenterX = mapRadius;
var mapCenterZ = mapRadius;

var numPlayers = getNumPlayers();
var baseRadius = 15;
var minPlayerRadius = Math.min(mapRadius - 1.5 * baseRadius, 5/8 * mapRadius);
var maxPlayerRadius = Math.min(mapRadius - baseRadius, 3/4 * mapRadius);

var playerStartLocX = [];
var playerStartLocZ = [];
var playerAngleStart = randFloat(0, 2 * Math.PI);
var playerAngleAddAvrg = 2 * Math.PI / numPlayers;
var playerAngleMaxOff = playerAngleAddAvrg/4;

var pathSucsessRadius = baseRadius/2;
var pathAngleOff = Math.PI/2;
var pathWidth = 10; // This is not really the path's thickness in tiles but the number of tiles in the clumbs of the path

var resourceRadius = 2/3 * mapRadius;

// Setup woods
// For large maps there are memory errors with too many trees.  A density of 256*192/mapArea works with 0 players.
// Around each player there is an area without trees so with more players the max density can increase a bit.
var maxTreeDensity = Math.min(256 * (192 + 8 * numPlayers) / mapArea, 1); // Has to be tweeked but works ok
var bushChance = 1/3; // 1 means 50% chance in deepest wood, 0.5 means 25% chance in deepest wood

////////////////
// Set height limits and water level by map size
////////////////

// Set target min and max height depending on map size to make average steepness about the same on all map sizes
var heightRange = {'min': MIN_HEIGHT * (g_Map.size + 512) / 8192, 'max': MAX_HEIGHT * (g_Map.size + 512) / 8192, 'avg': (MIN_HEIGHT * (g_Map.size + 512) +MAX_HEIGHT * (g_Map.size + 512))/16384};

// Set average water coverage
var averageWaterCoverage = 1/5; // NOTE: Since erosion is not predictable actual water coverage might vary much with the same values
var waterHeight = -MIN_HEIGHT + heightRange.min + averageWaterCoverage * (heightRange.max - heightRange.min);
var waterHeightAdjusted = waterHeight + MIN_HEIGHT;
setWaterHeight(waterHeight);

////////////////
// Generate base terrain
////////////////

// Setting a 3x3 Grid as initial heightmap
var initialReliefmap = [[heightRange.max, heightRange.max, heightRange.max], [heightRange.max, heightRange.min, heightRange.max], [heightRange.max, heightRange.max, heightRange.max]];

setBaseTerrainDiamondSquare(heightRange.min, heightRange.max, initialReliefmap);
// Apply simple erosion
for (var i = 0; i < 5; i++)
	globalSmoothHeightmap();
rescaleHeightmap(heightRange.min, heightRange.max);

//////////
// Setup height limit
//////////

// Height presets
var heighLimits = [
	heightRange.min + 1/3 * (waterHeightAdjusted - heightRange.min), // 0 Deep water
	heightRange.min + 2/3 * (waterHeightAdjusted - heightRange.min), // 1 Medium Water
	heightRange.min + (waterHeightAdjusted - heightRange.min), // 2 Shallow water
	waterHeightAdjusted + 1/8 * (heightRange.max - waterHeightAdjusted), // 3 Shore
	waterHeightAdjusted + 2/8 * (heightRange.max - waterHeightAdjusted), // 4 Low ground
	waterHeightAdjusted + 3/8 * (heightRange.max - waterHeightAdjusted), // 5 Player and path height
	waterHeightAdjusted + 4/8 * (heightRange.max - waterHeightAdjusted), // 6 High ground
	waterHeightAdjusted + 5/8 * (heightRange.max - waterHeightAdjusted), // 7 Lower forest border
	waterHeightAdjusted + 6/8 * (heightRange.max - waterHeightAdjusted), // 8 Forest
	waterHeightAdjusted + 7/8 * (heightRange.max - waterHeightAdjusted), // 9 Upper forest border
	waterHeightAdjusted + (heightRange.max - waterHeightAdjusted)]; // 10 Hilltop

//////////
// Place start locations and apply terrain texture and decorative props
//////////

// Get start locations
var startLocations = getStartLocationsByHeightmap({'min': heighLimits[4], 'max': heighLimits[5]});

var playerHeight = (heighLimits[4] + heighLimits[5]) / 2;

for (var i=0; i < numPlayers; i++)
{
	let playerAngle = (playerAngleStart + i * playerAngleAddAvrg + randFloat(0, playerAngleMaxOff)) % (2* Math.PI);
	let x = Math.round(mapCenterX + randFloat(minPlayerRadius, maxPlayerRadius) * Math.cos(playerAngle));
	let z = Math.round(mapCenterZ + randFloat(minPlayerRadius, maxPlayerRadius) * Math.sin(playerAngle));

	playerStartLocX[i] = x;
	playerStartLocZ[i] = z;

	rectangularSmoothToHeight({"x": x,"y": z} , 20, 20, playerHeight, 0.8);
}

placePlayerBases({
	"PlayerPlacement": [sortAllPlayers(), playerStartLocX.map(tilesToFraction), playerStartLocZ.map(tilesToFraction)],
	"BaseResourceClass": clBaseResource,
	"Walls": false,
	// player class painted below
	"CityPatch": {
		"radius": 0.8 * baseRadius,
		"smoothness": 1/8,
		"painters": [
			new TerrainPainter([baseTex], [baseRadius/4, baseRadius/4]),
			paintClass(clPlayer)
		]
	},
	// No chicken
	"Berries": {
		"template": "gaia/flora_bush_berry",
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
		"distance": 15,
		"minAngle": Math.PI / 2,
		"maxAngle": Math.PI
	},
	"Trees": {
		"template": "gaia/flora_tree_oak_large",
		"count": 2
	}
});

// Add further stone and metal mines
distributeEntitiesByHeight({ 'min': heighLimits[3], 'max': ((heighLimits[4] + heighLimits[3]) / 2) }, startLocations, 40, [templateStoneMine, templateMetalMine]);
distributeEntitiesByHeight({ 'min': ((heighLimits[5] + heighLimits[6]) / 2), 'max': heighLimits[7] }, startLocations, 40, [templateStoneMine, templateMetalMine]);

Engine.SetProgress(50);

//place water & open terrain textures and assign TileClasses
log("Painting textures...");

var betweenShallowAndShore = (heighLimits[3] + heighLimits[2]) / 2;
createArea(
	new HeightPlacer(Elevation_IncludeMin_IncludeMax, heighLimits[2], betweenShallowAndShore),
	new LayeredPainter([terrainBase, terrainBaseBorder], [5]));

paintTileClassBasedOnHeight(heighLimits[2], betweenShallowAndShore, 1, clOpen);

createArea(
	new HeightPlacer(Elevation_IncludeMin_IncludeMax, heightRange.min, heighLimits[2]),
	new LayeredPainter([tWaterBorder, tWater], [2]));

paintTileClassBasedOnHeight(heightRange.min,  heighLimits[2], 1, clWater);

Engine.SetProgress(60);

log("Placing paths...");

var doublePaths = true;
if (numPlayers > 4)
	doublePaths = false;

if (doublePaths === true)
	var maxI = numPlayers+1;
else
	var maxI = numPlayers;

for (var i = 0; i < maxI; i++)
{
	if (doublePaths === true)
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
		while (targetReached === false && tries < 2*mapSize)
		{
			var placer = new ClumpPlacer(pathWidth, 1, 1, 1, x, z);
			var painter = [new TerrainPainter(terrainPath), new SmoothElevationPainter(ELEVATION_MODIFY, -0.1, 1.0), paintClass(clPath)];
			createArea(placer, painter, avoidClasses(clPath, 0, clOpen, 0 ,clWater, 4, clBaseResource, 4));

			// addToClass(x, z, clPath); // Not needed...
			// Set vars for next loop
			angle = getAngle(x, z, targetX, targetZ);
			if (doublePaths === true) // Bended paths
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

Engine.SetProgress(75);

log("Creating decoration...");
createDecoration(
	[
		[new SimpleObject(aRockMedium, 1, 3, 0, 1)],
		[new SimpleObject(aRockLarge, 1, 2, 0, 1), new SimpleObject(aRockMedium, 1, 3, 0, 2)],
		[new SimpleObject(aGrassShort, 1, 2, 0, 1)],
		[new SimpleObject(aGrass, 2, 4, 0, 1.8), new SimpleObject(aGrassShort, 3, 6, 1.2, 2.5)],
		[new SimpleObject(aBushMedium, 1, 2, 0, 2), new SimpleObject(aBushSmall, 2, 4, 0, 2)]
	],
	[
		scaleByMapSize(16, 262),
		scaleByMapSize(8, 131),
		scaleByMapSize(13, 200),
		scaleByMapSize(13, 200),
		scaleByMapSize(13, 200)
	],
	avoidClasses(clForest, 1, clPlayer, 0, clPath, 3, clWater, 3));

Engine.SetProgress(80);

log("Growing fish...");
createFood(
	[
		[new SimpleObject(oFish, 2, 3, 0, 2)]
	],
	[
		100 * numPlayers
	],
	[avoidClasses(clFood, 5), stayClasses(clWater, 4)],
	clFood);

Engine.SetProgress(85);

log("Planting reeds...");
var types = [aReeds];
for (let type of types)
	createObjectGroupsDeprecated(
		new SimpleGroup([new SimpleObject(type, 1, 1, 0, 0)], true),
		0,
		borderClasses(clWater, 0, 6),
		scaleByMapSize(1, 2) * 1000,
		1000);

Engine.SetProgress(90);

log("Planting trees...");
for (var x = 0; x < mapSize; x++)
{
	for (var z = 0;z < mapSize;z++)
	{
		if (!g_Map.validT(x, z))
			continue;

		// The 0.5 is a correction for the entities placed on the center of tiles
		var radius = Math.euclidDistance2D(x + 0.5, z + 0.5, mapCenterX, mapCenterZ);
		var minDistToSL = mapSize;
		for (var i=0; i < numPlayers; i++)
			minDistToSL = Math.min(minDistToSL, Math.euclidDistance2D(playerStartLocX[i], playerStartLocZ[i], x, z));

		// Woods tile based
		var tDensFactSL = Math.max(Math.min((minDistToSL - baseRadius) / baseRadius, 1), 0);
		var tDensFactRad = Math.abs((resourceRadius - radius) / resourceRadius);
		var tDensActual = (maxTreeDensity * tDensFactSL * tDensFactRad)*0.75;

		if (!randBool(tDensActual))
			continue;

		let border = tDensActual < randFloat(0, bushChance * maxTreeDensity);
		createArea(
			new ClumpPlacer(1, 1, 1, 1, x, z),
			[
				new TerrainPainter(border ? terrainWoodBorder : terrainWood),
				paintClass(clForest)
			],
			border ?
				avoidClasses(clPath, 1, clOpen, 2, clWater, 3) :
				avoidClasses(clPath, 2, clOpen, 3, clWater, 4));
	}
}

Engine.SetProgress(100);

ExportMap();
