Engine.LoadLibrary("rmgen");

const tGrass1 = "savanna_grass_a";
const tDirt1 = "savanna_dirt_a";
const tDirt4 = "savanna_dirt_b";
const tCityTiles = "savanna_tile_a_dirt_red";
const tShore = "savanna_riparian_bank";
const tWater = "savanna_riparian_wet";

const oTree = "gaia/flora_tree_palm_tropic";
const oBerryBush = "gaia/flora_bush_berry";
const oRabbit = "gaia/fauna_rabbit";
const oTiger = "gaia/fauna_tiger";
const oCrocodile = "gaia/fauna_crocodile";
const oFish = "gaia/fauna_fish";
const oElephant = "gaia/fauna_elephant_asian";
const oElephantInfant = "gaia/fauna_elephant_asian_infant";
const oBoar = "gaia/fauna_boar";
const oStoneSmall = "gaia/geology_stone_savanna_small";
const oMetalLarge = "gaia/geology_metal_savanna_slabs";

const aBush = "actor|props/flora/bush_medit_sm_dry.xml";
const aRock = "actor|geology/stone_savanna_med.xml";

const heightSeaGround = -3;
const heightLand = 1;
const heightShore = 3;
const heightOffsetBump = 2;

InitMap(heightLand, tGrass1);

var numPlayers = getNumPlayers();
var mapSize = getMapSize();
var mapCenter = getMapCenter();

var clPlayer = createTileClass();
var clForest = createTileClass();
var clWater = createTileClass();
var clRock = createTileClass();
var clMetal = createTileClass();
var clFood = createTileClass();
var clBaseResource = createTileClass();

placePlayerBases({
	"PlayerPlacement": playerPlacementCircle(fractionToTiles(0.35)),
	"PlayerTileClass": clPlayer,
	"BaseResourceClass": clBaseResource,
	// No city patch
	"Chicken": {
	},
	"Berries": {
		"template": oBerryBush
	},
	"Mines": {
		"types": [
			{
				"template": oMetalLarge
			},
			{
				"type": "stone_formation",
				"template": oStoneSmall,
				"terrain": tDirt1
			}
		]
	},
	"Trees": {
		"template": oTree,
		"count": scaleByMapSize(3, 7),
		"minDist": 13,
		"maxDist": 15,
		"minDistGroup": 4,
		"maxDistGroup": 6
	}
	// No decoratives
});
Engine.SetProgress(20);

log("Creating bumps...");
createAreas(
	new ClumpPlacer(scaleByMapSize(20, 50), 0.5, 0.08, 1),
	new SmoothElevationPainter(ELEVATION_MODIFY, heightOffsetBump, 2),
	avoidClasses(clPlayer, 13),
	scaleByMapSize(300, 800)
);

log("Creating the half dried-up lake...");
createArea(
	new ChainPlacer(
		2,
		Math.floor(scaleByMapSize(2, 16)),
		Math.floor(scaleByMapSize(35, 200)),
		1,
		mapCenter,
		0,
		[Math.floor(scaleByMapSize(15, 40))]),
	[
		new SmoothElevationPainter(ELEVATION_SET, heightSeaGround, 4),
		paintClass(clWater)
	],
	avoidClasses(clPlayer, 2));

log("Creating more shore jaggedness...");
createAreas(
	new ChainPlacer(2, Math.floor(scaleByMapSize(4, 6)), 3, 1),
	[
		new SmoothElevationPainter(ELEVATION_SET, heightShore, 4),
		unPaintClass(clWater)
	],
	borderClasses(clWater, 4, 7),
	scaleByMapSize(12, 130) * 2, 150
);

paintTerrainBasedOnHeight(2.4, 3.4, 3, tGrass1);
paintTerrainBasedOnHeight(1, 2.4, 0, tShore);
paintTerrainBasedOnHeight(-8, 1, 2, tWater);
paintTileClassBasedOnHeight(-6, 0, 1, clWater);
Engine.SetProgress(55);

log("Creating stone mines...");
for (let i = 0; i < scaleByMapSize(12, 30); ++i)
{
	let position = new Vector2D(randIntInclusive(1, mapSize - 1), randIntInclusive(1, mapSize - 1));
	if (avoidClasses(clPlayer, 30, clRock, 25, clWater, 10).allows(position.x, position.y))
	{
		createStoneMineFormation(position, oStoneSmall, tDirt4);
		addToClass(position.x, position.y, clRock);
	}
}

