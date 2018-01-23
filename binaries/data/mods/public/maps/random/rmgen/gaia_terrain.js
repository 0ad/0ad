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
 * Create a mountain using a technique very similar to ChainPlacer.
 */
function createMountain(maxHeight, minRadius, maxRadius, numCircles, constraints, x, z, terrain, tileClass, fcc = 0, q = [])
{
	let position = new Vector2D(x, z);
	let constraint = new AndConstraint(constraints);

	if (!g_Map.inMapBounds(x, z) || !constraint.allows(position))
		return;

	let mapSize = getMapSize();
	let queueEmpty = !q.length;

	let gotRet = [];
	for (let i = 0; i < mapSize; ++i)
	{
		gotRet[i] = [];
		for (let j = 0; j < mapSize; ++j)
			gotRet[i][j] = -1;
	}

	--mapSize;

	minRadius = Math.max(1, Math.min(minRadius, maxRadius));

	let edges = [[x, z]];
	let circles = [];

	for (let i = 0; i < numCircles; ++i)
	{
		let badPoint = false;
		let [cx, cz] = pickRandom(edges);

		let radius;
		if (queueEmpty)
			radius = randIntInclusive(minRadius, maxRadius);
		else
		{
			radius = q.pop();
			queueEmpty = !q.length;
		}

		let sx = Math.max(0, cx - radius);
		let sz = Math.max(0, cz - radius);
		let lx = Math.min(cx + radius, mapSize);
		let lz = Math.min(cz + radius, mapSize);

		let radius2 = Math.square(radius);

		for (let ix = sx; ix <= lx; ++ix)
		{
			for (let iz = sz; iz <= lz; ++iz)
			{
				let pos = new Vector2D(ix, iz);

				if (Math.euclidDistance2D(ix, iz, cx, cz) > radius2 || !g_Map.inMapBounds(ix, iz))
					continue;

				if (!constraint.allows(pos))
				{
					badPoint = true;
					break;
				}

				let state = gotRet[ix][iz];
				if (state == -1)
				{
					gotRet[ix][iz] = -2;
				}
				else if (state >= 0)
				{
					edges.splice(state, 1);
					gotRet[ix][iz] = -2;

					for (let k = state; k < edges.length; ++k)
						--gotRet[edges[k][0]][edges[k][1]];
				}
			}

			if (badPoint)
				break;
		}

		if (badPoint)
			continue;

		circles.push([cx, cz, radius]);

		for (let ix = sx; ix <= lx; ++ix)
			for (let iz = sz; iz <= lz; ++iz)
			{
				if (gotRet[ix][iz] != -2 ||
				    fcc && (x - ix > fcc || ix - x > fcc || z - iz > fcc || iz - z > fcc) ||
				    ix > 0 && gotRet[ix-1][iz] == -1 ||
				    iz > 0 && gotRet[ix][iz-1] == -1 ||
				    ix < mapSize && gotRet[ix+1][iz] == -1 ||
				    iz < mapSize && gotRet[ix][iz+1] == -1)
					continue;

				edges.push([ix, iz]);
				gotRet[ix][iz] = edges.length - 1;
			}
	}

	for (let [cx, cz, radius] of circles)
	{
		let circlePosition = new Vector2D(cx, cz);
		let sx = Math.max(0, cx - radius);
		let sz = Math.max(0, cz - radius);
		let lx = Math.min(cx + radius, mapSize);
		let lz = Math.min(cz + radius, mapSize);

		let clumpHeight = radius / maxRadius * maxHeight * randFloat(0.8, 1.2);

		for (let ix = sx; ix <= lx; ++ix)
			for (let iz = sz; iz <= lz; ++iz)
			{
				let position = new Vector2D(ix, iz);
				let distance = position.distanceTo(circlePosition);

				let newHeight =
					randIntInclusive(0, 2) +
					Math.round(2/3 * clumpHeight * (Math.sin(Math.PI * 2/3 * (3/4 - distance / radius)) + 0.5));

				if (distance > radius)
					continue;

				if (getHeight(ix, iz) < newHeight)
					g_Map.setHeight(position, newHeight);
				else if (getHeight(ix, iz) >= newHeight && getHeight(ix, iz) < newHeight + 4)
					g_Map.setHeight(position, newHeight + 4);

				if (terrain !== undefined)
					createTerrain(terrain).place(ix, iz);

				if (tileClass !== undefined)
					addToClass(ix, iz, tileClass);
			}
	}
}

