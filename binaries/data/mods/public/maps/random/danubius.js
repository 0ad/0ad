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
const oStoneRuins = "gaia/special_ruins_standing_stone";
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
	"gaia/special_treasure_food_barrel",
	"gaia/special_treasure_food_bin",
	"gaia/special_treasure_stone",
	"gaia/special_treasure_wood",
	"gaia/special_treasure_metal"
];

const oCivicCenter = "structures/gaul_civil_centre";
const oHouse = "structures/gaul_house";
const oTemple = "structures/gaul_temple";
const oTavern = "structures/gaul_tavern";
const oTower = "structures/gaul_defense_tower";
const oSentryTower = "structures/gaul_sentry_tower";
const oOutpost = "structures/gaul_outpost";

const oHut = "other/celt_hut";
const oLongHouse = "other/celt_longhouse";
const oPalisadeTower = "other/palisades_rocks_watchtower";
const oTallSpikes = "other/palisades_tall_spikes";
const oAngleSpikes = "other/palisades_angle_spike";

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

InitMap();

const numPlayers = getNumPlayers();
const mapSize = getMapSize();
const mapCenter = getMapCenter();
const mapBounds = getMapBounds();

var clMiddle = createTileClass();
var clPlayer = createTileClass();
var clForest = createTileClass();
var clWater = createTileClass();
var clLand = [createTileClass(), createTileClass()];
var clLandPatrolPoint = [createTileClass(), createTileClass()];
var clCCAttackerPatrolPoint = [createTileClass(), createTileClass()];
var clShore = [createTileClass(), createTileClass()];
var clShoreUngarrisonPoint = [createTileClass(), createTileClass()];
var clShip = createTileClass();
var clShipPatrol = createTileClass();
var clDirt = createTileClass();
var clRock = createTileClass();
var clMetal = createTileClass();
var clFood = createTileClass();
var clBaseResource = createTileClass();
var clHill = createTileClass();
var clIsland = createTileClass();
var clTreasure = createTileClass();
var clWaterLog = createTileClass();
var clGauls = createTileClass();
var clTower = createTileClass();
var clOutpost = createTileClass();
var clPath = createTileClass();
var clRitualPlace = createTileClass();

var waterWidth = fractionToTiles(0.3);
var waterHeight = -3;
var shoreHeight = 1;
var landHeight = 2;

// How many treasures will be placed near the gallic civic centers
var gallicCCTreasureCount = randIntInclusive(8, 12);

// How many treasures will be placed randomly on the map at most
var randomTreasureCount = randIntInclusive(0, 3 * numPlayers);

