Engine.LoadLibrary("rmgen");

const tOceanRockDeep = "medit_sea_coral_deep";
const tOceanCoral = "medit_sea_coral_plants";
const tBeachWet = "medit_sand_wet";
const tBeachDry = "medit_sand";
const tBeach = ["medit_rocks_grass","medit_sand", "medit_rocks_grass_shrubs"];
const tBeachBlend = ["medit_rocks_grass", "medit_rocks_grass_shrubs"];
const tCity = "medit_city_tile";
const tGrassDry = ["medit_grass_field_dry", "medit_grass_field_b"];
const tGrass = ["medit_rocks_grass", "medit_rocks_grass","medit_dirt","medit_rocks_grass_shrubs"];
const tGrassShrubs = "medit_shrubs";
const tCliffShrubs = ["medit_cliff_aegean_shrubs", "medit_cliff_italia_grass","medit_cliff_italia"];
const tCliff = ["medit_cliff_italia", "medit_cliff_italia", "medit_cliff_italia_grass"];
const tForestFloor = "medit_forestfloor_a";

const oBeech = "gaia/flora_tree_euro_beech";
const oBerryBush = "gaia/flora_bush_berry";
const oCarob = "gaia/flora_tree_carob";
const oCypress1 = "gaia/flora_tree_cypress";
const oCypress2 = "gaia/flora_tree_cypress";
const oLombardyPoplar = "gaia/flora_tree_poplar_lombardy";
const oPalm = "gaia/flora_tree_medit_fan_palm";
const oPine = "gaia/flora_tree_aleppo_pine";
const oDateT = "gaia/flora_tree_cretan_date_palm_tall";
const oDateS = "gaia/flora_tree_cretan_date_palm_short";
const oDeer = "gaia/fauna_deer";
const oFish = "gaia/fauna_fish";
const oWhale = "gaia/fauna_whale_humpback";
const oStoneLarge = "gaia/geology_stonemine_medit_quarry";
const oStoneSmall = "gaia/geology_stone_mediterranean";
const oMetalLarge = "gaia/geology_metal_mediterranean_slabs";
const oShipwreck = "other/special_treasure_shipwreck";
const oShipDebris = "other/special_treasure_shipwreck_debris";

const aRockLarge = "actor|geology/stone_granite_large.xml";
const aRockMed = "actor|geology/stone_granite_med.xml";
const aRockSmall = "actor|geology/stone_granite_small.xml";

const pPalmForest = [tForestFloor+TERRAIN_SEPARATOR+oPalm, tGrass];
const pPineForest = [tForestFloor+TERRAIN_SEPARATOR+oPine, tGrass];
const pPoplarForest = [tForestFloor+TERRAIN_SEPARATOR+oLombardyPoplar, tGrass];
const pMainForest = [tForestFloor+TERRAIN_SEPARATOR+oCarob, tForestFloor+TERRAIN_SEPARATOR+oBeech, tGrass, tGrass];

const heightSeaGround = -5;
const heightLand = 3;
const heightHill = 12;
const heightOffsetBump = 2;

var g_Map = new RandomMap(heightSeaGround, tWater);

const numPlayers = getNumPlayers();
const mapSize = getMapSize();
const mapCenter = g_Map.getCenter();

var clCoral = createTileClass();
var clPlayer = createTileClass();
var clIsland = createTileClass();
var clCity = createTileClass();
var clDirt = createTileClass();
var clHill = createTileClass();
var clForest = createTileClass();
var clWater = createTileClass();
var clRock = createTileClass();
var clMetal = createTileClass();
var clFood = createTileClass();
var clBaseResource = createTileClass();

//array holding starting islands based on number of players
var startingPlaces=[[0],[0,3],[0,2,4],[0,1,3,4],[0,1,2,3,4],[0,1,2,3,4,5]];

var startAngle = randomAngle();

var islandRadius = scaleByMapSize(15, 40);
var islandCount = Math.max(6, numPlayers);
var islandPosition = distributePointsOnCircle(islandCount, startAngle, fractionToTiles(0.39), mapCenter)[0].map(position => position.round());

