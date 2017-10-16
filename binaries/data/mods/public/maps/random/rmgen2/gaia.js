var g_Props = {
	"barrels": "actor|props/special/eyecandy/barrels_buried.xml",
	"crate": "actor|props/special/eyecandy/crate_a.xml",
	"cart": "actor|props/special/eyecandy/handcart_1_broken.xml",
	"well": "actor|props/special/eyecandy/well_1_c.xml",
	"skeleton": "actor|props/special/eyecandy/skeleton.xml",
};

/**
 * Create bluffs, i.e. a slope hill reachable from ground level.
 * Fill it with wood, mines, animals and decoratives.
 *
 * @param {Array} constraint - where to place them
 * @param {number} size - size of the bluffs (1.2 would be 120% of normal)
 * @param {number} deviation - degree of deviation from the defined size (0.2 would be 20% plus/minus)
 * @param {number} fill - size of map to fill (1.5 would be 150% of normal)
 * @param {number} baseHeight - elevation of the floor, making the bluff reachable
 */
function addBluffs(constraint, size, deviation, fill, baseHeight)
{
	var constrastTerrain = g_Terrains.tier2Terrain;

	if (currentBiome() == "tropic")
		constrastTerrain = g_Terrains.dirt;

	if (currentBiome() == "autumn")
		constrastTerrain = g_Terrains.tier3Terrain;

	var count = fill * 15;
	var minSize = 5;
	var maxSize = 7;
	var elevation = 30;
	var spread = 100;

	for (var i = 0; i < count; ++i)
	{
		var offset = getRandomDeviation(size, deviation);

		var rendered = createAreas(
			new ChainPlacer(Math.floor(minSize * offset), Math.floor(maxSize * offset), Math.floor(spread * offset), 0.5),
			[
				new LayeredPainter([g_Terrains.cliff, g_Terrains.mainTerrain, constrastTerrain], [2, 3]),
				new SmoothElevationPainter(ELEVATION_MODIFY, Math.floor(elevation * offset), 2),
				paintClass(g_TileClasses.bluff)
			],
			constraint,
			1);

		// Find the bounding box of the bluff
		if (rendered[0] === undefined)
			continue;

		var points = rendered[0].points;

		var corners = findCorners(points);

		// Seed an array the size of the bounding box
		var bb = createBoundingBox(points, corners);

		// Get a random starting position for the baseline and the endline
		var angle = randIntInclusive(0, 3);
		var opAngle = angle - 2;
		if (angle < 2)
			opAngle = angle + 2;

		// Find the edges of the bluff
		var baseLine;
		var endLine;

		// If we can't access the bluff, try different angles
		var retries = 0;
		var bluffCat = 2;
		while (bluffCat != 0 && retries < 5)
		{
			baseLine = findClearLine(bb, corners, angle, baseHeight);
			endLine = findClearLine(bb, corners, opAngle, baseHeight);

			bluffCat = unreachableBluff(bb, corners, baseLine, endLine);
			++angle;
			if (angle > 3)
				angle = 0;

			opAngle = angle - 2;
			if (angle < 2)
				opAngle = angle + 2;

			++retries;
		}

		// Inaccessible, turn it into a plateau
		if (bluffCat > 0)
		{
			removeBluff(points);
			continue;
		}

		// Create an entrance area by using a small margin
		var margin = 0.08;
		var ground = createTerrain(g_Terrains.mainTerrain);
		var slopeLength = (1 - margin) * getDistance(baseLine.midX, baseLine.midZ, endLine.midX, endLine.midZ);

		// Adjust the height of each point in the bluff
		for (var p = 0; p < points.length; ++p)
		{
			var pt = points[p];
			var dist = distanceOfPointFromLine(baseLine.x1, baseLine.z1, baseLine.x2, baseLine.z2, pt.x, pt.z);

			var curHeight = g_Map.getHeight(pt.x, pt.z);
			var newHeight = curHeight - curHeight * (dist / slopeLength) - 2;

			newHeight = Math.max(newHeight, endLine.height);

			if (newHeight <= endLine.height + 2 && g_Map.validT(pt.x, pt.z) && g_Map.getTexture(pt.x, pt.z).indexOf('cliff') > -1)
				ground.place(pt.x, pt.z);

			g_Map.setHeight(pt.x, pt.z, newHeight);
		}

		// Smooth out the ground around the bluff
		fadeToGround(bb, corners.minX, corners.minZ, endLine.height);
	}

	addElements([
		{
			"func": addHills,
			"avoid": [
				g_TileClasses.hill, 3,
				g_TileClasses.player, 20,
				g_TileClasses.valley, 2,
				g_TileClasses.water, 2
			],
			"stay": [g_TileClasses.bluff, 3],
			"sizes": g_AllSizes,
			"mixes": g_AllMixes,
			"amounts": g_AllAmounts
		}
	]);

	addElements([
		{
			"func": addLayeredPatches,
			"avoid": [
				g_TileClasses.dirt, 5,
				g_TileClasses.forest, 2,
				g_TileClasses.mountain, 2,
				g_TileClasses.player, 12,
				g_TileClasses.water, 3
			],
			"stay": [g_TileClasses.bluff, 5],
			"sizes": ["normal"],
			"mixes": ["normal"],
			"amounts": ["normal"]
		}
	]);

	addElements([
		{
			"func": addDecoration,
			"avoid": [
				g_TileClasses.forest, 2,
				g_TileClasses.player, 12,
				g_TileClasses.water, 3
			],
			"stay": [g_TileClasses.bluff, 5],
			"sizes": ["normal"],
			"mixes": ["normal"],
			"amounts": ["normal"]
		}
	]);

	addElements([
		{
			"func": addProps,
			"avoid": [
				g_TileClasses.forest, 2,
				g_TileClasses.player, 12,
				g_TileClasses.prop, 40,
				g_TileClasses.water, 3
			],
			"stay": [
				g_TileClasses.bluff, 7,
				g_TileClasses.mountain, 7
			],
			"sizes": ["normal"],
			"mixes": ["normal"],
			"amounts": ["scarce"]
		}
	]);

	addElements(shuffleArray([
		{
			"func": addForests,
			"avoid": [
				g_TileClasses.berries, 5,
				g_TileClasses.forest, 18,
				g_TileClasses.metal, 5,
				g_TileClasses.mountain, 5,
				g_TileClasses.player, 20,
				g_TileClasses.rock, 5,
				g_TileClasses.water, 2
			],
			"stay": [g_TileClasses.bluff, 6],
			"sizes": g_AllSizes,
			"mixes": g_AllMixes,
			"amounts": ["normal", "many", "tons"]
		},
		{
			"func": addMetal,
			"avoid": [
				g_TileClasses.berries, 5,
				g_TileClasses.forest, 5,
				g_TileClasses.mountain, 2,
				g_TileClasses.player, 50,
				g_TileClasses.rock, 15,
				g_TileClasses.metal, 40,
				g_TileClasses.water, 3
			],
			"stay": [g_TileClasses.bluff, 6],
			"sizes": ["normal"],
			"mixes": ["same"],
			"amounts": ["normal"]
		},
		{
			"func": addStone,
			"avoid": [
				g_TileClasses.berries, 5,
				g_TileClasses.forest, 5,
				g_TileClasses.mountain, 2,
				g_TileClasses.player, 50,
				g_TileClasses.rock, 40,
				g_TileClasses.metal, 15,
				g_TileClasses.water, 3
			],
			"stay": [g_TileClasses.bluff, 6],
			"sizes": ["normal"],
			"mixes": ["same"],
			"amounts": ["normal"]
		}
	]));

	let savanna = currentBiome() == "savanna";
	addElements(shuffleArray([
		{
			"func": addStragglerTrees,
			"avoid": [
				g_TileClasses.berries, 5,
				g_TileClasses.forest, 10,
				g_TileClasses.metal, 5,
				g_TileClasses.mountain, 1,
				g_TileClasses.player, 12,
				g_TileClasses.rock, 5,
				g_TileClasses.water, 5
			],
			"stay": [g_TileClasses.bluff, 6],
			"sizes": savanna ? ["big"] : g_AllSizes,
			"mixes": savanna ? ["varied"] : g_AllMixes,
			"amounts": savanna ? ["tons"] : ["normal", "many", "tons"]
		},
		{
			"func": addAnimals,
			"avoid": [
				g_TileClasses.animals, 20,
				g_TileClasses.forest, 5,
				g_TileClasses.mountain, 1,
				g_TileClasses.player, 20,
				g_TileClasses.rock, 5,
				g_TileClasses.metal, 5,
				g_TileClasses.water, 3
			],
			"stay": [g_TileClasses.bluff, 6],
			"sizes": g_AllSizes,
			"mixes": g_AllMixes,
			"amounts": ["normal", "many", "tons"]
		},
		{
			"func": addBerries,
			"avoid": [
				g_TileClasses.berries, 50,
				g_TileClasses.forest, 5,
				g_TileClasses.metal, 10,
				g_TileClasses.mountain, 2,
				g_TileClasses.player, 20,
				g_TileClasses.rock, 10,
				g_TileClasses.water, 3
			],
			"stay": [g_TileClasses.bluff, 6],
			"sizes": g_AllSizes,
			"mixes": g_AllMixes,
			"amounts": ["normal", "many", "tons"]
		}
	]));
}