// Place a gallic village on small maps and larger
var gallicCC = mapSize >= smallMapSize;
if (gallicCC)
{
	log("Creating gallic villages...");
	let gaulCityRadius = 12;
	let gaulCityBorderDistance = mapSize < mediumMapSize ? 10 : 18;

	// Whether to add a celtic ritual and a path from the gallic city leading to it
	let addCelticRitual = randBool(0.9);

	// One village left and right of the river
	for (let i = 0; i < 2; ++i)
	{
		let gX = i == 0 ? mapBounds.left + gaulCityBorderDistance : mapBounds.right - gaulCityBorderDistance;
		let gZ = mapCenter.y;

		if (addCelticRitual)
		{
			// Don't position the meeting place at the center of the map
			let mX = i == 0 ? mapBounds.left + waterWidth : mapBounds.right - waterWidth;
			let mZ = mapCenter.y + fractionToTiles(randFloat(0.1, 0.4)) * (randBool() ? 1 : -1);

			// Radius of the meeting place
			let mRadius = scaleByMapSize(4, 6);

			// Create a path connecting the gallic city with a meeting place at the shoreline.
			// To avoid the path going through the palisade wall, start it at the gate, not at the city center.
			let placer = new PathPlacer(
				gX + gaulCityRadius * (i == 0 ? 1 : -1),
				gZ,
				mX,
				mZ,
				4, // width
				0.4, // waviness
				4, // smoothness
				0.2, // offset
				0.05); // tapering

			createArea(
				placer,
				[
					new LayeredPainter([tShore, tRoad, tRoad], [1, 3]),
					new SmoothElevationPainter(ELEVATION_SET, 5, 4),
					paintClass(clPath)
				]);

			// Create the meeting place near the shoreline at the end of the path
			createArea(
				new ClumpPlacer(mRadius * mRadius * Math.PI, 0.6, 0.3, 10, mX, mZ),
				[
					new LayeredPainter([tShore, tShore], [1]),
					paintClass(clPath),
					paintClass(clRitualPlace)
				]);

			placeObject(mX, mZ, aCampfire, 0, randFloat(0, 2 * Math.PI));

			let femaleCount = Math.round(mRadius * 2);
			let maleCount = Math.round(mRadius * 3);
			let benchCount = Math.round(mRadius * 2);
			let rugCount = Math.round(mRadius * 2.5);
			let goatCount = Math.round(mRadius * 1.5);

			let femaleRadius = mRadius * 0.3;
			let maleRadius = mRadius * 0.4;
			let benchRadius = mRadius * 0.5;
			let rugRadius = mRadius * 0.6;
			let goatRadius = mRadius * 0.8;

			let calcBend = entCount => Math.PI * 2 / entCount;
			let maleBend = calcBend(maleCount);

			g_WallStyles.celt_ritual = {
				"overlap": 0,
				"female":     { "angle": Math.PI, "length": femaleRadius, "indent": 0, "bend": calcBend(femaleCount), "templateName": oFemale },
				"skirmisher": { "angle": Math.PI, "length": maleRadius, "indent": 0, "bend": maleBend, "templateName": oSkirmisher },
				"healer":     { "angle": Math.PI, "length": maleRadius, "indent": 0, "bend": maleBend, "templateName": oHealer },
				"fanatic":    { "angle": Math.PI, "length": maleRadius, "indent": 0, "bend": maleBend, "templateName": oNakedFanatic },
				"bench":      { "angle": Math.PI / 2, "length": benchRadius, "indent": 0, "bend": calcBend(benchCount), "templateName": aBench },
				"rug":        { "angle": 0,       "length": rugRadius, "indent": 0, "bend": calcBend(rugCount), "templateName": aRug },
				"goat":       { "angle": Math.PI, "length": goatRadius, "indent": 0, "bend": calcBend(goatCount), "templateName": oGoat }
			};

			placeCustomFortress(mX, mZ, new Fortress("celt ritual females", new Array(femaleCount).fill("female")), "celt_ritual", 0, 0);

			placeCustomFortress(mX, mZ, new Fortress("celt ritual males", new Array(maleCount).fill(0).map(i =>
				pickRandom(["skirmisher", "healer", "fanatic"]))), "celt_ritual", 0, 0);

			placeCustomFortress(mX, mZ, new Fortress("celt ritual bench", new Array(benchCount).fill("bench")), "celt_ritual", 0, 0);
			placeCustomFortress(mX, mZ, new Fortress("celt ritual rug", new Array(rugCount).fill("rug")), "celt_ritual", 0, 0);
			placeCustomFortress(mX, mZ, new Fortress("celt ritual goat", new Array(goatCount).fill("goat")), "celt_ritual", 0, 0);
		}

		placeObject(gX, gZ, oCivicCenter, 0, BUILDING_ORIENTATION + Math.PI * 3/2 * i);

		// Create the city patch
		createArea(
			new ClumpPlacer(diskArea(gaulCityRadius), 0.6, 0.3, 10, gX, gZ),
			[
				new LayeredPainter([tShore, tShore], [1]),
				paintClass(clGauls)
			]);

		// Place palisade fortress and some city buildings
		// Use actors to avoid players capturing the buildings
		g_WallStyles.gaul = clone(g_WallStyles.gaul_stone);
		g_WallStyles.gaul.house = { "angle": Math.PI, "length": 0, "indent": 4, "bend": 0, "templateName": oHouse };
		g_WallStyles.gaul.hut = { "angle": Math.PI, "length": 0, "indent": 4, "bend": 0, "templateName": oHut };
		g_WallStyles.gaul.longhouse = { "angle": Math.PI, "length": 0, "indent": 4, "bend": 0, "templateName": oLongHouse };
		g_WallStyles.gaul.tavern = { "angle": Math.PI * 3/2, "length": 0, "indent": 4, "bend": 0, "templateName": oTavern };
		g_WallStyles.gaul.temple = { "angle": Math.PI * 3/2, "length": 0, "indent": 4, "bend": 0, "templateName": oTemple };
		g_WallStyles.gaul.defense_tower = { "angle": Math.PI / 2, "length": 0, "indent": 4, "bend": 0, "templateName": mapSize >= normalMapSize ? (isNomad() ? oSentryTower : oTower) : oPalisadeTower };

		// Replace stone walls with palisade walls
		for (let element of ["gate", "long", "short", "cornerIn", "cornerOut", "tower"])
			g_WallStyles.gaul[element] = getWallElement(element, "palisade");

		let wall = [
			"gate", "hut", "tower", "long", "long",
			"cornerIn", "defense_tower", "long", "long", "temple",
			"tower", "long", "house", "short", "tower", "gate", "tower", "longhouse", "long", "long",
			"cornerIn", "defense_tower", "long", "tavern", "long", "tower"];
		wall = wall.concat(wall);
		placeCustomFortress(gX, gZ, new Fortress("Geto-Dacian Tribal Confederation", wall), "gaul", 0, Math.PI);

		// Place spikes
		g_WallStyles.palisade.spikes_tall = readyWallElement("other/palisades_tall_spikes", "gaia");
		g_WallStyles.palisade.spike_single = readyWallElement("other/palisades_angle_spike", "gaia");

		let threeSpikes = new Array(3).fill("spikes_tall");
		let fiveSpikes = new Array(5).fill("spikes_tall");

		let spikes = [
			"gap_3.6",
			"spike_single", ...threeSpikes,
			"turn_0.25", "spike_single", "turn_0.25",
			...fiveSpikes, "spike_single",
			"gap_3.6", "spike_single", ...threeSpikes,
			"turn_0.25", "spike_single", "turn_0.25",
			...threeSpikes,
			"spike_single"
		];
		spikes = spikes.concat(spikes);
		placeCustomFortress(gX, gZ, new Fortress("spikes", spikes), "palisade", 0, Math.PI);

		// Place treasure, potentially inside buildings
		for (let i = 0; i < gallicCCTreasureCount; ++i)
			placeObject(
				gX + randFloat(-0.8, 0.8) * gaulCityRadius,
				gZ + randFloat(-0.8, 0.8) * gaulCityRadius,
				pickRandom(oTreasures),
				0,
				randFloat(0, 2 * Math.PI));
	}
}
Engine.SetProgress(10);

