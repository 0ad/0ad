RMS.LoadLibrary("rmgen");
RMS.LoadLibrary("rmbiome");
RMS.LoadLibrary("heightmap");

InitMap();

let genStartTime = Date.now();

/**
 * Drags a path to a target height smoothing it at the edges and return some points along the path.
 */
function placeRandomPathToHeight(
	start, target, targetHeight, tileClass = undefined, texture = "road_rome_a",
	width = 10, distance = 4, strength = 0.08, heightmap = g_Map.height)
{
	let pathPoints = [];
	let position = clone(start);
	while (true)
	{
		rectangularSmoothToHeight(position, width, width, targetHeight, strength, heightmap);
		if (texture)
		{
			if (tileClass !== undefined)
				createArea(new ClumpPlacer(0.3 * width * width, 1, 1, 1, floor(position.x), floor(position.y)),
					[new TerrainPainter(texture), paintClass(tileClass)]);
			else
				createArea(new ClumpPlacer(0.3 * width * width, 1, 1, 1, floor(position.x), floor(position.y)),
					new TerrainPainter(texture));
		}
		pathPoints.push({ "x": position.x, "y": position.y, "dist": distance });
		// Check for distance to target and setup for next loop if needed
		if (getDistance(position.x, position.y, target.x, target.y) < distance / 2)
			break;
		let angleToTarget = getAngle(position.x, position.y, target.x, target.y);
		let angleOff = randFloat(-PI/2, PI/2);
		position.x += distance * cos(angleToTarget + angleOff);
		position.y += distance * sin(angleToTarget + angleOff);
	}
	return pathPoints;
}

/**
 * Design resource spots
 */
// Mines
let decorations = [
	"actor|geology/gray1.xml", "actor|geology/gray_rock1.xml",
	"actor|geology/highland1.xml", "actor|geology/highland2.xml", "actor|geology/highland3.xml",
	"actor|geology/highland_c.xml", "actor|geology/highland_d.xml", "actor|geology/highland_e.xml",
	"actor|props/flora/bush.xml", "actor|props/flora/bush_dry_a.xml", "actor|props/flora/bush_highlands.xml",
	"actor|props/flora/bush_tempe_a.xml", "actor|props/flora/bush_tempe_b.xml", "actor|props/flora/ferns.xml"
];

function placeMine(point, centerEntity)
{
	placeObject(point.x, point.y, centerEntity, 0, randFloat(0, TWO_PI));
	let quantity = randIntInclusive(11, 23);
	let dAngle = TWO_PI / quantity;
	for (let i = 0; i < quantity; ++i)
	{
		let angle = dAngle * randFloat(i, i + 1);
		let dist = randFloat(2, 5);
		placeObject(point.x + dist * Math.cos(angle), point.y + dist * Math.sin(angle), pickRandom(decorations), 0, randFloat(0, 2 * PI));
	}
}

// Food, fences with domestic animals
wallStyles.other.sheepIn = new WallElement("sheepIn", "gaia/fauna_sheep", PI / 4, -1.5, 0.75, PI/2);
wallStyles.other.foodBin = new WallElement("foodBin", "gaia/special_treasure_food_bin", PI/2, 1.5);
wallStyles.other.sheep = new WallElement("sheep", "gaia/fauna_sheep", 0, 0, 0.75);
wallStyles.other.farm = new WallElement("farm", "structures/brit_farmstead", PI, 0, -3);
let fences = [
	new Fortress("fence", ["foodBin", "farm", "bench", "sheepIn", "fence", "sheepIn", "fence", "sheepIn", "fence"]),
	new Fortress("fence", ["foodBin", "farm", "fence", "sheepIn", "fence", "sheepIn", "bench", "sheep", "fence", "sheepIn", "fence"]),
	new Fortress("fence", [
		"foodBin", "farm", "cornerIn", "bench", "cornerOut", "fence_short", "sheepIn", "fence", "sheepIn",
		"fence", "sheepIn", "fence_short", "sheep", "fence"
	]),
	new Fortress("fence", [
		"foodBin", "farm", "cornerIn", "fence_short", "cornerOut", "bench", "sheepIn", "fence", "sheepIn",
		"fence", "sheepIn", "fence_short", "sheep", "fence"
	]),
	new Fortress("fence", [
		"foodBin", "farm", "fence", "sheepIn", "bench", "sheep", "fence", "sheepIn",
		"fence_short", "sheep", "fence", "sheepIn", "fence_short", "sheep", "fence"
	])
];
let num = fences.length;
for (let i = 0; i < num; ++i)
	fences.push(new Fortress("fence", clone(fences[i].wall).reverse()));