/**
 * Add grass, rocks and bushes.
 */
function addDecoration(constraint, size, deviation, fill)
{
	var offset = getRandomDeviation(size, deviation);
	var decorations = [
		[
			new SimpleObject(g_Decoratives.rockMedium, offset, 3 * offset, 0, offset)
		],
		[
			new SimpleObject(g_Decoratives.rockLarge, offset, 2 * offset, 0, offset),
			new SimpleObject(g_Decoratives.rockMedium, offset, 3 * offset, 0, 2 * offset)
		],
		[
			new SimpleObject(g_Decoratives.grassShort, offset, 2 * offset, 0, offset)
		],
		[
			new SimpleObject(g_Decoratives.grass, 2 * offset, 4 * offset, 0, 1.8 * offset),
			new SimpleObject(g_Decoratives.grassShort, 3 * offset, 6 * offset, 1.2 * offset, 2.5 * offset)
		],
		[
			new SimpleObject(g_Decoratives.bushMedium, offset, 2 * offset, 0, 2 * offset),
			new SimpleObject(g_Decoratives.bushSmall, 2 * offset, 4 * offset, 0, 2 * offset)
		]
	];

	var baseCount = 1;
	if (currentBiome() == "tropic")
		baseCount = 8;

	var counts = [
		scaleByMapSize(16, 262),
		scaleByMapSize(8, 131),
		baseCount * scaleByMapSize(13, 200),
		baseCount * scaleByMapSize(13, 200),
		baseCount * scaleByMapSize(13, 200)
	];

	for (var i = 0; i < decorations.length; ++i)
	{
		var decorCount = Math.floor(counts[i] * fill);
		var group = new SimpleGroup(decorations[i], true);
		createObjectGroupsDeprecated(group, 0, constraint, decorCount, 5);
	}
}

