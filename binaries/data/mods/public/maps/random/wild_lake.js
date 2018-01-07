Engine.LoadLibrary("rmgen");
Engine.LoadLibrary("rmbiome");
Engine.LoadLibrary("heightmap");

InitMap();

let genStartTime = Date.now();

/**
 * getArray - To ensure a terrain texture is contained within an array
 */
function getArray(stringOrArrayOfStrings)
{
	if (typeof stringOrArrayOfStrings == "string")
		return [stringOrArrayOfStrings];
	return stringOrArrayOfStrings;
}

setSelectedBiome();

// Terrain, entities and actors
let wildLakeBiome = [
	// 0 Deep water
	{
		"texture": getArray(g_Terrains.water),
		"actor": [[g_Gaia.fish], 0.01],
		"textureHS": getArray(g_Terrains.water),
		"actorHS": [[g_Gaia.fish], 0.03]
	},
	// 1 Shallow water
	{
		"texture": getArray(g_Terrains.water),
		"actor": [[g_Decoratives.lillies, g_Decoratives.reeds], 0.3],
		"textureHS": getArray(g_Terrains.water),
		"actorHS": [[g_Decoratives.lillies], 0.1]
	},
	// 2 Shore
	{
		"texture": getArray(g_Terrains.shore),
		"actor": [
			[
				g_Gaia.tree1, g_Gaia.tree1,
				g_Gaia.tree2, g_Gaia.tree2,
				g_Gaia.mainHuntableAnimal,
				g_Decoratives.grass, g_Decoratives.grass,
				g_Decoratives.rockMedium, g_Decoratives.rockMedium,
				g_Decoratives.bushMedium, g_Decoratives.bushMedium
			],
			0.3
		],
		"textureHS": getArray(g_Terrains.cliff),
		"actorHS": [[g_Decoratives.grassShort, g_Decoratives.rockMedium, g_Decoratives.bushSmall], 0.1]
	},
	// 3 Low ground
	{
		"texture": getArray(g_Terrains.tier1Terrain),
		"actor": [
			[
				g_Decoratives.grass,
				g_Decoratives.grassShort,
				g_Decoratives.rockLarge,
				g_Decoratives.rockMedium,
				g_Decoratives.bushMedium,
				g_Decoratives.bushSmall
			],
			0.2
		],
		"textureHS": getArray(g_Terrains.cliff),
		"actorHS": [[g_Decoratives.grassShort, g_Decoratives.rockMedium, g_Decoratives.bushSmall], 0.1]
	},
	// 4 Mid ground. Player and path height
	{
		"texture": getArray(g_Terrains.mainTerrain),
		"actor": [
			[
				g_Decoratives.grass,
				g_Decoratives.grassShort,
				g_Decoratives.rockLarge,
				g_Decoratives.rockMedium,
				g_Decoratives.bushMedium,
				g_Decoratives.bushSmall
			],
			0.2
		],
		"textureHS": getArray(g_Terrains.cliff),
		"actorHS": [[g_Decoratives.grassShort, g_Decoratives.rockMedium, g_Decoratives.bushSmall], 0.1]
	},
	// 5 High ground
	{
		"texture": getArray(g_Terrains.tier2Terrain),
		"actor": [
			[
				g_Decoratives.grass,
				g_Decoratives.grassShort,
				g_Decoratives.rockLarge,
				g_Decoratives.rockMedium,
				g_Decoratives.bushMedium,
				g_Decoratives.bushSmall
			],
			0.2
		],
		"textureHS": getArray(g_Terrains.cliff),
		"actorHS": [[g_Decoratives.grassShort, g_Decoratives.rockMedium, g_Decoratives.bushSmall], 0.1]
	},
	// 6 Lower hilltop forest border
	{
		"texture": getArray(g_Terrains.dirt),
		"actor": [
			[
				g_Gaia.tree1,
				g_Gaia.tree3,
				g_Gaia.fruitBush,
				g_Gaia.secondaryHuntableAnimal,
				g_Decoratives.grass,
				g_Decoratives.rockMedium,
				g_Decoratives.bushMedium
			],
			0.3
		],
		"textureHS": getArray(g_Terrains.cliff),
		"actorHS": [[g_Decoratives.grassShort, g_Decoratives.rockMedium, g_Decoratives.bushSmall], 0.1]
	},
	// 7 Hilltop forest
	{
		"texture": getArray(g_Terrains.forestFloor1),
		"actor": [
			[
				g_Gaia.tree1,
				g_Gaia.tree2,
				g_Gaia.tree3,
				g_Gaia.tree4,
				g_Gaia.tree5,
				g_Decoratives.tree,
				g_Decoratives.grass,
				g_Decoratives.rockMedium,
				g_Decoratives.bushMedium
			],
			0.5
		],
		"textureHS": getArray(g_Terrains.cliff),
		"actorHS": [[g_Decoratives.grassShort, g_Decoratives.rockMedium, g_Decoratives.bushSmall], 0.1]
	}
];

