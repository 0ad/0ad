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
const oTower= "structures/gaul_defense_tower";
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

// Percentage of the mapsize that the river takes up
const waterWidth = 0.3;

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
		let gX = i == 0 ? gaulCityBorderDistance : mapSize - gaulCityBorderDistance;
		let gZ = mapSize / 2;

		if (addCelticRitual)
		{
			// Don't position the meeting place at the center of the map
			let mLocation = randFloat(0.1, 0.4) * (randBool() ? 1 : -1);

			// Center of the meeting place
			let mX = i == 0 ?
				mapSize * waterWidth :
				mapSize * (1 - waterWidth);
			let mZ = gZ + mapSize * mLocation;

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
				new ClumpPlacer(mRadius * mRadius * PI, 0.6, 0.3, 10, mX, mZ),
				[new LayeredPainter([tShore, tShore], [1]), paintClass(clPath), paintClass(clRitualPlace)],
				null);

			placeObject(mX, mZ, aCampfire, 0, randFloat(0, 2 * PI));

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

			wallStyles.celt_ritual = {
				"female": new WallElement("female", oFemale, PI, femaleRadius, 0, 2 * PI / femaleCount),
				"skirmisher": new WallElement("skirmisher", oSkirmisher, PI, maleRadius, 0, 2 * PI / maleCount),
				"healer": new WallElement("healer", oHealer, PI, maleRadius, 0, 2 * PI / maleCount),
				"fanatic": new WallElement("fanatic", oNakedFanatic, PI, maleRadius, 0, 2 * PI / maleCount),
				"bench": new WallElement("bench", aBench, PI/2, benchRadius, 0, 2 * PI / benchCount),
				"rug": new WallElement("rug", aRug, 0, rugRadius, 0, 2 * PI / rugCount),
				"goat": new WallElement("goat", oGoat, PI, goatRadius, 0, 2 * PI / goatCount),
			};

			placeCustomFortress(mX, mZ, new Fortress("celt ritual females", new Array(femaleCount).fill("female")), "celt_ritual", 0, 0);

			placeCustomFortress(mX, mZ, new Fortress("celt ritual males", new Array(maleCount).fill(0).map(i =>
				pickRandom(["skirmisher", "healer", "fanatic"]))), "celt_ritual", 0, 0);

			placeCustomFortress(mX, mZ, new Fortress("celt ritual bench", new Array(benchCount).fill("bench")), "celt_ritual", 0, 0);
			placeCustomFortress(mX, mZ, new Fortress("celt ritual rug", new Array(rugCount).fill("rug")), "celt_ritual", 0, 0);
			placeCustomFortress(mX, mZ, new Fortress("celt ritual goat", new Array(goatCount).fill("goat")), "celt_ritual", 0, 0);
		}

		placeObject(gX, gZ, oCivicCenter, 0, BUILDING_ORIENTATION + PI * 3/2 * i);

		// Create the city patch
		createArea(
			new ClumpPlacer(gaulCityRadius * gaulCityRadius * PI, 0.6, 0.3, 10, gX, gZ),
			[new LayeredPainter([tShore, tShore], [1]), paintClass(clGauls)],
			null);

		// Place palisade fortress and some city buildings
		// Use actors to avoid players capturing the buildings
		wallStyles.gaul.house = new WallElement("house", oHouse, PI, 0, 4);
		wallStyles.gaul.hut = new WallElement("hut", oHut, PI, 0, 4);
		wallStyles.gaul.longhouse = new WallElement("longhouse", oLongHouse, PI, 0, 4);
		wallStyles.gaul.tavern = new WallElement("tavern", oTavern, PI * 3/2, 0, 4);
		wallStyles.gaul.temple = new WallElement("temple", oTemple, PI * 3/2, 0, 4);
		wallStyles.gaul.defense_tower = new WallElement("defense_tower",
			mapSize >= normalMapSize ? oTower : oPalisadeTower, PI/2, 0, 4);
		wallStyles.gaul.palisade_tower = wallStyles.palisades.tower;

		// Replace stone walls with palisade walls
		for (let template of ["gate", "wallLong", "cornerIn", "cornerOut"])
			wallStyles.gaul[template] = wallStyles.palisades[template];

		let wall = [
			"gate", "hut", "palisade_tower", "wallLong", "wallLong",
			"cornerIn", "defense_tower", "wallLong", "wallLong", "temple",
			"palisade_tower", "wallLong", "house", "gate", "palisade_tower", "longhouse", "wallLong", "wallLong",
			"cornerIn", "defense_tower", "wallLong", "tavern", "wallLong", "palisade_tower"];
		wall = wall.concat(wall);
		placeCustomFortress(gX, gZ, new Fortress("Geto-Dacian Tribal Confederation", wall), "gaul", 0, PI);

		// Place spikes
		wallStyles.palisades.tall_spikes = new WallElement("tall_spikes", oTallSpikes, PI/2, 2);
		wallStyles.palisades.spikeIn = new WallElement("spikeIn", oAngleSpikes, -PI/4, 2.1, 0.7, PI/2);
		wallStyles.palisades.spikeMid = new WallElement("spikeIn", oAngleSpikes, -PI/2, 0.7);
		wallStyles.palisades.gateGap = new WallElement("gateGap", undefined, PI, 3.6);

		let fourSpikes = new Array(4).fill("tall_spikes");
		let sixSpikes = new Array(6).fill("tall_spikes");

		let spikes = [
			"gateGap",
			"spikeMid", ...fourSpikes,
			"spikeIn", ...sixSpikes, "spikeMid",
			"gateGap", "spikeMid", ...fourSpikes,
			"spikeIn", ...fourSpikes,
			"spikeMid"
		];
		spikes = spikes.concat(spikes);
		placeCustomFortress(gX, gZ, new Fortress("spikes", spikes), "palisades", 0, PI);

		// Place treasure, potentially inside buildings
		for (let i = 0; i < gallicCCTreasureCount; ++i)
			placeObject(
				gX + randFloat(-0.8, 0.8) * gaulCityRadius,
				gZ + randFloat(-0.8, 0.8) * gaulCityRadius,
				pickRandom(oTreasures),
				0,
				randFloat(0, 2 * PI));
	}
}
Engine.SetProgress(10);

