Engine.LoadLibrary("rmgen");

// Spawn ships away from the shoreline, but patrol close to the shoreline
const triggerPointShipSpawn = "trigger/trigger_point_A";
const triggerPointShipPatrol = "trigger/trigger_point_B";
const triggerPointShipUnloadLeft = "trigger/trigger_point_C";
const triggerPointShipUnloadRight = "trigger/trigger_point_D";
const triggerPointLandPatrolLeft = "trigger/trigger_point_E";
const triggerPointLandPatrolRight = "trigger/trigger_point_F";
const triggerPointCCAttackerPatrolLeft = "trigger/trigger_point_G";
const triggerPointCCAttackerPatrolRight = "trigger/trigger_point_H";
const triggerPointRiverDirection = "trigger/trigger_point_I";

const tPrimary = ["temp_grass_aut", "temp_grass_plants_aut", "temp_grass_c_aut", "temp_grass_d_aut"];
const tRoad = "steppe_river_rocks";
const tIsland = ["temp_grass_long_b_aut", "temp_grass_plants_aut", "temp_forestfloor_aut"];
const tCliff = "temp_cliff_a";
const tForestFloor = "temp_forestfloor_aut";
const tGrass = "medit_shrubs_golden";
const tGrass2 ="grass_mediterranean_dry_1024test";
const tGrass3 = "medit_grass_field_b";
const tShore = "temp_dirt_gravel_b";
const tWater = "steppe_river_rocks_wet";
const tSeaDepths = "medit_sea_depths";

const oBerryBush = "gaia/flora_bush_berry";
const oDeer = "gaia/fauna_deer";
const oFish = "gaia/fauna_fish";
const oSheep = "gaia/fauna_sheep";
const oGoat = "gaia/fauna_goat";
const oWolf = "gaia/fauna_wolf";
const oHawk = "gaia/fauna_hawk";
const oRabbit = "gaia/fauna_rabbit";
const oBoar = "gaia/fauna_boar";
const oBear = "gaia/fauna_bear";
const oStoneLarge = "gaia/geology_stonemine_temperate_quarry";
const oStoneRuins = "gaia/ruins/standing_stone";
const oMetalLarge = "gaia/geology_metal_mediterranean_slabs";
const oApple = "gaia/flora_tree_apple";
const oAcacia = "gaia/flora_tree_acacia";
const oOak = "gaia/flora_tree_oak_aut";
const oOak2 = "gaia/flora_tree_oak_aut_new";
const oOak3 = "gaia/flora_tree_oak_dead";
const oOak4 = "gaia/flora_tree_oak";
const oPopolar = "gaia/flora_tree_poplar_lombardy";
const oBeech = "gaia/flora_tree_euro_beech_aut";
const oBeech2 = "gaia/flora_tree_euro_beech";
const oTreasures = [
	"gaia/treasure/food_barrel",
	"gaia/treasure/food_bin",
	"gaia/treasure/stone",
	"gaia/treasure/wood",
	"gaia/treasure/metal"
];

// Disable capturing on all parts of the village except the
// civic center and buildings occurring outside of the village
const oCivicCenter = "structures/gaul_civil_centre";
const oTower = "structures/gaul_defense_tower";
const oOutpost = "structures/gaul_outpost";

const oTemple = "uncapturable|structures/gaul_temple";
const oTavern = "uncapturable|structures/gaul_tavern";
const oHouse = "uncapturable|structures/gaul_house";
const oLongHouse = "uncapturable|other/celt_longhouse";
const oHut = "uncapturable|other/celt_hut";
const oSentryTower = "uncapturable|structures/gaul_sentry_tower";
const oWatchTower = "uncapturable|other/palisades_rocks_watchtower";

const oPalisadeTallSpikes = "uncapturable|other/palisades_tall_spikes";
const oPalisadeAngleSpikes = "uncapturable|other/palisades_angle_spike";
const oPalisadeCurve = "uncapturable|other/palisades_rocks_curve";
const oPalisadeShort = "uncapturable|other/palisades_rocks_short";
const oPalisadeMedium = "uncapturable|other/palisades_rocks_medium";
const oPalisadeLong = "uncapturable|other/palisades_rocks_long";
const oPalisadeGate = "uncapturable|other/palisades_rocks_gate";
const oPalisadePillar = "uncapturable|other/palisades_rocks_tower";

const oFemale = "units/gaul_support_female_citizen";
const oHealer = "units/gaul_support_healer_b";
const oSkirmisher = "units/gaul_infantry_javelinist_b";
const oNakedFanatic = "units/gaul_champion_fanatic";