// Groves, only Wood
let groveEntities = ["gaia/flora_bush_temperate", "gaia/flora_tree_euro_beech"];
let groveActors = [
	"actor|geology/highland1_moss.xml", "actor|geology/highland2_moss.xml",
	"actor|props/flora/bush.xml", "actor|props/flora/bush_dry_a.xml", "actor|props/flora/bush_highlands.xml",
	"actor|props/flora/bush_tempe_a.xml", "actor|props/flora/bush_tempe_b.xml", "actor|props/flora/ferns.xml"
];
let clGrove = createTileClass();

function placeGrove(point)
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
		createArea(new ClumpPlacer(5, 1, 1, 1, floor(x), floor(y)), [new TerrainPainter("temp_grass_plants"), paintClass(clGrove)]);
	}
}

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

function placeStartLocationResources(point, foodEntities = ["gaia/flora_bush_berry", "gaia/fauna_chicken", "gaia/fauna_chicken"])
{
	let currentAngle = randFloat(0, TWO_PI);
	// Stone and chicken
	let dAngle = TWO_PI * 2 / 9;
	let angle = currentAngle + randFloat(dAngle / 4, 3 * dAngle / 4);
	let dist = 12;
	let x = point.x + dist * Math.cos(angle);
	let y = point.y + dist * Math.sin(angle);
	placeMine({ "x": x, "y": y }, "gaia/geology_stonemine_temperate_quarry");

	currentAngle += dAngle;

	// Wood
	let quantity = 80;
	dAngle = TWO_PI / quantity / 3;
	for (let i = 0; i < quantity; ++i)
	{
		angle = currentAngle + randFloat(0, dAngle);
		dist = randFloat(10, 15);
		let objectList = groveEntities;
		if (i % 2 == 0)
			objectList = groveActors;
		x = point.x + dist * Math.cos(angle);
		y = point.y + dist * Math.sin(angle);
		placeObject(x, y, pickRandom(objectList), 0, randFloat(0, 2 * PI));
		createArea(new ClumpPlacer(5, 1, 1, 1, floor(x), floor(y)), [new TerrainPainter("temp_grass_plants"), paintClass(clGrove)]);
		currentAngle += dAngle;
	}

	// Metal and chicken
	dAngle = TWO_PI * 2 / 9;
	angle = currentAngle + randFloat(dAngle / 4, 3 * dAngle / 4);
	dist = 13;
	x = point.x + dist * Math.cos(angle);
	y = point.y + dist * Math.sin(angle);
	placeMine({ "x": x, "y": y }, "gaia/geology_metal_temperate_slabs");
	currentAngle += dAngle;

	// Berries
	quantity = 15;
	dAngle = TWO_PI / quantity * 2 / 9;
	for (let i = 0; i < quantity; ++i)
	{
		angle = currentAngle + randFloat(0, dAngle);
		dist = randFloat(10, 15);
		x = point.x + dist * Math.cos(angle);
		y = point.y + dist * Math.sin(angle);
		placeObject(x, y, pickRandom(foodEntities), 0, randFloat(0, 2 * PI));
		currentAngle += dAngle;
	}
}

log("Functions loaded after " + ((Date.now() - genStartTime) / 1000) + "s");

/**
 * Environment settings
 */
setBiome("alpine");
g_Environment.Fog.FogColor = { "r": 0.8, "g": 0.8, "b": 0.8, "a": 0.01 };
g_Environment.Water.WaterBody.Colour = { "r" : 0.3, "g" : 0.05, "b" : 0.1, "a" : 0.1 };
g_Environment.Water.WaterBody.Murkiness = 0.4;

/**
 * Base terrain shape generation and settings
 */
 // Height range by map size
let heightScale = (g_Map.size + 256) / 768 / 4;
let heightRange = { "min": MIN_HEIGHT * heightScale, "max": MAX_HEIGHT * heightScale };

// Water coverage
let averageWaterCoverage = 1/5; // NOTE: Since terrain generation is quite unpredictable actual water coverage might vary much with the same value
let waterHeight = -MIN_HEIGHT + heightRange.min + averageWaterCoverage * (heightRange.max - heightRange.min); // Water height in environment and the engine
let waterHeightAdjusted = waterHeight + MIN_HEIGHT; // Water height in RMGEN
setWaterHeight(waterHeight);

