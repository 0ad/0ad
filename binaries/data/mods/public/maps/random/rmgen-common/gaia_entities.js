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
 * Generates two variants of forests from the given terrain textures and tree templates.
 * The forest border has less trees than the inside.
 */
function createForests(terrainSet, constraint, tileClass, treeCount, retryFactor)
{
	if (!treeCount)
		return;

	// Construct different forest types from the terrain textures and template names.
	let [mainTerrain, terrainForestFloor1, terrainForestFloor2, terrainForestTree1, terrainForestTree2] = terrainSet;

	// The painter will pick a random Terrain for each part of the forest.
	let forestVariants = [
		{
			"borderTerrains": [terrainForestFloor2, mainTerrain, terrainForestTree1],
			"interiorTerrains": [terrainForestFloor2, terrainForestTree1]
		},
		{
			"borderTerrains": [terrainForestFloor1, mainTerrain, terrainForestTree2],
			"interiorTerrains": [terrainForestFloor1, terrainForestTree2]
		}
	];

	g_Map.log("Creating forests");
	let numberOfForests = Math.floor(treeCount / (scaleByMapSize(3, 6) * getNumPlayers() * forestVariants.length));
	for (let forestVariant of forestVariants)
		createAreas(
			new ChainPlacer(1, Math.floor(scaleByMapSize(3, 5)), treeCount / numberOfForests, 0.5),
			[
				new LayeredPainter([forestVariant.borderTerrains, forestVariant.interiorTerrains], [2]),
				new TileClassPainter(tileClass)
			],
			constraint,
			numberOfForests,
			retryFactor);
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
 * @param {String} template - The template name of the dock to be placed.
 * @param {Number} playerID - The owner of the dock.
 * @param {Number} count - The number of docks to be placed.
 * @param {Object} tileClassWater - The tileclass the water area is marked with.
 * @param {Object} tileClassDock - The dock position is marked with this class.
 * @param {Number} heightMin - The lowest height a dock could be placed.
 * @param {Number} heightMax - The greatest height a dock could be placed.
 * @param {Array|Constraint} constraints - Only consider dock positions valid that meet this Constraint.
 * @param {Number} offset - How many tiles to move the dock towards the direction of the water after having found a location.
 * @param {Number} retryFactor- How many different locations should be tested.
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