/**
 * Create varying elevations.
 *
 * @param {Array} constraint - avoid/stay-classes
 *
 * @param {Object} el - the element to be rendered, for example:
 *  "class": g_TileClasses.hill,
 *	"painter": [g_Terrains.mainTerrain, g_Terrains.mainTerrain],
 *	"size": 1,
 *	"deviation": 0.2,
 *	"fill": 1,
 *	"count": scaleByMapSize(4, 8),
 *	"minSize": Math.floor(scaleByMapSize(3, 8)),
 *	"maxSize": Math.floor(scaleByMapSize(5, 10)),
 *	"spread": Math.floor(scaleByMapSize(10, 20)),
 *	"minElevation": 6,
 *	"maxElevation": 12,
 *	"steepness": 1.5
 */

function addElevation(constraint, el)
{
	var count = el.fill * el.count;
	var minSize = el.minSize;
	var maxSize = el.maxSize;
	var spread = el.spread;

	var elType = ELEVATION_MODIFY;
	if (el.class == g_TileClasses.water)
		elType = ELEVATION_SET;

	var widths = [];

	// Allow for shore and cliff rendering
	for (var s = el.painter.length; s > 2; --s)
		widths.push(1);

	for (var i = 0; i < count; ++i)
	{
		var elevation = randIntExclusive(el.minElevation, el.maxElevation);
		var smooth = Math.floor(elevation / el.steepness);

		var offset = getRandomDeviation(el.size, el.deviation);
		var pMinSize = Math.floor(minSize * offset);
		var pMaxSize = Math.floor(maxSize * offset);
		var pSpread = Math.floor(spread * offset);
		var pSmooth = Math.abs(Math.floor(smooth * offset));
		var pElevation = Math.floor(elevation * offset);

		pElevation = Math.max(el.minElevation, Math.min(pElevation, el.maxElevation));
		pMinSize = Math.min(pMinSize, pMaxSize);
		pMaxSize = Math.min(pMaxSize, el.maxSize);
		pMinSize = Math.max(pMaxSize, el.minSize);
		pSmooth = Math.max(pSmooth, 1);

		createAreas(
			new ChainPlacer(pMinSize, pMaxSize, pSpread, 0.5),
			[
				new LayeredPainter(el.painter, [widths.concat(pSmooth)]),
				new SmoothElevationPainter(elType, pElevation, pSmooth),
				paintClass(el.class)
			],
			constraint,
			1);
	}
}

/**
 * Create rolling hills.
 */
function addHills(constraint, size, deviation, fill)
{
	addElevation(constraint, {
		"class": g_TileClasses.hill,
		"painter": [g_Terrains.mainTerrain, g_Terrains.mainTerrain],
		"size": size,
		"deviation": deviation,
		"fill": fill,
		"count": 8,
		"minSize": 5,
		"maxSize": 8,
		"spread": 20,
		"minElevation": 6,
		"maxElevation": 12,
		"steepness": 1.5
	});
}

/**
 * Create random lakes with fish in it.
 */
