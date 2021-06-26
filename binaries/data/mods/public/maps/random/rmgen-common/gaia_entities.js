/**
 * @file These functions are often used to place gaia entities, like forests, mines, animals or decorative bushes.
 */

/**
 * Returns the number of trees in forests and straggler trees.
 */
function getTreeCounts(minTrees, maxTrees, forestRatio)
{
	return [forestRatio, 1 - forestRatio].map(p => p * scaleByMapSize(minTrees, maxTrees));
}

/**
 * Places uniformly sized forests at random locations.
 * Unless you want a custom number of forest, prefer createDefaultForests.
 * Generates two variants of forests from the given terrain textures and tree templates.
 * The forest border has less trees than the inside.
 * @param terrainsSet - a list of 5 terrains to use. The first 3 are border terrains, the later 2 interior.
 * @param constraint - constraints to respect
 * @param tileClass - the tileclass to print
 * @param treeCount - Either { "nbForests": X, "treesPerForest": X } or (legacy) a number of trees.
 * @param retryFactor - @see createAreas
 */
function createForests(terrainSet, constraint, tileClass, treeCount, retryFactor)
{
	if (!treeCount)
		return;

	// Construct different forest types from the terrain textures and template names.
	const [mainTerrain, terrainForestFloor1, terrainForestFloor2, terrainForestTree1, terrainForestTree2] = terrainSet;

	// The painter will pick a random Terrain for each part of the forest.
	const forestVariants = [
		{
			"borderTerrains": [terrainForestFloor2, mainTerrain, terrainForestTree1],
			"interiorTerrains": [terrainForestFloor2, terrainForestTree1]
		},
		{
			"borderTerrains": [terrainForestFloor1, mainTerrain, terrainForestTree2],
			"interiorTerrains": [terrainForestFloor1, terrainForestTree2]
		}
	];

	let numberOfTrees;
	let numberOfForests;
	if (typeof treeCount === "number")
	{
		numberOfTrees = treeCount;
		numberOfForests = Math.floor(numberOfTrees / (scaleByMapSize(3, 6) * getNumPlayers() * forestVariants.length));
	}
	else
	{
		numberOfForests = treeCount.nbForests;
		numberOfTrees = numberOfForests * treeCount.treesPerForest;
	}

	if (!numberOfForests)
		return;

	g_Map.log("Creating forests");
	for (const forestVariant of forestVariants)
		createAreas(
			new ChainPlacer(1, Math.floor(scaleByMapSize(3, 5)), numberOfTrees / numberOfForests, 0.5),
			[
				new LayeredPainter([forestVariant.borderTerrains, forestVariant.interiorTerrains], [2]),
				new TileClassPainter(tileClass)
			],
			constraint,
			numberOfForests,
			retryFactor);
}

var g_DefaultNumberOfForests = scaleByMapSize(8, 36);

/**
 * Passes some sane defaults to createForests.
 */
function createDefaultForests(terrainSet, constraints, tileClass, totalNumberOfTrees)
{
	return createForests(
		terrainSet,
		constraints,
		tileClass,
		{
			"nbForests": g_DefaultNumberOfForests,
			"treesPerForest": totalNumberOfTrees / g_DefaultNumberOfForests,
		}
	);
}


/**
 * Places the given amount of Entities at random places meeting the given Constraint, chosing a different template for each.
 */
function createStragglerTrees(templateNames, constraint, tileClass, treeCount, retryFactor)
{
	g_Map.log("Creating straggler trees");
	for (let templateName of templateNames)
		createObjectGroupsDeprecated(
			new SimpleGroup([new SimpleObject(templateName, 1, 1, 0, 3)], true, tileClass),
			0,
			constraint,
			Math.floor(treeCount / templateNames.length),
			retryFactor);
}

/**
 * Places a SimpleGroup consisting of the given number of the given Objects
 * at random locations that meet the given Constraint.
 */
function createMines(objects, constraint, tileClass, count)
{
	for (let object of objects)
		createObjectGroupsDeprecated(
			new SimpleGroup(object, true, tileClass),
			0,
			constraint,
			count || scaleByMapSize(4, 16),
			70);
}