const aBush1 = "actor|props/flora/bush_tempe_sm.xml";
const aBush2 = "actor|props/flora/bush_tempe_me.xml";
const aBush3 = "actor|props/flora/bush_tempe_la.xml";
const aBush4 = "actor|props/flora/bush_tempe_me.xml";
const aRock1 = "actor|geology/stone_granite_med.xml";
const aRock2 = "actor|geology/stone_granite_boulder.xml";
const aRock3 = "actor|geology/stone_granite_greek_boulder.xml";
const aRock4 = "actor|geology/stonemine_alpine_a.xml";
const aFerns = "actor|props/flora/ferns.xml";
const aBucket = "actor|props/structures/celts/blacksmith_bucket";
const aBarrel = "actor|props/structures/gauls/storehouse_barrel_b";
const aTartan = "actor|props/structures/celts/tartan_a";
const aWheel = "actor|props/special/eyecandy/wheel_laying";
const aWell = "actor|props/special/eyecandy/well_1_b";
const aWoodcord = "actor|props/special/eyecandy/woodcord";
const aWaterLog = "actor|props/flora/water_log.xml";
const aCampfire = "actor|props/special/eyecandy/campfire";
const aBench = "actor|props/special/eyecandy/bench_1";
const aRug = "actor|props/special/eyecandy/rug_stand_iber";

const treeTypes = [oOak, oOak2, oOak3, oOak4, oBeech, oBeech2, oAcacia];

const pForest1 = [
	tForestFloor,
	tForestFloor + TERRAIN_SEPARATOR + oOak,
	tForestFloor + TERRAIN_SEPARATOR + oOak2,
	tForestFloor + TERRAIN_SEPARATOR + oOak3,
	tForestFloor + TERRAIN_SEPARATOR + oOak4,
	tForestFloor
];

const pForest2 = [
	tForestFloor,
	tForestFloor + TERRAIN_SEPARATOR + oPopolar,
	tForestFloor + TERRAIN_SEPARATOR + oBeech,
	tForestFloor + TERRAIN_SEPARATOR + oBeech2,
	tForestFloor + TERRAIN_SEPARATOR + oAcacia,
	tForestFloor
];

const smallMapSize = 192;
const mediumMapSize = 256;
const normalMapSize = 320;

// Minimum distance from the map border to ship ungarrison points
const ShorelineDistance = 15;

const heightSeaGround = -3;
const heightShore = 1;
const heightLand = 3;
const heightPath = 5;
const heightIsland = 6;

var g_Map = new RandomMap(heightLand, tPrimary);

const numPlayers = getNumPlayers();
const mapSize = g_Map.getSize();
const mapCenter = g_Map.getCenter();
const mapBounds = g_Map.getBounds();

var clPlayer = g_Map.createTileClass();
var clForest = g_Map.createTileClass();
var clWater = g_Map.createTileClass();
var clLand = [g_Map.createTileClass(), g_Map.createTileClass()];
var clPatrolPointSiegeEngine = [g_Map.createTileClass(), g_Map.createTileClass()];
var clPatrolPointSoldier = [g_Map.createTileClass(), g_Map.createTileClass()];
var clShore = [g_Map.createTileClass(), g_Map.createTileClass()];
var clShoreUngarrisonPoint = [g_Map.createTileClass(), g_Map.createTileClass()];
var clShip = g_Map.createTileClass();
var clShipPatrol = g_Map.createTileClass();
var clDirt = g_Map.createTileClass();
var clRock = g_Map.createTileClass();
var clMetal = g_Map.createTileClass();
var clFood = g_Map.createTileClass();
var clBaseResource = g_Map.createTileClass();
var clHill = g_Map.createTileClass();
var clIsland = g_Map.createTileClass();
var clTreasure = g_Map.createTileClass();
var clWaterLog = g_Map.createTileClass();
var clGauls = g_Map.createTileClass();
var clTower = g_Map.createTileClass();
var clOutpost = g_Map.createTileClass();
var clPath = g_Map.createTileClass();
var clRitualPlace = g_Map.createTileClass();

var startAngle = randomAngle();
var waterWidth = fractionToTiles(0.3);

// How many treasures will be placed near the gallic civic centers
var gallicCCTreasureCount = randIntInclusive(8, 12);

// How many treasures will be placed randomly on the map at most
var randomTreasureCount = randIntInclusive(0, scaleByMapSize(0, 2));