/**
 * Generates a volcano mountain. Smoke and lava are optional.
 *
 * @param {number} center - Vector2D location on the tilemap.
 * @param {number} tileClass - Painted onto every tile that is occupied by the volcano.
 * @param {string} terrainTexture - The texture painted onto the volcano hill.
 * @param {array} lavaTextures - Three different textures for the interior, from the outside to the inside.
 * @param {boolean} smoke - Whether to place smoke particles.
 * @param {number} elevationType - Elevation painter type, ELEVATION_SET = absolute or ELEVATION_MODIFY = relative.
 */
function createVolcano(position, tileClass, terrainTexture, lavaTextures, smoke, elevationType)
{
	log("Creating volcano...");

	let clLava = createTileClass();
	let layers = [
		{
			"clumps": diskArea(scaleByMapSize(18, 25)),
			"elevation": 15,
			"tileClass": tileClass,
			"steepness": 3
		},
		{
			"clumps": diskArea(scaleByMapSize(16, 23)),
			"elevation": 25,
			"tileClass": createTileClass(),
			"steepness": 3
		},
		{
			"clumps": diskArea(scaleByMapSize(10, 15)),
			"elevation": 45,
			"tileClass": createTileClass(),
			"steepness": 3
		},
		{
			"clumps": diskArea(scaleByMapSize(8, 11)),
			"elevation": 62,
			"tileClass": createTileClass(),
			"steepness": 3
		},
		{
			"clumps": diskArea(scaleByMapSize(4, 6)),
			"elevation": 42,
			"tileClass": clLava,
			"painter": lavaTextures && new LayeredPainter([terrainTexture, ...lavaTextures], [1, 1, 1]),
			"steepness": 1
		}
	];

	for (let i = 0; i < layers.length; ++i)
		createArea(
			new ClumpPlacer(layers[i].clumps, 0.7, 0.05, 100, position),
			[
				layers[i].painter || new LayeredPainter([terrainTexture, terrainTexture], [3]),
				new SmoothElevationPainter(elevationType, layers[i].elevation, layers[i].steepness),
				paintClass(layers[i].tileClass)
			],
			i == 0 ? null : stayClasses(layers[i - 1].tileClass, 1));

	if (smoke)
	{
		let num = Math.floor(diskArea(scaleByMapSize(3, 5)));
		createObjectGroup(
			new SimpleGroup(
				[new SimpleObject("actor|particle/smoke.xml", num, num, 0, 7)],
				false,
				clLava,
				position.x,
				position.y),
			0,
		stayClasses(tileClass, 1));
	}
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

/**
 * Creates a meandering river at the given location and width.
 * Optionally calls a function on the affected tiles.
 *
 * @property start - A Vector2D in tile coordinates stating where the river starts.
 * @property end - A Vector2D in tile coordinates stating where the river ends.
 * @property parallel - Whether the shorelines should be parallel or meander separately.
 * @property width - Size between the two shorelines.
 * @property fadeDist - Size of the shoreline.
 * @property deviation - Fuzz effect on the shoreline if greater than 0.
 * @property heightRiverbed - Ground height of the riverbed.
 * @proeprty heightLand - Ground height of the end of the shoreline.
 * @property meanderShort - Strength of frequent meanders.
 * @property meanderLong - Strength of less frequent meanders.
 * @property [constraint] - If given, ignores any tiles that don't satisfy the given Constraint.
 * @property [waterFunc] - Optional function called on tiles within the river.
 *                         Provides location on the tilegrid, new elevation and
 *                         the location on the axis parallel to the river as a fraction of the river length.
 * @property [landFunc] - Optional function called on land tiles, providing ix, iz, shoreDist1, shoreDist2.
 * @property [minHeight] - If given, only changes the elevation below this height while still calling the given functions.
 */
function paintRiver(args)
{
	log("Creating river...");

	// Model the river meandering as the sum of two sine curves.
	let meanderShort = fractionToTiles(args.meanderShort / scaleByMapSize(35, 160));
	let meanderLong = fractionToTiles(args.meanderLong / scaleByMapSize(35, 100));

	// Unless the river is parallel, each riverside will receive an own random seed and starting angle.
	let seed1 = randFloat(2, 3);
	let seed2 = randFloat(2, 3);

	let startingAngle1 = randFloat(0, 1);
	let startingAngle2 = randFloat(0, 1);

	// Computes the deflection of the river at a given point.
	let riverCurve = (riverFraction, startAngle, seed) =>
		meanderShort * rndRiver(startAngle + fractionToTiles(riverFraction) / 128, seed) +
		meanderLong * rndRiver(startAngle + fractionToTiles(riverFraction) / 256, seed);

	// Describe river location in vectors.
	let riverLength = args.start.distanceTo(args.end);
	let unitVecRiver = Vector2D.sub(args.start, args.end).normalize();

	// Describe river boundaries.
	let riverMinX = Math.min(args.start.x, args.end.x);
	let riverMinZ = Math.min(args.start.y, args.end.y);
	let riverMaxX = Math.max(args.start.x, args.end.x);
	let riverMaxZ = Math.max(args.start.y, args.end.y);

	let mapSize = getMapSize();
	for (let ix = 0; ix < mapSize; ++ix)
		for (let iz = 0; iz < mapSize; ++iz)
		{
			let vecPoint = new Vector2D(ix, iz);

			if (args.constraint && !args.constraint.allows(vecPoint))
				continue;

			// Compute the shortest distance to the river.
			let distanceToRiver = distanceOfPointFromLine(args.start, args.end, vecPoint);

			// Closest point on the river (i.e the foot of the perpendicular).
			let river = Vector2D.sub(vecPoint, unitVecRiver.perpendicular().mult(distanceToRiver));

			// Only process points that actually are perpendicular with the river.
			if (river.x < riverMinX || river.x > riverMaxX ||
			    river.y < riverMinZ || river.y > riverMaxZ)
				continue;

			// Coordinate between 0 and 1 on the axis parallel to the river.
			let riverFraction = river.distanceTo(args.start) / riverLength;

			// Amplitude of the river at this location.
			let riverCurve1 = riverCurve(riverFraction, startingAngle1, seed1);
			let riverCurve2 = args.parallel ? riverCurve1 : riverCurve(riverFraction, startingAngle2, seed2);

			// Add noise.
			let deviation = args.deviation * randFloat(-1, 1);

			// Compute the distance to the shoreline.
			let sign = Math.sign(distanceToRiver || 1);
			let shoreDist1 = sign * riverCurve1 + Math.abs(distanceToRiver) - deviation - args.width / 2;
			let shoreDist2 = sign * riverCurve2 + Math.abs(distanceToRiver) - deviation + args.width / 2;

			// Create the elevation for the water and the slopy shoreline and call the user functions.
			if (shoreDist1 < 0 && shoreDist2 > 0)
			{
				let height = args.heightRiverbed;

				if (shoreDist1 > -args.fadeDist)
					height += (args.heightLand - args.heightRiverbed) * (1 + shoreDist1 / args.fadeDist);
				else if (shoreDist2 < args.fadeDist)
					height += (args.heightLand - args.heightRiverbed) * (1 - shoreDist2 / args.fadeDist);

				if (args.minHeight === undefined || height < args.minHeight)
					g_Map.setHeight(vecPoint, height);

				if (args.waterFunc)
					args.waterFunc(vecPoint, height, riverFraction);
			}
			else if (args.landFunc)
				args.landFunc(vecPoint, shoreDist1, shoreDist2);
		}
}

/**
 * Helper function to create a meandering river.
 * It works the same as sin or cos function with the difference that it's period is 1 instead of 2 pi.
 */
function rndRiver(f, seed)
{
	let rndRw = seed;

	for (let i = 0; i <= f; ++i)
		rndRw = 10 * (rndRw % 1);

	let rndRr = f % 1;
	let retVal = (Math.floor(f) % 2 ? -1 : 1) * rndRr * (rndRr - 1);

	let rndRe = Math.floor(rndRw) % 5;
	if (rndRe == 0)
		retVal *= 2.3 * (rndRr - 0.5) * (rndRr - 0.5);
	else if (rndRe == 1)
		retVal *= 2.6 * (rndRr - 0.3) * (rndRr - 0.7);
	else if (rndRe == 2)
		retVal *= 22 * (rndRr - 0.2) * (rndRr - 0.3) * (rndRr - 0.3) * (rndRr - 0.8);
	else if (rndRe == 3)
		retVal *= 180 * (rndRr - 0.2) * (rndRr - 0.2) * (rndRr - 0.4) * (rndRr - 0.6) * (rndRr - 0.6) * (rndRr - 0.8);
	else if (rndRe == 4)
		retVal *= 2.6 * (rndRr - 0.5) * (rndRr - 0.7);

	return retVal;
}

/**
 * Add small rivers with shallows starting at a central river ending at the map border, if the given Constraint is met.
 */
function createTributaryRivers(riverAngle, riverCount, riverWidth, heightRiverbed, heightRange, maxAngle, tributaryRiverTileClass, shallowTileClass, constraint)
{
	log("Creating tributary rivers...");
	let waviness = 0.4;
	let smoothness = scaleByMapSize(3, 12);
	let offset = 0.1;
	let tapering = 0.05;
	let heightShallow = -2;

	let mapSize = getMapSize();
	let mapCenter = getMapCenter();
	let mapBounds = getMapBounds();

	let riverConstraint = avoidClasses(tributaryRiverTileClass, 3);
	if (shallowTileClass)
		riverConstraint = new AndConstraint([riverConstraint, avoidClasses(shallowTileClass, 2)]);

	for (let i = 0; i < riverCount; ++i)
	{
		// Determining tributary river location
		let searchCenter = new Vector2D(fractionToTiles(randFloat(tapering, 1 - tapering)), mapCenter.y);
		let sign = randBool() ? 1 : -1;
		let distanceVec = new Vector2D(0, sign * tapering);

		let searchStart = Vector2D.add(searchCenter, distanceVec).rotateAround(riverAngle, mapCenter);
		let searchEnd = Vector2D.sub(searchCenter, distanceVec).rotateAround(riverAngle, mapCenter);

		let start = findLocationInDirectionBasedOnHeight(searchStart, searchEnd, heightRange[0], heightRange[1], 4);
		if (!start)
			continue;

		start.round();
		let end = Vector2D.add(mapCenter, new Vector2D(mapSize, 0).rotate(riverAngle - sign * randFloat(maxAngle, 2 * Math.PI - maxAngle))).round();

		// Create river
		if (!createArea(
			new PathPlacer(start, end, riverWidth, waviness, smoothness, offset, tapering),
			[
				new SmoothElevationPainter(ELEVATION_SET, heightRiverbed, 4),
				paintClass(tributaryRiverTileClass)
			],
			new AndConstraint([constraint, riverConstraint])))
			continue;

		// Create small puddles at the map border to ensure players being separated
		createArea(
			new ClumpPlacer(diskArea(riverWidth / 2), 0.95, 0.6, 10, end),
			new SmoothElevationPainter(ELEVATION_SET, heightRiverbed, 3),
			constraint);
	}

	// Create shallows
	if (shallowTileClass)
		for (let z of [0.25, 0.75])
			createPassage({
				"start": new Vector2D(mapBounds.left, fractionToTiles(z)).rotateAround(riverAngle, mapCenter),
				"end": new Vector2D(mapBounds.right, fractionToTiles(z)).rotateAround(riverAngle, mapCenter),
				"startWidth": scaleByMapSize(8, 12),
				"endWidth": scaleByMapSize(8, 12),
				"smoothWidth": 2,
				"startHeight": heightShallow,
				"endHeight": heightShallow,
				"maxHeight": heightShallow,
				"tileClass": shallowTileClass
			});
}

/**
 * Creates a smooth, passable path between between start and end with the given startWidth and endWidth.
 * Paints the given tileclass and terrain.
 *
 * @property {Vector2D} start - Location of the passage.
 * @property {Vector2D} end
 * @property {number} startWidth - Size of the passage (perpendicular to the direction of the passage).
 * @property {number} endWidth
 * @property {number} [startHeight] - Fixed height to be used if the height at the location shouldn't be used.
 * @property {number} [endHeight]
 * @property {number} [maxHeight] - If given, do not touch any terrain above this height.
 * @property {number} smoothWidth - Number of tiles at the passage border to apply height interpolation.
 * @property {number} [tileClass] - Marks the passage with this tile class.
 * @property {string} [terrain] - Texture to be painted on the passage area.
 * @property {string} [edgeTerrain] - Texture to be painted on the borders of the passage.
 */
function createPassage(args)
{
	let bound = x => Math.max(0, Math.min(Math.round(x), getMapSize()));

	let startHeight = args.startHeight !== undefined ? args.startHeight : getHeight(bound(args.start.x), bound(args.start.y));
	let endHeight = args.endHeight !== undefined ? args.endHeight : getHeight(bound(args.end.x), bound(args.end.y));

	let passageVec = Vector2D.sub(args.end, args.start);
	let widthDirection = passageVec.perpendicular().normalize();
	let lengthStep = 1 / (2 * passageVec.length());

	for (let lengthFraction = 0; lengthFraction <= 1; lengthFraction += lengthStep)
	{
		let locationLength = Vector2D.add(args.start, Vector2D.mult(passageVec, lengthFraction));
		let halfPassageWidth = (args.startWidth + (args.endWidth - args.startWidth) * lengthFraction) / 2;
		let passageHeight = startHeight + (endHeight - startHeight) * lengthFraction;

		for (let stepWidth = -halfPassageWidth; stepWidth <= halfPassageWidth; stepWidth += 0.5)
		{
			let location = Vector2D.add(locationLength, Vector2D.mult(widthDirection, stepWidth)).round();

			if (!g_Map.inMapBounds(location.x, location.y) ||
			    args.maxHeight !== undefined && getHeight(location.x, location.y) > args.maxHeight)
				continue;

			let smoothDistance = args.smoothWidth + Math.abs(stepWidth) - halfPassageWidth;

			g_Map.setHeight(
				location,
				smoothDistance > 0 ?
					(getHeight(location.x, location.y) * smoothDistance + passageHeight / smoothDistance) / (smoothDistance + 1 / smoothDistance) :
					passageHeight);

			if (args.tileClass !== undefined)
				addToClass(location.x, location.y, args.tileClass);

			if (args.edgeTerrain && smoothDistance > 0)
				createTerrain(args.edgeTerrain).place(location.x, location.y);
			else if (args.terrain)
				createTerrain(args.terrain).place(location.x, location.y);
		}
	}
}

/**
 * Returns the first location between startPoint and endPoint that lies within the given heightrange.
 */
function findLocationInDirectionBasedOnHeight(startPoint, endPoint, minHeight, maxHeight, offset = 0)
{
	let stepVec = Vector2D.sub(endPoint, startPoint);
	let distance = Math.ceil(stepVec.length());
	stepVec.normalize();

	for (let i = 0; i < distance; ++i)
	{
		let pos = Vector2D.add(startPoint, Vector2D.mult(stepVec, i));
		let ipos = pos.clone().round();

		if (g_Map.validH(ipos.x, ipos.y) &&
		    getHeight(ipos.x, ipos.y) >= minHeight &&
		    getHeight(ipos.x, ipos.y) <= maxHeight)
			return pos.add(stepVec.mult(offset));
	}

	return undefined;
}
