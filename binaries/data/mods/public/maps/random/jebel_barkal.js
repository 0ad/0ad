/**
 * For historic reference, see http://www.jebelbarkal.org/images/maps/siteplan.jpg
 */

Engine.LoadLibrary("rmgen");
Engine.LoadLibrary("rmgen-common");
Engine.LoadLibrary("heightmap");

TILE_CENTERED_HEIGHT_MAP = true;

const tSand = "desert_sand_dunes_100";
const tHilltop = ["new_savanna_dirt_c", "new_savanna_dirt_d"];
const tHillGround = ["savanna_dirt_rocks_a", "savanna_dirt_rocks_b", "savanna_dirt_rocks_c"];
const tHillCliff = ["savanna_cliff_a_red", "savanna_cliff_b_red"];
const tRoadDesert = "savanna_tile_a";
const tRoadFertileLand = "savanna_tile_a";
const tWater = "desert_sand_wet";
const tGrass = ["savanna_shrubs_a_wetseason", "alpine_grass_b_wild", "medit_shrubs_a", "steppe_grass_green_a"];
const tForestFloorFertile = pickRandom(tGrass);
const tGrassTransition1 = "desert_grass_a";
const tGrassTransition2 = "steppe_grass_dirt_66";
const tPath = "road2";
const tPathWild = "road_med";

const oAcacia = "gaia/flora_tree_acacia";
const oPalmPath = "gaia/flora_tree_cretan_date_palm_tall";
const oPalms = [
	"gaia/flora_tree_cretan_date_palm_tall",
	"gaia/flora_tree_cretan_date_palm_short",
	"gaia/flora_tree_palm_tropic",
	"gaia/flora_tree_date_palm",
	"gaia/flora_tree_senegal_date_palm",
	"gaia/flora_tree_medit_fan_palm"
];
const oBerryBushGrapes = "gaia/flora_bush_grapes";
const oBerryBushDesert = "gaia/flora_bush_berry_desert";
const oStoneLarge = "gaia/geology_stonemine_desert_quarry";
const oStoneSmall = "gaia/geology_stone_desert_small";
const oMetalLarge = "gaia/geology_metal_desert_slabs";
const oMetalSmall = "gaia/geology_metal_desert_small";
const oFoodTreasureBin = "gaia/treasure/food_bin";
const oFoodTreasureCrate = "gaia/treasure/food_crate";
const oFoodTreasureJars = "gaia/treasure/food_jars";
const oWoodTreasure = "gaia/treasure/wood";
const oStoneTreasure = "gaia/treasure/stone";
const oMetalTreasure = "gaia/treasure/metal";
const oTreasuresHill = [oWoodTreasure, oStoneTreasure, oMetalTreasure];
const oTreasuresCity = [oFoodTreasureBin, oFoodTreasureCrate, oFoodTreasureJars].concat(oTreasuresHill);
const oGiraffe = "gaia/fauna_giraffe";
const oGiraffeInfant = "gaia/fauna_giraffe_infant";
const oGazelle = "gaia/fauna_gazelle";
const oRhino = "gaia/fauna_rhino";
const oWarthog = "gaia/fauna_boar";
const oElephant = "gaia/fauna_elephant_african_bush";
const oElephantInfant = "gaia/fauna_elephant_african_infant";
const oLion = "gaia/fauna_lion";
const oLioness = "gaia/fauna_lioness";
const oCrocodile = "gaia/fauna_crocodile";
const oFish = "gaia/fauna_fish_tilapia";
const oHawk = "gaia/fauna_hawk";
const oTempleApedemak = "structures/kush_temple";
const oTempleAmun = "structures/kush_temple_amun";
const oPyramidLarge = "structures/kush_pyramid_large";
const oPyramidSmall = "structures/kush_pyramid_small";
const oWonderPtol = "structures/ptol_wonder";
const oFortress = "structures/kush_fortress";
const oTower = g_MapSettings.Size >= 256 ? "structures/kush_defense_tower" : "structures/kush_sentry_tower";
const oHouse = "structures/kush_house";
const oMarket = "structures/kush_market";
const oBlacksmith = "structures/kush_blacksmith";
const oBlemmyeCamp = "structures/kush_blemmye_camp";
const oNubaVillage = "structures/kush_nuba_village";
const oCivicCenter = "structures/kush_civil_centre";
const oBarracks = "structures/kush_barracks";
const oStable = "structures/kush_stable";
const oElephantStables = "structures/kush_elephant_stables";
const oWallMedium = "structures/kush_wall_medium";
const oWallGate = "structures/kush_wall_gate";
const oWallTower = "structures/kush_wall_tower";
const oKushCitizenArcher = "units/kush_infantry_archer_b";
const oKushHealer = "units/kush_support_healer_b";
const oKushChampionArcher = "units/kush_champion_infantry";
const oKushChampions = [
	oKushChampionArcher,
	"units/kush_champion_infantry_amun",
	"units/kush_champion_infantry_apedemak"
];
const oPtolSiege = ["units/ptol_mechanical_siege_lithobolos_unpacked", "units/ptol_mechanical_siege_polybolos_unpacked"];
const oTriggerPointCityPath = "trigger/trigger_point_A";
const oTriggerPointAttackerPatrol = "trigger/trigger_point_B";

const aPalmPath = actorTemplate("flora/trees/palm_cretan_date_tall");
const aRock = actorTemplate("geology/stone_savanna_med");
const aHandcart = actorTemplate("props/special/eyecandy/handcart_1");
const aPlotFence = actorTemplate("props/special/common/plot_fence");
const aStatueKush = actorTemplate("props/special/eyecandy/statues_kush");
const aStatues = [
	"props/structures/kushites/statue_pedestal_rectangular",
	"props/structures/kushites/statue_pedestal_rectangular_lion"
].map(actorTemplate);
const aBushesFertileLand = [
	...new Array(3).fill("props/flora/shrub_spikes"),
	...new Array(3).fill("props/flora/ferns"),
	"props/flora/shrub_tropic_plant_a",
	"props/flora/shrub_tropic_plant_b",
	"props/flora/shrub_tropic_plant_flower",
	"props/flora/foliagebush",
	"props/flora/bush",
	"props/flora/bush_medit_la",
	"props/flora/bush_medit_la_lush",
	"props/flora/bush_medit_me_lush",
	"props/flora/bush_medit_sm",
	"props/flora/bush_medit_sm_lush",
	"props/flora/bush_tempe_la_lush"
].map(actorTemplate);
const aBushesCity = [
	"props/flora/bush_dry_a",
	"props/flora/bush_medit_la_dry",
	"props/flora/bush_medit_me_dry",
	"props/flora/bush_medit_sm",
	"props/flora/bush_medit_sm_dry",
].map(actorTemplate);
const aBushesDesert = [
	"props/flora/bush_tempe_me_dry",
	"props/flora/grass_soft_dry_large_tall",
	"props/flora/grass_soft_dry_small_tall"
].map(actorTemplate).concat(aBushesCity);
const aWaterDecoratives = ["props/flora/reeds_pond_lush_a"].map(actorTemplate);