var ritualParticipants = [
	{
		"radius": 0.6,
		"templates": [oFemale],
		"count": 9,
		"angle": Math.PI
	},
	{
		"radius": 0.8,
		"templates": [oSkirmisher, oHealer, oNakedFanatic],
		"count": 15,
		"angle": Math.PI
	},
	{
		"radius": 1,
		"templates": [aBench],
		"count": 10,
		"angle": Math.PI / 2
	},
	{
		"radius": 1.1,
		"templates": [oGoat],
		"count": 7,
		"angle": 0
	},
	{
		"radius": 1.2,
		"templates": [aRug],
		"count": 8,
		"angle": Math.PI
	}
];

g_WallStyles.danubius_village = {
	"house": { "angle": Math.PI, "length": 0, "indent": 4, "bend": 0, "templateName": oHouse },
	"hut": { "angle": Math.PI, "length": 0, "indent": 4, "bend": 0, "templateName": oHut },
	"longhouse": { "angle": Math.PI, "length": 0, "indent": 4, "bend": 0, "templateName": oLongHouse },
	"tavern": { "angle": Math.PI * 3/2, "length": 0, "indent": 4, "bend": 0, "templateName": oTavern },
	"temple": { "angle": Math.PI * 3/2, "length": 0, "indent": 4, "bend": 0, "templateName": oTemple },
	"defense_tower": { "angle": Math.PI / 2, "length": 0, "indent": 4, "bend": 0, "templateName": mapSize >= normalMapSize ? (isNomad() ? oSentryTower : oTower) : oWatchTower },
	"pillar": readyWallElement(oPalisadePillar),
	"gate": readyWallElement(oPalisadeGate),
	"long": readyWallElement(oPalisadeLong),
	"medium": readyWallElement(oPalisadeMedium),
	"short": readyWallElement(oPalisadeShort),
	"cornerIn": readyWallElement(oPalisadeCurve),
	"overlap": 0.05
};

g_WallStyles.danubius_spikes = {
	"spikes_tall": readyWallElement(oPalisadeTallSpikes, "gaia"),
	"spike_single": readyWallElement(oPalisadeAngleSpikes, "gaia"),
	"overlap": 0
};

var fortressDanubiusVillage = new Fortress(
	"Geto-Dacian Tribal Confederation",
	new Array(2).fill([
		"gate", "pillar", "hut", "long", "long",
		"cornerIn", "defense_tower", "long",  "temple", "long",
		"pillar", "house", "long", "short", "pillar", "gate", "pillar", "longhouse", "long", "long",
		"cornerIn", "defense_tower", "long", "tavern", "long", "pillar"
	]).reduce((result, items) => result.concat(items), []));

var palisadeCorner = ["turn_0.25", "spike_single", "turn_0.25"];
var palisadeGate = ["spike_single", "gap_3.6", "spike_single"];
var palisadeWallShort = new Array(3).fill("spikes_tall");
var palisadeWallLong = new Array(5).fill("spikes_tall");
var palisadeSideShort = [...palisadeGate, ...palisadeWallShort, ...palisadeCorner, ...palisadeWallShort];
var palisadeSideLong = [...palisadeGate, ...palisadeWallShort, ...palisadeCorner, ...palisadeWallLong];
var fortressDanubiusSpikes = new Fortress(
	"Spikes Of The Geto-Dacian Tribal Confederation",
	[...palisadeSideLong, ...palisadeSideShort, ...palisadeSideLong, ...palisadeSideShort]);

