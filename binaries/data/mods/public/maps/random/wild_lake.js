RMS.LoadLibrary("rmgen");
RMS.LoadLibrary("heightmap");

InitMap();

let genStartTime = Date.now();

/**
 * Returns an approximation of the heights of the tiles between the vertices, a tile centered heightmap
 * A tile centered heightmap is one smaller in width and height than an ordinary heightmap
 * It is meant to e.g. texture a map by height (x/y coordinates correspond to those of the terrain texture map)
 * Don't use this to override g_Map height (Potentially breaks the map)!
 * @param {array} [heightmap=g_Map.height] - A reliefmap the tile centered version should be build from
 */
function getTileCenteredHeightmap(heightmap = g_Map.height)
{
	let max_x = heightmap.length - 1;
	let max_y = heightmap[0].length - 1;
	let tchm = [];
	for (let x = 0; x < max_x; ++x)
	{
		tchm[x] = new Float32Array(max_y);
		for (let y = 0; y < max_y; ++y)
			tchm[x][y] = 0.25 * (heightmap[x][y] + heightmap[x + 1][y] + heightmap[x][y + 1] + heightmap[x + 1][y + 1]);
	}
	return tchm;
}

/**
 * Returns an inclination map corresponding to the tiles between the heightmaps vertices:
 * array of heightmap width-1 arrays of height-1 vectors (associative arrays) of the from:
 * {"x": x_slope, "y": y_slope] so a 2D Vector pointing to the hightest incline (with the length the incline in the vectors direction)
 * The x and y coordinates of a tile in the terrain texture map correspond to those of the inclination map
 * @param {array} [heightmap=g_Map.height] - The reliefmap the inclination map is to be generated from
 */
function getInclineMap(heightmap)
{
	heightmap = (heightmap || g_Map.height);
	let max_x = heightmap.length - 1;
	let max_y = heightmap[0].length - 1;
	let inclineMap = [];
	for (let x = 0; x < max_x; ++x)
	{
		inclineMap[x] = [];
		for (let y = 0; y < max_y; ++y)
		{
			let dx = heightmap[x + 1][y] - heightmap[x][y];
			let dy = heightmap[x][y + 1] - heightmap[x][y];
			let next_dx = heightmap[x + 1][y + 1] - heightmap[x][y + 1];
			let next_dy = heightmap[x + 1][y + 1] - heightmap[x + 1][y];
			inclineMap[x][y] = { "x" : 0.5 * (dx + next_dx), "y" : 0.5 * (dy + next_dy) };
		}
	}
	return inclineMap;
}

/**
 * Returns a slope map (same form as the a heightmap with one less width and height)
 * Not normalized. Only returns the steepness (float), not the direction of incline.
 * The x and y coordinates of a tile in the terrain texture map correspond to those of the slope map
 * @param {array} [inclineMap=getInclineMap(g_Map.height)] - A map with the absolute inclination for each tile
 */
function getSlopeMap(inclineMap = getInclineMap(g_Map.height))
{
	let max_x = inclineMap.length;
	let slopeMap = [];
	for (let x = 0; x < max_x; ++x)
	{
		let max_y = inclineMap[x].length;
		slopeMap[x] = new Float32Array(max_y);
		for (let y = 0; y < max_y; ++y)
			slopeMap[x][y] = Math.pow(inclineMap[x][y].x * inclineMap[x][y].x + inclineMap[x][y].y * inclineMap[x][y].y, 0.5);
	}
	return slopeMap;
}

/**
 * Returns the order to go through the points for the shortest closed path (array of indices)
 * @param {array} [points] - Points to be sorted of the form {"x": x_value, "y": y_value}
 */