placePlayerBases({
	"PlayerPlacement": playerPlacementRiver(0, 0.6),
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
		"template": aBush1
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
	"start": new Vector2D(mapCenter.x, mapBounds.top),
	"end": new Vector2D(mapCenter.x, mapBounds.bottom),
	"width": waterWidth,
	"fadeDist": scaleByMapSize(6, 25),
	"deviation": 0,
	"waterHeight": waterHeight,
	"landHeight": landHeight,
	"meanderShort": 30,
	"meanderLong": 0,
	"waterFunc": (ix, iz, height, riverFraction) => {
		// Distinguish left and right shoreline
		if (0 < height && height < 1 && iz > ShorelineDistance && iz < mapSize - ShorelineDistance)
			addToClass(ix, iz, clShore[ix < mapCenter.x ? 0 : 1]);
	},
	"landFunc": (ix, iz, shoreDist1, shoreDist2) => {

		if (shoreDist1 > 0)
			addToClass(ix, iz, clLand[0]);

		if (shoreDist2 < 0)
			addToClass(ix, iz, clLand[1]);
	}
});
Engine.SetProgress(30);

paintTileClassBasedOnHeight(-Infinity, 0.7, Elevation_ExcludeMin_ExcludeMax, clWater);

log("Creating shores...");
paintTerrainBasedOnHeight(-Infinity, shoreHeight, 0, tWater);
paintTerrainBasedOnHeight(shoreHeight, landHeight, 0, tShore);
Engine.SetProgress(35);

log("Creating bumps...");
createBumps(avoidClasses(clPlayer, 6, clWater, 2, clPath, 1), scaleByMapSize(30, 300), 1, 8, 4, 0, 3);
Engine.SetProgress(40);

log("Creating hills...");
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

log("Creating grass patches...");
createLayeredPatches(
	[scaleByMapSize(3, 6), scaleByMapSize(5, 10), scaleByMapSize(8, 21)],
	[[tGrass, tGrass2],[tGrass2, tGrass3], [tGrass3, tGrass]],
	[1, 1],
	avoidClasses(clForest, 0, clPlayer, 10, clWater, 2, clDirt, 2, clHill, 1, clGauls, 5, clPath, 1),
	scaleByMapSize(15, 45),
	clDirt);