const pForestPalms = [
	tForestFloorFertile,
	...oPalms.map(tree => tForestFloorFertile + TERRAIN_SEPARATOR + tree),
	tForestFloorFertile];

const heightScale = num => num * g_MapSettings.Size / 320;

const minHeightSource = 3;
const maxHeightSource = 800;

const g_Map = new RandomMap(0, tSand);
const mapSize = g_Map.getSize();
const mapCenter = g_Map.getCenter();
const mapBounds = g_Map.getBounds();
const numPlayers = getNumPlayers();

const clHill = g_Map.createTileClass();
const clCliff = g_Map.createTileClass();
const clDesert = g_Map.createTileClass();
const clFertileLand = g_Map.createTileClass();
const clWater = g_Map.createTileClass();
const clIrrigationCanal = g_Map.createTileClass();
const clPassage = g_Map.createTileClass();
const clPlayer = g_Map.createTileClass();
const clBaseResource = g_Map.createTileClass();
const clFood = g_Map.createTileClass();
const clForest = g_Map.createTileClass();
const clRock = g_Map.createTileClass();
const clMetal = g_Map.createTileClass();
const clTreasure = g_Map.createTileClass();
const clCity = g_Map.createTileClass();
const clPath = g_Map.createTileClass();
const clPathStatues = g_Map.createTileClass();
const clPathCrossing = g_Map.createTileClass();
const clStatue = g_Map.createTileClass();
const clWall = g_Map.createTileClass();
const clGate = g_Map.createTileClass();
const clTriggerPointCityPath = g_Map.createTileClass();
const clTriggerPointMap = g_Map.createTileClass();
const clSoldier = g_Map.createTileClass();
const clTower = g_Map.createTileClass();
const clFortress = g_Map.createTileClass();
const clTemple = g_Map.createTileClass();
const clRitualPlace = g_Map.createTileClass();
const clPyramid = g_Map.createTileClass();
const clHouse = g_Map.createTileClass();
const clBlacksmith = g_Map.createTileClass();
const clStable = g_Map.createTileClass();
const clElephantStables = g_Map.createTileClass();
const clCivicCenter = g_Map.createTileClass();
const clBarracks = g_Map.createTileClass();
const clBlemmyeCamp = g_Map.createTileClass();
const clNubaVillage = g_Map.createTileClass();
const clMarket = g_Map.createTileClass();
const clDecorative = g_Map.createTileClass();

const riverAngle = Math.PI * 0.05;

const hillRadius = scaleByMapSize(40, 120);
const positionPyramids = new Vector2D(fractionToTiles(0.15), fractionToTiles(0.75));

const pathWidth = 4;
const pathWidthCenter = 10;
const pathWidthSecondary = 6;

const placeNapataWall = getDifficulty() >= 3;

const layoutFertileLandTextures = [
	{
		"left": fractionToTiles(0),
		"right": fractionToTiles(0.04),
		"terrain": createTerrain(tGrassTransition1),
		"tileClass": clFertileLand
	},
	{
		"left": fractionToTiles(0.04),
		"right": fractionToTiles(0.08),
		"terrain": createTerrain(tGrassTransition2),
		"tileClass": clDesert
	}
];

var layoutKushTemples = [
	...new Array(2).fill(0).map((v, i) =>
	({
		"template": oTempleApedemak,
		"pathOffset": new Vector2D(0, 9),
		"minMapSize": i == 0 ? 320 : 0
	})),
	{
		"template": oTempleAmun,
		"pathOffset": new Vector2D(0, 12),
		"minMapSize": 256
	},
	{
		"template": oWonderPtol,
		"pathOffset": new Vector2D(0,  scaleByMapSize(9, 14)),
		"minMapSize": 0
	},
	{
		"template": oTempleAmun,
		"pathOffset": new Vector2D(0, 12),
		"minMapSize": 256
	},
	...new Array(2).fill(0).map((v, i) =>
	({
		"template": oTempleApedemak,
		"pathOffset": new Vector2D(0, 9),
		"minMapSize": i == 0 ? 320 : 0
	}))
].filter(temple => mapSize >= temple.minMapSize);

/**
 * The buildings are set as uncapturable, otherwise the player would gain the buildings via root territory and can delete them without effort.
 * Keep the entire city uncapturable as a consistent property of the city.
 */
const layoutKushCity = [
	{
		"templateName": "uncapturable|" + oHouse,
		"difficulty": "Very Easy",
		"painters": new TileClassPainter(clHouse)
	},
	{
		"templateName": "uncapturable|" + oFortress,
		"difficulty": "Medium",
		"constraints": [avoidClasses(clFortress, 25), new NearTileClassConstraint(clPath, 8)],
		"painters": new TileClassPainter(clFortress)
	},
	{
		"templateName": "uncapturable|" + oCivicCenter,
		"difficulty": "Easy",
		"constraints": [avoidClasses(clCivicCenter, 60), new NearTileClassConstraint(clPath, 8)],
		"painters": new TileClassPainter(clCivicCenter)
	},
	{
		"templateName": "uncapturable|" + oElephantStables,
		"difficulty": "Medium",
		"constraints": avoidClasses(clElephantStables, 10),
		"painters": new TileClassPainter(clElephantStables)
	},
	{
		"templateName": "uncapturable|" + oStable,
		"difficulty": "Easy",
		"constraints": avoidClasses(clStable, 20),
		"painters": new TileClassPainter(clStable)
	},
	{
		"templateName": "uncapturable|" + oBarracks,
		"difficulty": "Easy",
		"constraints": avoidClasses(clBarracks, 12),
		"painters": new TileClassPainter(clBarracks)
	},
	{
		"templateName": "uncapturable|" + oTower,
		"difficulty": "Medium",
		"constraints": avoidClasses(clTower, 17),
		"painters": new TileClassPainter(clTower)
	},
	{
		"templateName": "uncapturable|" + oMarket,
		"difficulty": "Very Easy",
		"constraints": avoidClasses(clMarket, 15),
		"painters": new TileClassPainter(clMarket)
	},
	{
		"templateName": "uncapturable|" + oBlacksmith,
		"difficulty": "Very Easy",
		"constraints": avoidClasses(clBlacksmith, 30),
		"painters": new TileClassPainter(clBlacksmith)
	},
	{
		"templateName": "uncapturable|" + oNubaVillage,
		"difficulty": "Easy",
		"constraints": avoidClasses(clNubaVillage, 30),
		"painters": new TileClassPainter(clNubaVillage)
	},
	{
		"templateName": "uncapturable|" + oBlemmyeCamp,
		"difficulty": "Easy",
		"constraints": avoidClasses(clBlemmyeCamp, 30),
		"painters": new TileClassPainter(clBlemmyeCamp)
	}
].filter(building => getDifficulty() >= getDifficulties().find(difficulty => difficulty.Name == building.difficulty).Difficulty);

