Engine.LoadLibrary("rmgen");

var templateStone = "gaia/geology_stone_temperate";
var templateStoneMine = "gaia/geology_stonemine_temperate_quarry";
var templateMetalMine = "gaia/geology_metal_temperate_slabs";
var templateTemple = "other/unfinished_greek_temple";

var terrainPrimary = ["temp_grass", "temp_grass_b", "temp_grass_c", "temp_grass_d", "temp_grass_long_b", "temp_grass_clovers_2", "temp_grass_mossy", "temp_grass_plants"];
var terrainWood = ['temp_grass_mossy|gaia/flora_tree_oak', 'temp_forestfloor_pine|gaia/flora_tree_pine', 'temp_mud_plants|gaia/flora_tree_dead',
	'temp_plants_bog|gaia/flora_tree_oak_large', "temp_dirt_gravel_plants|gaia/flora_tree_aleppo_pine", 'temp_forestfloor_autumn|gaia/flora_tree_carob']; //'temp_forestfloor_autumn|gaia/flora_tree_fig'
var terrainWoodBorder = ['temp_grass_plants|gaia/flora_tree_euro_beech', 'temp_grass_mossy|gaia/flora_tree_poplar', 'temp_grass_mossy|gaia/flora_tree_poplar_lombardy',
	'temp_grass_long|gaia/flora_bush_temperate', 'temp_mud_plants|gaia/flora_bush_temperate', 'temp_mud_plants|gaia/flora_bush_badlands',
	'temp_grass_long|gaia/flora_tree_apple', 'temp_grass_clovers|gaia/flora_bush_berry', 'temp_grass_clovers_2|gaia/flora_bush_grapes',
	'temp_grass_plants|gaia/fauna_deer', "temp_grass_long_b|gaia/fauna_rabbit", "temp_grass_plants"];
var terrainBase = ["temp_dirt_gravel", "temp_grass_b"];
var terrainBaseBorder = ["temp_grass_b", "temp_grass_b", "temp_grass", "temp_grass_c", "temp_grass_mossy"];
var terrainBaseCenter = ['temp_dirt_gravel', 'temp_dirt_gravel', 'temp_grass_b'];
var terrainPath = ['temp_road', "temp_road_overgrown", 'temp_grass_b'];
var terrainHill = ["temp_highlands", "temp_highlands", "temp_highlands", "temp_dirt_gravel_b", "temp_cliff_a"];
var terrainHillBorder = ["temp_highlands", "temp_highlands", "temp_highlands", "temp_dirt_gravel_b", "temp_dirt_gravel_plants",
	"temp_highlands", "temp_highlands", "temp_highlands", "temp_dirt_gravel_b", "temp_dirt_gravel_plants",
	"temp_highlands", "temp_highlands", "temp_highlands", "temp_cliff_b", "temp_dirt_gravel_plants",
	"temp_highlands", "temp_highlands", "temp_highlands", "temp_cliff_b", "temp_dirt_gravel_plants",
	"temp_highlands|gaia/fauna_goat"];

var heightPath = -2;
var heightLand = 0;
var heightOffsetRandomPath = 1;

InitMap(heightLand, terrainPrimary);

var mapSize = getMapSize();
var mapRadius = mapSize/2;
var mapCenter = getMapCenter();

var clPlayer = createTileClass();
var clPath = createTileClass();
var clHill = createTileClass();
var clForest = createTileClass();
var clBaseResource = createTileClass();

var numPlayers = getNumPlayers();
var baseRadius = 20;
var minPlayerRadius = Math.min(mapRadius - 1.5 * baseRadius, 5/8 * mapRadius);
var maxPlayerRadius = Math.min(mapRadius - baseRadius, 3/4 * mapRadius);
var playerPosition = [];
var playerAngle = [];
var playerAngleStart = randomAngle();
var playerAngleAddAvrg = 2 * Math.PI / numPlayers;
var playerAngleMaxOff = playerAngleAddAvrg/4;

var radiusEC = Math.max(mapRadius/8, baseRadius/2);
var resourceRadius = fractionToTiles(1/3);
var resourcePerPlayer = [templateStone, templateMetalMine];

// For large maps there are memory errors with too many trees.  A density of 256*192/mapArea works with 0 players.
// Around each player there is an area without trees so with more players the max density can increase a bit.
var maxTreeDensity = Math.min(256 * (192 + 8 * numPlayers) / Math.square(mapSize), 1); // Has to be tweeked but works ok
var bushChance = 1/3; // 1 means 50% chance in deepest wood, 0.5 means 25% chance in deepest wood

