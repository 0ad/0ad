RMS.LoadLibrary('rmgen');
RMS.LoadLibrary("heightmap");

log('Initializing map...');

InitMap();


////////////////
//
//  Initializing
//
////////////////

//sky
setSkySet("fog");
setFogFactor(0.35);
setFogThickness(0.19);

// water
setWaterColor(0.501961, 0.501961, 0.501961);
setWaterTint(0.25098, 0.501961, 0.501961);
setWaterWaviness(0.5);
setWaterType("clap");
setWaterMurkiness(0.75);

// post processing
setPPSaturation(0.37);
setPPContrast(0.4);
setPPBrightness(0.4);
setPPEffect("hdr");
setPPBloom(0.4);

// Setup tile classes
var clPlayer = createTileClass();
var clPath = createTileClass();
var clHill = createTileClass();
var clForest = createTileClass();
var clWater = createTileClass();
var clRock = createTileClass();
var clFood = createTileClass();
var clBaseResource = createTileClass();
var clOpen = createTileClass();

// Setup Templates
var templateStone = 'gaia/geology_stone_alpine_a';
var templateStoneMine = 'gaia/geology_stonemine_alpine_quarry';
var templateMetal = 'actor|geology/stone_granite_med.xml';
var templateMetalMine = 'gaia/geology_metal_alpine_slabs';
var startingResources = ['gaia/flora_tree_pine', 'gaia/flora_tree_pine','gaia/flora_tree_pine', templateStoneMine,
	'gaia/flora_bush_grapes', 'gaia/flora_tree_aleppo_pine','gaia/flora_tree_aleppo_pine','gaia/flora_tree_aleppo_pine', 'gaia/flora_bush_berry', templateMetalMine];
var aGrass = 'actor|props/flora/grass_soft_small_tall.xml';
var aGrassShort = 'actor|props/flora/grass_soft_large.xml';
var aRockLarge = 'actor|geology/stone_granite_med.xml';
var aRockMedium = 'actor|geology/stone_granite_med.xml';
var aBushMedium = 'actor|props/flora/bush_medit_me.xml';
var aBushSmall = 'actor|props/flora/bush_medit_sm.xml';
var aReeds = 'actor|props/flora/reeds_pond_lush_b.xml';
var oFish = "gaia/fauna_fish";


// Setup terrain
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

var terrainBaseCenter = ['temp_dirt_gravel_b'];

var baseTex = ['temp_road', 'temp_road_overgrown'];

var terrainPath = ['temp_road', 'temp_road_overgrown'];

var terrainHill = ['temp_highlands', 'temp_highlands', 'temp_highlands', 'temp_grass_plants_b', 'temp_cliff_a'];

var terrainHillBorder = ['temp_highlands', 'temp_highlands', 'temp_highlands', 'temp_grass_plants_b', 'temp_grass_plants_plants',
	'temp_highlands', 'temp_highlands', 'temp_highlands', 'temp_grass_plants_b', 'temp_grass_plants_plants',
	'temp_highlands', 'temp_highlands', 'temp_highlands', 'temp_cliff_b', 'temp_grass_plants_plants',
	'temp_highlands', 'temp_highlands', 'temp_highlands', 'temp_cliff_b', 'temp_grass_plants_plants',
	'temp_highlands|gaia/fauna_goat'];

var tWater = ['dirt_brown_d'];
var tWaterBorder = ['dirt_brown_d'];

// Setup map
var mapSize = getMapSize();
var mapRadius = mapSize/2;
var playableMapRadius = mapRadius - 5;
var mapCenterX = mapRadius;
var mapCenterZ = mapRadius;

// Setup players and bases
var numPlayers = getNumPlayers();
var baseRadius = 15;
var minPlayerRadius = min(mapRadius-1.5*baseRadius, 5*mapRadius/8);
var maxPlayerRadius = min(mapRadius-baseRadius, 3*mapRadius/4);

var playerStartLocX = new Array(numPlayers);
var playerStartLocZ = new Array(numPlayers);
var playerAngle = new Array(numPlayers);
var playerAngleStart = randFloat(0, 2*PI);
var playerAngleAddAvrg = 2*PI / numPlayers;
var playerAngleMaxOff = playerAngleAddAvrg/4;