// Generate base terrain shape
let medH = (heightRange.min + heightRange.max) / 2;
let initialHeightmap = [[medH, medH], [medH, medH]];
setBaseTerrainDiamondSquare(heightRange.min, heightRange.max, initialHeightmap, 0.8);

// Apply simple erosion
for (let i = 0; i < 5; ++i)
	splashErodeMap(0.1);

// Final rescale
rescaleHeightmap(heightRange.min, heightRange.max);

RMS.SetProgress(25);

/**
 * Prepare terrain texture placement
 */
let heighLimits = [
	heightRange.min + 1/3 * (waterHeightAdjusted - heightRange.min), // 0 Deep water
	heightRange.min + 2/3 * (waterHeightAdjusted - heightRange.min), // 1 Medium Water
	heightRange.min + (waterHeightAdjusted - heightRange.min), // 2 Shallow water
	waterHeightAdjusted + 1/8 * (heightRange.max - waterHeightAdjusted), // 3 Shore
	waterHeightAdjusted + 2/8 * (heightRange.max - waterHeightAdjusted), // 4 Low ground
	waterHeightAdjusted + 3/8 * (heightRange.max - waterHeightAdjusted), // 5 Player and path height
	waterHeightAdjusted + 4/8 * (heightRange.max - waterHeightAdjusted), // 6 High ground
	waterHeightAdjusted + 5/8 * (heightRange.max - waterHeightAdjusted), // 7 Lower forest border
	waterHeightAdjusted + 6/8 * (heightRange.max - waterHeightAdjusted), // 8 Forest
	waterHeightAdjusted + 7/8 * (heightRange.max - waterHeightAdjusted), // 9 Upper forest border
	waterHeightAdjusted + (heightRange.max - waterHeightAdjusted)]; // 10 Hilltop
let playerHeight = (heighLimits[4] + heighLimits[5]) / 2; // Average player height