function addLakes(constraint, size, deviation, fill)
{
	var lakeTile = g_Terrains.water;

	if (currentBiome() == "temperate" || currentBiome() == "tropic")
		lakeTile = g_Terrains.dirt;

	if (currentBiome() == "mediterranean")
		lakeTile = g_Terrains.tier2Terrain;

	if (currentBiome() == "autumn")
		lakeTile = g_Terrains.shore;

	addElevation(constraint, {
		"class": g_TileClasses.water,
		"painter": [lakeTile, lakeTile],
		"size": size,
		"deviation": deviation,
		"fill": fill,
		"count": 6,
		"minSize": 7,
		"maxSize": 9,
		"spread": 70,
		"minElevation": -15,
		"maxElevation": -2,
		"steepness": 1.5
	});

	addElements([
		{
			"func": addFish,
			"avoid": [
				g_TileClasses.fish, 12,
				g_TileClasses.hill, 8,
				g_TileClasses.mountain, 8,
				g_TileClasses.player, 8
			],
			"stay": [g_TileClasses.water, 7],
			"sizes": g_AllSizes,
			"mixes": g_AllMixes,
			"amounts": ["normal", "many", "tons"]
		}
	]);

	var group = new SimpleGroup([new SimpleObject(g_Decoratives.rockMedium, 1, 3, 1, 3)], true, g_TileClasses.dirt);
	createObjectGroupsDeprecated(group, 0, [stayClasses(g_TileClasses.water, 1), borderClasses(g_TileClasses.water, 4, 3)], 1000, 100);

	group = new SimpleGroup([new SimpleObject(g_Decoratives.reeds, 10, 15, 1, 3), new SimpleObject(g_Decoratives.rockMedium, 1, 3, 1, 3)], true, g_TileClasses.dirt);
	createObjectGroupsDeprecated(group, 0, [stayClasses(g_TileClasses.water, 2), borderClasses(g_TileClasses.water, 4, 3)], 1000, 100);
}

/**
 * Universal function to create layered patches.
 */
function addLayeredPatches(constraint, size, deviation, fill)
{
	var minRadius = 1;
	var maxRadius = Math.floor(scaleByMapSize(3, 5));
	var count = fill * scaleByMapSize(15, 45);

	var patchSizes = [
		scaleByMapSize(3, 6),
		scaleByMapSize(5, 10),
		scaleByMapSize(8, 21)
	];

	for (let patchSize of patchSizes)
	{
		var offset = getRandomDeviation(size, deviation);
		var patchMinRadius = Math.floor(minRadius * offset);
		var patchMaxRadius = Math.floor(maxRadius * offset);

		createAreas(
			new ChainPlacer(Math.min(patchMinRadius, patchMaxRadius), patchMaxRadius, Math.floor(patchSize * offset), 0.5),
			[
				new LayeredPainter(
					[
						[g_Terrains.mainTerrain, g_Terrains.tier1Terrain],
						[g_Terrains.tier1Terrain, g_Terrains.tier2Terrain],
						[g_Terrains.tier2Terrain, g_Terrains.tier3Terrain],
						[g_Terrains.tier4Terrain]
					],
					[1, 1]),
				paintClass(g_TileClasses.dirt)
			],
			constraint,
			count * offset);
	}
}

/**
 * Create steep mountains.
 */
function addMountains(constraint, size, deviation, fill)
{
	addElevation(constraint, {
		"class": g_TileClasses.mountain,
		"painter": [g_Terrains.cliff, g_Terrains.hill],
		"size": size,
		"deviation": deviation,
		"fill": fill,
		"count": 8,
		"minSize": 2,
		"maxSize": 4,
		"spread": 100,
		"minElevation": 100,
		"maxElevation": 120,
		"steepness": 4
	});
}

/**
 * Create plateaus.
 */
function addPlateaus(constraint, size, deviation, fill)
{
	var plateauTile = g_Terrains.dirt;

	if (currentBiome() == "snowy")
		plateauTile = g_Terrains.tier1Terrain;

	if (currentBiome() == "alpine" || currentBiome() == "savanna")
		plateauTile = g_Terrains.tier2Terrain;

	if (currentBiome() == "autumn")
		plateauTile = g_Terrains.tier4Terrain;

	addElevation(constraint, {
		"class": g_TileClasses.plateau,
		"painter": [g_Terrains.cliff, plateauTile],
		"size": size,
		"deviation": deviation,
		"fill": fill,
		"count": 15,
		"minSize": 2,
		"maxSize": 4,
		"spread": 200,
		"minElevation": 20,
		"maxElevation": 30,
		"steepness": 8
	});

	for (var i = 0; i < 40; ++i)
	{
		var hillElevation = randIntInclusive(4, 18);
		createAreas(
			new ChainPlacer(3, 15, 1, 0.5),
			[
				new LayeredPainter([plateauTile, plateauTile], [3]),
				new SmoothElevationPainter(ELEVATION_MODIFY, hillElevation, hillElevation - 2),
				paintClass(g_TileClasses.hill)
			],
			[
				avoidClasses(g_TileClasses.hill, 7),
				stayClasses(g_TileClasses.plateau, 7)
			],
			1);
	}

	addElements([
		{
			"func": addDecoration,
			"avoid": [
				g_TileClasses.dirt, 15,
				g_TileClasses.forest, 2,
				g_TileClasses.player, 12,
				g_TileClasses.water, 3
			],
			"stay": [g_TileClasses.plateau, 8],
			"sizes": ["normal"],
			"mixes": ["normal"],
			"amounts": ["tons"]
		},
		{
			"func": addProps,
			"avoid": [
				g_TileClasses.forest, 2,
				g_TileClasses.player, 12,
				g_TileClasses.prop, 40,
				g_TileClasses.water, 3
			],
			"stay": [g_TileClasses.plateau, 8],
			"sizes": ["normal"],
			"mixes": ["normal"],
			"amounts": ["scarce"]
		}
	]);
}