// Setup paths
var pathSucsessRadius = baseRadius/2;
var pathAngleOff = PI/2;
var pathWidth = 10; // This is not really the path's thickness in tiles but the number of tiles in the clumbs of the path

// Setup additional resources
var resourceRadius = 2*mapRadius/3; // 3*mapRadius/8;
//var resourcePerPlayer = [templateStone, templateMetalMine];

// Setup woods
// For large maps there are memory errors with too many trees.  A density of 256*192/mapArea works with 0 players.
// Around each player there is an area without trees so with more players the max density can increase a bit.
var maxTreeDensity = min(256 * (192 + 8 * numPlayers) / (mapSize * mapSize), 1); // Has to be tweeked but works ok
var bushChance = 1/3; // 1 means 50% chance in deepest wood, 0.5 means 25% chance in deepest wood


////////////////
//
//  Some general functions
//
////////////////

function HeightPlacer(lowerBound, upperBound) {
    this.lowerBound = lowerBound;
    this.upperBound = upperBound;
}

HeightPlacer.prototype.place = function (constraint) {
	constraint = (constraint || new NullConstraint());

    var ret = [];
    for (var x = 0; x < g_Map.size; x++) {
        for (var y = 0; y < g_Map.size; y++) {
            if (g_Map.height[x][y] >= this.lowerBound && g_Map.height[x][y] <= this.upperBound && constraint.allows(x, y)) {
                ret.push(new PointXZ(x, y));
            }
        }
    }
    return ret;
};


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

RMS.SetProgress(50);

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
	playerAngle[i] = (playerAngleStart + i*playerAngleAddAvrg + randFloat(0, playerAngleMaxOff))%(2*PI);

	var x = round(mapCenterX + randFloat(minPlayerRadius, maxPlayerRadius)*cos(playerAngle[i]));
	var z = round(mapCenterZ + randFloat(minPlayerRadius, maxPlayerRadius)*sin(playerAngle[i]));

	playerStartLocX[i] = x;
	playerStartLocZ[i] = z;

	// Place starting entities

	rectangularSmoothToHeight({"x": x,"y": z} , 20, 20, playerHeight, 0.8);

	placeCivDefaultEntities(x, z, i+1, { 'iberWall': false });

	// Place base texture
	var placer = new ClumpPlacer(2*baseRadius*baseRadius, 2/3, 1/8, 10, x, z);
	var painter = [new TerrainPainter([baseTex], [baseRadius/4, baseRadius/4]), paintClass(clPlayer)];
	createArea(placer, painter);

	// Place starting resources
	var distToSL = 15;
	var resStartAngle = playerAngle[i] + PI;
	var resAddAngle = 2*PI / startingResources.length;
	for (var rIndex = 0; rIndex < startingResources.length; rIndex++)
	{
		var angleOff = randFloat(-resAddAngle/2, resAddAngle/2);
		var placeX = x + distToSL*cos(resStartAngle + rIndex*resAddAngle + angleOff);
		var placeZ = z + distToSL*sin(resStartAngle + rIndex*resAddAngle + angleOff);
		placeObject(placeX, placeZ, startingResources[rIndex], 0, randFloat(0, 2*PI));
		addToClass(round(placeX), round(placeZ), clBaseResource);
	}
}

// Add further stone and metal mines
distributeEntitiesByHeight({ 'min': heighLimits[3], 'max': ((heighLimits[4] + heighLimits[3]) / 2) }, startLocations, 40);
distributeEntitiesByHeight({ 'min': ((heighLimits[5] + heighLimits[6]) / 2), 'max': heighLimits[7] }, startLocations, 40);

RMS.SetProgress(50);

//place water & open terrain textures and assign TileClasses
log("Painting textures...");
var placer = new HeightPlacer(heighLimits[2], (heighLimits[3]+heighLimits[2])/2);
var painter = new LayeredPainter([terrainBase, terrainBaseBorder], [5]);
createArea(placer, painter);
paintTileClassBasedOnHeight(heighLimits[2], (heighLimits[3]+heighLimits[2])/2, 1, clOpen);