/**
 * Place large/small mines on the map in such a way that it should be relatively fair.
 * @param oSmall - the small mine object
 * @param oLarge - the large mine object
 * @param clMine - the 'mine' class to paint.
 * @param constraints - Custom constraints. Note that the function automatically avoids clMine as well.
 * @param counts - a dict of numbers { "largeCount": 10, "smallCount": 100, "randomSmallCount": 2 }
 * @param randomness - randomize counts by a random multiplier between [1 - randomness, 1 + randomness]
 */
function createBalancedMines(oSmall, oLarge, clMine, constraints, counts, randomness)
{
	let largeCount = counts.largeCount;
	let smallCount = counts.smallCount;
	let randomSmallCount = counts.randomSmallCount;

	if (randomness > 0 && randomness < 1)
	{
		largeCount = Math.round(largeCount * randFloat(1 - randomness, 1 + randomness));
		smallCount = Math.round(smallCount * randFloat(1 - randomness, 1 + randomness));
		randomSmallCount = Math.round(randomSmallCount * randFloat(1 - randomness, 1 + randomness));
	}

	const arrayConstraints = Array.isArray(constraints) ? constraints : [constraints];

	// Plop large mines far away from each other.
	createObjectGroups(
		new SimpleGroup([new SimpleObject(oLarge, 1, 1, 0, 1)], true, clMine),
		0,
		new AndConstraint([avoidClasses(clMine, scaleByMapSize(25, 50)), ...arrayConstraints]),
		largeCount,
		100);

	// Plop smaller clusters of small mines, also somewhat farther away.
	createObjectGroups(
		new SimpleGroup([new SimpleObject(oSmall, 2, 3, 0, 2)], true, clMine),
		0,
		new AndConstraint([avoidClasses(clMine, scaleByMapSize(18, 35)), ...arrayConstraints]),
		smallCount,
		50);

	// Plop a few smaller clusters in a random fashion, occasionally making very good dropsites spots.
	createObjectGroups(
		new SimpleGroup([new SimpleObject(oSmall, 1, 2, 0, 2)], true, clMine),
		0,
		new AndConstraint([avoidClasses(clMine, 5), ...arrayConstraints]),
		randomSmallCount,
		50);
}

/**
 * Helper for createBalancedMines with default metal counts.
 * The current settings are so that a Small 1v1 has about 40K metal,
 * and a Normal 4v4 has about 140K.
 * The setup is biaised so that with fewer players, there are more small mines,
 * and with more players there are proportionally more big mines, to maintain
 * some randomness to the distribution but keep it somewhat fair in 1v1.
 */
function createBalancedMetalMines(oSmall, oLarge, clMine, constraints, counts = 1.0, randomness = 0.05)
{
	return createBalancedMines(
		oSmall,
		oLarge,
		clMine,
		constraints,
		{
			"largeCount": (Math.max(scaleByMapSize(1, 9), getNumPlayers() * 1.8 - 0.8)) * counts,
			"smallCount": (scaleByMapSize(4, 12)) * counts,
			"randomSmallCount": (scaleByMapSize(1, 8)) * counts,
		},
		randomness
	);
}

/**
 * Helper for createBalancedMines with default stone counts.
 * There is a little less stone than metal overall.
 */
function createBalancedStoneMines(oSmall, oLarge, clMine, constraints, counts = 1.0, randomness = 0.05)
{
	return createBalancedMines(
		oSmall,
		oLarge,
		clMine,
		constraints,
		{
			"largeCount": (Math.max(scaleByMapSize(1, 9), getNumPlayers() * 1.25)) * counts,
			"smallCount": (scaleByMapSize(1, 8)) * counts,
			"randomSmallCount": (scaleByMapSize(1, 8)) * counts,
		},
		randomness
	);
}


/**
 * Places Entities of the given templateName in a circular pattern (leaving out a quarter of the circle).
 */