var centralIslandRadius = scaleByMapSize(15, 30);
var centralIslandCount = Math.floor(scaleByMapSize(1, 4));
var centralIslandPosition = new Array(numPlayers).fill(0).map((v, i) =>
	Vector2D.add(mapCenter, new Vector2D(fractionToTiles(randFloat(0.1, 0.16)), 0).rotate(
		-startAngle - Math.PI * (i * 2 / centralIslandCount + randFloat(-1, 1) / 8)).round()));

var areas = [];

var nPlayer = 0;
var playerPosition = [];

function createCycladicArchipelagoIsland(position, tileClass, radius, coralRadius)
{
	log("Creating deep ocean rocks...");
	createArea(
		new ClumpPlacer(diskArea(radius + coralRadius), 0.7, 0.1, 10, position),
		[
			new LayeredPainter([tOceanRockDeep, tOceanCoral], [5]),
			paintClass(clCoral)
		],
		avoidClasses(clCoral, 0, clPlayer, 0));

	log("Creating island...");
	areas.push(
		createArea(
			new ClumpPlacer(diskArea(radius), 0.7, 0.1, 10, position),
			[
				new LayeredPainter([tOceanCoral, tBeachWet, tBeachDry, tBeach, tBeachBlend, tGrass], [1, 3, 1, 1, 2]),
				new SmoothElevationPainter(ELEVATION_SET, heightLand, 5),
				paintClass(tileClass)
			],
			avoidClasses(clPlayer, 0)));
}

log("Creating player islands...");
for (let i = 0; i < islandCount; ++i)
{
	let isPlayerIsland = numPlayers >= 6 || i == startingPlaces[numPlayers - 1][nPlayer];
	if (isPlayerIsland)
	{
		playerPosition[nPlayer] = islandPosition[i];
		++nPlayer;
	}
	createCycladicArchipelagoIsland(islandPosition[i], isPlayerIsland ? clPlayer : clIsland, islandRadius, scaleByMapSize(1, 5));
}

log("Creating central islands...");
for (let position of centralIslandPosition)
	createCycladicArchipelagoIsland(position, clIsland, centralIslandRadius, 2);

placePlayerBases({
	"PlayerPlacement": [sortAllPlayers(), playerPosition],
	// PlayerTileClass is clCity here and painted below
	"BaseResourceClass": clBaseResource,
	"Walls": "towers",
	"CityPatch": {
		"radius": 6,
		"outerTerrain": tGrass,
		"innerTerrain": tCity,
		"painters": [
			paintClass(clCity)
		]
	},
	"Chicken": {
	},
	"Berries": {
		"template": oBerryBush
	},
	"Mines": {
		"types": [
			{ "template": oMetalLarge },
			{ "template": oStoneLarge }
		]
	},
	"Trees": {
		"template": oPalm,
		"count": 2
	}
	// No decoratives
});
Engine.SetProgress(20);

log("Creating bumps...");
createAreasInAreas(
	new ClumpPlacer(scaleByMapSize(20, 60), 0.3, 0.06, 1),
	new SmoothElevationPainter(ELEVATION_MODIFY, heightOffsetBump, 3),
	avoidClasses(clCity, 0),
	scaleByMapSize(25, 75),15,
	areas);

Engine.SetProgress(34);

log("Creating hills...");
createAreasInAreas(
	new ClumpPlacer(scaleByMapSize(20, 150), 0.2, 0.1, 1),
	[
		new LayeredPainter([tCliff, tCliffShrubs], [2]),
		new SmoothElevationPainter(ELEVATION_SET, heightHill, 2),
		paintClass(clHill)
	],
	avoidClasses(clCity, 15, clHill, 15),
	scaleByMapSize(5, 30), 15,
	areas);

Engine.SetProgress(38);

paintTileClassBasedOnHeight(-Infinity, 0, Elevation_ExcludeMin_ExcludeMax, clWater);

