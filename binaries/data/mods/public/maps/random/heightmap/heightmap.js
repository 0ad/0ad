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
				if (!isCircular || r - getDistance(x, y, r, r) >= minDistToBorder) // Is far enough away from map border
					validStartLoc.push({ "x": x, "y": y });
	
	let maxMinDist = 0;
	let finalStartLoc;
	for (let tries = 0; tries < maxTries; ++tries)
	{
		let startLoc = [];
		let minDist = Infinity;
		for (let p = 0; p < numberOfPlayers; ++p)
			startLoc.push(validStartLoc[randInt(validStartLoc.length)]);
		for (let p1 = 0; p1 < numberOfPlayers - 1; ++p1)
		{
			for (let p2 = p1 + 1; p2 < numberOfPlayers; ++p2)
			{
				let dist = getDistance(startLoc[p1].x, startLoc[p1].y, startLoc[p2].x, startLoc[p2].y);
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
 * @param {array} [heightRange] - The height range in which to place the entities (An associative array with keys "min" and "max" each containing a float)
 * @param {array} [avoidPoints] - An array of 2D points (arrays of length 2), points that will be avoided in the given minDistance e.g. start locations
 * @param {integer} [minDistance=30] - How many tile widths the entities to place have to be away from each other, start locations and the map border
 * @param {array} [heightmap=g_Map.height] - The reliefmap the entities should be distributed on
 * @param {array} [entityList=[g_Gaia.stoneLarge, g_Gaia.metalLarge]] - Entity/actor strings to be placed with placeObject()
 * @param {integer} [maxTries=1000] - How often random player distributions are rolled to be compared
 * @param {boolean} [isCircular=g_MapSettings.CircularMap] - If the map is circular or rectangular
 */
function distributeEntitiesByHeight(heightRange, avoidPoints, minDistance = 30, entityList = [g_Gaia.stoneLarge, g_Gaia.metalLarge], maxTries = 1000, heightmap = g_Map.height, isCircular = g_MapSettings.CircularMap)
{
	let placements = deepcopy(avoidPoints);
	let validTiles = [];
	let r = 0.5 * (heightmap.length - 1); // Map center x/y as well as radius
	for (let x = minDistance; x < heightmap.length - minDistance; ++x)
		for (let y = minDistance; y < heightmap[0].length - minDistance; ++y)
			if (heightmap[x][y] > heightRange.min && heightmap[x][y] < heightRange.max) // Has the right height
				if (!isCircular || r - getDistance(x, y, r, r) >= minDistance) // Is far enough away from map border
					validTiles.push({ "x": x, "y": y });
	
	for (let tries = 0; tries < maxTries; ++tries)
	{
		let tile = validTiles[randInt(validTiles.length)];
		let isValid = true;
		for (let p = 0; p < placements.length; ++p)
		{
			if (getDistance(placements[p].x, placements[p].y, tile.x, tile.y) < minDistance)
			{
				isValid = false;
				break;
			}
		}
		if (isValid)
		{
			placeObject(tile.x, tile.y, entityList[randInt(entityList.length)], 0, randFloat(0, 2*PI));
			placements.push(tile);
		}
	}
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
		initialHeightmap = deepcopy(newHeightmap);
		offset /= Math.pow(2, smoothness);
	}
	
	// Cut initialHeightmap to fit target width
	let shift = [floor((newHeightmap.length - heightmap.length) / 2), floor((newHeightmap[0].length - heightmap[0].length) / 2)];
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
	let referenceHeightmap = deepcopy(heightmap);
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
	let x = round(center.x);
	let y = round(center.y);
	dx = round(dx);
	dy = round(dy);
	
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
