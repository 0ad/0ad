Engine.LoadLibrary("rmgen");
Engine.LoadLibrary("rmgen-common");

const templateStone = "gaia/rock/temperate_small";
const templateStoneMine = "gaia/rock/temperate_large";
const templateMetalMine = "gaia/ore/temperate_large";
const templateTemple = "gaia/ruins/unfinished_greek_temple";

const terrainPrimary = ["temp_grass", "temp_grass_b", "temp_grass_c", "temp_grass_d", "temp_grass_long_b", "temp_grass_clovers_2", "temp_grass_mossy", "temp_grass_plants"];
const terrainWood = ['temp_grass_mossy|gaia/tree/oak', 'temp_forestfloor_pine|gaia/tree/pine', 'temp_mud_plants|gaia/tree/dead',
	'temp_plants_bog|gaia/tree/oak_large', "temp_dirt_gravel_plants|gaia/tree/aleppo_pine", 'temp_forestfloor_autumn|gaia/tree/carob'];
const terrainWoodBorder = ['temp_grass_plants|gaia/tree/euro_beech', 'temp_grass_mossy|gaia/tree/poplar', 'temp_grass_mossy|gaia/tree/poplar_lombardy',
	'temp_grass_long|gaia/tree/bush_temperate', 'temp_mud_plants|gaia/tree/bush_temperate', 'temp_mud_plants|gaia/tree/bush_badlands',
	'temp_grass_long|gaia/fruit/apple', 'temp_grass_clovers|gaia/fruit/berry_01', 'temp_grass_clovers_2|gaia/fruit/grapes',
	'temp_grass_plants|gaia/fauna_deer', "temp_grass_long_b|gaia/fauna_rabbit", "temp_grass_plants"];
const terrainBase = ["temp_dirt_gravel", "temp_grass_b"];
const terrainBaseBorder = ["temp_grass_b", "temp_grass_b", "temp_grass", "temp_grass_c", "temp_grass_mossy"];
const terrainBaseCenter = ['temp_dirt_gravel', 'temp_dirt_gravel', 'temp_grass_b'];
const terrainPath = ['temp_road', "temp_road_overgrown", 'temp_grass_b'];
const terrainHill = ["temp_highlands", "temp_highlands", "temp_highlands", "temp_dirt_gravel_b", "temp_cliff_a"];
const terrainHillBorder = ["temp_highlands", "temp_highlands", "temp_highlands", "temp_dirt_gravel_b", "temp_dirt_gravel_plants",
	"temp_highlands", "temp_highlands", "temp_highlands", "temp_dirt_gravel_b", "temp_dirt_gravel_plants",
	"temp_highlands", "temp_highlands", "temp_highlands", "temp_cliff_b", "temp_dirt_gravel_plants",
	"temp_highlands", "temp_highlands", "temp_highlands", "temp_cliff_b", "temp_dirt_gravel_plants",
	"temp_highlands|gaia/fauna_goat"];

const heightPath = -2;
const heightLand = 0;
const heightOffsetRandomPath = 1;

const g_Map = new RandomMap(heightLand, terrainPrimary);

const mapSize = g_Map.getSize();
const mapRadius = mapSize/2;
const mapCenter = g_Map.getCenter();

const clPlayer = g_Map.createTileClass();
const clPath = g_Map.createTileClass();
const clHill = g_Map.createTileClass();
const clForest = g_Map.createTileClass();
const clBaseResource = g_Map.createTileClass();

const numPlayers = getNumPlayers();
const baseRadius = 20;
const minPlayerRadius = Math.min(mapRadius - 1.5 * baseRadius, 5/8 * mapRadius);
const maxPlayerRadius = Math.min(mapRadius - baseRadius, 3/4 * mapRadius);
let playerPosition = [];
let playerAngle = [];
const playerAngleStart = randomAngle();
const playerAngleAddAvrg = 2 * Math.PI / numPlayers;
const playerAngleMaxOff = playerAngleAddAvrg/4;

const radiusEC = Math.max(mapRadius/8, baseRadius/2);
const resourceRadius = fractionToTiles(1/3);
const resourcePerPlayer = [templateStone, templateMetalMine];

// For large maps there are memory errors with too many trees. A density of 256x192/mapArea works with 0 players.
// Around each player there is an area without trees so with more players the max density can increase a bit.
const maxTreeDensity = Math.min(256 * (192 + 8 * numPlayers) / Math.square(mapSize), 1); // Has to be tweeked but works ok
const bushChance = 1/3; // 1 means 50% chance in deepest wood, 0.5 means 25% chance in deepest wood

