var g_Props = {
	"barrels": "actor|props/special/eyecandy/barrels_buried.xml",
	"crate": "actor|props/special/eyecandy/crate_a.xml",
	"cart": "actor|props/special/eyecandy/handcart_1_broken.xml",
	"well": "actor|props/special/eyecandy/well_1_c.xml",
	"skeleton": "actor|props/special/eyecandy/skeleton.xml",
};

var g_DefaultDeviation = 0.1;

/**
 * Create bluffs, i.e. a slope hill reachable from ground level.
 * Fill it with wood, mines, animals and decoratives.
 *
 * @param {Array} constraint - where to place them
 * @param {number} size - size of the bluffs (1.2 would be 120% of normal)
 * @param {number} deviation - degree of deviation from the defined size (0.2 would be 20% plus/minus)
 * @param {number} fill - size of map to fill (1.5 would be 150% of normal)
 */
function addBluffs(constraint, size, deviation, fill)
{
	deviation = deviation || g_DefaultDeviation;
	size = size || 1;
	fill = fill || 1;

	var constrastTerrain = g_Terrains.tier2Terrain;

	if (g_MapInfo.biome == g_BiomeTropic)
		constrastTerrain = g_Terrains.dirt;

	if (g_MapInfo.biome == g_BiomeAutumn)
		constrastTerrain = g_Terrains.tier3Terrain;

	var count = fill * scaleByMapSize(15, 15);
	var minSize = scaleByMapSize(5, 5);
	var maxSize = scaleByMapSize(7, 7);
	var elevation = 30;
	var spread = scaleByMapSize(100, 100);

	for (var i = 0; i < count; ++i)
	{
		var offset = getRandomDeviation(size, deviation);

		var pMinSize = Math.floor(minSize * offset);
		var pMaxSize = Math.floor(maxSize * offset);
		var pSpread = Math.floor(spread * offset);
		var pElevation = Math.floor(elevation * offset);

		var placer = new ChainPlacer(pMinSize, pMaxSize, pSpread, 0.5);
		var terrainPainter = new LayeredPainter([g_Terrains.cliff, g_Terrains.mainTerrain, constrastTerrain], [2, 3]);
		var elevationPainter = new SmoothElevationPainter(ELEVATION_MODIFY, pElevation, 2);
		var rendered = createAreas(placer, [terrainPainter, elevationPainter, paintClass(g_TileClasses.bluff)], constraint, 1);

		// Find the bounding box of the bluff
		if (rendered[0] === undefined)
			continue;

		var points = rendered[0].points;

		var corners = findCorners(points);

		// Seed an array the size of the bounding box
		var bb = createBoundingBox(points, corners);

		// Get a random starting position for the baseline and the endline
		var angle = randInt(4);
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
			baseLine = findClearLine(bb, corners, angle);
			endLine = findClearLine(bb, corners, opAngle);

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

	let savanna = g_MapInfo.biome == g_BiomeSavanna;
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
	deviation = deviation || g_DefaultDeviation;
	size = size || 1;
	fill = fill || 1;

	var offset = getRandomDeviation(size, deviation);
	var decorations = [
		[
			new SimpleObject(g_Decoratives.rockMedium, 1 * offset, 3 * offset, 0, 1 * offset)
		],
		[
			new SimpleObject(g_Decoratives.rockLarge, 1 * offset, 2 * offset, 0, 1 * offset),
			new SimpleObject(g_Decoratives.rockMedium, 1 * offset, 3 * offset, 0, 2 * offset)
		],
		[
			new SimpleObject(g_Decoratives.grassShort, 1 * offset, 2 * offset, 0, 1 * offset, -PI / 8, PI / 8)
		],
		[
			new SimpleObject(g_Decoratives.grass, 2 * offset, 4 * offset, 0, 1.8 * offset, -PI / 8, PI / 8),
			new SimpleObject(g_Decoratives.grassShort, 3 * offset, 6 * offset, 1.2 * offset, 2.5 * offset, -PI / 8, PI / 8)
		],
		[
			new SimpleObject(g_Decoratives.bushMedium, 1 * offset, 2 * offset, 0, 2 * offset),
			new SimpleObject(g_Decoratives.bushSmall, 2 * offset, 4 * offset, 0, 2 * offset)
		]
	];

	var baseCount = 1;
	if (g_MapInfo.biome == g_BiomeTropic)
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
		createObjectGroups(group, 0, constraint, decorCount, 5);
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
 *	"count": scaleByMapSize(8, 8),
 *	"minSize": Math.floor(scaleByMapSize(5, 5)),
 *	"maxSize": Math.floor(scaleByMapSize(8, 8)),
 *	"spread": Math.floor(scaleByMapSize(20, 20)),
 *	"minElevation": 6,
 *	"maxElevation": 12,
 *	"steepness": 1.5
 */

function addElevation(constraint, el)
{
	var deviation = el.deviation || g_DefaultDeviation;
	var size = el.size || 1;
	var fill = el.fill || 1;

	var count = fill * el.count;
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
		var elevation = el.minElevation + randInt(el.maxElevation - el.minElevation);
		var smooth = Math.floor(elevation / el.steepness);

		var offset = getRandomDeviation(size, el.deviation);
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

		var pWidths = widths.concat(pSmooth);

		var placer = new ChainPlacer(pMinSize, pMaxSize, pSpread, 0.5);
		var terrainPainter = new LayeredPainter(el.painter, [pWidths]);
		var elevationPainter = new SmoothElevationPainter(elType, pElevation, pSmooth);
		createAreas(placer, [terrainPainter, elevationPainter, paintClass(el.class)], constraint, 1);
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
		"count": scaleByMapSize(8, 8),
		"minSize": Math.floor(scaleByMapSize(5, 5)),
		"maxSize": Math.floor(scaleByMapSize(8, 8)),
		"spread": Math.floor(scaleByMapSize(20, 20)),
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

	if (g_MapInfo.biome == g_BiomeTemperate || g_MapInfo.biome == g_BiomeTropic)
		lakeTile = g_Terrains.dirt;

	if (g_MapInfo.biome == g_BiomeMediterranean)
		lakeTile = g_Terrains.tier2Terrain;

	if (g_MapInfo.biome == g_BiomeAutumn)
		lakeTile = g_Terrains.shore;

	addElevation(constraint, {
		"class": g_TileClasses.water,
		"painter": [lakeTile, lakeTile],
		"size": size,
		"deviation": deviation,
		"fill": fill,
		"count": scaleByMapSize(6, 6),
		"minSize": Math.floor(scaleByMapSize(7, 7)),
		"maxSize": Math.floor(scaleByMapSize(9, 9)),
		"spread": Math.floor(scaleByMapSize(70, 70)),
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
	createObjectGroups(group, 0, [stayClasses(g_TileClasses.water, 1), borderClasses(g_TileClasses.water, 4, 3)], 1000, 100);

	group = new SimpleGroup([new SimpleObject(g_Decoratives.reeds, 10, 15, 1, 3), new SimpleObject(g_Decoratives.rockMedium, 1, 3, 1, 3)], true, g_TileClasses.dirt);
	createObjectGroups(group, 0, [stayClasses(g_TileClasses.water, 2), borderClasses(g_TileClasses.water, 4, 3)], 1000, 100);
}

/**
 * Universal function to create layered patches.
 */
function addLayeredPatches(constraint, size, deviation, fill)
{
	deviation = deviation || g_DefaultDeviation;
	size = size || 1;
	fill = fill || 1;

	var minRadius = 1;
	var maxRadius = Math.floor(scaleByMapSize(3, 5));
	var count = fill * scaleByMapSize(15, 45);

	var sizes = [
		scaleByMapSize(3, 6),
		scaleByMapSize(5, 10),
		scaleByMapSize(8, 21)
	];

	for (var i = 0; i < sizes.length; ++i)
	{
		var offset = getRandomDeviation(size, deviation);
		var patchMinRadius = Math.floor(minRadius * offset);
		var patchMaxRadius = Math.floor(maxRadius * offset);
		var patchSize = Math.floor(sizes[i] * offset);
		var patchCount = count * offset;

		if (patchMinRadius > patchMaxRadius)
			patchMinRadius = patchMaxRadius;

		var placer = new ChainPlacer(patchMinRadius, patchMaxRadius, patchSize, 0.5);
		var painter = new LayeredPainter(
			[
				[g_Terrains.mainTerrain, g_Terrains.tier1Terrain],
				[g_Terrains.tier1Terrain, g_Terrains.tier2Terrain],
				[g_Terrains.tier2Terrain, g_Terrains.tier3Terrain],
				[g_Terrains.tier4Terrain]
			],
			[1, 1] // widths
		);
		createAreas(placer, [painter, paintClass(g_TileClasses.dirt)], constraint, patchCount);
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
		"count": scaleByMapSize(8, 8),
		"minSize": Math.floor(scaleByMapSize(2, 2)),
		"maxSize": Math.floor(scaleByMapSize(4, 4)),
		"spread": Math.floor(scaleByMapSize(100, 100)),
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

	if (g_MapInfo.biome == g_BiomeSnowy)
		plateauTile = g_Terrains.tier1Terrain;

	if (g_MapInfo.biome == g_BiomeAlpine || g_MapInfo.biome == g_BiomeSavanna)
		plateauTile = g_Terrains.tier2Terrain;

	if (g_MapInfo.biome == g_BiomeAutumn)
		plateauTile = g_Terrains.tier4Terrain;

	addElevation(constraint, {
		"class": g_TileClasses.plateau,
		"painter": [g_Terrains.cliff, plateauTile],
		"size": size,
		"deviation": deviation,
		"fill": fill,
		"count": scaleByMapSize(15, 15),
		"minSize": Math.floor(scaleByMapSize(2, 2)),
		"maxSize": Math.floor(scaleByMapSize(4, 4)),
		"spread": Math.floor(scaleByMapSize(200, 200)),
		"minElevation": 20,
		"maxElevation": 30,
		"steepness": 8
	});

	for (var i = 0; i < 40; ++i)
	{
		var placer = new ChainPlacer(3, 15, 1, 0.5);
		var terrainPainter = new LayeredPainter([plateauTile, plateauTile], [3]);
		var hillElevation = 4 + randInt(15);
		var elevationPainter = new SmoothElevationPainter(ELEVATION_MODIFY, hillElevation, hillElevation - 2);

		createAreas(
			placer,
			[
				terrainPainter,
				elevationPainter,
				paintClass(g_TileClasses.hill)
			],
			[
				avoidClasses(g_TileClasses.hill, 7),
				stayClasses(g_TileClasses.plateau, 7)
			],
			1
		);
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
	deviation = deviation || g_DefaultDeviation;
	size = size || 1;
	fill = fill || 1;

	var offset = getRandomDeviation(size, deviation);

	var props = [
		[
			new SimpleObject(g_Props.skeleton, 1 * offset, 5 * offset, 0, 3 * offset + 2),
		],
		[
			new SimpleObject(g_Props.barrels, 1 * offset, 2 * offset, 2, 3 * offset + 2),
			new SimpleObject(g_Props.cart, 0, 1 * offset, 5, 2.5 * offset + 5),
			new SimpleObject(g_Props.crate, 1 * offset, 2 * offset, 2, 2 * offset + 2),
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
		createObjectGroups(group, 0, constraint, propCount, 5);
	}

	// Add decorative trees
	var trees = new SimpleObject(g_Decoratives.tree, 5 * offset, 30 * offset, 2, 3 * offset + 10);
	createObjectGroups(new SimpleGroup([trees], true), 0, constraint, counts[0] * 5 * fill, 5);
}

/**
 * Create valleys.
 */
function addValleys(constraint, size, deviation, fill)
{
	if (g_MapInfo.mapHeight < 6)
		return;

	var minElevation = (-1 * g_MapInfo.mapHeight) / (size * (1 + deviation)) + 1;
	if (minElevation < -1 * g_MapInfo.mapHeight)
		minElevation = -1 * g_MapInfo.mapHeight;

	var valleySlope = g_Terrains.tier1Terrain;
	var valleyFloor = g_Terrains.tier4Terrain;

	if (g_MapInfo.biome == g_BiomeDesert)
	{
		valleySlope = g_Terrains.tier3Terrain;
		valleyFloor = g_Terrains.dirt;
	}

	if (g_MapInfo.biome == g_BiomeMediterranean)
	{
		valleySlope = g_Terrains.tier2Terrain;
		valleyFloor = g_Terrains.dirt;
	}

	if (g_MapInfo.biome == g_BiomeAlpine || g_MapInfo.biome == g_BiomeSavanna)
		valleyFloor = g_Terrains.tier2Terrain;

	if (g_MapInfo.biome == g_BiomeTropic)
		valleySlope = g_Terrains.dirt;

	if (g_MapInfo.biome == g_BiomeAutumn)
		valleyFloor = g_Terrains.tier3Terrain;

	addElevation(constraint, {
		"class": g_TileClasses.valley,
		"painter": [valleySlope, valleyFloor],
		"size": size,
		"deviation": deviation,
		"fill": fill,
		"count": scaleByMapSize(8, 8),
		"minSize": Math.floor(scaleByMapSize(5, 5)),
		"maxSize": Math.floor(scaleByMapSize(8, 8)),
		"spread": Math.floor(scaleByMapSize(30, 30)),
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
	deviation = deviation || g_DefaultDeviation;
	size = size || 1;
	fill = fill || 1;

	var groupOffset = getRandomDeviation(size, deviation);

	var animals = [
		[new SimpleObject(g_Gaia.mainHuntableAnimal, 5 * groupOffset, 7 * groupOffset, 0, 4 * groupOffset)],
		[new SimpleObject(g_Gaia.secondaryHuntableAnimal, 2 * groupOffset, 3 * groupOffset, 0, 2 * groupOffset)]
	];

	var counts = [scaleByMapSize(30, 30) * fill, scaleByMapSize(30, 30) * fill];

	for (var i = 0; i < animals.length; ++i)
	{
		var group = new SimpleGroup(animals[i], true, g_TileClasses.animals);
		createObjectGroups(group, 0, constraint, Math.floor(counts[i]), 50);
	}
}

/**
 * Create fruits.
 */
function addBerries(constraint, size, deviation, fill)
{
	deviation = deviation || g_DefaultDeviation;
	size = size || 1;
	fill = fill || 1;

	var groupOffset = getRandomDeviation(size, deviation);

	var count = scaleByMapSize(50, 50) * fill;
	var berries = [[new SimpleObject(g_Gaia.fruitBush, 5 * groupOffset, 5 * groupOffset, 0, 3 * groupOffset)]];

	for (var i = 0; i < berries.length; ++i)
	{
		var group = new SimpleGroup(berries[i], true, g_TileClasses.berries);
		createObjectGroups(group, 0, constraint, Math.floor(count), 40);
	}
}

/**
 * Create fish.
 */
function addFish(constraint, size, deviation, fill)
{
	deviation = deviation || g_DefaultDeviation;
	size = size || 1;
	fill = fill || 1;

	var groupOffset = getRandomDeviation(size, deviation);

	var fish = [
		[new SimpleObject(g_Gaia.fish, 1 * groupOffset, 2 * groupOffset, 0, 2 * groupOffset)],
		[new SimpleObject(g_Gaia.fish, 2 * groupOffset, 4 * groupOffset, 10 * groupOffset, 20 * groupOffset)]
	];

	var counts = [scaleByMapSize(40, 40) * fill, scaleByMapSize(40, 40) * fill];

	for (var i = 0; i < fish.length; ++i)
	{
		var group = new SimpleGroup(fish[i], true, g_TileClasses.fish);
		createObjectGroups(group, 0, constraint, floor(counts[i]), 50);
	}
}

/**
 * Create dense forests.
 */
function addForests(constraint, size, deviation, fill)
{
	deviation = deviation || g_DefaultDeviation;
	size = size || 1;
	fill = fill || 1;

	// No forests for the african biome
	if (g_MapInfo.biome == g_BiomeSavanna)
		return;

	var types = [
		[
			[g_Terrains.forestFloor2, g_Terrains.mainTerrain, g_Forests.forest1],
			[g_Terrains.forestFloor2, g_Forests.forest1]
		],
		[
			[g_Terrains.forestFloor2, g_Terrains.mainTerrain, g_Forests.forest2],
			[g_Terrains.forestFloor1, g_Forests.forest2]],
		[
			[g_Terrains.forestFloor1, g_Terrains.mainTerrain, g_Forests.forest1],
			[g_Terrains.forestFloor2, g_Forests.forest1]],
		[
			[g_Terrains.forestFloor1, g_Terrains.mainTerrain, g_Forests.forest2],
			[g_Terrains.forestFloor1, g_Forests.forest2]
		]
	];

	for (var i = 0; i < types.length; ++i)
	{
		var offset = getRandomDeviation(size, deviation);
		var minSize = floor(scaleByMapSize(3, 5) * offset);
		var maxSize = Math.floor(scaleByMapSize(50, 50) * offset);
		var forestCount = scaleByMapSize(10, 10) * fill;

		var placer = new ChainPlacer(1, minSize, maxSize, 0.5);
		var painter = new LayeredPainter(types[i], [2]);
		createAreas(placer, [painter, paintClass(g_TileClasses.forest)], constraint, forestCount);
	}
}

/**
 * Create metal mines.
 */
function addMetal(constraint, size, deviation, fill)
{
	deviation = deviation || g_DefaultDeviation;
	size = size || 1;
	fill = fill || 1;

	var offset = getRandomDeviation(size, deviation);
	var count = 1 + scaleByMapSize(20, 20) * fill;
	var mines = [[new SimpleObject(g_Gaia.metalLarge, 1 * offset, 1 * offset, 0, 4 * offset)]];

	for (var i = 0; i < mines.length; ++i)
	{
		var group = new SimpleGroup(mines[i], true, g_TileClasses.metal);
		createObjectGroups(group, 0, constraint, count, 100);
	}
}

function addSmallMetal(constraint, size, mixes, amounts)
{
	let deviation = getRandomDeviation(size || 1, mixes || g_DefaultDeviation);
	let count = 1 + scaleByMapSize(20, 20) * (amounts || 1);
	let mines = [[new SimpleObject(g_Gaia.metalSmall, 2 * deviation, 5 * deviation, 1 * deviation, 3 * deviation)]];

	for (let i = 0; i < mines.length; ++i)
	{
		let group = new SimpleGroup(mines[i], true, g_TileClasses.metal);
		createObjectGroups(group, 0, constraint, count, 100);
	}
}

/**
 * Create stone mines.
 */
function addStone(constraint, size, deviation, fill)
{
	deviation = deviation || g_DefaultDeviation;
	size = size || 1;
	fill = fill || 1;

	var offset = getRandomDeviation(size, deviation);
	var count = 1 + scaleByMapSize(20, 20) * fill;
	var mines = [
		[
			new SimpleObject(g_Gaia.stoneSmall, 0, 2 * offset, 0, 4 * offset),
			new SimpleObject(g_Gaia.stoneLarge, 1 * offset, 1 * offset, 0, 4 * offset)
		],
		[
			new SimpleObject(g_Gaia.stoneSmall, 2 * offset, 5 * offset, 1 * offset, 3 * offset)
		]
	];

	for (var i = 0; i < mines.length; ++i)
	{
		var group = new SimpleGroup(mines[i], true, g_TileClasses.rock);
		createObjectGroups(group, 0, constraint, count, 100);
	}
}

/**
 * Create straggler trees.
 */
function addStragglerTrees(constraint, size, deviation, fill)
{
	deviation = deviation || g_DefaultDeviation;
	size = size || 1;
	fill = fill || 1;

	// Ensure minimum distribution on african biome
	if (g_MapInfo.biome == g_BiomeSavanna)
	{
		fill = Math.max(fill, 2);
		size = Math.max(size, 1);
	}

	var trees = [g_Gaia.tree1, g_Gaia.tree2, g_Gaia.tree3, g_Gaia.tree4];

	var treesPerPlayer = 40;
	var playerBonus = Math.max(1, (g_MapInfo.numPlayers - 3) / 2);

	var offset = getRandomDeviation(size, deviation);
	var treeCount = treesPerPlayer * playerBonus * fill;
	var totalTrees = scaleByMapSize(treeCount, treeCount);

	var count = Math.floor(totalTrees / trees.length) * fill;
	var min = 1 * offset;
	var max = 4 * offset;
	var minDist = 1 * offset;
	var maxDist = 5 * offset;

	// More trees for the african biome
	if (g_MapInfo.biome == g_BiomeSavanna)
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
		if (i == 2 && (g_MapInfo.biome == g_BiomeDesert || g_MapInfo.biome == g_BiomeMediterranean))
			treesMax = 1;

		min = Math.min(min, treesMax);

		var group = new SimpleGroup([new SimpleObject(trees[i], min, treesMax, minDist, maxDist)], true, g_TileClasses.forest);
		createObjectGroups(group, 0, constraint, count);
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
function findClearLine(bb, corners, angle)
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
				"height": g_MapInfo.mapHeight
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
	var minX = g_MapInfo.mapSize + 1;
	var minZ = g_MapInfo.mapSize + 1;
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
			if (!g_Map.validT(thisX, thisZ))
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
			if (thisX < 0 || thisX >= bb.length || thisZ < 0 || thisZ >= bb[x].length || (thisX == 0 && thisZ == 0))
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
	deviation = Math.min(base, deviation);
	deviation = base + randInt(20 * deviation) / 10 - deviation;
	return deviation.toFixed(2);
}

/**
 * Import a given digital elevation model.
 * Scale it to the mapsize and paint the textures specified by coordinate on it.
 *
 * @param heightmap - An array with a square number of heights
 * @param tilemap - The IDs of the palletmap to be painted for each heightmap tile
 * @param pallet - The tile texture names used by the tilemap.
 * @return the ratio of heightmap tiles per map size tiles
 */
function paintHeightmap(heightmap, tilemap, pallet, func = undefined)
{
	let mapSize = getMapSize(); // Width of the map in terrain tiles
	let hmSize = Math.sqrt(heightmap.length);
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
			for (let localYi = 0; localYi < 4; ++localYi)
				for (let localXi = 0; localXi < 4; ++localXi)
					neighbors.push(heightmap[(hmTile.x + localXi + shift.x - 1) * hmSize + (hmTile.y + localYi + shift.y - 1)]);

			setHeight(x, y, bicubicInterpolation(hmPoint.x - hmTile.x - shift.x, hmPoint.y - hmTile.y - shift.y, ...neighbors) / scale);

			if (x < mapSize && y < mapSize)
			{
				let i = hmTile.x * hmSize + hmTile.y;
				let tile = pallet[tilemap[i]];
				placeTerrain(x, y, tile);

				if (func)
					func(tile, x, y);
			}
		}

	return scale;
}
