/**
 * @file These functions are often used to create a landscape, for instance shaping mountains, hills, rivers or grass and dirt patches.
 */

/**
 * Bumps add slight, diverse elevation differences to otherwise completely level terrain.
 */
function createBumps(constraint, count, minSize, maxSize, spread, failFraction = 0, elevation = 2)
{
	log("Creating bumps...");
	createAreas(
		new ChainPlacer(
			minSize || 1,
			maxSize || Math.floor(scaleByMapSize(4, 6)),
			spread || Math.floor(scaleByMapSize(2, 5)),
			failFraction),
		new SmoothElevationPainter(ELEVATION_MODIFY, elevation, 2),
		constraint,
		count || scaleByMapSize(100, 200));
}

/**
 * Hills are elevated, planar, impassable terrain areas.
 */
function createHills(terrainset, constraint, tileClass, count, minSize, maxSize, spread, failFraction = 0.5, elevation = 18, elevationSmoothing = 2)
{
	log("Creating hills...");
	createAreas(
		new ChainPlacer(
			minSize || 1,
			maxSize || Math.floor(scaleByMapSize(4, 6)),
			spread || Math.floor(scaleByMapSize(16, 40)),
			failFraction),
		[
			new LayeredPainter(terrainset, [1, elevationSmoothing]),
			new SmoothElevationPainter(ELEVATION_SET, elevation, elevationSmoothing),
			paintClass(tileClass)
		],
		constraint,
		count || scaleByMapSize(1, 4) * getNumPlayers());
}

/**
 * Mountains are impassable smoothened cones.
 */
function createMountains(terrain, constraint, tileClass, count, maxHeight, minRadius, maxRadius, numCircles)
{
	log("Creating mountains...");
	let mapSize = getMapSize();

	for (let i = 0; i < (count || scaleByMapSize(1, 4) * getNumPlayers()); ++i)
		createMountain(
			maxHeight !== undefined ? maxHeight : Math.floor(scaleByMapSize(30, 50)),
			minRadius || Math.floor(scaleByMapSize(3, 4)),
			maxRadius || Math.floor(scaleByMapSize(6, 12)),
			numCircles || Math.floor(scaleByMapSize(4, 10)),
			constraint,
			randIntExclusive(0, mapSize),
			randIntExclusive(0, mapSize),
			terrain,
			tileClass,
			14);
}

/**
 * Paint the given terrain texture in the given sizes at random places of the map to diversify monotone land texturing.
 */
function createPatches(sizes, terrain, constraint, count,  tileClass, failFraction =  0.5)
{
	for (let size of sizes)
		createAreas(
			new ChainPlacer(1, Math.floor(scaleByMapSize(3, 5)), size, failFraction),
			[
				new TerrainPainter(terrain),
				paintClass(tileClass)
			],
			constraint,
			count);
}

/**
 * Same as createPatches, but each patch consists of a set of textures drawn depending to the distance of the patch border.
 */
function createLayeredPatches(sizes, terrains, terrainWidths, constraint, count, tileClass, failFraction = 0.5)
{
	for (let size of sizes)
		createAreas(
			new ChainPlacer(1, Math.floor(scaleByMapSize(3, 5)), size, failFraction),
			[
				new LayeredPainter(terrains, terrainWidths),
				paintClass(tileClass)
			],
			constraint,
			count);
}
