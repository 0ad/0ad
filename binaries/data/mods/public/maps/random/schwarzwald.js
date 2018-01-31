Engine.LoadLibrary('rmgen');
Engine.LoadLibrary("heightmap");

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

var oStoneLarge = 'gaia/geology_stonemine_alpine_quarry';
var oMetalLarge = 'gaia/geology_metal_alpine_slabs';
var oFish = "gaia/fauna_fish";

var aGrass = 'actor|props/flora/grass_soft_small_tall.xml';
var aGrassShort = 'actor|props/flora/grass_soft_large.xml';
var aRockLarge = 'actor|geology/stone_granite_med.xml';
var aRockMedium = 'actor|geology/stone_granite_med.xml';
var aBushMedium = 'actor|props/flora/bush_medit_me.xml';
var aBushSmall = 'actor|props/flora/bush_medit_sm.xml';
var aReeds = 'actor|props/flora/reeds_pond_lush_b.xml';

var terrainPrimary = ["temp_grass_plants", "temp_plants_bog"];
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

const heightLand = 1;
const heightOffsetPath = -0.1;

var g_Map = new RandomMap(heightLand, terrainPrimary);

var clPlayer = g_Map.createTileClass();
var clPath = g_Map.createTileClass();
var clForest = g_Map.createTileClass();
var clWater = g_Map.createTileClass();
var clMetal = g_Map.createTileClass();
var clRock = g_Map.createTileClass();
var clFood = g_Map.createTileClass();
var clBaseResource = g_Map.createTileClass();
var clOpen = g_Map.createTileClass();

var mapSize = g_Map.getSize();
var mapCenter = g_Map.getCenter();
var mapRadius = mapSize/2;

var numPlayers = getNumPlayers();
var baseRadius = 15;
var minPlayerRadius = Math.min(mapRadius - 1.5 * baseRadius, 5/8 * mapRadius);
var maxPlayerRadius = Math.min(mapRadius - baseRadius, 3/4 * mapRadius);

var playerPosition = [];
var playerAngleStart = randomAngle();
var playerAngleAddAvrg = 2 * Math.PI / numPlayers;
var playerAngleMaxOff = playerAngleAddAvrg/4;

var resourceRadius = fractionToTiles(1/3);

// Setup woods
// For large maps there are memory errors with too many trees.  A density of 256*192/mapArea works with 0 players.
// Around each player there is an area without trees so with more players the max density can increase a bit.
var maxTreeDensity = Math.min(256 * (192 + 8 * numPlayers) / Math.square(mapSize), 1); // Has to be tweeked but works ok
var bushChance = 1/3; // 1 means 50% chance in deepest wood, 0.5 means 25% chance in deepest wood

// Set height limits and water level by map size

// Set target min and max height depending on map size to make average steepness about the same on all map sizes
var heightRange = {'min': MIN_HEIGHT * (g_Map.size + 512) / 8192, 'max': MAX_HEIGHT * (g_Map.size + 512) / 8192, 'avg': (MIN_HEIGHT * (g_Map.size + 512) +MAX_HEIGHT * (g_Map.size + 512))/16384};

// Set average water coverage
var averageWaterCoverage = 1/5; // NOTE: Since erosion is not predictable actual water coverage might vary much with the same values
var heightSeaGround = -MIN_HEIGHT + heightRange.min + averageWaterCoverage * (heightRange.max - heightRange.min);
var heightSeaGroundAdjusted = heightSeaGround + MIN_HEIGHT;
setWaterHeight(heightSeaGround);

// Setting a 3x3 Grid as initial heightmap
var initialReliefmap = [[heightRange.max, heightRange.max, heightRange.max], [heightRange.max, heightRange.min, heightRange.max], [heightRange.max, heightRange.max, heightRange.max]];

setBaseTerrainDiamondSquare(heightRange.min, heightRange.max, initialReliefmap);

for (var i = 0; i < 5; i++)
	globalSmoothHeightmap();

rescaleHeightmap(heightRange.min, heightRange.max);

var heighLimits = [
	heightRange.min + 1/3 * (heightSeaGroundAdjusted - heightRange.min), // 0 Deep water
	heightRange.min + 2/3 * (heightSeaGroundAdjusted - heightRange.min), // 1 Medium Water
	heightRange.min + (heightSeaGroundAdjusted - heightRange.min), // 2 Shallow water
	heightSeaGroundAdjusted + 1/8 * (heightRange.max - heightSeaGroundAdjusted), // 3 Shore
	heightSeaGroundAdjusted + 2/8 * (heightRange.max - heightSeaGroundAdjusted), // 4 Low ground
	heightSeaGroundAdjusted + 3/8 * (heightRange.max - heightSeaGroundAdjusted), // 5 Player and path height
	heightSeaGroundAdjusted + 4/8 * (heightRange.max - heightSeaGroundAdjusted), // 6 High ground
	heightSeaGroundAdjusted + 5/8 * (heightRange.max - heightSeaGroundAdjusted), // 7 Lower forest border
	heightSeaGroundAdjusted + 6/8 * (heightRange.max - heightSeaGroundAdjusted), // 8 Forest
	heightSeaGroundAdjusted + 7/8 * (heightRange.max - heightSeaGroundAdjusted), // 9 Upper forest border
	heightSeaGroundAdjusted + (heightRange.max - heightSeaGroundAdjusted)]; // 10 Hilltop