g_WallStyles.napata = {
	"short": readyWallElement("uncapturable|" + oWallMedium),
	"medium": readyWallElement("uncapturable|" + oWallMedium),
	"tower": readyWallElement("uncapturable|" + oWallTower),
	"gate": readyWallElement("uncapturable|" + oWallGate),
	"overlap": 0.05
};

Engine.SetProgress(10);

g_Map.log("Loading hill heightmap");
createArea(
	new MapBoundsPlacer(),
	new HeightmapPainter(
		translateHeightmap(
			new Vector2D(-12, scaleByMapSize(-12, -25)),
			undefined,
			convertHeightmap1Dto2D(Engine.LoadMapTerrain("maps/random/jebel_barkal.pmp").height)),
		minHeightSource,
		maxHeightSource));

const heightDesert = g_Map.getHeight(mapCenter);
const heightFertileLand = heightDesert - heightScale(2);
const heightShoreline = heightFertileLand - heightScale(0.5);
const heightWaterLevel = heightFertileLand - heightScale(3);
const heightPassage = heightWaterLevel - heightScale(1.5);
const heightIrrigationCanal = heightWaterLevel - heightScale(4);
const heightSeaGround = heightWaterLevel - heightScale(8);
const heightHill = heightDesert + heightScale(4);
const heightHilltop = heightHill + heightScale(90);
const heightHillArchers = (heightHilltop + heightHill) / 2;
const heightOffsetPath = heightScale(-2.5);
const heightOffsetWalls = heightScale(2.5);
const heightOffsetStatue = heightScale(2.5);

g_Map.log("Flattening land");
createArea(
	new MapBoundsPlacer(),
	new ElevationPainter(heightDesert),
	new HeightConstraint(-Infinity, heightDesert));

// Fertile land
paintRiver({
	"parallel": true,
	"start": new Vector2D(mapBounds.left, mapBounds.bottom).rotateAround(-riverAngle, mapCenter),
	"end": new Vector2D(mapBounds.right, mapBounds.bottom).rotateAround(-riverAngle, mapCenter),
	"width": fractionToTiles(0.65),
	"fadeDist": 8,
	"deviation": 0,
	"heightLand": heightDesert,
	"heightRiverbed": heightFertileLand,
	"meanderShort": 40,
	"meanderLong": 0,
	"waterFunc": (position, height, riverFraction) => {
		createTerrain(tGrass).place(position);
		clFertileLand.add(position);
	},
	"landFunc": (position, shoreDist1, shoreDist2) => {

		for (let riv of layoutFertileLandTextures)
			if (riv.left < +shoreDist1 && +shoreDist1 < riv.right ||
			    riv.left < -shoreDist2 && -shoreDist2 < riv.right)
			{
				riv.tileClass.add(position);
				riv.terrain.place(position);
			}
	}
});

// Nile
paintRiver({
	"parallel": true,
	"start": new Vector2D(mapBounds.left, mapBounds.bottom).rotateAround(-riverAngle, mapCenter),
	"end": new Vector2D(mapBounds.right, mapBounds.bottom).rotateAround(-riverAngle, mapCenter),
	"width": fractionToTiles(0.2),
	"fadeDist": 4,
	"deviation": 0,
	"heightLand": heightFertileLand,
	"heightRiverbed": heightSeaGround,
	"meanderShort": 40,
	"meanderLong": 0
});
Engine.SetProgress(30);

g_Map.log("Computing player locations");
const playerIDs = primeSortAllPlayers();
const playerPosition = playerPlacementCustomAngle(
	fractionToTiles(0.38),
	mapCenter,
	i => Math.PI * (-0.42 / numPlayers * (i + i % 2) - (i % 2) / 2))[0];

if (!isNomad())
{
	g_Map.log("Marking player positions");
	for (let position of playerPosition)
		addCivicCenterAreaToClass(position, clPlayer);
}

g_Map.log("Marking water");
createArea(
		new MapBoundsPlacer(),
		[
			new TileClassPainter(clWater),
			new TileClassUnPainter(clFertileLand)
		],
		new HeightConstraint(-Infinity, heightWaterLevel));

g_Map.log("Marking desert");
createArea(
	new MapBoundsPlacer(),
	new TileClassPainter(clDesert),
	[
		new HeightConstraint(-Infinity, heightHill),
		avoidClasses(clWater, 0, clFertileLand, 0)
	]);

g_Map.log("Finding possible irrigation canal locations");
var irrigationCanalAreas = [];
for (let i = 0; i < 30; ++i)
{
	let x = fractionToTiles(randFloat(0, 1));
	irrigationCanalAreas.push(
		createArea(
			new PathPlacer(
				new Vector2D(x, mapBounds.bottom).rotateAround(-riverAngle, mapCenter),
				new Vector2D(x, mapBounds.top).rotateAround(-riverAngle, mapCenter),
				3,
				0,
				10,
				0.1,
				0.01,
				Infinity),
			undefined,
			avoidClasses(clDesert, 2)));
}

g_Map.log("Creating irrigation canals");
var irrigationCanalLocations = [];
for (let area of irrigationCanalAreas)
	{
		if (!area.getPoints().length ||
		    area.getPoints().some(point => !avoidClasses(clPlayer, scaleByMapSize(8, 13), clIrrigationCanal, scaleByMapSize(15, 25)).allows(point)))
			continue;

		irrigationCanalLocations.push(pickRandom(area.getPoints()).clone().rotateAround(riverAngle, mapCenter).x);
		createArea(
			new MapBoundsPlacer(),
			[
				new SmoothElevationPainter(ELEVATION_SET, heightIrrigationCanal, 1),
				new TileClassPainter(clIrrigationCanal)
			],
			[new StayAreasConstraint([area]), new HeightConstraint(heightIrrigationCanal, heightDesert)]);
	}

g_Map.log("Creating passages");
irrigationCanalLocations.sort((a, b) => a - b);
for (let i = 0; i < irrigationCanalLocations.length; ++i)
{
	let previous = i == 0 ? mapBounds.left : irrigationCanalLocations[i - 1];
	let next = i == irrigationCanalLocations.length - 1 ? mapBounds.right : irrigationCanalLocations[i + 1];

	let x1 = (irrigationCanalLocations[i] + previous) / 2;
	let x2 = (irrigationCanalLocations[i] + next) / 2;
	let y;

	// The passages should be at different locations, so that enemies can't attack each other easily
	for (let tries = 0; tries < 100; ++tries)
	{
		y = randIntInclusive(0, mapCenter.y);
		let pos = new Vector2D((x1 + x2) / 2, y).rotateAround(-riverAngle, mapCenter).round();

		if (g_Map.validTilePassable(new Vector2D(pos.x, pos.y)) &&
		    avoidClasses(clDesert, 12).allows(pos) &&
		    new HeightConstraint(heightIrrigationCanal, heightFertileLand).allows(pos))
			break;
	}

	createPassage({
		"start": new Vector2D(x1, y).rotateAround(-riverAngle, mapCenter),
		"end": new Vector2D(x2, y).rotateAround(-riverAngle, mapCenter),
		"startHeight": heightPassage,
		"endHeight": heightPassage,
		"constraints": [new HeightConstraint(-Infinity, heightPassage), stayClasses(clFertileLand, 2)],
		"tileClass": clPassage,
		"startWidth": 10,
		"endWidth": 10,
		"smoothWidth": 2
	});
}
Engine.SetProgress(40);