/**
 * Place less usual decoratives like barrels or crates.
 */
function addProps(constraint, size, deviation, fill)
{
	var offset = getRandomDeviation(size, deviation);

	var props = [
		[
			new SimpleObject(g_Props.skeleton, offset, 5 * offset, 0, 3 * offset + 2),
		],
		[
			new SimpleObject(g_Props.barrels, offset, 2 * offset, 2, 3 * offset + 2),
			new SimpleObject(g_Props.cart, 0, offset, 5, 2.5 * offset + 5),
			new SimpleObject(g_Props.crate, offset, 2 * offset, 2, 2 * offset + 2),
			new SimpleObject(g_Props.well, 0, 1, 2, 2 * offset + 2)
		]
	];

	var baseCount = 1;

	var counts = [
		scaleByMapSize(16, 262),
		scaleByMapSize(8, 131),
		baseCount * scaleByMapSize(13, 200),
		baseCount * scaleByMapSize(13, 200),
		baseCount * scaleByMapSize(13, 200)
	];

	// Add small props
	for (var i = 0; i < props.length; ++i)
	{
		var propCount = Math.floor(counts[i] * fill);
		var group = new SimpleGroup(props[i], true);
		createObjectGroupsDeprecated(group, 0, constraint, propCount, 5);
	}

	// Add decorative trees
	var trees = new SimpleObject(g_Decoratives.tree, 5 * offset, 30 * offset, 2, 3 * offset + 10);
	createObjectGroupsDeprecated(new SimpleGroup([trees], true), 0, constraint, counts[0] * 5 * fill, 5);
}

function addValleys(constraint, size, deviation, fill, baseHeight)
{
	if (baseHeight < 6)
		return;

	let minElevation = Math.max(-baseHeight, 1 - baseHeight / (size * (deviation + 1)));

	var valleySlope = g_Terrains.tier1Terrain;
	var valleyFloor = g_Terrains.tier4Terrain;

	if (currentBiome() == "desert")
	{
		valleySlope = g_Terrains.tier3Terrain;
		valleyFloor = g_Terrains.dirt;
	}

	if (currentBiome() == "mediterranean")
	{
		valleySlope = g_Terrains.tier2Terrain;
		valleyFloor = g_Terrains.dirt;
	}

	if (currentBiome() == "alpine" || currentBiome() == "savanna")
		valleyFloor = g_Terrains.tier2Terrain;

	if (currentBiome() == "tropic")
		valleySlope = g_Terrains.dirt;

	if (currentBiome() == "autumn")
		valleyFloor = g_Terrains.tier3Terrain;

	addElevation(constraint, {
		"class": g_TileClasses.valley,
		"painter": [valleySlope, valleyFloor],
		"size": size,
		"deviation": deviation,
		"fill": fill,
		"count": 8,
		"minSize": 5,
		"maxSize": 8,
		"spread": 30,
		"minElevation": minElevation,
		"maxElevation": -2,
		"steepness": 4
	});
}

/**
 * Create huntable animals.
 */
function addAnimals(constraint, size, deviation, fill)
{
	var groupOffset = getRandomDeviation(size, deviation);

	var animals = [
		[new SimpleObject(g_Gaia.mainHuntableAnimal, 5 * groupOffset, 7 * groupOffset, 0, 4 * groupOffset)],
		[new SimpleObject(g_Gaia.secondaryHuntableAnimal, 2 * groupOffset, 3 * groupOffset, 0, 2 * groupOffset)]
	];

	for (let animal of animals)
		createObjectGroupsDeprecated(
			new SimpleGroup(animal, true, g_TileClasses.animals),
			0,
			constraint,
			Math.floor(30 * fill),
			50);
}

function addBerries(constraint, size, deviation, fill)
{
	let groupOffset = getRandomDeviation(size, deviation);

	createObjectGroupsDeprecated(
		new SimpleGroup([new SimpleObject(g_Gaia.fruitBush, 5 * groupOffset, 5 * groupOffset, 0, 3 * groupOffset)], true, g_TileClasses.berries),
		0,
		constraint,
		Math.floor(50 * fill),
		40);
}

function addFish(constraint, size, deviation, fill)
{
	var groupOffset = getRandomDeviation(size, deviation);

	var fishes = [
		[new SimpleObject(g_Gaia.fish, groupOffset, 2 * groupOffset, 0, 2 * groupOffset)],
		[new SimpleObject(g_Gaia.fish, 2 * groupOffset, 4 * groupOffset, 10 * groupOffset, 20 * groupOffset)]
	];

	for (let fish of fishes)
		createObjectGroupsDeprecated(
			new SimpleGroup(fish, true, g_TileClasses.fish),
			0,
			constraint,
			Math.floor(40 * fill),
			50);
}