var mercenaryCampGuards = {
	"temperate": [
		{ "Template" : "structures/merc_camp_egyptian" },
		{ "Template" : "units/mace_infantry_javelinist_b", "Count" : 4 },
		{ "Template" : "units/mace_cavalry_spearman_e", "Count" : 3 },
		{ "Template" : "units/mace_infantry_archer_a", "Count" : 4 },
		{ "Template" : "units/mace_champion_infantry_a", "Count" : 3 }
	],
	"snowy": [
		{ "Template" : "structures/ptol_mercenary_camp" },
		{ "Template" : "units/brit_infantry_javelinist_b", "Count" : 4 },
		{ "Template" : "units/brit_cavalry_swordsman_e", "Count" : 3 },
		{ "Template" : "units/brit_infantry_slinger_a", "Count" : 4 },
		{ "Template" : "units/brit_champion_infantry", "Count" : 3 }
	],
	"desert": [
		{ "Template" : "structures/ptol_mercenary_camp" },
		{ "Template" : "units/pers_infantry_javelinist_b", "Count" : 4 },
		{ "Template" : "units/pers_cavalry_swordsman_e", "Count" : 3 },
		{ "Template" : "units/pers_infantry_archer_a", "Count" : 4 },
		{ "Template" : "units/pers_champion_infantry", "Count" : 3 }
	],
	"alpine": [
		{ "Template" : "structures/ptol_mercenary_camp" },
		{ "Template" : "units/rome_infantry_swordsman_b", "Count" : 4 },
		{ "Template" : "units/rome_cavalry_spearman_e", "Count" : 3 },
		{ "Template" : "units/rome_infantry_javelinist_a", "Count" : 4 },
		{ "Template" : "units/rome_champion_infantry", "Count" : 3 }
	],
	"mediterranean": [
		{ "Template" : "structures/merc_camp_egyptian" },
		{ "Template" : "units/iber_infantry_javelinist_b", "Count" : 4 },
		{ "Template" : "units/iber_cavalry_spearman_e", "Count" : 3 },
		{ "Template" : "units/iber_infantry_slinger_a", "Count" : 4 },
		{ "Template" : "units/iber_champion_infantry", "Count" : 3 }
	],
	"savanna": [
		{ "Template" : "structures/merc_camp_egyptian" },
		{ "Template" : "units/sele_infantry_javelinist_b", "Count" : 4 },
		{ "Template" : "units/sele_cavalry_spearman_merc_e", "Count" : 3 },
		{ "Template" : "units/sele_infantry_spearman_a", "Count" : 4 },
		{ "Template" : "units/sele_champion_infantry_swordsman", "Count" : 3 }
	],
	"tropic": [
		{ "Template" : "structures/merc_camp_egyptian" },
		{ "Template" : "units/ptol_infantry_javelinist_b", "Count" : 4 },
		{ "Template" : "units/ptol_cavalry_archer_e", "Count" : 3 },
		{ "Template" : "units/ptol_infantry_slinger_a", "Count" : 4 },
		{ "Template" : "units/ptol_champion_infantry_pikeman", "Count" : 3 }
	],
	"autumn": [
		{ "Template" : "structures/ptol_mercenary_camp" },
		{ "Template" : "units/gaul_infantry_javelinist_b", "Count" : 4 },
		{ "Template" : "units/gaul_cavalry_swordsman_e", "Count" : 3 },
		{ "Template" : "units/gaul_infantry_slinger_a", "Count" : 4 },
		{ "Template" : "units/gaul_champion_infantry", "Count" : 3 }
	]
};