g_Map.log("Marking hill");
createArea(
	new MapBoundsPlacer(),
	new TileClassPainter(clHill),
	new HeightConstraint(heightHill, Infinity));

g_Map.log("Marking water");
const areaWater = createArea(
	new MapBoundsPlacer(),
	new TileClassPainter(clWater),
	new HeightConstraint(-Infinity, heightWaterLevel));

g_Map.log("Painting water and shoreline");
createArea(
	new MapBoundsPlacer(),
	new TerrainPainter(tWater),
	new HeightConstraint(-Infinity, heightShoreline));

g_Map.log("Painting hill");
const areaHill = createArea(
	new MapBoundsPlacer(),
	new TerrainPainter(tHillGround),
	new HeightConstraint(heightHill, Infinity));

g_Map.log("Painting hilltop");
const areaHilltop = createArea(
	new MapBoundsPlacer(),
	new TerrainPainter(tHilltop),
	[
		new HeightConstraint(heightHilltop, Infinity),
		new SlopeConstraint(-Infinity, 2)
	]);

Engine.SetProgress(50);

for (let i = 0; i < numPlayers; ++i)
{
	let isDesert = clDesert.has(playerPosition[i]);
	placePlayerBase({
		"playerID": playerIDs[i],
		"playerPosition": playerPosition[i],
		"PlayerTileClass": clPlayer,
		"BaseResourceClass": clBaseResource,
		"baseResourceConstraint": avoidClasses(clPlayer, 4, clWater, 4),
		"Walls": mapSize <= 256 || getDifficulty() >= 3 ? "towers" : "walls",
		"CityPatch": {
			"outerTerrain": isDesert ? tRoadDesert : tRoadFertileLand,
			"innerTerrain": isDesert ? tRoadDesert : tRoadFertileLand
		},
		"Chicken": {
			"template": oGazelle,
			"distance": 15,
			"minGroupDistance": 2,
			"maxGroupDistance": 4,
			"minGroupCount": 2,
			"maxGroupCount": 3
		},
		"Berries": {
			"template": isDesert ? oBerryBushDesert : oBerryBushGrapes
		},
		"Mines": {
			"types": [
				{ "template": oMetalLarge },
				{ "template": oStoneLarge }
			]
		},
		"Trees": {
			"template": isDesert ? oAcacia : pickRandom(oPalms),
			"count": isDesert ? scaleByMapSize(5, 10) : scaleByMapSize(15, 30)
		},
		"Treasures": {
			"types":
			[
				{
					"template": oWoodTreasure,
					"count": isDesert ? 4 : 0
				},
				{
					"template": oStoneTreasure,
					"count": isDesert ? 1 : 0
				},
				{
					"template": oMetalTreasure,
					"count": isDesert ? 1 : 0
				}
			]
		},
		"Decoratives": {
			"template": isDesert ? aRock : pickRandom(aBushesFertileLand)
		}
	});
}

g_Map.log("Placing pyramids");
const areaPyramids = createArea(new DiskPlacer(scaleByMapSize(5, 14), positionPyramids));
// Retry loops are needed due to the self-avoidance
createObjectGroupsByAreas(
	new SimpleGroup(
		[new RandomObject(
			["uncapturable|" + oPyramidLarge, "uncapturable|" + oPyramidSmall],
			scaleByMapSize(1, 6),
			scaleByMapSize(2, 8),
			scaleByMapSize(6, 8),
			scaleByMapSize(6, 14),
			Math.PI * 1.35,
			Math.PI * 1.5,
			scaleByMapSize(6, 8))],
		true,
		clPyramid),
	0,
	undefined,
	1,
	50,
	[areaPyramids]);

Engine.SetProgress(60);

// The city is a circle segment of this maximum size
g_Map.log("Computing city grid");
var gridCenter = new Vector2D(0, fractionToTiles(0.3)).rotate(-riverAngle).add(mapCenter).round();
var gridMaxAngle = scaleByMapSize(Math.PI / 3, Math.PI);
var gridStartAngle = -Math.PI / 2 -gridMaxAngle / 2 + riverAngle;
var gridRadius = y => hillRadius + 18 * y;

var gridPointsX = layoutKushTemples.length;
var gridPointsY = Math.floor(scaleByMapSize(2, 4));
var gridPointXCenter = Math.floor(gridPointsX / 2);
var gridPointYCenter = Math.floor(gridPointsY / 2);

// Maps from grid position to map position
var cityGridPosition = [];
var cityGridAngle = [];
for (let y = 0; y < gridPointsY; ++y)
	[cityGridPosition[y], cityGridAngle[y]] = distributePointsOnCircularSegment(
		gridPointsX, gridMaxAngle, gridStartAngle, gridRadius(y), gridCenter);

g_Map.log("Marking city path crossings");
for (let y in cityGridPosition)
	for (let x in cityGridPosition[y])
	{
		cityGridPosition[y][x].round();
		createArea(
			new DiskPlacer(pathWidth, cityGridPosition[y][x]),
			[
				new TileClassPainter(clPath),
				new TileClassPainter(clPathCrossing)
			]);
	}

g_Map.log("Marking horizontal city paths");
for (let y = 0; y < gridPointsY; ++y)
	for (let x = 1; x < gridPointsX; ++x)
	{
		let width = y == gridPointYCenter ? pathWidthSecondary : pathWidth;
		createArea(
			new PathPlacer(cityGridPosition[y][x - 1], cityGridPosition[y][x], width, 0, 8, 0, 0, Infinity),
			new TileClassPainter(clPath));
	}

g_Map.log("Marking vertical city paths");
for (let y = 1; y < gridPointsY; ++y)
	for (let x = 0; x < gridPointsX; ++x)
	{
		let width =
			Math.abs(x - gridPointXCenter) == 0 ?
				pathWidthCenter :
			Math.abs(x - gridPointXCenter) == 1 ?
				pathWidthSecondary :
				pathWidth;

		createArea(
			new PathPlacer(cityGridPosition[y - 1][x], cityGridPosition[y][x], width, 0, 8, 0, 0, Infinity),
			new TileClassPainter(clPath));
	}