function getOrderOfPointsForShortestClosePath(points)
{
	let order = [];
	let distances = [];
	if (points.length <= 3)
	{
		for (let i = 0; i < points.length; ++i)
			order.push(i);

		return order;
	}

	// Just add the first 3 points
	let pointsToAdd = deepcopy(points);
	for (let i = 0; i < 3; ++i)
	{
		order.push(i);
		pointsToAdd.shift(i);
		if (i)
			distances.push(getDistance(points[order[i]].x, points[order[i]].y, points[order[i - 1]].x, points[order[i - 1]].y));
	}

	distances.push(getDistance(
		points[order[0]].x,
		points[order[0]].y,
		points[order[order.length - 1]].x,
		points[order[order.length - 1]].y));

	// Add remaining points so the path lengthens the least
	let numPointsToAdd = pointsToAdd.length;
	for (let i = 0; i < numPointsToAdd; ++i)
	{
		let indexToAddTo;
		let minEnlengthen = Infinity;
		let minDist1 = 0;
		let minDist2 = 0;
		for (let k = 0; k < order.length; ++k)
		{
			let dist1 = getDistance(pointsToAdd[0].x, pointsToAdd[0].y, points[order[k]].x, points[order[k]].y);
			let dist2 = getDistance(pointsToAdd[0].x, pointsToAdd[0].y, points[order[(k + 1) % order.length]].x, points[order[(k + 1) % order.length]].y);
			let enlengthen = dist1 + dist2 - distances[k];
			if (enlengthen < minEnlengthen)
			{
				indexToAddTo = k;
				minEnlengthen = enlengthen;
				minDist1 = dist1;
				minDist2 = dist2;
			}
		}
		order.splice(indexToAddTo + 1, 0, i + 3);
		distances.splice(indexToAddTo, 1, minDist1, minDist2);
		pointsToAdd.shift();
	}

	return order;
}

function getGrad(wrapped = true, scalarField = g_Map.height)
{
	let vectorField = [];
	let max_x = scalarField.length;
	let max_y = scalarField[0].length;
	if (!wrapped)
	{
		max_x -= 1;
		max_y -= 1;
	}

	for (let x = 0; x < max_x; ++x)
	{
		vectorField.push([]);
		for (let y = 0; y < max_y; ++y)
		{
			vectorField[x].push({
				"x" : scalarField[(x + 1) % max_x][y] - scalarField[x][y],
				"y" : scalarField[x][(y + 1) % max_y] - scalarField[x][y]
			});
		}
	}

	return vectorField;
}

function splashErodeMap(strength = 1, heightmap = g_Map.height)
{
	let max_x = heightmap.length;
	let max_y = heightmap[0].length;

	let dHeight = getGrad(heightmap);

	for (let x = 0; x < max_x; ++x)
	{
		let next_x = (x + 1) % max_x;
		let prev_x = (x + max_x - 1) % max_x;
		for (let y = 0; y < max_y; ++y)
		{
			let next_y = (y + 1) % max_y;
			let prev_y = (y + max_y - 1) % max_y;

			let slopes = [- dHeight[x][y].x, - dHeight[x][y].y, dHeight[prev_x][y].x, dHeight[x][prev_y].y];

			let sumSlopes = 0;
			for (let i = 0; i < slopes.length; ++i)
				if (slopes[i] > 0)
					sumSlopes += slopes[i];

			let drain = [];
			for (let i = 0; i < slopes.length; ++i)
			{
				drain.push(0);
				if (slopes[i] > 0)
					drain[i] += min(strength * slopes[i] / sumSlopes, slopes[i]);
			}

			let sumDrain = 0;
			for (let i = 0; i < drain.length; ++i)
				sumDrain += drain[i];

			// Apply changes to maps
			heightmap[x][y] -= sumDrain;
			heightmap[next_x][y] += drain[0];
			heightmap[x][next_y] += drain[1];
			heightmap[prev_x][y] += drain[2];
			heightmap[x][prev_y] += drain[3];
		}
	}
}

/**
 * Meant to place e.g. resource spots within a height range
 * @param {object} [heightRange] - The height range in which to place the entities (An associative array with keys "min" and "max" each containing a float)
 * @param {object} [avoidPoints=[]] - An array of objects of the form { "x" : int, "y" : int, "dist" : int }, points that will be avoided in the given dist e.g. start locations
 * @param {object} [avoidClass=undefined] - TileClass to be avoided
 * @param {integer} [minDistance=20] - How many tile widths the entities to place have to be away from each other, start locations and the map border
 * @param {object} [heightmap=g_Map.height] - The reliefmap the entities should be distributed on
 * @param {integer} [maxTries=2 * g_Map.size] - How often random player distributions are rolled to be compared (256 to 1024)
 * @param {boolean} [isCircular=g_MapSettings.CircularMap] - If the map is circular or rectangular
 */