var [playerIDs, playerX, playerZ] = playerPlacementRiver(0, 0.6);

for (let i = 0; i < numPlayers; ++i)
{
	let id = playerIDs[i];
	log("Creating base for player " + id + "...");

	let radius = scaleByMapSize(15, 25);

	let fx = fractionToTiles(playerX[i]);
	let fz = fractionToTiles(playerZ[i]);
	let ix = Math.floor(fx);
	let iz = Math.floor(fz);
	addToClass(ix, iz, clPlayer);

	// Create the city patch
	let cityRadius = radius / 3;
	createArea(
		new ClumpPlacer(PI * cityRadius * cityRadius, 0.6, 0.3, 10, ix, iz),
		new LayeredPainter([tShore, tRoad], [1]),
		null);

	placeCivDefaultEntities(fx, fz, id, { 'iberWall': false });

	placeDefaultChicken(fx, fz, clBaseResource);

	// Create berry bushes
	let angle = randFloat(0, 2 * PI);
	let dist = 10;
	createObjectGroup(
		new SimpleGroup(
			[new SimpleObject(oBerryBush, 5, 5, 0, 3)],
			true,
			clBaseResource,
			Math.round(fx + dist * Math.cos(angle)),
			Math.round(fz + dist * Math.sin(angle))
		),
		0);

	// Create metal mine
	dist = scaleByMapSize(9, 14);
	angle += randFloat(PI/4, PI/3);
	createObjectGroup(
		new SimpleGroup(
			[new SimpleObject(oMetalLarge, 1, 1, 0, 0)],
			true,
			clBaseResource,
			Math.round(fx + dist * Math.cos(angle)),
			Math.round(fz + dist * Math.sin(angle))
		),
		0);

	// Create stone mines
	angle += randFloat(PI/3, PI/2);
	createObjectGroup(
		new SimpleGroup(
			[new SimpleObject(oStoneLarge, 1, 1, 0, 2)],
			true,
			clBaseResource,
			Math.round(fx + dist * Math.cos(angle)),
			Math.round(fz + dist * Math.sin(angle))
		),
		0);

	// Create starting trees
	let num = 20;
	angle += randFloat(-PI/3, PI * 4/3);
	dist = randFloat(10, 14);
	createObjectGroup(
		new SimpleGroup(
			[new SimpleObject(oOak, num, num, 0, 5)],
			false,
			clBaseResource,
			Math.round(fx + dist * Math.cos(angle)),
			Math.round(fz + dist * Math.sin(angle))
		),
		0,
		avoidClasses(clBaseResource, 4));

	placeDefaultDecoratives(fx, fz, aBush1, clBaseResource, radius);
}
Engine.SetProgress(20);

paintRiver({
	"parallel": true,
	"startX": 0.5,
	"startZ": 0,
	"endX": 0.5,
	"endZ": 1,
	"width": waterWidth,
	"fadeDist": 0.025,
	"deviation": 0,
	"waterHeight": -3,
	"landHeight": 2,
	"meanderShort": 30,
	"meanderLong": 0,
	"waterFunc": (ix, iz, height, riverFraction) => {

		if (height < 0.7)
			addToClass(ix, iz, clWater);

		// Distinguish left and right shoreline
		if (0 < height && height < 1 && iz > ShorelineDistance && iz < mapSize - ShorelineDistance)
			addToClass(ix, iz, clShore[ix < mapSize / 2 ? 0 : 1]);
	},
	"landFunc": (ix, iz, shoreDist1, shoreDist2) => {

		if (shoreDist1 > 0)
			addToClass(ix, iz, clLand[0]);

		if (shoreDist2 < 0)
			addToClass(ix, iz, clLand[1]);
	}
});

Engine.SetProgress(30);

log("Creating shores...");
paintTerrainBasedOnHeight(-20, 1, 0, tWater);
paintTerrainBasedOnHeight(1, 2, 0, tShore);
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
		[new SimpleObject(aTartan, 3, 3, 4, 4, PI/4, PI/2)],
		[new SimpleObject(aWheel, 2, 4, 1, 2)],
		[new SimpleObject(aWell, 1, 1, 0, 2)],
		[new SimpleObject(aWoodcord, 1, 2, 2, 2, PI/2, PI/2)]
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