/**
 * Resource spots and other points of interest
 */

// Mines
function placeMine(point, centerEntity,
	decorativeActors = [
		g_Decoratives.grass, g_Decoratives.grassShort,
		g_Decoratives.rockLarge, g_Decoratives.rockMedium,
		g_Decoratives.bushMedium, g_Decoratives.bushSmall
	]
)
{
	placeObject(point.x, point.y, centerEntity, 0, randFloat(0, TWO_PI));
	let quantity = randIntInclusive(11, 23);
	let dAngle = TWO_PI / quantity;
	for (let i = 0; i < quantity; ++i)
	{
		let angle = dAngle * randFloat(i, i + 1);
		let dist = randFloat(2, 5);
		placeObject(point.x + dist * Math.cos(angle), point.y + dist * Math.sin(angle), pickRandom(decorativeActors), 0, randFloat(0, 2 * PI));
	}
}

// Groves, only Wood
let groveActors = [g_Decoratives.grass, g_Decoratives.rockMedium, g_Decoratives.bushMedium];
let clGrove = createTileClass();

function placeGrove(point,
	groveEntities = [
		g_Gaia.tree1, g_Gaia.tree1, g_Gaia.tree1, g_Gaia.tree1, g_Gaia.tree1,
		g_Gaia.tree2, g_Gaia.tree2, g_Gaia.tree2, g_Gaia.tree2,
		g_Gaia.tree3, g_Gaia.tree3, g_Gaia.tree3,
		g_Gaia.tree4, g_Gaia.tree4, g_Gaia.tree5
	],
	groveActors = [g_Decoratives.grass, g_Decoratives.rockMedium, g_Decoratives.bushMedium], groveTileClass = undefined,
	groveTerrainTexture = getArray(g_Terrains.forestFloor1)
)
{
	placeObject(point.x, point.y, pickRandom(["structures/gaul_outpost", "gaia/flora_tree_oak_new"]), 0, randFloat(0, 2 * PI));
	let quantity = randIntInclusive(20, 30);
	let dAngle = TWO_PI / quantity;
	for (let i = 0; i < quantity; ++i)
	{
		let angle = dAngle * randFloat(i, i + 1);
		let dist = randFloat(2, 5);
		let objectList = groveEntities;
		if (i % 3 == 0)
			objectList = groveActors;
		let x = point.x + dist * Math.cos(angle);
		let y = point.y + dist * Math.sin(angle);
		placeObject(x, y, pickRandom(objectList), 0, randFloat(0, 2 * PI));
		if (groveTileClass)
			createArea(new ClumpPlacer(5, 1, 1, 1, floor(x), floor(y)), [new TerrainPainter(groveTerrainTexture), paintClass(groveTileClass)]);
		else
			createArea(new ClumpPlacer(5, 1, 1, 1, floor(x), floor(y)), [new TerrainPainter(groveTerrainTexture)]);
	}
}

var farmEntities = {
	"temperate": { "building": "structures/mace_farmstead", "animal": "gaia/fauna_pig" },
	"snowy": { "building": "structures/brit_farmstead", "animal": "gaia/fauna_sheep" },
	"desert": { "building": "structures/pers_farmstead", "animal": "gaia/fauna_camel" },
	"alpine": { "building": "structures/rome_farmstead", "animal": "gaia/fauna_sheep" },
	"mediterranean": { "building": "structures/iber_farmstead", "animal": "gaia/fauna_pig" },
	"savanna": { "building": "structures/sele_farmstead", "animal": "gaia/fauna_horse" },
	"tropic": { "building": "structures/ptol_farmstead", "animal": "gaia/fauna_camel" },
	"autumn": { "building": "structures/gaul_farmstead", "animal": "gaia/fauna_horse" }
};