Engine.SetProgress(70);

g_Map.log("Placing kushite temples");
var entitiesTemples = [];
var templePosition = [];
for (let i = 0; i < layoutKushTemples.length; ++i)
{
	let x = i + (gridPointsX - layoutKushTemples.length) / 2;
	templePosition[i] = Vector2D.add(cityGridPosition[0][x], layoutKushTemples[i].pathOffset.rotate(-Math.PI / 2 - cityGridAngle[0][x]));
	entitiesTemples[i] = g_Map.placeEntityPassable(layoutKushTemples[i].template, 0, templePosition[i], cityGridAngle[0][x]);
}

g_Map.log("Marking temple area");
createArea(
	new EntitiesObstructionPlacer(entitiesTemples, 0, Infinity),
	new TileClassPainter(clTemple));

g_Map.log("Smoothing temple ground");
createArea(
	new MapBoundsPlacer(),
	new ElevationBlendingPainter(heightDesert, 0.8),
	new NearTileClassConstraint(clTemple, 0));

g_Map.log("Painting cliffs");
createArea(
	new MapBoundsPlacer(),
	[
		new TerrainPainter(tHillCliff),
		new TileClassPainter(clCliff)
	],
	[
		stayClasses(clHill, 0),
		new SlopeConstraint(2, Infinity)
	]);

g_Map.log("Painting temple ground");
createArea(
	new MapBoundsPlacer(),
	new TerrainPainter(tPathWild),
	[
		new NearTileClassConstraint(clTemple, 1),
		avoidClasses(clPath, 0, clCliff, 1)
	]);

g_Map.log("Placing lion statues in the central path");
var statueCount = scaleByMapSize(10, 40);
var centralPathStart = cityGridPosition[0][gridPointXCenter];
var centralPathLength = centralPathStart.distanceTo(cityGridPosition[gridPointsY - 1][gridPointXCenter]);
var centralPathAngle = cityGridAngle[0][gridPointXCenter];
for (let i = 0; i < 2; ++i)
	for (let stat = 0; stat < statueCount; ++stat)
	{
		let start = new Vector2D(0, pathWidthCenter * 3/4 * (i - 0.5)).rotate(centralPathAngle).add(centralPathStart);
		let position = new Vector2D(centralPathLength, 0).mult(stat / statueCount).rotate(-centralPathAngle).add(start).add(new Vector2D(0.5, 0.5));

		if (!avoidClasses(clPathCrossing, 2).allows(position))
			continue;

		g_Map.placeEntityPassable(pickRandom(aStatues), 0, position, centralPathAngle - Math.PI * (i + 0.5));
		clPathStatues.add(position.round());
	}

g_Map.log("Placing guardian infantry in the central path");
var centralChampionsCount = scaleByMapSize(2, 40);
for (let i = 0; i < 2; ++i)
	for (let champ = 0; champ < centralChampionsCount; ++champ)
	{
		let start = new Vector2D(0, pathWidthCenter * 1/2 * (i - 0.5)).rotate(-centralPathAngle).add(centralPathStart);
		let position = new Vector2D(centralPathLength, 0).mult(champ / centralChampionsCount).rotate(-centralPathAngle).add(start).add(new Vector2D(0.5, 0.5));

		if (!avoidClasses(clPathCrossing, 2).allows(position))
			continue;

		g_Map.placeEntityPassable(pickRandom(oKushChampions), 0, position, centralPathAngle - Math.PI * (i - 0.5));
		clPathStatues.add(position.round());
	}

g_Map.log("Placing kushite statues in the secondary paths");
for (let x of [gridPointXCenter - 1, gridPointXCenter + 1])
{
	g_Map.placeEntityAnywhere(aStatueKush, 0, cityGridPosition[gridPointYCenter][x], cityGridAngle[gridPointYCenter][x]);
	clPathStatues.add(cityGridPosition[gridPointYCenter][x]);
}

g_Map.log("Creating ritual place near the wonder");
var ritualPosition = Vector2D.average([
	templePosition[Math.floor(templePosition.length / 2) - 1],
	templePosition[Math.ceil(templePosition.length / 2) - 1],
	cityGridPosition[0][gridPointXCenter],
	cityGridPosition[0][gridPointXCenter - 1]
]).round();

var ritualAngle = (cityGridAngle[0][gridPointXCenter] + cityGridAngle[0][gridPointXCenter - 1]) / 2 + Math.PI / 2;

g_Map.placeEntityPassable(aStatueKush, 0, ritualPosition, ritualAngle - Math.PI / 2);

createArea(
	new DiskPlacer(scaleByMapSize(4, 6), ritualPosition),
	[
		new LayeredPainter([tPathWild, tPath], [1]),
		new SmoothElevationPainter(ELEVATION_MODIFY, heightOffsetPath, 2),
		new TileClassPainter(clRitualPlace)
	],
	avoidClasses(clCliff, 1));

createArea(
	new DiskPlacer(0, new Vector2D(-1, -1).add(ritualPosition)),
	new ElevationPainter(heightDesert + heightOffsetStatue));

g_Map.log("Placing healers at the ritual place");
var [healerPosition, healerAngle] = distributePointsOnCircularSegment(
	scaleByMapSize(2, 10), Math.PI, ritualAngle, scaleByMapSize(2, 3), ritualPosition);
for (let i = 0; i < healerPosition.length; ++i)
	g_Map.placeEntityPassable(oKushHealer, 0, healerPosition[i], healerAngle[i] + Math.PI);

g_Map.log("Placing statues at the ritual place");
var [statuePosition, statueAngle] = distributePointsOnCircularSegment(
	scaleByMapSize(4, 8), Math.PI, ritualAngle, scaleByMapSize(3, 4), ritualPosition);
for (let i = 0; i < statuePosition.length; ++i)
	g_Map.placeEntityPassable(pickRandom(aStatues), 0, statuePosition[i], statueAngle[i] + Math.PI);

g_Map.log("Placing palms at the ritual place");
var [palmPosition, palmAngle] = distributePointsOnCircularSegment(
	scaleByMapSize(6, 16), Math.PI, ritualAngle, scaleByMapSize(4, 5), ritualPosition);
for (let i = 0; i < palmPosition.length; ++i)
	if (avoidClasses(clTemple, 1).allows(palmPosition[i]))
		g_Map.placeEntityPassable(oPalmPath, 0, palmPosition[i], randomAngle());

g_Map.log("Painting city paths");
var areaPaths = createArea(
	new MapBoundsPlacer(),
	[
		new LayeredPainter([tPathWild, tPath], [1]),
		new SmoothElevationPainter(ELEVATION_MODIFY, heightOffsetPath, 1)
	],
	stayClasses(clPath, 0));