function getPointsByHeight(heightRange, avoidPoints = [], avoidClass = undefined, minDistance = 20, maxTries = 2 * g_Map.size, heightmap = g_Map.height, isCircular = g_MapSettings.CircularMap)
{
	let points = [];
	let placements = deepcopy(avoidPoints);
	let validVertices = [];
	let r = 0.5 * (heightmap.length - 1); // Map center x/y as well as radius
	let avoidMap;

	if (avoidClass !== undefined)
		avoidMap = g_Map.tileClasses[avoidClass].inclusionCount;

	for (let x = minDistance; x < heightmap.length - minDistance; ++x)
	{
		for (let y = minDistance; y < heightmap[0].length - minDistance; ++y)
		{
			if (avoidClass !== undefined && // Avoid adjecting tiles in avoidClass
			    (avoidMap[max(x - 1, 0)][y] > 0 ||
			    avoidMap[x][max(y - 1, 0)] > 0 ||
			    avoidMap[min(x + 1, avoidMap.length - 1)][y] > 0 ||
			    avoidMap[x][min(y + 1, avoidMap[0].length - 1)] > 0))
				continue;

			if (heightmap[x][y] > heightRange.min && heightmap[x][y] < heightRange.max && // Has correct height
			    (!isCircular || r - getDistance(x, y, r, r) >= minDistance)) // Enough distance to map border
				validVertices.push({ "x": x, "y": y , "dist": minDistance });
		}
	}

	for (let tries = 0; tries < maxTries; ++tries)
	{
		let point = pickRandom(validVertices);
		if (placements.every(p => getDistance(p.x, p.y, point.x, point.y) > max(minDistance, p.dist)))
		{
			points.push(point);
			placements.push(point);
		}
		if (tries != 0 && tries % 100 == 0) // Time Check
			log(points.length + " points found after " + tries + " tries after " + ((Date.now() - genStartTime) / 1000) + "s");
	}

	return points;
}


/**
 * getArray - To ensure a terrain texture is contained within an array
 */
function getArray(stringOrArrayOfStrings)
{
	if (typeof stringOrArrayOfStrings == "string")
		return [stringOrArrayOfStrings];
	return stringOrArrayOfStrings;
}


/**
 * Biome settings
 */

randomizeBiome()

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

// Mercenary camps
var mercenaryCampGuards = [
	// Temperate 1 mace
	[
		{ "Template" : "structures/merc_camp_egyptian" },
		{ "Template" : "units/mace_infantry_javelinist_b", "Count" : 4 },
		{ "Template" : "units/mace_cavalry_spearman_e", "Count" : 3 },
		{ "Template" : "units/mace_infantry_archer_a", "Count" : 4 },
		{ "Template" : "units/mace_champion_infantry_a", "Count" : 3 }
	],
	// Snowy 2 brit
	[
		{ "Template" : "structures/ptol_mercenary_camp" },
		{ "Template" : "units/brit_infantry_javelinist_b", "Count" : 4 },
		{ "Template" : "units/brit_cavalry_swordsman_e", "Count" : 3 },
		{ "Template" : "units/brit_infantry_slinger_a", "Count" : 4 },
		{ "Template" : "units/brit_champion_infantry", "Count" : 3 }
	],
	// Desert 3 pers
	[
		{ "Template" : "structures/ptol_mercenary_camp" },
		{ "Template" : "units/pers_infantry_javelinist_b", "Count" : 4 },
		{ "Template" : "units/pers_cavalry_swordsman_e", "Count" : 3 },
		{ "Template" : "units/pers_infantry_archer_a", "Count" : 4 },
		{ "Template" : "units/pers_champion_infantry", "Count" : 3 }
	],
	// Alpine 4 rome
	[
		{ "Template" : "structures/ptol_mercenary_camp" },
		{ "Template" : "units/rome_infantry_swordsman_b", "Count" : 4 },
		{ "Template" : "units/rome_cavalry_spearman_e", "Count" : 3 },
		{ "Template" : "units/rome_infantry_javelinist_a", "Count" : 4 },
		{ "Template" : "units/rome_champion_infantry", "Count" : 3 }
	],
	// Mediterranean 5 iber
	[
		{ "Template" : "structures/merc_camp_egyptian" },
		{ "Template" : "units/iber_infantry_javelinist_b", "Count" : 4 },
		{ "Template" : "units/iber_cavalry_spearman_e", "Count" : 3 },
		{ "Template" : "units/iber_infantry_slinger_a", "Count" : 4 },
		{ "Template" : "units/iber_champion_infantry", "Count" : 3 }
	],
	// Savanna 6 sele
	[
		{ "Template" : "structures/merc_camp_egyptian" },
		{ "Template" : "units/sele_infantry_javelinist_b", "Count" : 4 },
		{ "Template" : "units/sele_cavalry_spearman_merc_e", "Count" : 3 },
		{ "Template" : "units/sele_infantry_spearman_a", "Count" : 4 },
		{ "Template" : "units/sele_champion_infantry_swordsman", "Count" : 3 }
	],
	// Tropic 7 ptol
	[
		{ "Template" : "structures/merc_camp_egyptian" },
		{ "Template" : "units/ptol_infantry_javelinist_b", "Count" : 4 },
		{ "Template" : "units/ptol_cavalry_archer_e", "Count" : 3 },
		{ "Template" : "units/ptol_infantry_slinger_a", "Count" : 4 },
		{ "Template" : "units/ptol_champion_infantry_pikeman", "Count" : 3 }
	],
	// Autumn 8 gaul
	[
		{ "Template" : "structures/ptol_mercenary_camp" },
		{ "Template" : "units/gaul_infantry_javelinist_b", "Count" : 4 },
		{ "Template" : "units/gaul_cavalry_swordsman_e", "Count" : 3 },
		{ "Template" : "units/gaul_infantry_slinger_a", "Count" : 4 },
		{ "Template" : "units/gaul_champion_infantry", "Count" : 3 }
	]
];

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