// Place a gallic village on small maps and larger
var gallicCC = mapSize >= smallMapSize;
if (gallicCC)
{
	g_Map.log("Creating gallic villages");
	let gaulCityRadius = 12;
	let gaulCityBorderDistance = mapSize < mediumMapSize ? 10 : 18;

	// Whether to add a celtic ritual and a path from the gallic city leading to it
	let addCelticRitual = randBool(0.9);

	// One village left and right of the river
	for (let i = 0; i < 2; ++i)
	{
		let civicCenterPosition = new Vector2D(
			i == 0 ? mapBounds.left + gaulCityBorderDistance : mapBounds.right - gaulCityBorderDistance,
			mapCenter.y).rotateAround(startAngle, mapCenter);

		if (addCelticRitual)
		{
			// Don't position the meeting place at the center of the map
			let meetingPlacePosition = new Vector2D(
				i == 0 ? mapBounds.left + waterWidth : mapBounds.right - waterWidth,
				mapCenter.y + fractionToTiles(randFloat(0.1, 0.4)) * (randBool() ? 1 : -1)).rotateAround(startAngle, mapCenter);

			// Radius of the meeting place
			let mRadius = scaleByMapSize(4, 6);

			// Create a path connecting the gallic city with a meeting place at the shoreline.
			// To avoid the path going through the palisade wall, start it at the gate, not at the city center.
			let pathStart = Vector2D.add(civicCenterPosition, new Vector2D(gaulCityRadius * (i == 0 ? 1 : -1), 0).rotate(startAngle));
			createArea(
				new PathPlacer(pathStart, meetingPlacePosition, 4, 0.4, 4, 0.2, 0.05),
				[
					new LayeredPainter([tShore, tRoad, tRoad], [1, 3]),
					new SmoothElevationPainter(ELEVATION_SET, heightPath, 4),
					new TileClassPainter(clPath)
				]);

			// Create the meeting place near the shoreline at the end of the path
			createArea(
				new ClumpPlacer(diskArea(mRadius), 0.6, 0.3, Infinity, meetingPlacePosition),
				[
					new TerrainPainter(tShore),
					new TileClassPainter(clPath),
					new TileClassPainter(clRitualPlace)
				]);

			g_Map.placeEntityAnywhere(aCampfire, 0, meetingPlacePosition, randomAngle());

			for (let participants of ritualParticipants)
			{
				let [positions, angles] = distributePointsOnCircle(participants.count, startAngle, participants.radius * mRadius, meetingPlacePosition);
				for (let i = 0; i < positions.length; ++i)
					g_Map.placeEntityPassable(pickRandom(participants.templates), 0, positions[i], angles[i] + participants.angle);
			}
		}

		g_Map.placeEntityPassable(oCivicCenter, 0, civicCenterPosition, startAngle + BUILDING_ORIENTATION + Math.PI * 3/2 * i);

		// Create the city patch
		createArea(
			new ClumpPlacer(diskArea(gaulCityRadius), 0.6, 0.3, Infinity, civicCenterPosition),
			[
				new TerrainPainter(tShore),
				new TileClassPainter(clGauls)
			]);

		// Place walls and buildings
		placeCustomFortress(civicCenterPosition, fortressDanubiusVillage, "danubius_village", 0, startAngle + Math.PI);
		placeCustomFortress(civicCenterPosition, fortressDanubiusSpikes, "danubius_spikes", 0, startAngle + Math.PI);

		// Place treasure, potentially inside buildings
		for (let i = 0; i < gallicCCTreasureCount; ++i)
			g_Map.placeEntityPassable(
				pickRandom(oTreasures),
				0,
				Vector2D.add(civicCenterPosition, new Vector2D(randFloat(-0.8, 0.8) * gaulCityRadius, 0).rotate(randomAngle())),
				randomAngle());
	}
}
Engine.SetProgress(10);

placePlayerBases({
	"PlayerPlacement": playerPlacementRiver(startAngle, fractionToTiles(0.6)),
	"PlayerTileClass": clPlayer,
	"BaseResourceClass": clBaseResource,
	"Walls": false,
	"CityPatch": {
		"outerTerrain": tShore,
		"innerTerrain": tRoad
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
		],
		"distance": scaleByMapSize(9, 14)
	},
	"Trees": {
		"template": oOak,
		"count": 20,
		"minDist": 10,
		"maxDist": 14
	},
	"Decoratives": {
		"template": aBush1
	}
});
Engine.SetProgress(20);

paintRiver({
	"parallel": true,
	"start": new Vector2D(mapCenter.x, mapBounds.top).rotateAround(startAngle, mapCenter),
	"end": new Vector2D(mapCenter.x, mapBounds.bottom).rotateAround(startAngle, mapCenter),
	"width": waterWidth,
	"fadeDist": scaleByMapSize(6, 25),
	"deviation": 0,
	"heightRiverbed": heightSeaGround,
	"heightLand": heightLand,
	"meanderShort": 30,
	"meanderLong": 0,
	"waterFunc": (position, height, riverFraction) => {
		let origPos = position.clone().rotateAround(-startAngle, mapCenter);
		// Distinguish left and right shoreline
		if (0 < height && height < 1 &&
			origPos.y > ShorelineDistance && origPos.y < mapSize - ShorelineDistance)
			clShore[origPos.x < mapCenter.x ? 0 : 1].add(position);
	},
	"landFunc": (position, shoreDist1, shoreDist2) => {

		if (shoreDist1 > 0)
			clLand[0].add(position);

		if (shoreDist2 < 0)
			clLand[1].add(position);
	}
});
Engine.SetProgress(30);