function addForests(constraint, size, deviation, fill)
{
	if (currentBiome() == "savanna")
		return;

	let treeTypes = [
		[
			g_Terrains.forestFloor2 + TERRAIN_SEPARATOR + g_Gaia.tree1,
			g_Terrains.forestFloor2 + TERRAIN_SEPARATOR + g_Gaia.tree2,
			g_Terrains.forestFloor2
		],
		[
			g_Terrains.forestFloor1 + TERRAIN_SEPARATOR + g_Gaia.tree4,
			g_Terrains.forestFloor1 + TERRAIN_SEPARATOR + g_Gaia.tree5,
			g_Terrains.forestFloor1
		]
	];

	let forestTypes = [
		[
			[g_Terrains.forestFloor2, g_Terrains.mainTerrain, treeTypes[0]],
			[g_Terrains.forestFloor2, treeTypes[0]]
		],
		[
			[g_Terrains.forestFloor2, g_Terrains.mainTerrain, treeTypes[1]],
			[g_Terrains.forestFloor1, treeTypes[1]]],
		[
			[g_Terrains.forestFloor1, g_Terrains.mainTerrain, treeTypes[0]],
			[g_Terrains.forestFloor2, treeTypes[0]]],
		[
			[g_Terrains.forestFloor1, g_Terrains.mainTerrain, treeTypes[1]],
			[g_Terrains.forestFloor1, treeTypes[1]]
		]
	];

	for (let forestType of forestTypes)
	{
		let offset = getRandomDeviation(size, deviation);
		createAreas(
			new ChainPlacer(1, Math.floor(scaleByMapSize(3, 5) * offset), Math.floor(50 * offset), 0.5),
			[
				new LayeredPainter(forestType, [2]),
				paintClass(g_TileClasses.forest)
			],
			constraint,
			10 * fill);
	}
}

function addMetal(constraint, size, deviation, fill)
{
	var offset = getRandomDeviation(size, deviation);
	createObjectGroupsDeprecated(
		new SimpleGroup([new SimpleObject(g_Gaia.metalLarge, offset, offset, 0, 4 * offset)], true, g_TileClasses.metal),
		0,
		constraint,
		1 + 20 * fill,
		100);
}

function addSmallMetal(constraint, size, mixes, amounts)
{
	let deviation = getRandomDeviation(size, mixes);
	createObjectGroupsDeprecated(
		new SimpleGroup([new SimpleObject(g_Gaia.metalSmall, 2 * deviation, 5 * deviation, deviation, 3 * deviation)], true, g_TileClasses.metal),
		0,
		constraint,
		1 + 20 * amounts,
		100);
}

/**
 * Create stone mines.
 */
function addStone(constraint, size, deviation, fill)
{
	var offset = getRandomDeviation(size, deviation);

	var mines = [
		[
			new SimpleObject(g_Gaia.stoneSmall, 0, 2 * offset, 0, 4 * offset),
			new SimpleObject(g_Gaia.stoneLarge, offset, offset, 0, 4 * offset)
		],
		[
			new SimpleObject(g_Gaia.stoneSmall, 2 * offset, 5 * offset, offset, 3 * offset)
		]
	];

	for (let mine of mines)
		createObjectGroupsDeprecated(
			new SimpleGroup(mine, true, g_TileClasses.rock),
			0,
			constraint,
			1 + 20 * fill,
			100);
}

/**
 * Create straggler trees.
 */
function addStragglerTrees(constraint, size, deviation, fill)
{
	// Ensure minimum distribution on african biome
	if (currentBiome() == "savanna")
	{
		fill = Math.max(fill, 2);
		size = Math.max(size, 1);
	}

	var trees = [g_Gaia.tree1, g_Gaia.tree2, g_Gaia.tree3, g_Gaia.tree4];

	var treesPerPlayer = 40;
	var playerBonus = Math.max(1, (getNumPlayers() - 3) / 2);

	var offset = getRandomDeviation(size, deviation);
	var treeCount = treesPerPlayer * playerBonus * fill;
	var totalTrees = scaleByMapSize(treeCount, treeCount);

	var count = Math.floor(totalTrees / trees.length) * fill;
	var min = offset;
	var max = 4 * offset;
	var minDist = offset;
	var maxDist = 5 * offset;

	// More trees for the african biome
	if (currentBiome() == "savanna")
	{
		min = 3 * offset;
		max = 5 * offset;
		minDist = 2 * offset + 1;
		maxDist = 3 * offset + 2;
	}

	for (var i = 0; i < trees.length; ++i)
	{
		var treesMax = max;

		// Don't clump fruit trees
		if (i == 2 && (currentBiome() == "desert" || currentBiome() == "mediterranean"))
			treesMax = 1;

		min = Math.min(min, treesMax);

		var group = new SimpleGroup([new SimpleObject(trees[i], min, treesMax, minDist, maxDist)], true, g_TileClasses.forest);
		createObjectGroupsDeprecated(group, 0, constraint, count);
	}
}