g_WallStyles.other = {
	"overlap": 0,
	"fence": readyWallElement("other/fence_long", "gaia"),
	"fence_short": readyWallElement("other/fence_short", "gaia"),
	"bench":     { "angle": Math.PI / 2, "length": 1.5,  "indent": 0,    "bend": 0, "templateName": "other/bench" },
	"foodBin":   { "angle": Math.PI / 2, "length": 1.5,  "indent": 0,    "bend": 0, "templateName": "gaia/special_treasure_food_bin" },
	"animal":    { "angle": 0,           "length": 0,    "indent": 0.75, "bend": 0, "templateName": farmEntities[currentBiome()].animal },
	"farmstead": { "angle": Math.PI,     "length": 0,    "indent": -3,   "bend": 0, "templateName": farmEntities[currentBiome()].building }
};

let fences = [
	new Fortress("fence", [
		"foodBin", "farmstead", "bench",
		"turn_0.25", "animal", "turn_0.25", "fence",
		"turn_0.25", "animal", "turn_0.25", "fence",
		"turn_0.25", "animal", "turn_0.25", "fence"
	]),
	new Fortress("fence", [
		"foodBin", "farmstead", "fence",
		"turn_0.25", "animal", "turn_0.25", "fence",
		"turn_0.25", "animal", "turn_0.25", "bench", "animal", "fence",
		"turn_0.25", "animal", "turn_0.25", "fence"
	]),
	new Fortress("fence", [
		"foodBin", "farmstead", "turn_0.5", "bench", "turn_-0.5", "fence_short",
		"turn_0.25", "animal", "turn_0.25", "fence",
		"turn_0.25", "animal", "turn_0.25", "fence",
		"turn_0.25", "animal", "turn_0.25", "fence_short", "animal", "fence"
	]),
	new Fortress("fence", [
		"foodBin", "farmstead", "turn_0.5", "fence_short", "turn_-0.5", "bench",
		"turn_0.25", "animal", "turn_0.25", "fence",
		"turn_0.25", "animal", "turn_0.25", "fence",
		"turn_0.25", "animal", "turn_0.25", "fence_short", "animal", "fence"
	]),
	new Fortress("fence", [
		"foodBin", "farmstead", "fence",
		"turn_0.25", "animal", "turn_0.25", "bench", "animal", "fence",
		"turn_0.25", "animal", "turn_0.25", "fence_short", "animal", "fence",
		"turn_0.25", "animal", "turn_0.25", "fence_short", "animal", "fence"
	])
];
let num = fences.length;
for (let i = 0; i < num; ++i)
	fences.push(new Fortress("fence", clone(fences[i].wall).reverse()));

// Camps with fire and gold treasure
function placeCamp(point,
	centerEntity = "actor|props/special/eyecandy/campfire.xml",
	otherEntities = ["gaia/special_treasure_metal", "gaia/special_treasure_standing_stone",
		"units/brit_infantry_slinger_b", "units/brit_infantry_javelinist_b", "units/gaul_infantry_slinger_b", "units/gaul_infantry_javelinist_b", "units/gaul_champion_fanatic",
		"actor|props/special/common/waypoint_flag.xml", "actor|props/special/eyecandy/barrel_a.xml", "actor|props/special/eyecandy/basket_celt_a.xml", "actor|props/special/eyecandy/crate_a.xml", "actor|props/special/eyecandy/dummy_a.xml", "actor|props/special/eyecandy/handcart_1.xml", "actor|props/special/eyecandy/handcart_1_broken.xml", "actor|props/special/eyecandy/sack_1.xml", "actor|props/special/eyecandy/sack_1_rough.xml"
	]
)
{
	placeObject(point.x, point.y, centerEntity, 0, randFloat(0, TWO_PI));
	let quantity = randIntInclusive(5, 11);
	let dAngle = TWO_PI / quantity;
	for (let i = 0; i < quantity; ++i)
	{
		let angle = dAngle * randFloat(i, i + 1);
		let dist = randFloat(1, 3);
		placeObject(point.x + dist * Math.cos(angle), point.y + dist * Math.sin(angle), pickRandom(otherEntities), 0, randFloat(0, 2 * PI));
	}
}