var playerIDs = sortAllPlayers();
for (var i=0; i < numPlayers; i++)
{
	playerAngle[i] = (playerAngleStart + i * playerAngleAddAvrg + randFloat(0, playerAngleMaxOff)) % (2 * Math.PI);
	playerPosition[i] = Vector2D.add(mapCenter, new Vector2D(randFloat(minPlayerRadius, maxPlayerRadius), 0).rotate(-playerAngle[i]).round());
}
Engine.SetProgress(10);

placePlayerBases({
	"PlayerPlacement": [playerIDs, playerPosition],
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
	"Chicken": {
	},
	"Berries": {
		"template": "gaia/flora_bush_grapes",
		"minCount": 2,
		"maxCount": 2,
		"distance": 12,
		"minDist": 5,
		"maxDist": 8
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

log("Painting paths...");
var pathBlending = numPlayers <= 4;
for (let i = 0; i < numPlayers + (pathBlending ? 1 : 0); ++i)
	for (let j = pathBlending ? 0 : i + 1; j < numPlayers + 1; ++j)
	{
		let pathStart = i < numPlayers ? playerPosition[i] : mapCenter;
		let pathEnd = j < numPlayers ? playerPosition[j] : mapCenter;

		createArea(
			new RandomPathPlacer(pathStart, pathEnd, 1.25, baseRadius / 2, pathBlending),
			[
				new TerrainPainter(terrainPath),
				new SmoothElevationPainter(ELEVATION_SET, heightPath, 2, heightOffsetRandomPath),
				paintClass(clPath)
			],
			avoidClasses(clHill, 0, clBaseResource, 4));
	}
Engine.SetProgress(50);

log("Placing expansion resources...");
for (let i = 0; i < numPlayers; ++i)
	for (let rIndex = 0; rIndex < resourcePerPlayer.length; ++rIndex)
	{
		let angleDist = numPlayers > 1 ?
			(playerAngle[(i + 1) % numPlayers] - playerAngle[i] + 2 * Math.PI) % (2 * Math.PI) :
			2 * Math.PI;

		// they are supposed to be in between players on the same radius
		let angle = playerAngle[i] + angleDist * (rIndex + 1) / (resourcePerPlayer.length + 1);
		let position = Vector2D.add(mapCenter, new Vector2D(resourceRadius, 0).rotate(-angle)).round();

		placeObject(position.x, position.y, resourcePerPlayer[rIndex], 0, randomAngle());

		createArea(
			new ClumpPlacer(40, 1/2, 1/8, 1, position),
			[
				new LayeredPainter([terrainHillBorder, terrainHill], [1]),
				new ElevationPainter(randFloat(1, 2)),
				paintClass(clHill)
			]);
	}
Engine.SetProgress(60);

log("Placing temple...");
placeObject(mapCenter.x, mapCenter.y, templateTemple, 0, randomAngle());
addToClass(mapCenter.x, mapCenter.y, clBaseResource);

log("Creating central mountain...");
createArea(
	new ClumpPlacer(Math.square(radiusEC), 1/2, 1/8, 1, mapCenter),
	[
		new LayeredPainter([terrainHillBorder, terrainHill], [radiusEC/4]),
		new ElevationPainter(randFloat(1, 2)),
		paintClass(clHill)
	]);

// Woods and general hight map
for (var x = 0; x < mapSize; x++)
	for (var z = 0;z < mapSize;z++)
	{
		let position = new Vector2D(x, z);

		// The 0.5 is a correction for the entities placed on the center of tiles
		var radius = mapCenter.distanceTo(Vector2D.add(position, new Vector2D(0.5, 0.5)));
		var minDistToSL = mapSize;
		for (var i=0; i < numPlayers; i++)
			minDistToSL = Math.min(minDistToSL, position.distanceTo(playerPosition[i]));

		// Woods tile based
		var tDensFactSL = Math.max(Math.min((minDistToSL - baseRadius) / baseRadius, 1), 0);
		var tDensFactRad = Math.abs((resourceRadius - radius) / resourceRadius);
		var tDensFactEC = Math.max(Math.min((radius - radiusEC) / radiusEC, 1), 0);
		var tDensActual = maxTreeDensity * tDensFactSL * tDensFactRad * tDensFactEC;

		if (randBool(tDensActual) && g_Map.validT(x, z))
		{
			let border = tDensActual < randFloat(0, bushChance * maxTreeDensity);
			createArea(
				new RectPlacer(position.x, position.y, position.x + 1, position.y + 1),
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
		g_Map.setHeight(position, g_Map.getHeight(position) + hVarMiddleHill + hVarHills + 1);
	}
Engine.SetProgress(95);

placePlayersNomad(clPlayer, avoidClasses(clForest, 1, clBaseResource, 4));

ExportMap();