///////////
// Terrain Helpers
///////////

/**
 * Determine if the endline of the bluff is within the tilemap.
 *
 * @returns {Number} 0 if the bluff is reachable, otherwise a positive number
 */
function unreachableBluff(bb, corners, baseLine, endLine)
{
	// If we couldn't find a slope line
	if (typeof baseLine.midX === "undefined" || typeof endLine.midX === "undefined")
		return 1;

	// If the end points aren't on the tilemap
	if (!g_Map.validT(endLine.x1, endLine.z1) && !g_Map.validT(endLine.x2, endLine.z2))
		return 2;

	var minTilesInGroup = 1;
	var insideBluff = false;
	var outsideBluff = false;

	// If there aren't enough points in each row
	for (var x = 0; x < bb.length; ++x)
	{
		var count = 0;
		for (var z = 0; z < bb[x].length; ++z)
		{
			if (!bb[x][z].isFeature)
				continue;

			var valid = g_Map.validT(x + corners.minX, z + corners.minZ);

			if (valid)
				++count;

			if (!insideBluff && valid)
				insideBluff = true;

			if (outsideBluff && valid)
				return 3;
		}

		// We're expecting the end of the bluff
		if (insideBluff && count < minTilesInGroup)
			outsideBluff = true;
	}

	var insideBluff = false;
	var outsideBluff = false;

	// If there aren't enough points in each column
	for (var z = 0; z < bb[0].length; ++z)
	{
		var count = 0;
		for (var x = 0; x < bb.length; ++x)
		{
			if (!bb[x][z].isFeature)
				continue;

			var valid = g_Map.validT(x + corners.minX, z + corners.minZ);

			if (valid)
				++count;

			if (!insideBluff && valid)
				insideBluff = true;

			if (outsideBluff && valid)
				return 3;
		}

		// We're expecting the end of the bluff
		if (insideBluff && count < minTilesInGroup)
			outsideBluff = true;
	}

	// Bluff is reachable
	return 0;
}

/**
 * Remove the bluff class and turn it into a plateau.
 */
function removeBluff(points)
{
	for (var i = 0; i < points.length; ++i)
		addToClass(points[i].x, points[i].z, g_TileClasses.mountain);
}

/**
 * Create an array of points the fill a bounding box around a terrain feature.
 */
function createBoundingBox(points, corners)
{
	var bb = [];
	var width = corners.maxX - corners.minX + 1;
	var length = corners.maxZ - corners.minZ + 1;
	for (var w = 0; w < width; ++w)
	{
		bb[w] = [];
		for (var l = 0; l < length; ++l)
		{
			var curHeight = g_Map.getHeight(w + corners.minX, l + corners.minZ);
			bb[w][l] = {
				"height": curHeight,
				"isFeature": false
			};
		}
	}

	// Define the coordinates that represent the bluff
	for (var p = 0; p < points.length; ++p)
	{
		var pt = points[p];
		bb[pt.x - corners.minX][pt.z - corners.minZ].isFeature = true;
	}

	return bb;
}

/**
 * Flattens the ground touching a terrain feature.
 */
function fadeToGround(bb, minX, minZ, elevation)
{
	var ground = createTerrain(g_Terrains.mainTerrain);
	for (var x = 0; x < bb.length; ++x)
		for (var z = 0; z < bb[x].length; ++z)
		{
			var pt = bb[x][z];
			if (!pt.isFeature && nextToFeature(bb, x, z))
			{
				var newEl = smoothElevation(x + minX, z + minZ);
				g_Map.setHeight(x + minX, z + minZ, newEl);
				ground.place(x + minX, z + minZ);
			}
		}
}

/**
 * Find a 45 degree line in a bounding box that does not intersect any terrain feature.
 */
function findClearLine(bb, corners, angle, baseHeight)
{
	// Angle - 0: northwest; 1: northeast; 2: southeast; 3: southwest
	var z = corners.maxZ;
	var xOffset = -1;
	var zOffset = -1;

	switch(angle)
	{
		case 1:
			xOffset = 1;
			break;
		case 2:
			xOffset = 1;
			zOffset = 1;
			z = corners.minZ;
			break;
		case 3:
			zOffset = 1;
			z = corners.minZ;
			break;
	}

	var clearLine = {};

	for (var x = corners.minX; x <= corners.maxX; ++x)
	{
		var x2 = x;
		var z2 = z;

		var clear = true;

		while (x2 >= corners.minX && x2 <= corners.maxX && z2 >= corners.minZ && z2 <= corners.maxZ)
		{
			var bp = bb[x2 - corners.minX][z2 - corners.minZ];
			if (bp.isFeature && g_Map.validT(x2, z2))
			{
				clear = false;
				break;
			}

			x2 = x2 + xOffset;
			z2 = z2 + zOffset;
		}

		if (clear)
		{
			var lastX = x2 - xOffset;
			var lastZ = z2 - zOffset;
			var midX = Math.floor((x + lastX) / 2);
			var midZ = Math.floor((z + lastZ) / 2);
			clearLine = {
				"x1": x,
				"z1": z,
				"x2": lastX,
				"z2": lastZ,
				"midX": midX,
				"midZ": midZ,
				"height": baseHeight
			};
		}

		if (clear && (angle == 1 || angle == 2))
			break;

		if (!clear && (angle == 0 || angle == 3))
			break;
	}

	return clearLine;
}