log("Creating forests...");
var forestTypes = [
	[[tForestFloor, tGrass, pPalmForest], [tForestFloor, pPalmForest]],
	[[tForestFloor, tGrass, pPineForest], [tForestFloor, pPineForest]],
	[[tForestFloor, tGrass, pPoplarForest], [tForestFloor, pPoplarForest]],
	[[tForestFloor, tGrass, pMainForest], [tForestFloor, pMainForest]]
];

for (let type of forestTypes)
	createAreasInAreas(
		new ClumpPlacer(randIntInclusive(6, 17), 0.1, 0.1, 1),
		[
			new LayeredPainter(type, [2]),
			paintClass(clForest)
		],
		avoidClasses(clCity, 1, clWater, 3, clForest, 3, clHill, 1, clBaseResource, 4),
		scaleByMapSize(10, 64),
		20,
		areas);
Engine.SetProgress(42);

log("Creating stone mines...");
var group = new SimpleGroup([new SimpleObject(oStoneSmall, 0,2, 0,4), new SimpleObject(oStoneLarge, 1,1, 0,4)], true, clRock);
createObjectGroupsByAreasDeprecated(group, 0,
	[avoidClasses(clWater, 1, clForest, 1, clHill, 1, clPlayer, 5, clRock, 6)],
	scaleByMapSize(4,16), 200, areas
);
Engine.SetProgress(46);

log("Creating small stone mines...");
var group = new SimpleGroup([new SimpleObject(oStoneSmall, 2,5, 1,3)], true, clRock);
createObjectGroupsByAreasDeprecated(group, 0,
	[avoidClasses(clWater, 1, clForest, 1, clHill, 1, clPlayer, 5, clRock, 2)],
	scaleByMapSize(4,16), 200, areas
);
Engine.SetProgress(50);

log("Creating metal mines...");
group = new SimpleGroup([new SimpleObject(oMetalLarge, 1,1, 0,4)], true, clMetal);
createObjectGroupsByAreasDeprecated(group, 0,
	[avoidClasses(clWater, 1, clForest, 1, clHill, 1, clPlayer, 5, clMetal, 6, clRock, 6)],
	scaleByMapSize(4,16), 200, areas
);
Engine.SetProgress(54);

log("Creating shrub patches...");
for (let size of [scaleByMapSize(2, 32), scaleByMapSize(3, 48), scaleByMapSize(5, 80)])
	createAreasInAreas(
		new ClumpPlacer(size, 0.3, 0.06, 0.5),
		[
			new LayeredPainter([tBeachBlend, tGrassShrubs], [1]),
			paintClass(clDirt)
		],
		avoidClasses(clWater, 3, clHill, 0, clDirt, 6, clCity, 0, clBaseResource, 4),
		scaleByMapSize(4, 16),
		20,
		areas);
Engine.SetProgress(58);

log("Creating grass patches...");
for (let size of [scaleByMapSize(2, 32), scaleByMapSize(3, 48), scaleByMapSize(5, 80)])
	createAreasInAreas(
		new ClumpPlacer(size, 0.3, 0.06, 0.5),
		[
			new LayeredPainter([tGrassDry], []),
			paintClass(clDirt)
		],
		avoidClasses(clWater, 3, clHill, 0, clDirt, 6, clCity, 0, clBaseResource, 4),
		scaleByMapSize(4, 16),
		20,
		areas);
Engine.SetProgress(62);

log("Creating straggler trees...");
for (let tree of [oCarob, oBeech, oLombardyPoplar, oLombardyPoplar, oPine])
	createObjectGroupsByAreasDeprecated(
		new SimpleGroup([new SimpleObject(tree, 1,1, 0,1)], true, clForest),
		0,
		avoidClasses(clWater, 2, clForest, 2, clCity, 3, clBaseResource, 4, clRock, 6, clMetal, 6, clPlayer, 1, clHill, 1),
		scaleByMapSize(2, 38), 50, areas
	);
Engine.SetProgress(66);