Engine.SetProgress(55);

log("Creating islands...");
createAreas(
	new ChainPlacer(Math.floor(scaleByMapSize(3, 4)), Math.floor(scaleByMapSize(4, 8)), Math.floor(scaleByMapSize(50, 80)), 0.5),
	[
		new LayeredPainter([tWater, tShore, tIsland], [2, 1]),
		new SmoothElevationPainter(ELEVATION_SET, 6, 4),
		paintClass(clIsland)
	],
	[avoidClasses(clIsland, 30), stayClasses (clWater, 10)],
	scaleByMapSize(1, 4) * numPlayers
);
Engine.SetProgress(60);

log("Creating island bumps...");
createBumps(stayClasses(clIsland, 2), scaleByMapSize(50, 400), 1, 8, 4, 0, 3);

log("Paint seabed...");
paintTerrainBasedOnHeight(-20, -3, 3, tSeaDepths);

log("Creating island metal mines...");
createObjectGroupsDeprecated(
	new SimpleGroup([new SimpleObject(oMetalLarge, 1, 1, 0, 4)], true, clMetal),
	0,
	[avoidClasses(clMetal, 50, clRock, 10), stayClasses(clIsland, 5)],
	500, 1
);

log("Creating island stone mines...");
createObjectGroupsDeprecated(
	new SimpleGroup([new SimpleObject(oStoneLarge, 1, 1, 0, 4)], true, clRock),
	0,
	[avoidClasses(clMetal, 10, clRock, 50), stayClasses(clIsland, 5)],
	500, 1
);
Engine.SetProgress(65);

log("Creating island towers...");
createObjectGroupsDeprecated(
	new SimpleGroup([new SimpleObject(oTower, 1, 1, 0, 4)], true, clTower),
	0,
	[avoidClasses(clMetal, 4, clRock, 4, clTower, 20), stayClasses(clIsland, 7)],
	500, 1
);

log("Creating island outposts...");
createObjectGroupsDeprecated(
	new SimpleGroup([new SimpleObject(oOutpost, 1, 1, 0, 4)], true, clOutpost),
	0,
	[avoidClasses(clMetal, 4, clRock, 4, clTower, 5, clOutpost, 20), stayClasses(clIsland, 7)],
	500, 1
);

log("Creating metal mines...");
createObjectGroupsDeprecated(
	new SimpleGroup([new SimpleObject(oMetalLarge, 1, 1, 0, 4)], true, clMetal),
	0,
	[avoidClasses(clForest, 4, clBaseResource, 20, clMetal, 50, clRock, 20, clWater, 4, clHill, 4, clGauls, 5, clPath, 5)],
	500, 1
);

log("Creating stone mines...");
createObjectGroupsDeprecated(
	new SimpleGroup([new SimpleObject(oStoneLarge, 1, 1, 0, 4)], true, clRock),
	0,
	[avoidClasses(clForest, 4, clBaseResource, 20, clMetal, 20, clRock, 50, clWater, 4, clHill, 4, clGauls, 5, clPath, 5)],
	500, 1
);

log("Creating stone ruins...");
createObjectGroupsDeprecated(
		new SimpleGroup([new SimpleObject(oStoneRuins, 1, 1, 0, 4)], true, clRock),
		0,
		[avoidClasses(clForest, 2, clPlayer, 12, clMetal, 6, clRock, 25, clWater, 4, clHill, 4, clGauls, 5, clPath, 1)],
		500, 1
	);
Engine.SetProgress(70);

log("Creating decoratives...");
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
			[stayClasses(clIsland, 4) , avoidClasses(clForest, 1, clRock, 4, clMetal, 4)]
	);
Engine.SetProgress(75);

log("Creating fish...");
createFood(
	[
		[new SimpleObject(oFish, 2, 3, 0, 2)]
	],
	[
		20 * scaleByMapSize(5, 20)
	],
	[avoidClasses(clIsland, 2, clFood, 10, clPath, 1), stayClasses(clWater, 5)],
	clFood
);
Engine.SetProgress(80);

log("Creating huntable animals...");
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

log("Creating violent animals...");
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

