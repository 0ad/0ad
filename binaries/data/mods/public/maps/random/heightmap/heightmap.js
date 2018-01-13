/**
 * Heightmap manipulation functionality
 *
 * A heightmapt is an array of width arrays of height floats
 * Width and height is normally mapSize+1 (Number of vertices is one bigger than number of tiles in each direction)
 * The default heightmap is g_Map.height (See the Map object)
 *
 * @warning - Ambiguous naming and potential confusion:
 * To use this library use TILE_CENTERED_HEIGHT_MAP = false (default)
 * Otherwise TILE_CENTERED_HEIGHT_MAP has nothing to do with any tile centered map in this library
 * @todo - TILE_CENTERED_HEIGHT_MAP should be removed and g_Map.height should never be tile centered
 */

/**
 * Get the height range of a heightmap
 * @param {array} [heightmap=g_Map.height] - The reliefmap the minimum and maximum height should be determined for
 * @return {object} [height] - Height range with 2 floats in properties "min" and "max"
 */
function getMinAndMaxHeight(heightmap = g_Map.height)
{
	let height = {};
	height.min = Infinity;
	height.max = - Infinity;
	for (let x = 0; x < heightmap.length; ++x)
	{
		for (let y = 0; y < heightmap[x].length; ++y)
		{
			if (heightmap[x][y] < height.min)
				height.min = heightmap[x][y];
			else if (heightmap[x][y] > height.max)
				height.max = heightmap[x][y];
		}
	}
	return height;
}

/**
 * Rescales a heightmap so its minimum and maximum height is as the arguments told preserving it's global shape
 * @param {float} [minHeight=MIN_HEIGHT] - Minimum height that should be used for the resulting heightmap
 * @param {float} [maxHeight=MAX_HEIGHT] - Maximum height that should be used for the resulting heightmap
 * @param {array} [heightmap=g_Map.height] - A reliefmap
 * @todo Add preserveCostline to leave a certain height untoucht and scale below and above that seperately
 */
function rescaleHeightmap(minHeight = MIN_HEIGHT, maxHeight = MAX_HEIGHT, heightmap = g_Map.height)
{
	let oldHeightRange = getMinAndMaxHeight(heightmap);
	let max_x = heightmap.length;
	let max_y = heightmap[0].length;
	for (let x = 0; x < max_x; ++x)
		for (let y = 0; y < max_y; ++y)
			heightmap[x][y] = minHeight + (heightmap[x][y] - oldHeightRange.min) / (oldHeightRange.max - oldHeightRange.min) * (maxHeight - minHeight);
}

/**
 * Get start location with the largest minimum distance between players
 * @param {array} [heightRange] - The height range start locations are allowed
 * @param {integer} [maxTries=1000] - How often random player distributions are rolled to be compared
 * @param {float} [minDistToBorder=20] - How far start locations have to be away from the map border
 * @param {integer} [numberOfPlayers=g_MapSettings.PlayerData.length] - How many start locations should be placed
 * @param {array} [heightmap=g_Map.height] - The reliefmap for the start locations to be placed on
 * @param {boolean} [isCircular=g_MapSettings.CircularMap] - If the map is circular or rectangular
 * @return {array} [finalStartLoc] - Array of 2D points in the format { "x": float, "y": float}
 */
function getStartLocationsByHeightmap(heightRange, maxTries = 1000, minDistToBorder = 20, numberOfPlayers = g_MapSettings.PlayerData.length - 1, heightmap = g_Map.height, isCircular = g_MapSettings.CircularMap)
{
	let validStartLoc = [];
	let r = 0.5 * (heightmap.length - 1); // Map center x/y as well as radius
	for (let x = minDistToBorder; x < heightmap.length - minDistToBorder; ++x)
		for (let y = minDistToBorder; y < heightmap[0].length - minDistToBorder; ++y)
			if (heightmap[x][y] > heightRange.min && heightmap[x][y] < heightRange.max) // Is in height range
				if (!isCircular || r - Math.euclidDistance2D(x, y, r, r) >= minDistToBorder) // Is far enough away from map border
					validStartLoc.push({ "x": x, "y": y });

	let maxMinDist = 0;
	let finalStartLoc;
	for (let tries = 0; tries < maxTries; ++tries)
	{
		let startLoc = [];
		let minDist = Infinity;
		for (let p = 0; p < numberOfPlayers; ++p)
			startLoc.push(pickRandom(validStartLoc));
		for (let p1 = 0; p1 < numberOfPlayers - 1; ++p1)
		{
			for (let p2 = p1 + 1; p2 < numberOfPlayers; ++p2)
			{
				let dist = Math.euclidDistance2D(startLoc[p1].x, startLoc[p1].y, startLoc[p2].x, startLoc[p2].y);
				if (dist < minDist)
					minDist = dist;
			}
		}
		if (minDist > maxMinDist)
		{
			maxMinDist = minDist;
			finalStartLoc = startLoc;
		}
	}

	return finalStartLoc;
}