log("Create straggler cypresses...");
group = new SimpleGroup(
	[new SimpleObject(oCypress2, 1,3, 0,3), new SimpleObject(oCypress1, 0,2, 0,2)],
	true
);
createObjectGroupsByAreasDeprecated(group, 0,
	avoidClasses(clWater, 2, clForest, 2, clCity, 3, clBaseResource, 4, clRock, 6, clMetal, 6, clPlayer, 1, clHill, 1),
	scaleByMapSize(5, 75), 50, areas
);
Engine.SetProgress(70);

log("Create straggler date palms...");
group = new SimpleGroup(
	[new SimpleObject(oDateS, 1,3, 0,3), new SimpleObject(oDateT, 0,2, 0,2)],
	true
);
createObjectGroupsByAreasDeprecated(group, 0,
	avoidClasses(clWater, 2, clForest, 1, clCity, 0, clBaseResource, 4, clRock, 6, clMetal, 6, clPlayer, 1, clHill, 1),
	scaleByMapSize(5, 75), 50, areas
);
Engine.SetProgress(74);

log("Creating rocks...");
group = new SimpleGroup(
	[new SimpleObject(aRockSmall, 0,3, 0,2), new SimpleObject(aRockMed, 0,2, 0,2),
	new SimpleObject(aRockLarge, 0,1, 0,2)]
);
createObjectGroupsDeprecated(group, 0,
	avoidClasses(clWater, 0, clCity, 0),
	scaleByMapSize(30, 180), 50
);
Engine.SetProgress(78);

log("Creating deer...");
group = new SimpleGroup(
	[new SimpleObject(oDeer, 5,7, 0,4)],
	true, clFood
);
createObjectGroupsDeprecated(group, 0,
	avoidClasses(clWater, 5, clForest, 1, clHill, 1, clCity, 10, clMetal, 6, clRock, 2, clFood, 8),
	3 * numPlayers, 50
);
Engine.SetProgress(82);

log("Creating berry bushes...");
group = new SimpleGroup([new SimpleObject(oBerryBush, 5,7, 0,3)], true, clFood);
createObjectGroupsDeprecated(group, 0,
	avoidClasses(clWater, 2, clForest, 1, clHill, 1, clCity, 10, clMetal, 6, clRock, 2, clFood, 8),
	1.5 * numPlayers, 100
);
Engine.SetProgress(86);

log("Creating Fish...");
group = new SimpleGroup([new SimpleObject(oFish, 1,1, 0,3)], true, clFood);
createObjectGroupsDeprecated(group, 0,
	[stayClasses(clWater,1),avoidClasses(clFood, 8)],
	scaleByMapSize(40,200), 100
);
Engine.SetProgress(90);

log("Creating Whales...");
group = new SimpleGroup([new SimpleObject(oWhale, 1,1, 0,3)], true, clFood);
createObjectGroupsDeprecated(group, 0,
	[stayClasses(clWater,1),avoidClasses(clFood, 8, clPlayer,4,clIsland,4)],
	scaleByMapSize(10,40), 100
);
Engine.SetProgress(94);

log("Creating shipwrecks...");
group = new SimpleGroup([new SimpleObject(oShipwreck, 1,1, 0,3)], true, clFood);
createObjectGroupsDeprecated(group, 0,
	[stayClasses(clWater,1),avoidClasses(clFood, 8)],
	scaleByMapSize(6,16), 100
);
Engine.SetProgress(98);

log("Creating shipwreck debris...");
group = new SimpleGroup([new SimpleObject(oShipDebris, 1,2, 0,4)], true, clFood);
createObjectGroupsDeprecated(group, 0,
	[stayClasses(clWater,1),avoidClasses(clFood, 8)],
	scaleByMapSize(10,20), 100
);
Engine.SetProgress(99);

placePlayersNomad(clPlayer, avoidClasses(clWater, 4, clForest, 1, clBaseResource, 4, clHill, 4, clMetal, 4, clRock, 4, clFood, 1));

setSkySet("sunny");
setWaterColor(0.2,0.294,0.49);
setWaterTint(0.208, 0.659, 0.925);
setWaterMurkiness(0.72);
setWaterWaviness(3.0);
setWaterType("ocean");

g_Map.ExportMap();