function placeStartLocationResources(
	point,
	foodEntities = [g_Gaia.fruitBush, g_Gaia.chicken],
	groveEntities = [
		g_Gaia.tree1, g_Gaia.tree1, g_Gaia.tree1, g_Gaia.tree1, g_Gaia.tree1,
		g_Gaia.tree2, g_Gaia.tree2, g_Gaia.tree2, g_Gaia.tree2,
		g_Gaia.tree3, g_Gaia.tree3, g_Gaia.tree3,
		g_Gaia.tree4, g_Gaia.tree4, g_Gaia.tree5
	],
	groveTerrainTexture = getArray(g_Terrains.forestFloor1),
	averageDistToCC = 10,
	dAverageDistToCC = 2
)
{
	function getRandDist()
	{
		return averageDistToCC + randFloat(-dAverageDistToCC, dAverageDistToCC);
	}

	let currentAngle = randFloat(0, TWO_PI);
	// Stone
	let dAngle = TWO_PI * 2 / 9;
	let angle = currentAngle + randFloat(dAngle / 4, 3 * dAngle / 4);
	placeMine({ "x": point.x + averageDistToCC * Math.cos(angle), "y": point.y + averageDistToCC * Math.sin(angle) }, g_Gaia.stoneLarge);

	currentAngle += dAngle;

	// Wood
	let quantity = 80;
	dAngle = TWO_PI / quantity / 3;
	for (let i = 0; i < quantity; ++i)
	{
		angle = currentAngle + randFloat(0, dAngle);
		let dist = getRandDist();
		let objectList = groveEntities;
		if (i % 2 == 0)
			objectList = groveActors;
		let x = point.x + dist * Math.cos(angle);
		let y = point.y + dist * Math.sin(angle);
		placeObject(x, y, pickRandom(objectList), 0, randFloat(0, 2 * PI));
		createArea(new ClumpPlacer(5, 1, 1, 1, floor(x), floor(y)), [new TerrainPainter(groveTerrainTexture), paintClass(clGrove)]);
		currentAngle += dAngle;
	}

	// Metal
	dAngle = TWO_PI * 2 / 9;
	angle = currentAngle + randFloat(dAngle / 4, 3 * dAngle / 4);
	placeMine({ "x": point.x + averageDistToCC * Math.cos(angle), "y": point.y + averageDistToCC * Math.sin(angle) }, g_Gaia.metalLarge);
	currentAngle += dAngle;

	// Berries and domestic animals
	quantity = 15;
	dAngle = TWO_PI / quantity * 2 / 9;
	for (let i = 0; i < quantity; ++i)
	{
		angle = currentAngle + randFloat(0, dAngle);
		let dist = getRandDist();
		placeObject(point.x + dist * Math.cos(angle), point.y + dist * Math.sin(angle), pickRandom(foodEntities), 0, randFloat(0, 2 * PI));
		currentAngle += dAngle;
	}
}

log("Functions loaded after " + ((Date.now() - genStartTime) / 1000) + "s");

/**
 * Base terrain shape generation and settings
 */
 // Height range by map size
let heightScale = (g_Map.size + 256) / 768 / 4;
let heightRange = { "min": MIN_HEIGHT * heightScale, "max": MAX_HEIGHT * heightScale };

// Water coverage
let averageWaterCoverage = 1/5; // NOTE: Since terrain generation is quite unpredictable actual water coverage might vary much with the same value
let waterHeight = -MIN_HEIGHT + heightRange.min + averageWaterCoverage * (heightRange.max - heightRange.min); // Water height in environment and the engine
let waterHeightAdjusted = waterHeight + MIN_HEIGHT; // Water height as terrain height
setWaterHeight(waterHeight);

// Generate base terrain shape
let lowH = heightRange.min;
let medH = (heightRange.min + heightRange.max) / 2;

