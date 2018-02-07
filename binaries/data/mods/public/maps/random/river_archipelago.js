Engine.LoadLibrary("rmgen");

const tGrass = ["tropic_grass_c", "tropic_grass_c", "tropic_grass_c", "tropic_grass_c", "tropic_grass_plants", "tropic_plants", "tropic_plants_b"];
const tGrassA = "tropic_plants_c";
const tGrassB = "tropic_plants_c";
const tGrassC = "tropic_grass_c";
const tForestFloor = "tropic_grass_plants";
const tCliff = ["tropic_cliff_a", "tropic_cliff_a", "tropic_cliff_a", "tropic_cliff_a_plants"];
const tPlants = "tropic_plants";
const tRoad = "tropic_citytile_a";
const tRoadWild = "tropic_citytile_plants";
const tShoreBlend = "tropic_beach_dry_plants";
const tShore = "tropic_beach_dry";
const tWater = "tropic_beach_wet";

const oTree = "gaia/flora_tree_toona";
const oPalm1 = "gaia/flora_tree_palm_tropic";
const oPalm2 = "gaia/flora_tree_palm_tropical";
const oStoneLarge = "gaia/geology_stonemine_tropic_quarry";
const oStoneSmall = "gaia/geology_stone_tropic_a";
const oMetalLarge = "gaia/geology_metal_tropic_slabs";
const oFish = "gaia/fauna_fish";
const oDeer = "gaia/fauna_deer";
const oTiger = "gaia/fauna_tiger";
const oBoar = "gaia/fauna_boar";
const oPeacock = "gaia/fauna_peacock";
const oBush = "gaia/flora_bush_berry";
const oSpearman = "units/maur_infantry_spearman_b";
const oArcher = "units/maur_infantry_archer_b";
const oArcherElephant = "units/maur_elephant_archer_b";

const aRockLarge = "actor|geology/stone_granite_large.xml";
const aRockMedium = "actor|geology/stone_granite_med.xml";
const aBush1 = "actor|props/flora/plant_tropic_a.xml";
const aBush2 = "actor|props/flora/plant_lg.xml";
const aBush3 = "actor|props/flora/plant_tropic_large.xml";

const pForestD = [tForestFloor + TERRAIN_SEPARATOR + oTree, tForestFloor];
const pForestP1 = [tForestFloor + TERRAIN_SEPARATOR + oPalm1, tForestFloor];
const pForestP2 = [tForestFloor + TERRAIN_SEPARATOR + oPalm2, tForestFloor];

const heightSeaGround = -8;
const heightLand = 3;
const heightHill = 25;

var g_Map = new RandomMap(heightSeaGround, tGrass);

const numPlayers = getNumPlayers();
const mapCenter = g_Map.getCenter();
const mapBounds = g_Map.getBounds();

var clPlayer = g_Map.createTileClass();
var clPlayerTerritory = g_Map.createTileClass();
var clHill = g_Map.createTileClass();
var clForest = g_Map.createTileClass();
var clWater = g_Map.createTileClass();
var clDirt = g_Map.createTileClass();
var clRock = g_Map.createTileClass();
var clMetal = g_Map.createTileClass();
var clFood = g_Map.createTileClass();
var clBaseResource = g_Map.createTileClass();
var clGaia = g_Map.createTileClass();
var clStrip = [];

var startAngle = randomAngle();
var connectPlayers = randBool();

// Map layout
var stripWidthsLeft = connectPlayers ?
	[[0.03, 0.09], [0.14, 0.25], [0.36, 0.46]] : 
	[[0, 0.06], [0.12, 0.23], [0.33, 0.43]];

// Mirror
var stripWidthsRight = clone(stripWidthsLeft);
stripWidthsRight.reverse();
stripWidthsRight = stripWidthsRight.map(strip => [1 - strip[1], 1 - strip[0]]);

var stripWidths = stripWidthsLeft.concat(stripWidthsRight);

g_Map.log("Creating strips");
for (let i = 0; i < stripWidths.length; ++i)
{
	clStrip[i] = g_Map.createTileClass();

	let isPlayerStrip = i == 2 || i == 3;
	for (let j = 0; j < scaleByMapSize(20, 100); ++j)
	{
		let position = new Vector2D(
			randFloat(mapBounds.bottom, mapBounds.top),
			fractionToTiles(randFloat(...stripWidths[i]))).rotateAround(startAngle, mapCenter).round();

		createArea(
			new ChainPlacer(
				1,
				Math.floor(scaleByMapSize(3, connectPlayers && isPlayerStrip ? 8 : 7)),
				Math.floor(scaleByMapSize(30, 60)),
				Infinity,
				position),
			[
				new LayeredPainter([tGrass, tGrass], [2]),
				new SmoothElevationPainter(ELEVATION_SET, heightLand, 3),
				new TileClassPainter(clStrip[i])
			]);
	}
}
Engine.SetProgress(20);