function createStoneMineFormation(position, templateName, terrain, radius = 2.5, count = 8, startAngle = undefined, maxOffset = 1)
{
	createArea(
		new ChainPlacer(radius / 2, radius, 2, Infinity, position, undefined, [5]),
		new TerrainPainter(terrain));

	let angle = startAngle !== undefined ? startAngle : randomAngle();

	for (let i = 0; i < count; ++i)
	{
		let pos = Vector2D.add(position, new Vector2D(radius + randFloat(0, maxOffset), 0).rotate(-angle)).round();
		g_Map.placeEntityPassable(templateName, 0, pos, randomAngle());
		angle += 3/2 * Math.PI / count;
	}
}

/**
 * Places the given amounts of the given Objects at random locations meeting the given Constraint.
 */
function createFood(objects, counts, constraint, tileClass)
{
	g_Map.log("Creating food");
	for (let i = 0; i < objects.length; ++i)
		createObjectGroupsDeprecated(
			new SimpleGroup(objects[i], true, tileClass),
			0,
			constraint,
			counts[i],
			50);
}

/**
 * Same as createFood, but doesn't mark the terrain with a TileClass.
 */
function createDecoration(objects, counts, constraint)
{
	g_Map.log("Creating decoration");
	for (let i = 0; i < objects.length; ++i)
		createObjectGroupsDeprecated(
			new SimpleGroup(objects[i], true),
			0,
			constraint,
			counts[i],
			5);
}

/**
 * Places docks in situations where the location of land and water is not known in advance.
 * Do determine the position, it picks a random point on the land, find the closest, significantly large body of water,
 * then places the dock at the first point close to that body of water within the given heightrange.
 *
 * @param {string} template - The template name of the dock to be placed.
 * @param {number} playerID - The owner of the dock.
 * @param {number} count - The number of docks to be placed.
 * @param {Object} tileClassWater - The tileclass the water area is marked with.
 * @param {Object} tileClassDock - The dock position is marked with this class.
 * @param {number} heightMin - The lowest height a dock could be placed.
 * @param {number} heightMax - The greatest height a dock could be placed.
 * @param {Array|Constraint} constraints - Only consider dock positions valid that meet this Constraint.
 * @param {number} offset - How many tiles to move the dock towards the direction of the water after having found a location.
 * @param {number} retryFactor- How many different locations should be tested.
 */
function placeDocks(template, playerID, count, tileClassWater, tileClassDock, heightMin, heightMax, constraints, offset, retryFactor)
{
	let mapCenter = g_Map.getCenter();

	g_Map.log("Marking dock search start area");
	let areaSearchStart = createArea(
		new DiskPlacer(fractionToTiles(0.5) - 10, mapCenter),
		undefined,
		avoidClasses(tileClassWater, 6));

	g_Map.log("Marking dock search end area");
	let areaSearchEnd = createArea(
		new DiskPlacer(fractionToTiles(0.5) - 10, mapCenter),
		undefined,
		stayClasses(tileClassWater, 20));

	g_Map.log("Marking land area");
	let areaLand = createArea(
		new MapBoundsPlacer(),
		undefined,
		avoidClasses(tileClassWater, 0));

	g_Map.log("Marking water area");
	let areaWater = createArea(
		new MapBoundsPlacer(),
		undefined,
		stayClasses(tileClassWater, 0));

	if (!areaSearchEnd || !areaSearchEnd.getPoints().length)
		return;

	// TODO: computing the exact intersection with the waterplane would both not require us to pass reasonable heights and be more precise
	let constraint = new AndConstraint(constraints);
	g_Map.log("Placing docks");
	for (let i = 0; i < count; ++i)
		for (let tries = 0; tries < retryFactor; ++tries)
		{
			let positionLand = pickRandom(areaSearchStart.getPoints());
			let positionWaterLarge = areaSearchEnd.getClosestPointTo(positionLand);
			let positionDock = findLocationInDirectionBasedOnHeight(positionWaterLarge, positionLand, heightMin, heightMax, offset);
			if (!positionDock)
				continue;

			positionDock.round();

			if (!g_Map.inMapBounds(positionDock) || !constraint.allows(positionDock))
				continue;

			let angle = positionDock.angleTo(Vector2D.average(new DiskPlacer(8, positionDock).place(stayClasses(tileClassWater, 0))));

			g_Map.placeEntityPassable(template, playerID, positionDock, -angle + Math.PI / 2);
			tileClassDock.add(positionDock);
			break;
		}
}