paintTileClassBasedOnHeight(-Infinity, 0.7, Elevation_ExcludeMin_ExcludeMax, clWater);

var areasLand = [0, 1].map(i =>
	createArea(
		new MapBoundsPlacer(),
		undefined,
		stayClasses(clLand[i], 0)));

var areasWater =
	[createArea(
		new MapBoundsPlacer(),
		undefined,
		new HeightConstraint(-Infinity, heightLand))];

g_Map.log("Creating shores");
paintTerrainBasedOnHeight(-Infinity, heightShore, 0, tWater);
paintTerrainBasedOnHeight(heightShore, heightLand, 0, tShore);
Engine.SetProgress(35);

createBumps(avoidClasses(clPlayer, 6, clWater, 2, clPath, 1, clGauls, 1), scaleByMapSize(30, 300), 1, 8, 4, 0, 3);
Engine.SetProgress(40);

if (randBool())
	createHills(
		[tCliff, tCliff, tCliff],
		avoidClasses(clPlayer, 18, clHill, 20, clWater, 2, clGauls, 5, clPath, 1),
		clHill,
		scaleByMapSize(3, 15));
else
	createMountains(
		tCliff,
		avoidClasses(clPlayer, 18, clHill, 20, clWater, 2, clGauls, 5, clPath, 1),
		clHill,
		scaleByMapSize(3, 15));

Engine.SetProgress(45);

var [forestTrees, stragglerTrees] = getTreeCounts(500, 3000, 0.7);
createForests(
	[tForestFloor, tForestFloor, tForestFloor, pForest1, pForest2],
	avoidClasses(clPlayer, 16, clForest, 17, clWater, 5, clHill, 2, clGauls, 5, clPath, 1),
	clForest,
	forestTrees);

Engine.SetProgress(50);

g_Map.log("Creating grass patches");
createLayeredPatches(
	[scaleByMapSize(3, 6), scaleByMapSize(5, 10), scaleByMapSize(8, 21)],
	[[tGrass, tGrass2],[tGrass2, tGrass3], [tGrass3, tGrass]],
	[1, 1],
	avoidClasses(clForest, 0, clPlayer, 10, clWater, 2, clDirt, 2, clHill, 1, clGauls, 5, clPath, 1),
	scaleByMapSize(15, 45),
	clDirt);

Engine.SetProgress(55);

g_Map.log("Creating islands");
var areaIslands = createAreas(
	new ChainPlacer(Math.floor(scaleByMapSize(3, 4)), Math.floor(scaleByMapSize(4, 8)), Math.floor(scaleByMapSize(50, 80)), 0.5),
	[
		new LayeredPainter([tWater, tShore, tIsland], [2, 1]),
		new SmoothElevationPainter(ELEVATION_SET, heightIsland, 4),
		new TileClassPainter(clIsland)
	],
	[avoidClasses(clIsland, 30), stayClasses(clWater, 10)],
	scaleByMapSize(1, 4) * numPlayers);

Engine.SetProgress(60);

createBumps(stayClasses(clIsland, 2), scaleByMapSize(50, 400), 1, 8, 4, 0, 3);

g_Map.log("Painting seabed");
paintTerrainBasedOnHeight(-20, -3, 3, tSeaDepths);

g_Map.log("Creating island metal mines");
createObjectGroupsByAreas(
	new SimpleGroup([new SimpleObject(oMetalLarge, 1, 1, 0, 4)], true, clMetal),
	0,
	[avoidClasses(clMetal, 50, clRock, 10), stayClasses(clIsland, 5)],
	scaleByMapSize(3, 10),
	20,
	areaIslands);

g_Map.log("Creating island stone mines");
createObjectGroupsByAreas(
	new SimpleGroup([new SimpleObject(oStoneLarge, 1, 1, 0, 4)], true, clRock),
	0,
	[avoidClasses(clMetal, 10, clRock, 50), stayClasses(clIsland, 5)],
	scaleByMapSize(3, 10),
	20,
	areaIslands);
Engine.SetProgress(65);

g_Map.log("Creating island towers");
createObjectGroupsByAreas(
	new SimpleGroup([new SimpleObject(oTower, 1, 1, 0, 4)], true, clTower),
	0,
	[avoidClasses(clMetal, 4, clRock, 4, clTower, 20), stayClasses(clIsland, 7)],
	scaleByMapSize(3, 10),
	20,
	areaIslands);