var playerPosition = playerPlacementLine(startAngle, mapCenter, fractionToTiles(1 - stripWidthsLeft[2][0] - stripWidthsLeft[2][1]));

// Either left vs right or top vs bottom
var playerIDs = randBool() ? sortAllPlayers() : primeSortAllPlayers();

g_Map.log("Ensuring player territory");
var playerRadius = scaleByMapSize(12, 20);
for (let i = 0; i < numPlayers; ++i)
	createArea(
		new ChainPlacer(1, 6, 40, 1, playerPosition[i], 0, [Math.floor(playerRadius)]),
		[
			new LayeredPainter([tGrass, tGrass, tGrass], [1, 4]),
			new SmoothElevationPainter(ELEVATION_SET, heightLand, 4),
			new TileClassPainter(clPlayerTerritory)
		]);

placePlayerBases({
	"PlayerPlacement": [playerIDs, playerPosition],
	"PlayerTileClass": clPlayer,
	"BaseResourceClass": clBaseResource,
	"baseResourceConstraint": stayClasses(clPlayerTerritory, 4),
	"Walls": "towers",
	"CityPatch": {
		"outerTerrain": tRoadWild,
		"innerTerrain": tRoad,
		"radius": playerRadius / 3
	},
	"Chicken": {
		"template": oPeacock
	},
	"Berries": {
		"template": oBush
	},
	"Mines": {
		"types": [
			{ "template": oMetalLarge },
			{ "template": oStoneLarge }
		]
	},
	"Trees": {
		"template": oTree,
		"count": 40
	}
	// No decoratives
});
Engine.SetProgress(35);

paintTerrainBasedOnHeight(-10, 0, 1, tWater);
paintTileClassBasedOnHeight(-10, 0, 1, clWater);
paintTerrainBasedOnHeight(1, 2.8, 1, tShoreBlend);
paintTerrainBasedOnHeight(0, 1, 1, tShore);
Engine.SetProgress(40);

if (!isNomad())
{
	g_Map.log("Creating gaia");
	for (let i = 0; i < 2; ++i)
		for (let j = 0; j < scaleByMapSize(1, 8); ++j)
			createObjectGroupsDeprecated(
				new SimpleGroup(
					[
						new SimpleObject(oSpearman, 8, 12, 2, 3),
						new SimpleObject(oArcher, 8, 12, 2, 3),
						new SimpleObject(oArcherElephant, 2, 3, 4, 5)
					],
					true,
					clGaia),
				0,
				[
					avoidClasses(
						clWater, 2,
						clForest, 1,
						clPlayerTerritory, 0,
						clHill, 1,
						clGaia, 15),
					stayClasses(clStrip[i == 0 ? 0 : stripWidths.length - 1], 1)
				],
				scaleByMapSize(5, 10),
				50);
}

g_Map.log("Creating hills");
createAreas(
	new ChainPlacer(1, Math.floor(scaleByMapSize(4, 6)), Math.floor(scaleByMapSize(16, 40)), 0.1),
	[
		new LayeredPainter([tCliff, tGrass], [3]),
		new SmoothElevationPainter(ELEVATION_SET, heightHill, 3),
		new TileClassPainter(clHill)
	],
	[
		avoidClasses(
			clPlayerTerritory, 0,
			clHill, 5,
			clGaia, 1,
			clWater, 2)
	],
	scaleByMapSize(1, 5));

createBumps(avoidClasses(clPlayer, 8, clWater, 2), scaleByMapSize(20, 150), 2, 8, 4, 1, 4);
Engine.SetProgress(50);