/**
 * Meant to place e.g. resource spots within a height range
 *
 * @param {array} heightRange - The height range in which to place the entities (An associative array with keys "min" and "max" each containing a float)
 * @param {array} avoidPoints - An array of 2D points (arrays of length 2), points that will be avoided in the given minDistance e.g. start locations
 * @param {number} minDistance - How many tile widths the entities to place have to be away from each other, start locations and the map border
 * @param {array} entityList - Entity/actor strings to be placed with placeObject()
 * @param {array} [heightmap=g_Map.height] - The reliefmap the entities should be distributed on
 * @param {number} [playerID=0] - Index of the player the entities should be placed for. Gaia is 0.
 * @param {number} [maxTries=1000] - How often random player distributions are rolled to be compared
 * @param {boolean} [isCircular=g_MapSettings.CircularMap] - If the map is circular or rectangular
 * @return {array} [placements] Array of points where entities were placed
 */
function distributeEntitiesByHeight(heightRange, avoidPoints, minDistance, entityList,
	playerID = 0, maxTries = 1000, heightmap = g_Map.height, isCircular = g_MapSettings.CircularMap)
{
	let validPoints = [];
	let r = 0.5 * (heightmap.length - 1); // Map center x/y as well as radius
	for (let x = minDistance; x < heightmap.length - minDistance; ++x)
	{
		for (let y = minDistance; y < heightmap[0].length - minDistance; ++y)
		{
			if (heightmap[x][y] < heightRange.min || heightmap[x][y] > heightRange.max)
				continue; // Out of height range
			let checkpoint = { "x" : x + 0.5, "y" : y + 0.5 };
			if (isCircular && r - Math.euclidDistance2D(checkpoint.x, checkpoint.y, r, r) < minDistance)
				continue; // Too close to map border
			// Avoid points by minDistance, else add to validPoints
			if (avoidPoints.every(ap => Math.euclidDistance2D(checkpoint.x, checkpoint.y, ap.x, ap.y) > minDistance))
				validPoints.push(checkpoint);
		}
	}

	let placements = [];
	if (!validPoints.length)
	{
		log("No placement points found for the given arguments (entityList=" + uneval(entityList) + "):\n" + new Error().stack);
		return placements;
	}

	for (let tries = 0; tries < maxTries; ++tries)
	{
		let checkPointIndex = randIntExclusive(0, validPoints.length);
		let checkPoint = validPoints[checkPointIndex];
		if (placements.every(p => Math.euclidDistance2D(p.x, p.y, checkPoint.x, checkPoint.y) > minDistance))
		{
			placeObject(checkPoint.x, checkPoint.y, pickRandom(entityList), playerID, randFloat(0, 2 * Math.PI));
			placements.push(checkPoint);
		}

		validPoints.splice(checkPointIndex);
		if (!validPoints.length)
			break; // No more valid points left
	}

	if (!placements.length)
		log("Nothing was placed:\n" + new Error().stack);

	return placements;
}

/**
 * Sets a given heightmap to entirely random values within a given range
 * @param {float} [minHeight=MIN_HEIGHT] - Lower limit of the random height to be rolled
 * @param {float} [maxHeight=MAX_HEIGHT] - Upper limit of the random height to be rolled
 * @param {array} [heightmap=g_Map.height] - The reliefmap that should be randomized
 */
function setRandomHeightmap(minHeight = MIN_HEIGHT, maxHeight = MAX_HEIGHT, heightmap = g_Map.height)
{
	for (let x = 0; x < heightmap.length; ++x)
		for (let y = 0; y < heightmap[0].length; ++y)
			heightmap[x][y] = randFloat(minHeight, maxHeight);
}