// Food, fences with domestic animals
let farmEntities = [
	// Temperate 1 mace
	{ "building" : "structures/mace_farmstead", "animal" : "gaia/fauna_pig" },
	// Snowy 2 brit
	{ "building" : "structures/brit_farmstead", "animal" : "gaia/fauna_sheep" },
	// Desert 3 pers
	{ "building" : "structures/pers_farmstead", "animal" : "gaia/fauna_camel" },
	// Alpine 4 rome
	{ "building" : "structures/rome_farmstead", "animal" : "gaia/fauna_sheep" },
	// Mediterranean 5 iber
	{ "building" : "structures/iber_farmstead", "animal" : "gaia/fauna_pig" },
	// Savanna 6 sele
	{ "building" : "structures/sele_farmstead", "animal" : "gaia/fauna_horse" },
	// Tropic 7 ptol
	{ "building" : "structures/ptol_farmstead", "animal" : "gaia/fauna_camel" },
	// Autumn 8 gaul
	{ "building" : "structures/gaul_farmstead", "animal" : "gaia/fauna_horse" }
];
wallStyles.other.sheepIn = new WallElement("sheepIn", farmEntities[g_BiomeID - 1].animal, PI / 4, -1.5, 0.75, PI/2);
wallStyles.other.foodBin = new WallElement("foodBin", "gaia/special_treasure_food_bin", PI/2, 1.5);
wallStyles.other.sheep = new WallElement("sheep", farmEntities[g_BiomeID - 1].animal, 0, 0, 0.75);
wallStyles.other.farm = new WallElement("farm", farmEntities[g_BiomeID - 1].building, PI, 0, -3);
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
	fences.push(new Fortress("fence", deepcopy(fences[i].wall).reverse()));

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
		return averageDistToCC + randFloat(-dAverageDistToCC, dAverageDistToCC)
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
let higH = heightRange.max;

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

RMS.SetProgress(25);

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

/**
 * Get start locations
 */
let startLocations = getStartLocationsByHeightmap(playerHeightRange, 1000, 30);

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
				maxTeamDist = Math.max(dist, maxTeamDist);
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
		let actor = undefined;
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
RMS.SetProgress(80);

/**
 * Get resource spots after players start locations calculation and paths
 */
let avoidPoints = deepcopy(startLocations);
for (let i = 0; i < avoidPoints.length; ++i)
	avoidPoints[i].dist = 30;
let resourceSpots = getPointsByHeight(resourceSpotHeightRange, avoidPoints, clPath);

log("Resource spots chosen after " + ((Date.now() - genStartTime) / 1000) + "s");
RMS.SetProgress(55);

/**
 * Add start locations and resource spots after terrain texture and path painting
 */
for (let p = 0; p < playerIDs.length; ++p)
{
	let point = startLocations[p];
	placeCivDefaultEntities(point.x, point.y, playerIDs[p], { "iberWall": g_Map.size > 192 });
	placeStartLocationResources(point);
}

let mercenaryCamps = ceil(g_Map.size / 256);
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
			createStartingPlayerEntities(resourceSpots[i].x, resourceSpots[i].y, 0, mercenaryCampGuards[g_BiomeID - 1]);
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