const playerIDs = sortAllPlayers();
for (let i=0; i < numPlayers; i++)
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
			new TileClassPainter(clPlayer)
		]
	},
	"StartingAnimal": {
	},
	"Berries": {
		"template": "gaia/fruit/grapes",
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
		"template": "gaia/tree/oak_large",
		"count": 2
	}
});
Engine.SetProgress(30);

g_Map.log("Painting paths");
const pathBlending = numPlayers <= 4;
for (let i = 0; i < numPlayers + (pathBlending ? 1 : 0); ++i)
	for (let j = pathBlending ? 0 : i + 1; j < numPlayers + 1; ++j)
	{
		const pathStart = i < numPlayers ? playerPosition[i] : mapCenter;
		const pathEnd = j < numPlayers ? playerPosition[j] : mapCenter;

		createArea(
			new RandomPathPlacer(pathStart, pathEnd, 1.25, baseRadius / 2, pathBlending),
			[
				new TerrainPainter(terrainPath),
				new SmoothElevationPainter(ELEVATION_SET, heightPath, 2, heightOffsetRandomPath),
				new TileClassPainter(clPath)
			],
			avoidClasses(clBaseResource, 4));
	}
Engine.SetProgress(50);

g_Map.log("Placing expansion resources");
for (let i = 0; i < numPlayers; ++i)
	for (let rIndex = 0; rIndex < resourcePerPlayer.length; ++rIndex)
	{
		const angleDist = numPlayers > 1 ?
			(playerAngle[(i + 1) % numPlayers] - playerAngle[i] + 2 * Math.PI) % (2 * Math.PI) :
			2 * Math.PI;

		// they are supposed to be in between players on the same radius
		const angle = playerAngle[i] + angleDist * (rIndex + 1) / (resourcePerPlayer.length + 1);
		const position = Vector2D.add(mapCenter, new Vector2D(resourceRadius, 0).rotate(-angle)).round();

		g_Map.placeEntityPassable(resourcePerPlayer[rIndex], 0, position, randomAngle());

		createArea(
			new ClumpPlacer(40, 1/2, 1/8, Infinity, position),
			[
				new LayeredPainter([terrainHillBorder, terrainHill], [1]),
				new ElevationPainter(randFloat(1, 2)),
				new TileClassPainter(clHill)
			]);
	}
Engine.SetProgress(60);

g_Map.log("Placing temple");
g_Map.placeEntityPassable(templateTemple, 0, mapCenter, randomAngle());
clBaseResource.add(mapCenter);

g_Map.log("Creating central mountain");
createArea(
	new ClumpPlacer(Math.square(radiusEC), 1/2, 1/8, Infinity, mapCenter),
	[
		new LayeredPainter([terrainHillBorder, terrainHill], [radiusEC/4]),
		new ElevationPainter(randFloat(1, 2)),
		new TileClassPainter(clHill)
	]);

// Woods and general height map
for (let x = 0; x < mapSize; x++)
	for (let z = 0; z < mapSize; z++)
	{
		const position = new Vector2D(x, z);

		// The 0.5 is a correction for the entities placed on the center of tiles
		const radius = mapCenter.distanceTo(Vector2D.add(position, new Vector2D(0.5, 0.5)));
		let minDistToSL = mapSize;
		for (let i=0; i < numPlayers; i++)
			minDistToSL = Math.min(minDistToSL, position.distanceTo(playerPosition[i]));

		// Woods tile based
		const tDensFactSL = Math.max(Math.min((minDistToSL - baseRadius) / baseRadius, 1), 0);
		const tDensFactRad = Math.abs((resourceRadius - radius) / resourceRadius);
		const tDensFactEC = Math.max(Math.min((radius - radiusEC) / radiusEC, 1), 0);
		const tDensActual = maxTreeDensity * tDensFactSL * tDensFactRad * tDensFactEC;

		if (randBool(tDensActual) && g_Map.validTile(position))
		{
			const border = tDensActual < randFloat(0, bushChance * maxTreeDensity);
			if (avoidClasses(clPath, 1, clHill, border ? 0 : 1).allows(position))
			{
				createTerrain(border ? terrainWoodBorder : terrainWood).place(position);
				g_Map.setHeight(position, randFloat(0, 1));
				clForest.add(position);
			}
		}

		// General height map
		const hVarMiddleHill = fractionToTiles(1 / 64) * (1 + Math.cos(3/2 * Math.PI * radius / mapRadius));
		const hVarHills = 5 * (1 + Math.sin(x / 10) * Math.sin(z / 10));
		g_Map.setHeight(position, g_Map.getHeight(position) + hVarMiddleHill + hVarHills + 1);
	}
Engine.SetProgress(95);

placePlayersNomad(clPlayer, avoidClasses(clForest, 1, clBaseResource, 4, clHill, 4));

g_Map.ExportMap();