g_Map.log("Creating forests");
var [forestTrees, stragglerTrees] = getTreeCounts(1000, 4000, 0.7);
var types = [
	[[tGrass, tGrass, tGrass, tGrass, pForestD], [tGrass, tGrass, tGrass, pForestD]],
	[[tGrass, tGrass, tGrass, tGrass, pForestP1], [tGrass, tGrass, tGrass, pForestP1]],
	[[tGrass, tGrass, tGrass, tGrass, pForestP2], [tGrass, tGrass, tGrass, pForestP2]]
];
var size = forestTrees / (scaleByMapSize(3, 6) * numPlayers);
var num = Math.floor(size / types.length);
for (let type of types)
	createAreas(
		new ChainPlacer(
			1,
			Math.floor(scaleByMapSize(3, 5)),
			forestTrees / (num * Math.floor(scaleByMapSize(2, 4))),
			0.5),
		[
			new LayeredPainter(type, [2]),
			new TileClassPainter(clForest)
		],
		avoidClasses(
			clPlayer, 12,
			clForest, 6,
			clHill, 0,
			clGaia, 1,
			clWater, 2),
		num);

createStragglerTrees(
	[oTree, oPalm1, oPalm2],
	avoidClasses(
		clWater, 5,
		clForest, 1,
		clHill, 1,
		clPlayer, 8,
		clBaseResource, 4,
		clGaia, 1,
		clMetal, 4,
		clRock, 4),
	clForest,
	stragglerTrees);

Engine.SetProgress(60);

g_Map.log("Creating grass patches");
for (let size of [scaleByMapSize(3, 6), scaleByMapSize(5, 10), scaleByMapSize(8, 21)])
	createAreas(
		new ChainPlacer(1, Math.floor(scaleByMapSize(3, 5)), size, 0.5),
		[
			new LayeredPainter([tGrassC, tGrassA, tGrassB], [2, 1]),
			new TileClassPainter(clDirt)
		],
		avoidClasses(
			clWater, 8,
			clForest, 0,
			clHill, 0,
			clGaia, 1,
			clPlayerTerritory, 0,
			clDirt, 16),
		scaleByMapSize(20, 80));

g_Map.log("Creating dirt patches");
for (let size of [scaleByMapSize(2, 4), scaleByMapSize(3, 7), scaleByMapSize(5, 15)])
	createAreas(
		new ChainPlacer(1, Math.floor(scaleByMapSize(3, 5)), size, 0.5),
		[
			new LayeredPainter([tPlants, tPlants], [1]),
			new TileClassPainter(clDirt)
		],
		avoidClasses(
			clWater, 8,
			clForest, 0,
			clHill, 0,
			clGaia, 1,
			clPlayerTerritory, 0,
			clDirt, 16),
		scaleByMapSize(20, 80));

g_Map.log("Creating stone mines");
createObjectGroupsDeprecated(
	new SimpleGroup(
		[
			new SimpleObject(oStoneSmall, 0, 2, 0, 4),
			new SimpleObject(oStoneLarge, 1, 1, 0, 4)
		],
		true,
		clRock),
	0,
	avoidClasses(
		clWater, 3,
		clForest, 1,
		clPlayerTerritory, 0,
		clGaia, 1,
		clRock, 10,
		clHill, 1),
		9 * scaleByMapSize(1, 4),
		100);

g_Map.log("Creating small stone mines");
createObjectGroupsDeprecated(
	new SimpleGroup([new SimpleObject(oStoneSmall, 2, 5, 1, 3)], true, clRock),
	0,
	avoidClasses(
		clWater, 4,
		clForest, 1,
		clPlayerTerritory, 0,
		clGaia, 1,
		clRock, 10,
		clHill, 1),
		9 * scaleByMapSize(1, 4),
	100);

g_Map.log("Creating metal mines");
createObjectGroupsDeprecated(
	new SimpleGroup([new SimpleObject(oMetalLarge, 1, 1, 0, 4)], true, clMetal),
	0,
	avoidClasses(
		clWater, 4,
		clForest, 1,
		clPlayerTerritory, 0,
		clGaia, 1,
		clMetal, 10,
		clRock, 5,
		clHill, 1),
	9 * scaleByMapSize(1, 4),
	100);

g_Map.log("Creating small decorative rocks");
createObjectGroupsDeprecated(
	new SimpleGroup([new SimpleObject(aRockMedium, 1, 3, 0, 1)], true),
	0,
	avoidClasses(
		clWater, 2,
		clForest, 1,
		clGaia, 1,
		clPlayer, 8,
		clBaseResource, 4,
		clHill, 0),
	3 * scaleByMapSize(16, 262),
	50);

g_Map.log("Creating large decorative rocks");
createObjectGroupsDeprecated(
	new SimpleGroup([
			new SimpleObject(aRockLarge, 1, 2, 0, 1),
			new SimpleObject(aRockMedium, 1, 3, 0, 2)
		],
		true),
	0,
	avoidClasses(
		clWater, 2,
		clForest, 1,
		clGaia, 1,
		clPlayer, 8,
		clBaseResource, 4,
		clHill, 0),
	3 * scaleByMapSize(8, 131),
	50);