/**
 * Sets the heightmap to a relatively realistic shape
 * The function doubles the size of the initial heightmap (if given, else a random 2x2 one) until it's big enough, then the extend is cut off
 * @note min/maxHeight will not necessarily be present in the heightmap
 * @note On circular maps the edges (given by initialHeightmap) may not be in the playable map area
 * @note The impact of the initial heightmap depends on its size and target map size
 * @param {float} [minHeight=MIN_HEIGHT] - Lower limit of the random height to be rolled
 * @param {float} [maxHeight=MAX_HEIGHT] - Upper limit of the random height to be rolled
 * @param {array} [initialHeightmap] - Optional, Small (e.g. 3x3) heightmap describing the global shape of the map e.g. an island [[MIN_HEIGHT, MIN_HEIGHT, MIN_HEIGHT], [MIN_HEIGHT, MAX_HEIGHT, MIN_HEIGHT], [MIN_HEIGHT, MIN_HEIGHT, MIN_HEIGHT]]
 * @param {float} [smoothness=0.5] - Float between 0 (rough, more local structures) to 1 (smoother, only larger scale structures)
 * @param {array} [heightmap=g_Map.height] - The reliefmap that will be set by this function
 */
function setBaseTerrainDiamondSquare(minHeight = MIN_HEIGHT, maxHeight = MAX_HEIGHT, initialHeightmap = undefined, smoothness = 0.5, heightmap = g_Map.height)
{
	initialHeightmap = (initialHeightmap || [[randFloat(minHeight / 2, maxHeight / 2), randFloat(minHeight / 2, maxHeight / 2)], [randFloat(minHeight / 2, maxHeight / 2), randFloat(minHeight / 2, maxHeight / 2)]]);
	let heightRange = maxHeight - minHeight;
	if (heightRange <= 0)
		warn("setBaseTerrainDiamondSquare: heightRange <= 0");

	let offset = heightRange / 2;

	// Double initialHeightmap width until target width is reached (diamond square method)
	let newHeightmap = [];
	while (initialHeightmap.length < heightmap.length)
	{
		newHeightmap = [];
		let oldWidth = initialHeightmap.length;
		// Square
		for (let x = 0; x < 2 * oldWidth - 1; ++x)
		{
			newHeightmap.push([]);
			for (let y = 0; y < 2 * oldWidth - 1; ++y)
			{
				if (x % 2 == 0 && y % 2 == 0) // Old tile
					newHeightmap[x].push(initialHeightmap[x/2][y/2]);
				else if (x % 2 == 1 && y % 2 == 1) // New tile with diagonal old tile neighbors
				{
					newHeightmap[x].push((initialHeightmap[(x-1)/2][(y-1)/2] + initialHeightmap[(x+1)/2][(y-1)/2] + initialHeightmap[(x-1)/2][(y+1)/2] + initialHeightmap[(x+1)/2][(y+1)/2]) / 4);
					newHeightmap[x][y] += (newHeightmap[x][y] - minHeight) / heightRange * randFloat(-offset, offset);
				}
				else // New tile with straight old tile neighbors
					newHeightmap[x].push(undefined); // Define later
			}
		}
		// Diamond
		for (let x = 0; x < 2 * oldWidth - 1; ++x)
		{
			for (let y = 0; y < 2 * oldWidth - 1; ++y)
			{
				if (newHeightmap[x][y] !== undefined)
					continue;

				if (x > 0 && x + 1 < newHeightmap.length - 1 && y > 0 && y + 1 < newHeightmap.length - 1) // Not a border tile
				{
					newHeightmap[x][y] = (newHeightmap[x+1][y] + newHeightmap[x][y+1] + newHeightmap[x-1][y] + newHeightmap[x][y-1]) / 4;
					newHeightmap[x][y] += (newHeightmap[x][y] - minHeight) / heightRange * randFloat(-offset, offset);
				}
				else if (x < newHeightmap.length - 1 && y > 0 && y < newHeightmap.length - 1) // Left border
				{
					newHeightmap[x][y] = (newHeightmap[x+1][y] + newHeightmap[x][y+1] + newHeightmap[x][y-1]) / 3;
					newHeightmap[x][y] += (newHeightmap[x][y] - minHeight) / heightRange * randFloat(-offset, offset);
				}
				else if (x > 0 && y > 0 && y < newHeightmap.length - 1) // Right border
				{
					newHeightmap[x][y] = (newHeightmap[x][y+1] + newHeightmap[x-1][y] + newHeightmap[x][y-1]) / 3;
					newHeightmap[x][y] += (newHeightmap[x][y] - minHeight) / heightRange * randFloat(-offset, offset);
				}
				else if (x > 0 && x < newHeightmap.length - 1 && y < newHeightmap.length - 1) // Bottom border
				{
					newHeightmap[x][y] = (newHeightmap[x+1][y] + newHeightmap[x][y+1] + newHeightmap[x-1][y]) / 3;
					newHeightmap[x][y] += (newHeightmap[x][y] - minHeight) / heightRange * randFloat(-offset, offset);
				}
				else if (x > 0 && x < newHeightmap.length - 1 && y > 0) // Top border
				{
					newHeightmap[x][y] = (newHeightmap[x+1][y] + newHeightmap[x-1][y] + newHeightmap[x][y-1]) / 3;
					newHeightmap[x][y] += (newHeightmap[x][y] - minHeight) / heightRange * randFloat(-offset, offset);
				}
			}
		}
		initialHeightmap = clone(newHeightmap);
		offset /= Math.pow(2, smoothness);
	}

	// Cut initialHeightmap to fit target width
	let shift = [Math.floor((newHeightmap.length - heightmap.length) / 2), Math.floor((newHeightmap[0].length - heightmap[0].length) / 2)];
	for (let x = 0; x < heightmap.length; ++x)
		for (let y = 0; y < heightmap[0].length; ++y)
			heightmap[x][y] = newHeightmap[x + shift[0]][y + shift[1]];
}