var playerHeight = (heighLimits[4] + heighLimits[5]) / 2;

for (let i = 0; i < numPlayers; ++i)
{
	playerPosition[i] = Vector2D.add(
		mapCenter,
		new Vector2D(randFloat(minPlayerRadius, maxPlayerRadius), 0).rotate(
			-((playerAngleStart + i * playerAngleAddAvrg + randFloat(0, playerAngleMaxOff)) % (2 * Math.PI)))).round();

	rectangularSmoothToHeight(playerPosition[i], 20, 20, playerHeight, 0.8);
}

placePlayerBases({
	"PlayerPlacement": [sortAllPlayers(), playerPosition],
	"BaseResourceClass": clBaseResource,
	"Walls": false,
	// player class painted below
	"CityPatch": {
		"radius": 0.8 * baseRadius,
		"smoothness": 1/8,
		"painters": [
			new TerrainPainter([baseTex], [baseRadius/4, baseRadius/4]),
			new TileClassPainter(clPlayer)
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
			{ "template": oMetalLarge },
			{ "template": oStoneLarge }
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

g_Map.log("Creating mines");
for (let [minHeight, maxHeight] of [[heighLimits[3], (heighLimits[4] + heighLimits[3]) / 2], [(heighLimits[5] + heighLimits[6]) / 2, heighLimits[7]]])
	for (let [template, tileClass] of [[oStoneLarge, clRock], [oMetalLarge, clMetal]])
		createObjectGroups(
			new SimpleGroup([new SimpleObject(template, 1, 1, 0, 4)], true, tileClass),
			0,
			[
				new HeightConstraint(minHeight, maxHeight),
				avoidClasses(clForest, 4, clPlayer, 20, clMetal, 40, clRock, 40)
			],
			scaleByMapSize(2, 8),
			100,
			false);

Engine.SetProgress(50);

g_Map.log("Painting textures");
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

g_Map.log("Painting paths");
var pathBlending = numPlayers <= 4;
for (let i = 0; i < numPlayers + (pathBlending ? 1 : 0); ++i)
	for (let j = pathBlending ? 0 : i + 1; j < numPlayers + 1; ++j)
	{
		let pathStart = i < numPlayers ? playerPosition[i] : mapCenter;
		let pathEnd = j < numPlayers ? playerPosition[j] : mapCenter;

		createArea(
			new RandomPathPlacer(pathStart, pathEnd, 1.75, baseRadius / 2, pathBlending),
			[
				new TerrainPainter(terrainPath),
				new SmoothElevationPainter(ELEVATION_MODIFY, heightOffsetPath, 1),
				new TileClassPainter(clPath)
			],
			avoidClasses(clPath, 0, clOpen, 0 ,clWater, 4, clBaseResource, 4));
	}
Engine.SetProgress(75);

g_Map.log("Creating decoration");
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

g_Map.log("Growing fish");
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

g_Map.log("Planting reeds");
var types = [aReeds];
for (let type of types)
	createObjectGroupsDeprecated(
		new SimpleGroup([new SimpleObject(type, 1, 1, 0, 0)], true),
		0,
		borderClasses(clWater, 0, 6),
		scaleByMapSize(1, 2) * 1000,
		1000);

Engine.SetProgress(90);

g_Map.log("Planting trees");
for (var x = 0; x < mapSize; x++)
	for (var z = 0;z < mapSize;z++)
	{
		let position = new Vector2D(x, z);

		if (!g_Map.validTile(position))
			continue;

		// The 0.5 is a correction for the entities placed on the center of tiles
		let radius = Vector2D.add(position, new Vector2D(0.5, 0.5)).distanceTo(mapCenter);
		var minDistToSL = mapSize;
		for (let i = 0; i < numPlayers; ++i)
			minDistToSL = Math.min(minDistToSL, position.distanceTo(playerPosition[i]));

		// Woods tile based
		var tDensFactSL = Math.max(Math.min((minDistToSL - baseRadius) / baseRadius, 1), 0);
		var tDensFactRad = Math.abs((resourceRadius - radius) / resourceRadius);
		var tDensActual = (maxTreeDensity * tDensFactSL * tDensFactRad)*0.75;

		if (!randBool(tDensActual))
			continue;

		let border = tDensActual < randFloat(0, bushChance * maxTreeDensity);

		let constraint = border ?
			avoidClasses(clPath, 1, clOpen, 2, clWater, 3, clMetal, 4, clRock, 4) :
			avoidClasses(clPath, 2, clOpen, 3, clWater, 4, clMetal, 4, clRock, 4);

		if (constraint.allows(position))
		{
			clForest.add(position);
			createTerrain(border ? terrainWoodBorder : terrainWood).place(position);
		}
	}

placePlayersNomad(clPlayer, avoidClasses(clWater, 4, clForest, 1, clFood, 2, clMetal, 4, clRock, 4));

Engine.SetProgress(100);

g_Map.ExportMap();