g_Map.log("Creating island outposts");
createObjectGroupsByAreas(
	new SimpleGroup([new SimpleObject(oOutpost, 1, 1, 0, 4)], true, clOutpost),
	0,
	[avoidClasses(clMetal, 4, clRock, 4, clTower, 5, clOutpost, 20), stayClasses(clIsland, 7)],
	scaleByMapSize(3, 10),
	20,
	areaIslands);

g_Map.log("Creating metal mines");
createObjectGroupsByAreas(
	new SimpleGroup([new SimpleObject(oMetalLarge, 1, 1, 0, 4)], true, clMetal),
	0,
	avoidClasses(clForest, 4, clBaseResource, 20, clMetal, 50, clRock, 20, clWater, 4, clHill, 4, clGauls, 5, clPath, 5),
	scaleByMapSize(4, 20),
	50,
	areasLand);

g_Map.log("Creating stone mines");
createObjectGroupsByAreas(
	new SimpleGroup([new SimpleObject(oStoneLarge, 1, 1, 0, 4)], true, clRock),
	0,
	avoidClasses(clForest, 4, clBaseResource, 20, clMetal, 20, clRock, 50, clWater, 4, clHill, 4, clGauls, 5, clPath, 5),
	scaleByMapSize(4, 20),
	50,
	areasLand);

g_Map.log("Creating stone ruins");
createObjectGroupsByAreas(
	new SimpleGroup([new SimpleObject(oStoneRuins, 1, 1, 0, 4)], true, clRock),
	0,
	avoidClasses(clForest, 2, clPlayer, 12, clMetal, 6, clRock, 25, clWater, 4, clHill, 4, clGauls, 5, clPath, 1),
	scaleByMapSize(2, 10),
	20,
	areasLand);
Engine.SetProgress(70);

g_Map.log("Creating decoratives");
for (let i = 0; i < 2; ++i)
	createDecoration(
		[
			[new SimpleObject(aRock1, 1, 1, 0, 1)],
			[new SimpleObject(aRock2, 1, 1, 0, 1)],
			[new SimpleObject(aRock3, 1, 1, 0, 1)],
			[new SimpleObject(aRock4, 1, 1, 0, 1)],

			[new SimpleObject(aBush1, 1, 3, 0, 2)],
			[new SimpleObject(aBush2, 1, 2, 0, 1)],
			[new SimpleObject(aBush3, 1, 3, 0, 2)],
			[new SimpleObject(aBush4, 1, 2, 0, 1)],

			[new SimpleObject(aFerns, 2, 5, 2, 4)]
		],
		[
			scaleByMapSize(5, 80),
			scaleByMapSize(5, 80),
			scaleByMapSize(5, 80),
			scaleByMapSize(5, 80),

			scaleByMapSize(5, 80),
			scaleByMapSize(5, 80),
			scaleByMapSize(5, 80),
			scaleByMapSize(5, 80),

			scaleByMapSize(20, 80)
		],
		i == 0 ?
			avoidClasses(clWater, 4, clForest, 1, clPlayer, 16, clRock, 4, clMetal, 4, clHill, 4, clGauls, 5, clPath, 1) :
			[stayClasses(clIsland, 4) , avoidClasses(clForest, 1, clRock, 4, clMetal, 4)]);
Engine.SetProgress(75);

g_Map.log("Creating fish");
createFood(
	[
		[new SimpleObject(oFish, 2, 3, 0, 2)]
	],
	[
		20 * scaleByMapSize(5, 20)
	],
	[avoidClasses(clIsland, 2, clFood, 10, clPath, 1), stayClasses(clWater, 5)],
	clFood);

Engine.SetProgress(80);

g_Map.log("Creating huntable animals");
createFood(
	[
		[new SimpleObject(oSheep, 5, 5, 0, 4)],
		[new SimpleObject(oGoat, 5, 5, 0, 4)],
		[new SimpleObject(oRabbit, 5, 8, 0, 4)],
		[new SimpleObject(oDeer, 4, 6, 0, 2)],
		[new SimpleObject(oHawk, 1, 1, 0, 4)]
	],
	[
		scaleByMapSize(5, 20),
		scaleByMapSize(5, 20),
		scaleByMapSize(5, 20),
		scaleByMapSize(5, 20),
		scaleByMapSize(5, 10)
	],
	avoidClasses(clIsland, 2, clFood, 10, clWater, 5, clPlayer, 16, clHill, 2, clGauls, 5, clPath, 1),
	clFood);