log("Creating fruits...");
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

log("Creating animals on islands...");
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
	clFood
);
Engine.SetProgress(98);

log("Creating treasures...");
for (let i = 0; i < randomTreasureCount; ++i)
	createObjectGroupsDeprecated(
		new SimpleGroup(
			[new SimpleObject(pickRandom(oTreasures), 1, 1, 0, 2)],
			true, clTreasure
		),
		0,
		avoidClasses(clForest, 1, clPlayer, 15, clHill, 1, clWater, 5, clFood, 1, clRock, 4, clMetal, 4, clTreasure, 10, clGauls, 5),
		1,
		50
	);

log("Creating gallic decoratives...");
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
	avoidClasses(clForest, 1, clPlayer, 10, clBaseResource, 5, clHill, 1, clFood, 1, clWater, 5, clRock, 4, clMetal, 4, clGauls, 5, clPath, 1)
);

log("Creating spawn points for ships...");
createObjectGroupsDeprecated(
	new SimpleGroup([new SimpleObject(triggerPointShipSpawn, 1, 1, 0, 0)], true, clShip),
	0,
	[avoidClasses(clShip, 5, clIsland, 4), stayClasses(clWater, 10)],
	10000,
	1000
);

log("Creating patrol points for ships...");
createObjectGroupsDeprecated(
	new SimpleGroup([new SimpleObject(triggerPointShipPatrol, 1, 1, 0, 0)], true, clShipPatrol),
	0,
	[avoidClasses(clShipPatrol, 5, clIsland, 3), stayClasses(clWater, 4)],
	10000,
	1000
);

log("Creating ungarrison points for ships...");
for (let i = 0; i < 2; ++i)
	createObjectGroupsDeprecated(
		new SimpleGroup(
			[new SimpleObject(
				i == 0 ? triggerPointShipUnloadLeft : triggerPointShipUnloadRight,
				1, 1,
				0, 0)],
			true,
			clShoreUngarrisonPoint[i]),
		0,
		[avoidClasses(clShoreUngarrisonPoint[i], 4), stayClasses(clShore[i], 0)],
		20000,
		1
	);

log("Creating patrol points for land attackers...");
addToClass(mapSize/2, mapSize/2, clMiddle);
for (let i = 0; i < 2; ++i)
{
	createObjectGroupsDeprecated(
		new SimpleGroup(
			[new SimpleObject(
				i == 0 ? triggerPointLandPatrolLeft : triggerPointLandPatrolRight,
				1, 1,
				0, 0)],
			true,
			clLandPatrolPoint[i]),
		0,
		[
			avoidClasses(clWater, 5, clForest, 3, clHill, 3, clFood, 1, clRock, 5, clMetal, 5, clPlayer, 10, clGauls, 5, clLandPatrolPoint[i], 5),
			stayClasses(clLand[i], 0)
		],
		10000,
		100
	);

	if (gallicCC)
		createObjectGroupsDeprecated(
			new SimpleGroup(
				[new SimpleObject(
					i == 0 ? triggerPointCCAttackerPatrolLeft : triggerPointCCAttackerPatrolRight,
					1, 1,
					0, 0)],
				true,
				clCCAttackerPatrolPoint[i]),
			0,
			[
				// Don't avoid the forest, so that as many places as possible on the border are visited
				avoidClasses(
					clWater, 5,
					clHill, 3,
					clFood, 1,
					clRock, 4,
					clMetal, 4,
					clPlayer, 15,
					clGauls, 0,
					clCCAttackerPatrolPoint[i], 5,
					clMiddle, mapSize * 0.5 - 15),
				stayClasses(clLand[i], 0)
			],
			10000,
			100
		);
}

log("Creating water logs...");
createObjectGroupsDeprecated(
	new SimpleGroup([new SimpleObject(aWaterLog, 1, 1, 0, 0)], true, clWaterLog),
	0,
	[avoidClasses(clShip, 3, clIsland, 4), stayClasses(clWater, 4)],
	scaleByMapSize(15, 60),
	100
);

placePlayersNomad(clPlayer, avoidClasses(clWater, 4, clIsland, 4, clGauls, 20, clRitualPlace, 20, clForest, 1, clBaseResource, 4, clHill, 4, clFood, 2));

if (randBool(2/3))
{
	// Day
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
	// Night
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

ExportMap();