var placer = new HeightPlacer(heightRange.min, heighLimits[2]);
var painter = new LayeredPainter([tWaterBorder, tWater], [2]);
createArea(placer, painter);
paintTileClassBasedOnHeight(heightRange.min,  heighLimits[2], 1, clWater);

RMS.SetProgress(60);

// Place paths
log("Placing paths...");

var doublePaths = true;
if (numPlayers > 4)
	doublePaths = false;

var doublePathMayPlayers = 4;
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
		x += round(pathSucsessRadius*cos(angle));
		z += round(pathSucsessRadius*sin(angle));

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
				x += round(cos(angle + randFloat(-pathAngleOff/2, 3*pathAngleOff/2)));
				z += round(sin(angle + randFloat(-pathAngleOff/2, 3*pathAngleOff/2)));
			}
			else // Straight paths
			{
				x += round(cos(angle + randFloat(-pathAngleOff, pathAngleOff)));
				z += round(sin(angle + randFloat(-pathAngleOff, pathAngleOff)));
			}

			if (getDistance(x, z, targetX, targetZ) < pathSucsessRadius)
				targetReached = true;

			tries++;
		}
	}
}

RMS.SetProgress(75);

//create general decoration
log("Creating decoration...");
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
  scaleByMapSize(13, 200),
  scaleByMapSize(13, 200),
  scaleByMapSize(13, 200)
 ],
 avoidClasses(clForest, 1, clPlayer, 0, clPath, 3, clWater, 3)
);

RMS.SetProgress(80);

//create fish
log("Growing fish...");
createFood
(
 [
  [new SimpleObject(oFish, 2,3, 0,2)]
 ],
 [
  100 * numPlayers
 ],
 [avoidClasses(clFood, 5), stayClasses(clWater, 4)]
);

RMS.SetProgress(85);

// create reeds
log("Planting reeds...");
var types = [aReeds];	// some variation
for (var i = 0; i < types.length; ++i)
{
	var group = new SimpleGroup([new SimpleObject(types[i], 1,1, 0,0)], true);
	createObjectGroups(group, 0,
		borderClasses(clWater, 0, 6),
		scaleByMapSize(960, 2000), 1000
	);
}

RMS.SetProgress(90);

// place trees
log("Planting trees...");
for (var x = 0; x < mapSize; x++)
{
	for (var z = 0;z < mapSize;z++)
	{
		var radius = Math.pow(Math.pow(mapCenterX - x - 0.5, 2) + Math.pow(mapCenterZ - z - 0.5, 2), 1/2); // The 0.5 is a correction for the entities placed on the center of tiles
		var minDistToSL = mapSize;
		for (var i=0; i < numPlayers; i++)
			minDistToSL = min(minDistToSL, getDistance(playerStartLocX[i], playerStartLocZ[i], x, z));

		// Woods tile based
		var tDensFactSL = max(min((minDistToSL - baseRadius) / baseRadius, 1), 0);
		var tDensFactRad = abs((resourceRadius - radius) / resourceRadius);
		var tDensActual = (maxTreeDensity * tDensFactSL * tDensFactRad)*0.75;

		if (randFloat() < tDensActual && radius < playableMapRadius)
		{
			if (tDensActual < bushChance*randFloat()*maxTreeDensity)
			{
				var placer = new ClumpPlacer(1, 1.0, 1.0, 1, x, z);
				var painter = [new TerrainPainter(terrainWoodBorder), paintClass(clForest)];
				createArea(placer, painter, avoidClasses(clPath, 1, clOpen, 2, clWater,3));
			}
			else
			{
				var placer = new ClumpPlacer(1, 1.0, 1.0, 1, x, z);
				var painter = [new TerrainPainter(terrainWood), paintClass(clForest)];
				createArea(placer, painter, avoidClasses(clPath, 2, clOpen, 3, clWater, 4));}
		}
	}
}

RMS.SetProgress(100);

ExportMap();