g_Map.log("Placing triggerpoints on city paths");
createObjectGroupsByAreas(
	new SimpleGroup([new SimpleObject(oTriggerPointCityPath, 1, 1, 0, 0)], true, clTriggerPointCityPath),
	0,
	[avoidClasses(clTriggerPointCityPath, 8), stayClasses(clPathCrossing, 2)],
	scaleByMapSize(20, 100),
	30,
	[areaPaths]);

g_Map.log("Placing city districts");
for (let y = 1; y < gridPointsY; ++y)
	for (let x = 1; x < gridPointsX; ++x)
	createArea(
		new ConvexPolygonPlacer([cityGridPosition[y - 1][x - 1], cityGridPosition[y - 1][x], cityGridPosition[y][x - 1], cityGridPosition[y][x]], Infinity),
		[
			new TerrainPainter(tRoadDesert),
			new CityPainter(layoutKushCity, (-cityGridAngle[y][x - 1] - cityGridAngle[y][x]) / 2, 0),
			new TileClassPainter(clCity)
		],
		new StaticConstraint(avoidClasses(clPath, 0)));

if (placeNapataWall)
{
	g_Map.log("Placing front walls");
	let wallGridMaxAngleSummand = Math.PI / 32;
	let wallGridStartAngle = gridStartAngle - wallGridMaxAngleSummand / 2;
	let wallGridRadiusFront = gridRadius(gridPointsY - 1) + pathWidth - 1;
	let wallGridMaxAngleFront = gridMaxAngle + wallGridMaxAngleSummand;
	let entitiesWalls = placeCircularWall(
		gridCenter,
		wallGridRadiusFront,
		["tower", "short", "tower", "gate", "tower", "medium", "tower", "short"],
		"napata",
		0,
		wallGridStartAngle,
		wallGridMaxAngleFront,
		true,
		0,
		0);

	g_Map.log("Placing side and back walls");
	let wallGridRadiusBack = hillRadius - scaleByMapSize(15, 25);
	let wallGridMaxAngleBack = gridMaxAngle + wallGridMaxAngleSummand;
	let wallGridPositionFront = distributePointsOnCircularSegment(gridPointsX, wallGridMaxAngleBack, wallGridStartAngle, wallGridRadiusFront, gridCenter)[0];
	let wallGridPositionBack = distributePointsOnCircularSegment(gridPointsX, wallGridMaxAngleBack, wallGridStartAngle, wallGridRadiusBack, gridCenter)[0];
	let wallGridPosition = [wallGridPositionFront[0], ...wallGridPositionBack, wallGridPositionFront[wallGridPositionFront.length - 1]];
	for (let x = 1; x < wallGridPosition.length; ++x)
		entitiesWalls = entitiesWalls.concat(
			placeLinearWall(
				wallGridPosition[x - 1],
				wallGridPosition[x],
				["tower", "gate", "tower", "short", "tower", "short", "tower"],
				"napata",
				0,
				false,
				avoidClasses(clHill, 0, clTemple, 0)));

	g_Map.log("Marking walls");
	createArea(
		new EntitiesObstructionPlacer(entitiesWalls, 0, Infinity),
		new TileClassPainter(clWall));

	g_Map.log("Marking gates");
	let entitiesGates = entitiesWalls.filter(entity => entity.templateName.endsWith(oWallGate));
	createArea(
		new EntitiesObstructionPlacer(entitiesGates, 0, Infinity),
		new TileClassPainter(clGate));

	g_Map.log("Painting wall terrain");
	createArea(
		new MapBoundsPlacer(),
		[
			new SmoothElevationPainter(ELEVATION_MODIFY, heightOffsetWalls, 2),
			new TerrainPainter(tPathWild)
		],
		[
			new NearTileClassConstraint(clWall, 1),
			avoidClasses(clCliff, 0)
		]);

	g_Map.log("Painting gate terrain");
	for (let entity of entitiesGates)
		createArea(
			new DiskPlacer(pathWidth, entity.GetPosition2D()),
			[
				new LayeredPainter([tPathWild, tPath], [1]),
				new SmoothElevationPainter(ELEVATION_MODIFY, heightOffsetPath, 2),
			],
			[
				avoidClasses(clCliff, 0, clPath, 0, clCity, 0),
				new NearTileClassConstraint(clPath, pathWidth + 1)
			]);
}
Engine.SetProgress(70);

g_Map.log("Marking city bush area");
var areaCityBushes =
	createArea(
		new MapBoundsPlacer(),
		undefined,
		[
			new NearTileClassConstraint(clPath, 1),
			avoidClasses(
				clPath, 0,
				clPyramid, 20,
				clRitualPlace, 8,
				clTemple, 3,
				clWall, 3,
				clTower, 1,
				clFortress, 1,
				clPyramid, 1,
				clHouse, 1,
				clBlacksmith, 1,
				clElephantStables, 1,
				clStable, 1,
				clCivicCenter, 1,
				clBarracks, 1,
				clBlemmyeCamp, 1,
				clNubaVillage, 1,
				clMarket, 1)
		]);

g_Map.log("Marking city palm area");
var areaCityPalms =
	createArea(
		new MapBoundsPlacer(),
		undefined,
		[
			new StayAreasConstraint([areaCityBushes]),
			avoidClasses(clElephantStables, 3)
		]);

g_Map.log("Placing city palms");
createObjectGroupsByAreas(
	new SimpleGroup([new SimpleObject(aPalmPath, 1, 1, 0, 0)], true, clForest),
	0,
	avoidClasses(clForest, 3),
	scaleByMapSize(40, 400),
	15,
	[areaCityPalms]);

g_Map.log("Placing city bushes");
createObjectGroupsByAreas(
	new SimpleGroup([new RandomObject(aBushesCity, 1, 1, 0, 0)], true, clForest),
	0,
	avoidClasses(clForest, 1),
	scaleByMapSize(20, 200),
	15,
	[areaCityBushes]);

if (placeNapataWall)
{
	g_Map.log("Marking wall palm area");
	var areaWallPalms = createArea(
		new MapBoundsPlacer(),
		undefined,
		new StaticConstraint([
			new NearTileClassConstraint(clWall, 2),
			avoidClasses(clPath, 1, clWall, 1, clGate, 3, clTemple, 2, clHill, 6)
		]));

	g_Map.log("Placing wall palms");
	createObjectGroupsByAreas(
		new SimpleGroup([new SimpleObject(oPalmPath, 1, 1, 0, 0)], true, clForest),
		0,
		avoidClasses(clForest, 2),
		scaleByMapSize(40, 200),
		50,
		[areaWallPalms]);
}

createBumps(new StaticConstraint(avoidClasses(clPlayer, 6, clCity, 0, clWater, 2, clHill, 0, clPath, 0, clTemple, 4, clPyramid, 8)), scaleByMapSize(30, 300), 1, 8, 4, 0, 3);
Engine.SetProgress(75);