g_Map.log("Creating small grass tufts");
createObjectGroupsDeprecated(
	new SimpleGroup([new SimpleObject(aBush1, 1, 2, 0, 1, -Math.PI / 8, Math.PI / 8)]),
	0,
	avoidClasses(
		clWater, 4,
		clHill, 2,
		clPlayer, 8,
		clGaia, 1,
		clBaseResource, 4,
		clDirt, 0),
	8 * scaleByMapSize(13, 200));
Engine.SetProgress(70);

g_Map.log("Creating large grass tufts");
	createObjectGroupsDeprecated(
		new SimpleGroup([
			new SimpleObject(aBush2, 2, 4, 0, 1.8, -Math.PI / 8, Math.PI / 8),
			new SimpleObject(aBush1, 3, 6, 1.2, 2.5, -Math.PI / 8, Math.PI / 8)
		]),
		0,
		avoidClasses(
			clWater, 4,
			clHill, 2,
			clGaia, 1,
			clPlayer, 8,
			clBaseResource, 4,
			clDirt, 1,
			clForest, 0),
		8 * scaleByMapSize(13, 200));
Engine.SetProgress(85);

g_Map.log("Creating bushes");
	createObjectGroupsDeprecated(
		new SimpleGroup([
			new SimpleObject(aBush3, 1, 2, 0, 2),
			new SimpleObject(aBush2, 2, 4, 0, 2)
		]), 0,
		avoidClasses(
			clWater, 4,
			clHill, 1,
			clPlayerTerritory, 0,
			clGaia, 1,
			clDirt, 1),
		8 * scaleByMapSize(13, 200), 50);

g_Map.log("Creating deer");
	createObjectGroupsDeprecated(
		new SimpleGroup([new SimpleObject(oDeer, 5, 7, 0, 4)], true, clFood),
		0,
		avoidClasses(
			clWater, 4,
			clForest, 0,
			clPlayerTerritory, 0,
			clGaia, 1,
			clHill, 1,
			clFood, 20),
		3 * numPlayers,
		50);

g_Map.log("Creating boar");
	createObjectGroupsDeprecated(
		new SimpleGroup([new SimpleObject(oBoar, 2, 4, 0, 4)], true, clFood),
		0,
		avoidClasses(
			clWater, 4,
			clForest, 0,
			clPlayerTerritory, 0,
			clGaia, 1,
			clHill, 1,
			clFood, 20),
		3 * numPlayers,
		50);

g_Map.log("Creating tigers");
createObjectGroupsDeprecated(
	new SimpleGroup([new SimpleObject(oTiger, 1, 1, 0, 4)], true, clFood),
	0,
	avoidClasses(
		clWater, 4,
		clForest, 0,
		clPlayerTerritory, 0,
		clGaia, 1,
		clHill, 1,
		clFood, 20),
	3 * numPlayers,
	50);
Engine.SetProgress(95);

g_Map.log("Creating berry bush");
createObjectGroupsDeprecated(
	new SimpleGroup([new SimpleObject(oBush, 5, 7, 0, 4)], true, clFood),
	0,
	avoidClasses(
		clWater, 4,
		clForest, 0,
		clPlayerTerritory, 0,
		clGaia, 1,
		clHill, 1,
		clFood, 10),
		randIntInclusive(1, 4) * numPlayers + 2,
		50);

g_Map.log("Creating fish");
createObjectGroupsDeprecated(
	new SimpleGroup([new SimpleObject(oFish, 2, 3, 0, 2)], true, clFood),
	0,
	[avoidClasses(clFood, 15), stayClasses(clWater, 4)],
	200,
	100);

placePlayersNomad(clPlayer, avoidClasses(clWater, 4, clForest, 1, clMetal, 4, clRock, 4, clFood, 2));

setSunColor(0.6, 0.6, 0.6);
setSunElevation(Math.PI/ 3);

setWaterColor(0.424, 0.534, 0.639);
setWaterTint(0.369, 0.765, 0.745);
setWaterWaviness(1.0);
setWaterType("default");
setWaterMurkiness(0.35);

setFogFactor(0.03);
setFogThickness(0.2);

setPPEffect("hdr");
setPPContrast(0.7);
setPPSaturation(0.65);
setPPBloom(0.6);

setSkySet("stratus");
g_Map.ExportMap();