// Lake
let initialHeightmap = [
	[medH, medH, medH, medH, medH, medH],
	[medH, medH, medH, medH, medH, medH],
	[medH, medH, lowH, lowH, medH, medH],
	[medH, medH, lowH, lowH, medH, medH],
	[medH, medH, medH, medH, medH, medH],
	[medH, medH, medH, medH, medH, medH],
];
if (g_Map.size < 256)
{
	initialHeightmap = [
		[medH, medH, medH, medH, medH],
		[medH, medH, medH, medH, medH],
		[medH, medH, lowH, medH, medH],
		[medH, medH, medH, medH, medH],
		[medH, medH, medH, medH, medH]
	];
}
if (g_Map.size >= 384)
{
	initialHeightmap = [
		[medH, medH, medH, medH, medH, medH, medH, medH],
		[medH, medH, medH, medH, medH, medH, medH, medH],
		[medH, medH, medH, medH, medH, medH, medH, medH],
		[medH, medH, medH, lowH, lowH, medH, medH, medH],
		[medH, medH, medH, lowH, lowH, medH, medH, medH],
		[medH, medH, medH, medH, medH, medH, medH, medH],
		[medH, medH, medH, medH, medH, medH, medH, medH],
		[medH, medH, medH, medH, medH, medH, medH, medH],
	];
}

setBaseTerrainDiamondSquare(heightRange.min, heightRange.max, initialHeightmap, 0.8);

// Apply simple erosion
for (let i = 0; i < 5; ++i)
	splashErodeMap(0.1);
globalSmoothHeightmap();

// Final rescale
rescaleHeightmap(heightRange.min, heightRange.max);

Engine.SetProgress(25);

/**
 * Prepare terrain texture placement
 */
let heighLimits = [
	heightRange.min + 3/4 * (waterHeightAdjusted - heightRange.min), // 0 Deep water
	waterHeightAdjusted, // 1 Shallow water
	waterHeightAdjusted + 2/8 * (heightRange.max - waterHeightAdjusted), // 2 Shore
	waterHeightAdjusted + 3/8 * (heightRange.max - waterHeightAdjusted), // 3 Low ground
	waterHeightAdjusted + 4/8 * (heightRange.max - waterHeightAdjusted), // 4 Player and path height
	waterHeightAdjusted + 6/8 * (heightRange.max - waterHeightAdjusted), // 5 High ground
	waterHeightAdjusted + 7/8 * (heightRange.max - waterHeightAdjusted), // 6 Lower forest border
	heightRange.max // 7 Forest
];
let playerHeightRange = { "min" : heighLimits[3], "max" : heighLimits[4] };
let resourceSpotHeightRange = { "min" : (heighLimits[2] + heighLimits[3]) / 2, "max" : (heighLimits[4] + heighLimits[5]) / 2 };
let playerHeight = (playerHeightRange.min + playerHeightRange.max) / 2; // Average player height

log("Terrain shape generation and biome presets after " + ((Date.now() - genStartTime) / 1000) + "s");

let [playerIDs, startLocations] = sortPlayersByLocation(getStartLocationsByHeightmap(playerHeightRange, 1000, 30));

log("Start location chosen after " + ((Date.now() - genStartTime) / 1000) + "s");
Engine.SetProgress(30);

/**
 * Smooth Start Locations before height region calculation
 */
let playerBaseRadius = 35;
if (g_Map.size < 256)
	playerBaseRadius = 25;
for (let p = 0; p < playerIDs.length; ++p)
	rectangularSmoothToHeight(startLocations[p], playerBaseRadius, playerBaseRadius, playerHeight, 0.7);

/**
 * Calculate tile centered height map after start position smoothing but before placing paths
 * This has nothing to to with TILE_CENTERED_HEIGHT_MAP which should be false!
 */
let tchm = getTileCenteredHeightmap();

/**
 * Add paths (If any)
 */
let clPath = createTileClass();

/**
 * Divide tiles in areas by height and avoid paths
 */
let areas = [];
for (let h = 0; h < heighLimits.length; ++h)
	areas.push([]);

for (let x = 0; x < tchm.length; ++x)
{
	for (let y = 0; y < tchm[0].length; ++y)
	{
		if (g_Map.tileClasses[clPath].inclusionCount[x][y] > 0) // Avoid paths
			continue;

		let minHeight = heightRange.min;
		for (let h = 0; h < heighLimits.length; ++h)
		{
			if (tchm[x][y] >= minHeight && tchm[x][y] <= heighLimits[h])
			{
				areas[h].push({ "x": x, "y": y });
				break;
			}
			else
				minHeight = heighLimits[h];
		}
	}
}