log("Creating metal mines...");
createObjectGroupsDeprecated(
	new SimpleGroup([new SimpleObject(oMetalLarge, 1, 1, 0, 4)], true, clMetal),
	0,
	avoidClasses(clPlayer, 20, clMetal, 10, clRock, 8, clWater, 4),
	scaleByMapSize(2, 12), 100
);
Engine.SetProgress(65);

log("Creating small decorative rocks...");
createObjectGroupsDeprecated(
	new SimpleGroup(
		[new SimpleObject(aRock, 1, 3, 0, 3)],
		true
	),
	0,
	avoidClasses(clPlayer, 7, clWater, 1),
	scaleByMapSize(200, 1200), 1
);
Engine.SetProgress(70);

log("Creating boar...");
createObjectGroupsDeprecated(
	new SimpleGroup(
		[new SimpleObject(oBoar, 1, 2, 0, 4)],
		true, clFood
	),
	0,
	avoidClasses(clWater, 1, clPlayer, 20, clFood, 11),
	scaleByMapSize(4, 12), 50
);

log("Creating tigers...");
createObjectGroupsDeprecated(
	new SimpleGroup(
		[new SimpleObject(oTiger, 2, 2, 0, 4)],
		true, clFood
	),
	0,
	avoidClasses(clWater, 1, clPlayer, 20, clFood, 11),
	scaleByMapSize(4, 12), 50
);

log("Creating crocodiles...");
createObjectGroupsDeprecated(
	new SimpleGroup(
		[new SimpleObject(oCrocodile, 2, 4, 0, 4)],
		true, clFood
	), 0,
	stayClasses(clWater, 1),
	scaleByMapSize(4, 12), 50
);

log("Creating elephants...");
createObjectGroupsDeprecated(
	new SimpleGroup(
		[
			new SimpleObject(oElephant, 2, 4, 0, 4),
			new SimpleObject(oElephantInfant, 1, 2, 0, 4)
		],
		true, clFood
	),
	0,
	avoidClasses(clWater, 1, clPlayer, 20, clFood, 11),
	scaleByMapSize(4, 12), 50
);

log("Creating rabbits...");
createObjectGroupsDeprecated(
	new SimpleGroup(
		[new SimpleObject(oRabbit, 5, 6, 0, 4)],
		true, clFood
	),
	0,
	avoidClasses(clWater, 1, clPlayer, 20, clFood, 11),
	scaleByMapSize(4, 12), 50
);

createFood(
	[
		[new SimpleObject(oFish, 2, 3, 0, 2)]
	],
	[
		25 * numPlayers
	],
	[avoidClasses(clFood, 20), stayClasses(clWater, 2)],
	clFood);

log("Creating berry bush...");
createObjectGroupsDeprecated(
	new SimpleGroup(
		[new SimpleObject(oBerryBush, 5, 7, 0, 4)],
		true, clFood
	),
	0,
	avoidClasses(clWater, 3, clPlayer, 20, clFood, 12, clRock, 7, clMetal, 2),
	randIntInclusive(1, 4) * numPlayers + 2, 50
);
Engine.SetProgress(85);

log("Creating trees...");
createObjectGroupsDeprecated(
	new SimpleGroup(
		[new SimpleObject(oTree, 1, 7, 0, 3)],
		true, clForest
	),
	0,
	avoidClasses(clForest, 1, clPlayer, 20, clMetal, 1, clRock, 7, clWater, 1),
	scaleByMapSize(70, 500)
);

log("Creating large grass tufts...");
createObjectGroupsDeprecated(
	new SimpleGroup(
		[new SimpleObject(aBush, 2, 4, 0, 1.8, -Math.PI/8, Math.PI/8)]
	),
	0,
	avoidClasses(clWater, 3, clPlayer, 2, clForest, 0),
	scaleByMapSize(100, 1200)
);

placePlayersNomad(clPlayer, avoidClasses(clWater, 4, clForest, 1, clMetal, 4, clRock, 4, clFood, 2));

setSunColor(0.87451, 0.847059, 0.647059);
setWaterColor(0.741176, 0.592157, 0.27451);
setWaterTint(0.741176, 0.592157, 0.27451);
setWaterWaviness(2.0);
setWaterType("clap");
setWaterMurkiness(0.835938);

setUnitsAmbientColor(0.57, 0.58, 0.55);
setTerrainAmbientColor(0.447059, 0.509804, 0.54902);

setFogFactor(0.25);
setFogThickness(0.15);
setFogColor(0.847059, 0.737255, 0.482353);

setPPEffect("hdr");
setPPContrast(0.57031);
setPPBloom(0.34);

ExportMap();