g_Map.log("Setting up common constraints");
const stayDesert = new StaticConstraint(stayClasses(clDesert, 0));
const stayFertileLand = new StaticConstraint(stayClasses(clFertileLand, 0));
const nearWater = new NearTileClassConstraint(clWater, 3);
var avoidCollisions = new AndConstraint(
	[
		new StaticConstraint(avoidClasses(
			clCliff, 0, clHill, 0, clPlayer, 15, clWater, 1, clPath, 2, clRitualPlace, 10,
			clTemple, 4, clPyramid, 7, clCity, 4, clWall, 4, clGate, 8)),
		avoidClasses(clForest, 1, clRock, 4, clMetal, 4, clFood, 6, clSoldier, 1, clTreasure, 1)
	]);

g_Map.log("Setting up common areas");
const areaDesert = createArea(new MapBoundsPlacer(), undefined, stayDesert);
const areaFertileLand = createArea(new MapBoundsPlacer(), undefined, stayFertileLand);

createForests(
	[tForestFloorFertile, tForestFloorFertile, tForestFloorFertile, pForestPalms, pForestPalms],
	[stayFertileLand, avoidClasses(clForest, 15), new StaticConstraint([avoidClasses(clWater, 2), avoidCollisions])],
	clForest,
	scaleByMapSize(250, 2000));

const avoidCollisionsMines = new StaticConstraint([
	isNomad() ? new NullConstraint() : avoidClasses(clFertileLand, 10),
	avoidClasses(
		clWater, 4, clCliff, 4, clCity, 4, clRitualPlace, 10,
		clPlayer, 20, clForest, 4, clPyramid, 6, clTemple, 4, clPath, 4, clGate, 8)]);

g_Map.log("Creating stone mines");
createMines(
	[
		[new SimpleObject(oStoneSmall, 0, 2, 0, 4, 0, 2 * Math.PI, 1), new SimpleObject(oStoneLarge, 1, 1, 0, 4, 0, 2 * Math.PI, 4)],
		[new SimpleObject(oStoneSmall, 2, 5, 1, 3, 0, 2 * Math.PI, 1)]
	],
	[avoidCollisionsMines, avoidClasses(clRock, 10)],
	clRock,
	scaleByMapSize(8, 26));

g_Map.log("Creating metal mines");
createMines(
	[
		[new SimpleObject(oMetalSmall, 0, 2, 0, 4, 0, 2 * Math.PI, 1), new SimpleObject(oMetalLarge, 1, 1, 0, 4, 0, 2 * Math.PI, 4)],
		[new SimpleObject(oMetalSmall, 2, 5, 1, 3, 0, 2 * Math.PI, 1)]
	],
	[avoidCollisionsMines, avoidClasses(clMetal, 10, clRock, 5)],
	clMetal,
	scaleByMapSize(8, 26));

g_Map.log("Placing triggerpoints for attackers");
createObjectGroups(
	new SimpleGroup([new SimpleObject(oTriggerPointAttackerPatrol, 1, 1, 0, 0)], true, clTriggerPointMap),
	0,
	[avoidClasses(clCity, 8, clCliff, 4, clHill, 4, clWater, 0, clWall, 2, clForest, 1, clRock, 4, clMetal, 4, clTriggerPointMap, 15)],
	scaleByMapSize(20, 100),
	30);

g_Map.log("Creating berries");
createObjectGroupsByAreas(
	new SimpleGroup([new SimpleObject(oBerryBushGrapes, 4, 6, 1, 2)], true, clFood),
	0,
	avoidCollisions,
	scaleByMapSize(3, 15),
	50,
	[areaFertileLand]);

g_Map.log("Creating rhinos");
createObjectGroupsByAreas(
	new SimpleGroup([new SimpleObject(oRhino, 1, 1, 0, 1)], true, clFood),
	0,
	avoidCollisions,
	scaleByMapSize(2, 10),
	50,
	[areaDesert]);

g_Map.log("Creating warthogs");
createObjectGroupsByAreas(
	new SimpleGroup([new SimpleObject(oWarthog, 1, 1, 0, 1)], true, clFood),
	0,
	avoidCollisions,
	scaleByMapSize(2, 10),
	50,
	[areaFertileLand]);

g_Map.log("Creating giraffes");
createObjectGroups(
	new SimpleGroup([new SimpleObject(oGiraffe, 2, 3, 2, 4), new SimpleObject(oGiraffeInfant, 2, 3, 2, 4)], true, clFood),
	0,
	avoidCollisions,
	scaleByMapSize(2, 10),
	50);

g_Map.log("Creating gazelles");
createObjectGroups(
	new SimpleGroup([new SimpleObject(oGazelle, 5, 7, 2, 4)], true, clFood),
	0,
	avoidCollisions,
	scaleByMapSize(2, 10),
	50,
	[areaDesert]);

if (!isNomad())
{
	g_Map.log("Creating lions");
	createObjectGroupsByAreas(
		new SimpleGroup([new SimpleObject(oLion, 1, 2, 2, 4), new SimpleObject(oLioness, 2, 3, 2, 4)], true, clFood),
		0,
		[avoidCollisions, avoidClasses(clPlayer, 20)],
		scaleByMapSize(2, 10),
		50,
		[areaDesert]);
}

g_Map.log("Creating elephants");
createObjectGroupsByAreas(
	new SimpleGroup([new SimpleObject(oElephant, 2, 3, 2, 4), new SimpleObject(oElephantInfant, 2, 3, 2, 4)], true, clFood),
	0,
	avoidCollisions,
	scaleByMapSize(2, 10),
	50,
	[areaDesert]);

g_Map.log("Creating crocodiles");
if (!isNomad())
	createObjectGroupsByAreas(
		new SimpleGroup([new SimpleObject(oCrocodile, 2, 3, 3, 5)], true, clFood),
		0,
		[nearWater, avoidCollisions],
		scaleByMapSize(1, 6),
		50,
		[areaFertileLand]);

Engine.SetProgress(85);

g_Map.log("Marking irrigation canal tree area");
var areaIrrigationCanalTrees = createArea(
	new MapBoundsPlacer(),
	undefined,
	[
		nearWater,
		avoidClasses(clPassage, 3),
		avoidCollisions
	]);

g_Map.log("Creating irrigation canal trees");
createObjectGroupsByAreas(
	new SimpleGroup([new RandomObject(oPalms, 1, 1, 1, 1)], true, clForest),
	0,
	avoidClasses(clForest, 1),
	scaleByMapSize(100, 600),
	50,
	[areaIrrigationCanalTrees]);

createStragglerTrees(
	oPalms,
	[stayFertileLand, avoidCollisions],
	clForest,
	scaleByMapSize(50, 400),
	200);

createStragglerTrees(
	[oAcacia],
	[stayDesert, avoidCollisions],
	clForest,
	scaleByMapSize(50, 400),
	200);