/**
 * Returns the corners of a bounding box.
 */
function findCorners(points)
{
	// Find the bounding box of the terrain feature
	var mapSize = getMapSize();
	var minX = mapSize + 1;
	var minZ = mapSize + 1;
	var maxX = -1;
	var maxZ = -1;

	for (var p = 0; p < points.length; ++p)
	{
		var pt = points[p];

		minX = Math.min(pt.x, minX);
		minZ = Math.min(pt.z, minZ);

		maxX = Math.max(pt.x, maxX);
		maxZ = Math.max(pt.z, maxZ);
	}

	return {
		"minX": minX,
		"minZ": minZ,
		"maxX": maxX,
		"maxZ": maxZ
	};
}

/**
 * Finds the average elevation around a point.
 */
function smoothElevation(x, z)
{
	var min = g_Map.getHeight(x, z);

	for (var xOffset = -1; xOffset <= 1; ++xOffset)
		for (var zOffset = -1; zOffset <= 1; ++zOffset)
		{
			var thisX = x + xOffset;
			var thisZ = z + zOffset;
			if (!g_Map.validH(thisX, thisZ))
				continue;

			var height = g_Map.getHeight(thisX, thisZ);
			if (height < min)
				min = height;
		}

	return min;
}

/**
 * Determines if a point in a bounding box array is next to a terrain feature.
 */
function nextToFeature(bb, x, z)
{
	for (var xOffset = -1; xOffset <= 1; ++xOffset)
		for (var zOffset = -1; zOffset <= 1; ++zOffset)
		{
			var thisX = x + xOffset;
			var thisZ = z + zOffset;
			if (thisX < 0 || thisX >= bb.length || thisZ < 0 || thisZ >= bb[x].length || thisX == 0 && thisZ == 0)
				continue;

			if (bb[thisX][thisZ].isFeature)
				return true;
		}

	return false;
}

/**
 * Returns a number within a random deviation of a base number.
 */
function getRandomDeviation(base, deviation)
{
	return base + randFloat(-1, 1) * Math.min(base, deviation);
}

/**
 * Import a given digital elevation model.
 * Scale it to the mapsize and paint the textures specified by coordinate on it.
 *
 * @return the ratio of heightmap tiles per map size tiles
 */
function paintHeightmap(mapName, func = undefined)
{
	/**
	 * @property heightmap - An array with a square number of heights.
	 * @property tilemap - The IDs of the palletmap to be painted for each heightmap tile.
	 * @property pallet - The tile texture names used by the tilemap.
	 */
	let mapData = RMS.ReadJSONFile("maps/random/" + mapName + ".hmap");

	let mapSize = getMapSize(); // Width of the map in terrain tiles
	let hmSize = Math.sqrt(mapData.heightmap.length);
	let scale = hmSize / (mapSize + 1); // There are mapSize + 1 vertices (each 1 tile is surrounded by 2x2 vertices)

	for (let x = 0; x <= mapSize; ++x)
		for (let y = 0; y <= mapSize; ++y)
		{
			let hmPoint = { "x": x * scale, "y": y * scale };
			let hmTile = { "x": Math.floor(hmPoint.x), "y": Math.floor(hmPoint.y) };
			let shift = { "x": 0, "y": 0 };

			if (hmTile.x == 0)
				shift.x = 1;
			else if (hmTile.x == hmSize - 1)
				shift.x = - 2;
			else if (hmTile.x == hmSize - 2)
				shift.x = - 1;

			if (hmTile.y == 0)
				shift.y = 1;
			else if (hmTile.y == hmSize - 1)
				shift.y = - 2;
			else if (hmTile.y == hmSize - 2)
				shift.y = - 1;

			let neighbors = [];
			for (let localXi = 0; localXi < 4; ++localXi)
				for (let localYi = 0; localYi < 4; ++localYi)
					neighbors.push(mapData.heightmap[(hmTile.x + localXi + shift.x - 1) * hmSize + (hmTile.y + localYi + shift.y - 1)]);

			setHeight(x, y, bicubicInterpolation(hmPoint.x - hmTile.x - shift.x, hmPoint.y - hmTile.y - shift.y, ...neighbors) / scale);

			if (x < mapSize && y < mapSize)
			{
				let i = hmTile.x * hmSize + hmTile.y;
				let tile = mapData.pallet[mapData.tilemap[i]];
				placeTerrain(x, y, tile);

				if (func)
					func(tile, x, y);
			}
		}

	return scale;
}