/**
 * Get max slope of each area
 */
let slopeMap = getSlopeMap();
let minSlope = [];
let maxSlope = [];
for (let h = 0; h < heighLimits.length; ++h)
{
	minSlope[h] = Infinity;
	maxSlope[h] = 0;
	for (let t = 0; t < areas[h].length; ++t)
	{
		let x = areas[h][t].x;
		let y = areas[h][t].y;
		let slope = slopeMap[x][y];

		if (slope > maxSlope[h])
			maxSlope[h] = slope;

		if (slope < minSlope[h])
			minSlope[h] = slope;
	}
}

/**
 * Paint areas by height and slope
 */
for (let h = 0; h < heighLimits.length; ++h)
{
	for (let t = 0; t < areas[h].length; ++t)
	{
		let x = areas[h][t].x;
		let y = areas[h][t].y;
		let actor;
		let texture = pickRandom(wildLakeBiome[h].texture);

		if (slopeMap[x][y] < 0.5 * (minSlope[h] + maxSlope[h]))
		{
			if (randBool(wildLakeBiome[h].actor[1]))
				actor = pickRandom(wildLakeBiome[h].actor[0]);
		}
		else
		{
			texture = pickRandom(wildLakeBiome[h].textureHS);
			if (randBool(wildLakeBiome[h].actorHS[1]))
				actor = pickRandom(wildLakeBiome[h].actorHS[0]);
		}

		g_Map.texture[x][y] = g_Map.getTextureID(texture);

		if (actor)
			placeObject(randFloat(x, x + 1), randFloat(y, y + 1), actor, 0, randFloat(0, 2 * PI));
	}
}

log("Terrain texture placement finished after " + ((Date.now() - genStartTime) / 1000) + "s");
Engine.SetProgress(80);

/**
 * Get resource spots after players start locations calculation and paths
 */
let avoidPoints = clone(startLocations);
for (let i = 0; i < avoidPoints.length; ++i)
	avoidPoints[i].dist = 30;
let resourceSpots = getPointsByHeight(resourceSpotHeightRange, avoidPoints, clPath);

log("Resource spots chosen after " + ((Date.now() - genStartTime) / 1000) + "s");
Engine.SetProgress(55);

/**
 * Add start locations and resource spots after terrain texture and path painting
 */
for (let p = 0; p < playerIDs.length; ++p)
{
	let point = startLocations[p];
	placeCivDefaultEntities(point.x, point.y, playerIDs[p], { "iberWall": g_Map.size > 192 });
	placeStartLocationResources(point);
}

let mercenaryCamps = Math.ceil(g_Map.size / 256);
log("Maximum number of mercenary camps: " + uneval(mercenaryCamps));
for (let i = 0; i < resourceSpots.length; ++i)
{
	let choice = i % 5;
	if (choice == 0)
		placeMine(resourceSpots[i], g_Gaia.stoneLarge);
	if (choice == 1)
		placeMine(resourceSpots[i], g_Gaia.metalLarge);
	if (choice == 2)
		placeGrove(resourceSpots[i]);
	if (choice == 3)
	{
		placeCamp(resourceSpots[i]);
		rectangularSmoothToHeight(resourceSpots[i], 5, 5, g_Map.height[resourceSpots[i].x][resourceSpots[i].y] - 10, 0.5);
	}
	if (choice == 4)
	{
		if (mercenaryCamps)
		{
			placeStartingEntities(resourceSpots[i].x, resourceSpots[i].y, 0, mercenaryCampGuards[currentBiome()]);
			rectangularSmoothToHeight(resourceSpots[i], 15, 15, g_Map.height[resourceSpots[i].x][resourceSpots[i].y], 0.5);
			--mercenaryCamps;
		}
		else
		{
			placeCustomFortress(resourceSpots[i].x, resourceSpots[i].y, pickRandom(fences), "other", 0, randFloat(0, 2 * PI));
			rectangularSmoothToHeight(resourceSpots[i], 10, 10, g_Map.height[resourceSpots[i].x][resourceSpots[i].y], 0.5);
		}
	}
}

log("Map generation finished after " + ((Date.now() - genStartTime) / 1000) + "s");

ExportMap();