/**
 * Smoothens the entire map
 * @param {float} [strength=0.8] - How strong the smooth effect should be: 0 means no effect at all, 1 means quite strong, higher values might cause interferences, better apply it multiple times
 * @param {array} [heightmap=g_Map.height] - The heightmap to be smoothed
 * @param {array} [smoothMap=[[1, 0], [1, 1], [0, 1], [-1, 1], [-1, 0], [-1, -1], [0, -1], [1, -1]]] - Array of offsets discribing the neighborhood tiles to smooth the height of a tile to
 */
function globalSmoothHeightmap(strength = 0.8, heightmap = g_Map.height, smoothMap = [[1, 0], [1, 1], [0, 1], [-1, 1], [-1, 0], [-1, -1], [0, -1], [1, -1]])
{
	let referenceHeightmap = clone(heightmap);
	let max_x = heightmap.length;
	let max_y = heightmap[0].length;
	for (let x = 0; x < max_x; ++x)
	{
		for (let y = 0; y < max_y; ++y)
		{
			for (let i = 0; i < smoothMap.length; ++i)
			{
				let mapX = x + smoothMap[i][0];
				let mapY = y + smoothMap[i][1];
				if (mapX >= 0 && mapX < max_x && mapY >= 0 && mapY < max_y)
					heightmap[x][y] += strength / smoothMap.length * (referenceHeightmap[mapX][mapY] - referenceHeightmap[x][y]);
			}
		}
	}
}

/**
 * Pushes a rectangular area towards a given height smoothing it into the original terrain
 * @note The window function to determine the smooth is not exactly a gaussian to ensure smooth edges
 * @param {object} [center] - The x and y coordinates of the center point (rounded in this function)
 * @param {float} [dx] - Distance from the center in x direction the rectangle ends (half width, rounded in this function)
 * @param {float} [dy] - Distance from the center in y direction the rectangle ends (half depth, rounded in this function)
 * @param {float} [targetHeight] - Height the center of the rectangle will be pushed to
 * @param {float} [strength=1] - How strong the height is pushed: 0 means not at all, 1 means the center will be pushed to the target height
 * @param {array} [heightmap=g_Map.height] - The heightmap to be manipulated
 * @todo Make the window function an argument and maybe add some
 */
