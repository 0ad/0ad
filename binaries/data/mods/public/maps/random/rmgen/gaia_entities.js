/**
 * @file These functions are often used to place gaia entities, like forests, mines, animals or decorative bushes.
 */

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
function createStoneMineFormation(x, z, templateName, terrain, radius = 2.5, count = 8, startAngle = undefined, maxOffset = 1)
{
	log("Creating small stone mine circle...");
	createArea(
		new ChainPlacer(radius / 2, radius, 2, 1, x, z, undefined, [5]),
		new TerrainPainter(terrain),
		null);

	let angle = startAngle !== undefined ? startAngle : randFloat(0, 2 * Math.PI);

	for (let i = 0; i < count; ++i)
	{
		placeObject(
			Math.round(x + (radius + randFloat(0, maxOffset)) * Math.cos(angle)),
			Math.round(z + (radius + randFloat(0, maxOffset)) * Math.sin(angle)),
			templateName,
			0,
			randFloat(0, 2 * Math.PI));
		angle += 3/2 * Math.PI / count;
	}
}

/**
 * Places the given amounts of the given Objects at random locations meeting the given Constraint.
 */
function createFood(objects, counts, constraint, tileClass)
{
	log("Creating food...");
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
	log("Creating decoration...");
	for (let i = 0; i < objects.length; ++i)
		createObjectGroupsDeprecated(
			new SimpleGroup(objects[i], true),
			0,
			constraint,
			counts[i],
			5);
}