// Texture and actor presets
let myBiome = [];
myBiome.push({ // 0 Deep water
	"texture": ["shoreline_stoney_a"],
	"actor": [["gaia/fauna_fish", "actor|geology/stone_granite_boulder.xml"], 0.02],
	"textureHS": ["alpine_mountainside"], "actorHS": [["gaia/fauna_fish"], 0.1]
});
myBiome.push({ // 1 Medium Water
	"texture": ["shoreline_stoney_a", "alpine_shore_rocks"],
	"actor": [["actor|geology/stone_granite_boulder.xml", "actor|geology/stone_granite_med.xml"], 0.03],
	"textureHS": ["alpine_mountainside"], "actorHS": [["actor|geology/stone_granite_boulder.xml", "actor|geology/stone_granite_med.xml"], 0.0]
});
myBiome.push({ // 2 Shallow water
	"texture": ["alpine_shore_rocks"],
	"actor": [["actor|props/flora/reeds_pond_dry.xml", "actor|geology/stone_granite_large.xml", "actor|geology/stone_granite_med.xml", "actor|props/flora/reeds_pond_lush_b.xml"], 0.2],
	"textureHS": ["alpine_mountainside"], "actorHS": [["actor|props/flora/reeds_pond_dry.xml", "actor|geology/stone_granite_med.xml"], 0.1]
});
myBiome.push({ // 3 Shore
	"texture": ["alpine_shore_rocks_grass_50", "alpine_grass_rocky"],
	"actor": [["gaia/flora_tree_pine", "gaia/flora_bush_badlands", "actor|geology/highland1_moss.xml", "actor|props/flora/grass_soft_tuft_a.xml", "actor|props/flora/bush.xml"], 0.3],
	"textureHS": ["alpine_mountainside"], "actorHS": [["actor|props/flora/grass_soft_tuft_a.xml"], 0.1]
});
myBiome.push({ // 4 Low ground
	"texture": ["alpine_dirt_grass_50", "alpine_grass_rocky"],
	"actor": [["actor|geology/stone_granite_med.xml", "actor|props/flora/grass_soft_tuft_a.xml", "actor|props/flora/bush.xml", "actor|props/flora/grass_medit_flowering_tall.xml"], 0.2],
	"textureHS": ["alpine_grass_rocky"], "actorHS": [["actor|geology/stone_granite_med.xml", "actor|props/flora/grass_soft_tuft_a.xml"], 0.1]
});
myBiome.push({ // 5 Player and path height
	"texture": ["new_alpine_grass_c", "new_alpine_grass_b", "new_alpine_grass_d"],
	"actor": [["actor|geology/stone_granite_small.xml", "actor|props/flora/grass_soft_small.xml", "actor|props/flora/grass_medit_flowering_tall.xml"], 0.2],
	"textureHS": ["alpine_grass_rocky"], "actorHS": [["actor|geology/stone_granite_small.xml", "actor|props/flora/grass_soft_small.xml"], 0.1]
});
myBiome.push({ // 6 High ground
	"texture": ["new_alpine_grass_a", "alpine_grass_rocky"],
	"actor": [["actor|geology/stone_granite_med.xml", "actor|props/flora/grass_tufts_a.xml", "actor|props/flora/bush_highlands.xml", "actor|props/flora/grass_medit_flowering_tall.xml"], 0.2],
	"textureHS": ["alpine_grass_rocky"], "actorHS": [["actor|geology/stone_granite_med.xml", "actor|props/flora/grass_tufts_a.xml"], 0.1]
});
myBiome.push({ // 7 Lower forest border
	"texture": ["new_alpine_grass_mossy", "alpine_grass_rocky"],
	"actor": [["gaia/flora_tree_pine", "gaia/flora_tree_oak", "actor|props/flora/grass_tufts_a.xml", "gaia/flora_bush_berry", "actor|geology/highland2_moss.xml", "gaia/fauna_goat", "actor|props/flora/bush_tempe_underbrush.xml"], 0.3],
	"textureHS": ["alpine_cliff_c"], "actorHS": [["actor|props/flora/grass_tufts_a.xml", "actor|geology/highland2_moss.xml"], 0.1]
});
myBiome.push({ // 8 Forest
	"texture": ["alpine_forrestfloor"],
	"actor": [["gaia/flora_tree_pine", "gaia/flora_tree_pine", "gaia/flora_tree_pine", "gaia/flora_tree_pine", "actor|geology/highland2_moss.xml", "actor|props/flora/bush_highlands.xml"], 0.5],
	"textureHS": ["alpine_cliff_c"], "actorHS": [["actor|geology/highland2_moss.xml", "actor|geology/stone_granite_med.xml"], 0.1]
});
myBiome.push({ // 9 Upper forest border
	"texture": ["alpine_forrestfloor_snow", "new_alpine_grass_dirt_a"],
	"actor": [["gaia/flora_tree_pine", "actor|geology/snow1.xml"], 0.3],
	"textureHS": ["alpine_cliff_b"], "actorHS": [["actor|geology/stone_granite_med.xml", "actor|geology/snow1.xml"], 0.1]
});
myBiome.push({ // 10 Hilltop
	"texture": ["alpine_cliff_a", "alpine_cliff_snow"],
	"actor": [["actor|geology/highland1.xml"], 0.05],
	"textureHS": ["alpine_cliff_c"], "actorHS": [["actor|geology/highland1.xml"], 0.0]
});

log("Terrain shape generation and texture presets after " + ((Date.now() - genStartTime) / 1000) + "s");

/**
 * Get start locations
 */
let startLocations = getStartLocationsByHeightmap({ "min": heighLimits[4], "max": heighLimits[5] }, 1000, 30);

// Sort start locations to form a "ring"
let startLocationOrder = getOrderOfPointsForShortestClosePath(startLocations);
let newStartLocations = [];
for (let i = 0; i < startLocations.length; ++i)
	newStartLocations.push(startLocations[startLocationOrder[i]]);
startLocations = newStartLocations;

// Sort players by team
let playerIDs = [];
let teams = [];
for (let i = 0; i < g_MapSettings.PlayerData.length - 1; ++i)
{
	playerIDs.push(i+1);
	let t = g_MapSettings.PlayerData[i + 1].Team;
	if (teams.indexOf(t) == -1 && t !== undefined)
		teams.push(t);
}
playerIDs = sortPlayers(playerIDs);