g_Map.log("Creating violent animals");
if (!isNomad())
	createFood(
		[
			[new SimpleObject(oWolf, 1, 3, 0, 4)],
			[new SimpleObject(oBoar, 1, 1, 0, 4)],
			[new SimpleObject(oBear, 1, 1, 0, 4)]
		],
		[
			scaleByMapSize(5, 20),
			scaleByMapSize(5, 20),
			scaleByMapSize(5, 20)
		],
		avoidClasses(clIsland, 2, clFood, 10, clWater, 5, clPlayer, 24, clHill, 2, clGauls, 5, clPath, 1),
		clFood);

Engine.SetProgress(85);

g_Map.log("Creating fruits");
createFood(
	[
		[new SimpleObject(oApple, 3, 5, 4, 7)],
		[new SimpleObject(oBerryBush, 4, 6, 0, 4)]
	],
	[
		scaleByMapSize(5, 20),
		scaleByMapSize(5, 20)
	],
	avoidClasses(clWater, 5, clForest, 2, clPlayer, 16, clHill, 4, clFood, 10, clMetal, 4, clRock, 4, clGauls, 5, clPath, 1),
	clFood);

Engine.SetProgress(90);

createStragglerTrees(
	treeTypes,
	avoidClasses(clForest, 2, clWater, 8, clPlayer, 16, clMetal, 4, clRock, 4, clFood, 1, clHill, 2, clGauls, 5, clPath, 5),
	clForest,
	stragglerTrees);

createStragglerTrees(
	treeTypes,
	[stayClasses(clIsland, 4), avoidClasses(clMetal, 4, clRock, 4, clTower, 4, clOutpost, 4)],
	clForest,
	stragglerTrees * 7);

Engine.SetProgress(95);

g_Map.log("Creating animals on islands");
createFood(
	[
		[new SimpleObject(oSheep, 4, 6, 0, 4)],
		[new SimpleObject(oGoat, 4, 6, 0, 4)],
		[new SimpleObject(oRabbit, 5, 8, 0, 4)]
	],
	[
		10 * scaleByMapSize(5, 20),
		10 * scaleByMapSize(5, 20),
		10 * scaleByMapSize(5, 20)
	],
	[avoidClasses(clRock, 4, clMetal, 4, clFood, 3, clForest, 1, clOutpost, 2, clTower, 2), stayClasses(clIsland, 4)],
	clFood);

Engine.SetProgress(98);

g_Map.log("Creating treasures");
createObjectGroupsByAreas(
	new SimpleGroup(
		[new SimpleObject(pickRandom(oTreasures), 1, 1, 0, 2)],
		true, clTreasure
	),
	0,
	avoidClasses(clForest, 1, clPlayer, 15, clHill, 1, clWater, 5, clFood, 1, clRock, 4, clMetal, 4, clTreasure, 10, clGauls, 5),
	randomTreasureCount,
	50,
	areasLand);

g_Map.log("Creating gallic decoratives");
createDecoration(
	[
		[new SimpleObject(aBucket, 1, 1, 0, 1)],
		[new SimpleObject(aBarrel, 1, 1, 0, 1)],
		[new SimpleObject(aTartan, 3, 3, 4, 4, Math.PI/4, Math.PI/2)],
		[new SimpleObject(aWheel, 2, 4, 1, 2)],
		[new SimpleObject(aWell, 1, 1, 0, 2)],
		[new SimpleObject(aWoodcord, 1, 2, 2, 2, Math.PI/2, Math.PI/2)]
	],
	[
		scaleByMapSize(2, 10),
		scaleByMapSize(2, 10),
		scaleByMapSize(2, 10),
		scaleByMapSize(2, 10),
		scaleByMapSize(3, 4),
		scaleByMapSize(2, 10)
	],
	avoidClasses(clForest, 1, clPlayer, 10, clBaseResource, 5, clHill, 1, clFood, 1, clWater, 5, clRock, 4, clMetal, 4, clGauls, 5, clPath, 1));

g_Map.log("Creating spawn points for ships");
createObjectGroupsByAreas(
	new SimpleGroup([new SimpleObject(triggerPointShipSpawn, 1, 1, 0, 0)], true, clShip),
	0,
	[avoidClasses(clShip, 5, clIsland, 4), stayClasses(clWater, 10)],
	scaleByMapSize(10, 75),
	10,
	areasWater);

g_Map.log("Creating patrol points for ships");
createObjectGroupsByAreas(
	new SimpleGroup([new SimpleObject(triggerPointShipPatrol, 1, 1, 0, 0)], true, clShipPatrol),
	0,
	[avoidClasses(clShipPatrol, 5, clIsland, 3), stayClasses(clWater, 4)],
	scaleByMapSize(20, 150),
	10,
	areasWater);