function rectangularSmoothToHeight(center, dx, dy, targetHeight, strength = 0.8, heightmap = g_Map.height)
{
	let x = Math.round(center.x);
	let y = Math.round(center.y);
	dx = Math.round(dx);
	dy = Math.round(dy);

	let heightmapWin = [];
	for (let wx = 0; wx < 2 * dx + 1; ++wx)
	{
		heightmapWin.push([]);
		for (let wy = 0; wy < 2 * dy + 1; ++wy)
		{
			let actualX = x - dx + wx;
			let actualY = y - dy + wy;
			if (actualX >= 0 && actualX < heightmap.length - 1 && actualY >= 0 && actualY < heightmap[0].length - 1) // Is in map
				heightmapWin[wx].push(heightmap[actualX][actualY]);
			else
				heightmapWin[wx].push(targetHeight);
		}
	}
	for (let wx = 0; wx < 2 * dx + 1; ++wx)
	{
		for (let wy = 0; wy < 2 * dy + 1; ++wy)
		{
			let actualX = x - dx + wx;
			let actualY = y - dy + wy;
			if (actualX >= 0 && actualX < heightmap.length - 1 && actualY >= 0 && actualY < heightmap[0].length - 1) // Is in map
			{
				// Window function polynomial 2nd degree
				let scaleX = 1 - (wx / dx - 1) * (wx / dx - 1);
				let scaleY = 1 - (wy / dy - 1) * (wy / dy - 1);

				heightmap[actualX][actualY] = heightmapWin[wx][wy] + strength * scaleX * scaleY * (targetHeight - heightmapWin[wx][wy]);
			}
		}
	}
}

/**
 * Meant to place e.g. resource spots within a height range
 * @param {array} [heightRange] - The height range in which to place the entities (An associative array with keys "min" and "max" each containing a float)
 * @param {array} [avoidPoints=[]] - An array of objects of the form { "x": int, "y": int, "dist": int }, points that will be avoided in the given dist e.g. start locations
 * @param {object} [avoidClass=undefined] - TileClass to be avoided
 * @param {integer} [minDistance=30] - How many tile widths the entities to place have to be away from each other, start locations and the map border
 * @param {array} [heightmap=g_Map.height] - The reliefmap the entities should be distributed on
 * @param {integer} [maxTries=2 * g_Map.size] - How often random player distributions are rolled to be compared (256 to 1024)
 * @param {boolean} [isCircular=g_MapSettings.CircularMap] - If the map is circular or rectangular
 */
function getPointsByHeight(heightRange, avoidPoints = [], avoidClass = undefined, minDistance = 20, maxTries = 2 * g_Map.size, heightmap = g_Map.height, isCircular = g_MapSettings.CircularMap)
{
	let points = [];
	let placements = clone(avoidPoints);
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
				(avoidMap[Math.max(x - 1, 0)][y] > 0 ||
				avoidMap[x][Math.max(y - 1, 0)] > 0 ||
				avoidMap[Math.min(x + 1, avoidMap.length - 1)][y] > 0 ||
				avoidMap[x][Math.min(y + 1, avoidMap[0].length - 1)] > 0))
				continue;

			if (heightmap[x][y] > heightRange.min && heightmap[x][y] < heightRange.max && // Has correct height
				(!isCircular || r - Math.euclidDistance2D(x, y, r, r) >= minDistance)) // Enough distance to the map border
				validVertices.push({ "x": x, "y": y , "dist": minDistance});
		}
	}

	for (let tries = 0; tries < maxTries; ++tries)
	{
		let point = pickRandom(validVertices);
		if (placements.every(p => Math.euclidDistance2D(p.x, p.y, point.x, point.y) > Math.max(minDistance, p.dist)))
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
			slopeMap[x][y] = Math.euclidDistance2D(0, 0, inclineMap[x][y].x, inclineMap[x][y].y);
	}
	return slopeMap;
}

/**
 * Returns an inclination map corresponding to the tiles between the heightmaps vertices:
 * array of heightmap width-1 arrays of height-1 vectors (associative arrays) of the form:
 * { "x": x_slope, "y": y_slope ] so a 2D Vector pointing to the hightest incline (with the length the incline in the vectors direction)
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
					drain[i] += Math.min(strength * slopes[i] / sumSlopes, slopes[i]);
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