// Minimize maximum distance between players within a team
if (teams.length)
{
	let minDistance = Infinity;
	let bestShift;
	for (let s = 0; s < playerIDs.length; ++s)
	{
		let maxTeamDist = 0;
		for (let pi = 0; pi < playerIDs.length - 1; ++pi)
		{
			let p1 = playerIDs[(pi + s) % playerIDs.length] - 1;
			let t1 = getPlayerTeam(p1);

			if (teams.indexOf(t1) === -1)
				continue;

			for (let pj = pi + 1; pj < playerIDs.length; ++pj)
			{
				let p2 = playerIDs[(pj + s) % playerIDs.length] - 1;
				let t2 = getPlayerTeam(p2);
				if (t2 != t1)
					continue;

				let l1 = startLocations[pi];
				let l2 = startLocations[pj];
				let dist = getDistance(l1.x, l1.y, l2.x, l2.y);
				if (dist > maxTeamDist)
					maxTeamDist = dist;
			}
		}

		if (maxTeamDist < minDistance)
		{
			minDistance = maxTeamDist;
			bestShift = s;
		}
	}
	if (bestShift)
	{
		let newPlayerIDs = [];
		for (let i = 0; i < playerIDs.length; ++i)
			newPlayerIDs.push(playerIDs[(i + bestShift) % playerIDs.length]);
		playerIDs = newPlayerIDs;
	}
}

log("Start location chosen after " + ((Date.now() - genStartTime) / 1000) + "s");
RMS.SetProgress(30);

/**
 * Smooth Start Locations before height region calculation
 */
for (let p = 0; p < playerIDs.length; ++p)
	rectangularSmoothToHeight(startLocations[p], 35, 35, playerHeight, 0.7);

/**
 * Add paths
 */
let tchm = getTileCenteredHeightmap(); // Calculate tileCenteredHeightMap (This has nothing to to with TILE_CENTERED_HEIGHT_MAP which should be false)
let pathPoints = [];
let clPath = createTileClass();
for (let i = 0; i < startLocations.length; ++i)
{
	let start = startLocations[i];
	let target = startLocations[(i + 1) % startLocations.length];
	pathPoints = pathPoints.concat(placeRandomPathToHeight(start, target, playerHeight, clPath));
}

log("Paths placed after " + ((Date.now() - genStartTime) / 1000) + "s");
RMS.SetProgress(45);

/**
 * Get resource spots after players start locations calculation
 */
let avoidPoints = clone(startLocations);
for (let i = 0; i < avoidPoints.length; ++i)
	avoidPoints[i].dist = 30;
let resourceSpots = getPointsByHeight({ "min": (heighLimits[3] + heighLimits[4]) / 2, "max": (heighLimits[5] + heighLimits[6]) / 2 }, avoidPoints, clPath);

log("Resource spots chosen after " + ((Date.now() - genStartTime) / 1000) + "s");
RMS.SetProgress(55);

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
let slopeMap = getSlopeMap(); // Calculate slope map
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
		let actor = undefined;
		let texture = pickRandom(myBiome[h].texture);

		if (slopeMap[x][y] < 0.4 * (minSlope[h] + maxSlope[h]))
		{
			if (randBool(myBiome[h].actor[1]))
				actor = pickRandom(myBiome[h].actor[0]);
		}
		else
		{
			texture = pickRandom(myBiome[h].textureHS);
			if (randBool(myBiome[h].actorHS[1]))
				actor = pickRandom(myBiome[h].actorHS[0]);
		}

		g_Map.texture[x][y] = g_Map.getTextureID(texture);

		if (actor)
			placeObject(randFloat(x, x + 1), randFloat(y, y + 1), actor, 0, randFloat(0, 2 * PI));
	}
}

log("Terrain texture placement finished after " + ((Date.now() - genStartTime) / 1000) + "s");
RMS.SetProgress(80);

/**
 * Add start locations and resource spots after terrain texture and path painting
 */
for (let p = 0; p < playerIDs.length; ++p)
{
	let point = startLocations[p];
	placeCivDefaultEntities(point.x, point.y, playerIDs[p], { "iberWall": true });
	placeStartLocationResources(startLocations[p]);
}

for (let i = 0; i < resourceSpots.length; ++i)
{
	let choice = i % 5;
	if (choice == 0)
		placeMine(resourceSpots[i], "gaia/geology_stonemine_temperate_formation");
	if (choice == 1)
		placeMine(resourceSpots[i], "gaia/geology_metal_temperate_slabs");
	if (choice == 2)
		placeCustomFortress(resourceSpots[i].x, resourceSpots[i].y, pickRandom(fences), "other", 0, randFloat(0, 2 * PI));
	if (choice == 3)
		placeGrove(resourceSpots[i]);
	if (choice == 4)
		placeCamp(resourceSpots[i]);
}

/**
 * Stop Timer
 */
log("Map generation finished after " + ((Date.now() - genStartTime) / 1000) + "s");

/**
 * Export map data
 */
ExportMap();