g_Map.log("Creating ungarrison points for ships");
for (let i = 0; i < 2; ++i)
{
	let areaShore = [createArea(
		new MapBoundsPlacer(),
		undefined,
		stayClasses(clShore[i], 0))];

	createObjectGroupsByAreas(
		new SimpleGroup(
			[new SimpleObject(
				i == 0 ? triggerPointShipUnloadLeft : triggerPointShipUnloadRight,
				1, 1,
				0, 0)],
			true,
			clShoreUngarrisonPoint[i]),
		0,
		avoidClasses(clShoreUngarrisonPoint[i], 4),
		scaleByMapSize(60, 200),
		20,
		areaShore);
}

g_Map.log("Creating riverdirection triggerpoint");
g_Map.placeEntityAnywhere(triggerPointRiverDirection, 0, Vector2D.add(mapCenter, new Vector2D(0, 1).rotate(startAngle)), randomAngle());

g_Map.log("Creating patrol points for siege engines");
for (let i = 0; i < 2; ++i)
	// Patrol points for siege engines
	createObjectGroupsByAreas(
		new SimpleGroup(
			[new SimpleObject(
				i == 0 ? triggerPointLandPatrolLeft : triggerPointLandPatrolRight,
				1, 1,
				0, 0)],
			true,
			clPatrolPointSiegeEngine[i]),
		0,
		avoidClasses(clWater, 5, clForest, 3, clHill, 3, clFood, 1, clRock, 5, clMetal, 5, clPlayer, 10, clGauls, 5, clPatrolPointSiegeEngine[i], 5),
		scaleByMapSize(20, 150),
		10,
		[areasLand[i]]);

if (gallicCC)
{
	g_Map.log("Creating patrol points for soldiers");
	for (let i = 0; i < 2; ++i)
		createObjectGroupsByAreas(
			new SimpleGroup(
				[new SimpleObject(
					i == 0 ? triggerPointCCAttackerPatrolLeft : triggerPointCCAttackerPatrolRight,
					1, 1,
					0, 0)],
				true,
				clPatrolPointSoldier[i]),
			0,
			// Don't avoid the forest, so that as many places as possible on the border are visited
			avoidClasses(
				clWater, 5,
				clHill, 3,
				clFood, 1,
				clRock, 4,
				clMetal, 4,
				clPlayer, 15,
				clGauls, 0,
				clPatrolPointSoldier[i], 5),
			scaleByMapSize(20, 150),
			20,
			[areasLand[i]]);
}

g_Map.log("Creating water logs");
createObjectGroupsByAreas(
	new SimpleGroup([new SimpleObject(aWaterLog, 1, 1, 0, 0, startAngle, startAngle)], true, clWaterLog),
	0,
	[avoidClasses(clShip, 3, clIsland, 4), stayClasses(clWater, 4)],
	scaleByMapSize(1, 4),
	10,
	areasWater);

placePlayersNomad(clPlayer, avoidClasses(clWater, 4, clMetal, 4, clRock, 4, clIsland, 4, clGauls, 20, clRitualPlace, 20, clForest, 1, clBaseResource, 4, clHill, 4, clFood, 2));

if (randBool(2/3))
{
	g_Map.log("Setting day theme");
	setSkySet("cumulus");

	setSunColor(0.9, 0.8, 0.5);

	setFogFactor(0.05);
	setFogThickness(0.25);

	setWaterColor(0.317, 0.396, 0.294);
	setWaterTint(0.439, 0.403, 0.262);

	setPPContrast(0.62);
	setPPSaturation(0.51);
	setPPBloom(0.12);
}
else
{
	g_Map.log("Setting night theme");
	setSkySet("dark");

	setSunColor(0.4, 0.9, 1.2);
	setSunElevation(0.13499);
	setSunRotation(-2.5);

	setTerrainAmbientColor(0.25, 0.3, 0.45);
	setUnitsAmbientColor(0.3, 0.35, 0.5);

	setFogFactor(0.004);
	setFogThickness(0.25);
	setFogColor(0.35, 0.45, 0.5);

	setWaterColor(0.074, 0.101, 0.090);
	setWaterTint(0.129, 0.160, 0.137);
}

setPPEffect("hdr");
setWaterWaviness(2.0);
setWaterType("lake");
setWaterMurkiness(0.97);
setWaterHeight(21);

g_Map.ExportMap();