g_Map.log("Placing archer groups on the hilltop");
createObjectGroupsByAreas(
	new SimpleGroup([new RandomObject([oKushCitizenArcher, oKushChampionArcher], scaleByMapSize(4, 10), scaleByMapSize(6, 20), 1, 4)], true, clSoldier),
	0,
	new StaticConstraint([avoidClasses(clCliff, 1), new NearTileClassConstraint(clCliff, 5)]),
	scaleByMapSize(1, 5) / 3 * getDifficulty(),
	250,
	[areaHilltop]);

g_Map.log("Placing individual archers on the hill");
createObjectGroupsByAreas(
	new SimpleGroup([new RandomObject([oKushCitizenArcher, oKushChampionArcher], 1, 1, 1, 3)], true, clSoldier),
	0,
	new StaticConstraint([
		new HeightConstraint(heightHillArchers, heightHilltop),
		avoidClasses(clCliff, 1, clSoldier, 1),
		new NearTileClassConstraint(clCliff, 5)
	]),
	scaleByMapSize(8, 100) / 3 * getDifficulty(),
	250,
	[areaHill]);

g_Map.log("Placing siege engines on the hilltop");
createObjectGroupsByAreas(
	new SimpleGroup([new RandomObject(oPtolSiege, 1, 1, 1, 3)], true, clSoldier),
	0,
	new StaticConstraint([new NearTileClassConstraint(clCliff, 5), avoidClasses(clCliff, 1, clSoldier, 1)]),
	scaleByMapSize(1, 6) / 3 * getDifficulty(),
	250,
	[areaHilltop]);

g_Map.log("Placing soldiers near pyramids");
const avoidCollisionsPyramids = new StaticConstraint([avoidCollisions, new NearTileClassConstraint(clPyramid, 10)]);
createObjectGroupsByAreas(
	new SimpleGroup([new SimpleObject(oKushCitizenArcher, 1, 1, 1, 1)], true, clSoldier),
	0,
	avoidCollisionsPyramids,
	scaleByMapSize(3, 8),
	250,
	[areaPyramids]);

g_Map.log("Placing treasures at the pyramid");
createObjectGroupsByAreas(
	new SimpleGroup([new RandomObject(oTreasuresHill, 1, 1, 2, 2)], true, clTreasure),
	0,
	avoidCollisionsPyramids,
	scaleByMapSize(1, 10),
	250,
	[areaPyramids]);

g_Map.log("Placing treasures on the hilltop");
createObjectGroupsByAreas(
	new SimpleGroup([new RandomObject(oTreasuresHill, 1, 1, 2, 2)], true, clTreasure),
	0,
	avoidClasses(clCliff, 1, clTreasure, 1),
	scaleByMapSize(8, 35),
	250,
	[areaHilltop]);

g_Map.log("Placing treasures in the city");
var pathBorderConstraint = new AndConstraint([
	new StaticConstraint([new NearTileClassConstraint(clCity, 1)]),
	avoidClasses(clTreasure, 2, clStatue, 10, clPathStatues, 4, clWall, 2, clForest, 1)
]);
createObjectGroupsByAreas(
	new SimpleGroup([new RandomObject(oTreasuresCity, 1, 1, 0, 2)], true, clTreasure),
	0,
	pathBorderConstraint,
	scaleByMapSize(2, 60),
	500,
	[areaPaths]);

g_Map.log("Placing handcarts on the paths");
createObjectGroupsByAreas(
	new SimpleGroup([new SimpleObject(aHandcart, 1, 1, 1, 1)], true, clDecorative),
	0,
	[pathBorderConstraint, avoidClasses(clDecorative, 10)],
	scaleByMapSize(0, 5),
	250,
	[areaPaths]);

g_Map.log("Placing fence in fertile land");
createObjectGroupsByAreas(
	new SimpleGroup([new SimpleObject(aPlotFence, 1, 1, 1, 1)], true, clDecorative),
	0,
	new StaticConstraint(avoidCollisions, avoidClasses(clWater, 6, clDecorative, 10)),
	scaleByMapSize(1, 10),
	250,
	[areaFertileLand]);

g_Map.log("Creating fish");
createObjectGroups(
	new SimpleGroup([new SimpleObject(oFish, 3, 4, 2, 3)], true, clFood),
	0,
	[new StaticConstraint(stayClasses(clWater, 6)), avoidClasses(clFood, 12)],
	scaleByMapSize(20, 120),
	50);

Engine.SetProgress(95);

avoidCollisions = new StaticConstraint(avoidCollisions);

createDecoration(
	aBushesDesert.map(bush => [new SimpleObject(bush, 0, 3, 2, 4)]),
	aBushesDesert.map(bush => scaleByMapSize(20, 150) * randIntInclusive(1, 3)),
	[stayDesert, avoidCollisions]);

createDecoration(
	aBushesFertileLand.map(bush => [new SimpleObject(bush, 0, 4, 2, 4)]),
	aBushesFertileLand.map(bush => scaleByMapSize(20, 150) * randIntInclusive(1, 3)),
	[stayFertileLand, avoidCollisions]);

createDecoration(
	[[new SimpleObject(aRock, 0, 4, 2, 4)]],
	[[scaleByMapSize(80, 500)]],
	[stayDesert, avoidCollisions]);

createDecoration(
	aBushesFertileLand.map(bush => [new SimpleObject(bush, 0, 3, 2, 4)]),
	aBushesFertileLand.map(bush => scaleByMapSize(100, 800)),
	[new HeightConstraint(heightWaterLevel, heightShoreline), avoidCollisions]);

g_Map.log("Creating reeds");
createObjectGroupsByAreas(
	new SimpleGroup([new RandomObject(aWaterDecoratives, 2, 4, 1, 2)], true),
	0,
	new StaticConstraint(new NearTileClassConstraint(clFertileLand, 4)),
	scaleByMapSize(50, 400),
	20,
	[areaWater]);

g_Map.log("Creating hawk");
for (let i = 0; i < scaleByMapSize(0, 2); ++i)
	g_Map.placeEntityAnywhere(oHawk, 0, mapCenter, randomAngle());

placePlayersNomad(clPlayer, [avoidClasses(clHill, 15, clSoldier, 15, clCity, 15), avoidCollisions]);

setWindAngle(-0.43);
setWaterHeight(heightWaterLevel + SEA_LEVEL);
setWaterTint(0.161, 0.286, 0.353);
setWaterColor(0.129, 0.176, 0.259);
setWaterWaviness(8);
setWaterMurkiness(0.87);
setWaterType("lake");

setTerrainAmbientColor(0.58, 0.443, 0.353);

setSunColor(0.733, 0.746, 0.574);
setSunRotation(Math.PI / 2 * randFloat(-1, 1));
setSunElevation(Math.PI / 7);

setFogFactor(0);
setFogThickness(0);
setFogColor(0.69, 0.616, 0.541);

setPPEffect("hdr");
setPPContrast(0.67);
setPPSaturation(0.42);
setPPBloom(0.23);

g_Map.ExportMap();
