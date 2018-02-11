/**
 * @file A Painter modifies an arbitrary feature in a given Area, for instance terrain textures, elevation or calling other painters on that Area.
 * Typically the area is determined by a Placer called from createArea or createAreas.
 */

/**
 * Marks the affected area with the given tileclass.
 */
function TileClassPainter(tileClass)
{
	this.tileClass = tileClass;
}

TileClassPainter.prototype.paint = function(area)
{
	for (let point of area.points)
		this.tileClass.add(point);
};

/**
 * Removes the given tileclass from a given area.
 */
function TileClassUnPainter(tileClass)
{
	this.tileClass = tileClass;
}

TileClassUnPainter.prototype.paint = function(area)
{
	for (let point of area.points)
		this.tileClass.remove(point);
};

/**
 * The MultiPainter applies several painters to the given area.
 */
function MultiPainter(painters)
{
	if (painters instanceof Array)
		this.painters = painters;
	else if (!painters)
		this.painters = [];
	else
		this.painters = [painters];
}

MultiPainter.prototype.paint = function(area)
{
	for (let painter of this.painters)
		painter.paint(area);
};

/**
 * The TerrainPainter draws a given terrain texture over the given area.
 * When used with TERRAIN_SEPARATOR, an entity is placed on each tile.
 */
function TerrainPainter(terrain)
{
	this.terrain = createTerrain(terrain);
}

TerrainPainter.prototype.paint = function(area)
{
	for (let point of area.points)
		this.terrain.place(point);
};

/**
 * The LayeredPainter sets different Terrains within the Area.
 * It choses the Terrain depending on the distance to the border of the Area.
 *
 * The Terrains given in the first array are painted from the border of the area towards the center (outermost first).
 * The widths array has one item less than the Terrains array.
 * Each width specifies how many tiles the corresponding Terrain should be wide (distance to the prior Terrain border).
 * The remaining area is filled with the last terrain.
 */
function LayeredPainter(terrainArray, widths)
{
	if (!(terrainArray instanceof Array))
		throw new Error("LayeredPainter: terrains must be an array!");

	this.terrains = terrainArray.map(terrain => createTerrain(terrain));
	this.widths = widths;
}

LayeredPainter.prototype.paint = function(area)
{
	breadthFirstSearchPaint({
		"area": area,
		"brushSize": 1,
		"gridSize": g_Map.getSize(),
		"withinArea": (areaID, position) => g_Map.area[position.x][position.y] == areaID,
		"paintTile": (point, distance) => {
			let width = 0;
			let i = 0;

			for (; i < this.widths.length; ++i)
			{
				width += this.widths[i];
				if (width >= distance)
					break;
			}

			this.terrains[i].place(point);
		}
	});
};

/**
 * Applies smoothing to the given area using Inverse-Distance-Weighting / Shepard's method.
 *
 * @param {Number} size - Determines the number of neighboring heights to interpolate. The area is a square with the length twice this size.
 * @param {Number} strength - Between 0 (no effect) and 1 (only neighbor heights count). This parameter has the lowest performance impact.
 * @param {Number} iterations - How often the process should be repeated. Typically 1. Can be used to gain even more smoothing.
 */
function SmoothingPainter(size, strength, iterations)
{
	if (size <= 0)
		throw new Error("Invalid size: " + size);

	if (strength <= 0 || strength > 1)
		throw new Error("Invalid strength: " + strength);

	if (iterations <= 0)
		throw new Error("Invalid iterations: " + iterations);

	this.size = size;
	this.strength = strength;
	this.iterations = iterations;
}

SmoothingPainter.prototype.paint = function(area)
{
	let brushPoints = getPointsInBoundingBox(getBoundingBox(
		new Array(2).fill(0).map((zero, i) => new Vector2D(1, 1).mult(this.size).floor().mult(i ? 1 : -1))));

	for (let i = 0; i < this.iterations; ++i)
	{
		let heightmap = clone(g_Map.height);

		// Additional complexity to process all 4 vertices of each tile, i.e the last row too
		let seen = new Array(heightmap.length).fill(0).map(zero => new Uint8Array(heightmap.length).fill(0));

		for (let point of area.points)
			for (let tileVertex of g_TileVertices)
			{
				let vertex = Vector2D.add(point, tileVertex);
				if (!g_Map.validHeight(vertex) || seen[vertex.x][vertex.y])
					continue;

				seen[vertex.x][vertex.y] = 1;

				let sumWeightedHeights = 0;
				let sumWeights = 0;

				for (let brushPoint of brushPoints)
				{
					let position = Vector2D.add(vertex, brushPoint);
					let distance = Math.abs(brushPoint.x) + Math.abs(brushPoint.y);
					if (!distance || !g_Map.validHeight(position))
						continue;

					sumWeightedHeights += g_Map.getHeight(position) / distance;
					sumWeights += 1 / distance;
				}

				g_Map.setHeight(
					vertex,
					this.strength * sumWeightedHeights / sumWeights +
					(1 - this.strength) * g_Map.getHeight(vertex));
			}
	}
};

/**
 * Sets the given height in the given Area.
 */
function ElevationPainter(elevation)
{
	this.elevation = elevation;
}

ElevationPainter.prototype.paint = function(area)
{
	for (let point of area.points)
		for (let vertex of g_TileVertices)
		{
			let position = Vector2D.add(point, vertex);
			if (g_Map.validHeight(position))
				g_Map.setHeight(position, this.elevation);
		}
};

/**
 * Absolute height change.
 */
const ELEVATION_SET = 0;

/**
 * Relative height change.
 */
const ELEVATION_MODIFY = 1;

/**
 * Sets the elevation of the Area in dependence to the given blendRadius and
 * interpolates it with the existing elevation.
 *
 * @param type - ELEVATION_MODIFY or ELEVATION_SET.
 * @param elevation - target height.
 * @param blendRadius - How steep the elevation change is.
 * @param randomElevation - maximum random elevation difference added to each vertex.
 */
function SmoothElevationPainter(type, elevation, blendRadius, randomElevation = 0)
{
	this.type = type;
	this.elevation = elevation;
	this.blendRadius = blendRadius;
	this.randomElevation = randomElevation;

	if (type != ELEVATION_SET && type != ELEVATION_MODIFY)
		throw new Error("SmoothElevationPainter: invalid type '" + type + "'");
}

SmoothElevationPainter.prototype.paint = function(area)
{
	// The heightmap grid has one more vertex per side than the tile grid
	let heightmapSize = g_Map.height.length;

	// Remember height inside the area before changing it
	let gotHeightPt = [];
	let newHeight = [];
	for (let i = 0; i < heightmapSize; ++i)
	{
		gotHeightPt[i] = new Uint8Array(heightmapSize);
		newHeight[i] = new Float32Array(heightmapSize);
	}

	// Get heightmap grid vertices within or adjacent to the area
	let brushSize = 2;
	let heightPoints = [];
	for (let point of area.points)
		for (let dx = -1; dx < 1 + brushSize; ++dx)
		{
			let nx = point.x + dx;
			for (let dz = -1; dz < 1 + brushSize; ++dz)
			{
				let nz = point.y + dz;
				let position = new Vector2D(nx, nz);

				if (g_Map.validHeight(position) && !gotHeightPt[nx][nz])
				{
					newHeight[nx][nz] = g_Map.getHeight(position);
					gotHeightPt[nx][nz] = 1;
					heightPoints.push(position);
				}
			}
		}

	// Every vertex of a tile is considered within the area
	let withinArea = (areaID, position) => {
		for (let vertex of g_TileVertices)
		{
			let vertexPos = Vector2D.sub(position, vertex);
			if (g_Map.inMapBounds(vertexPos) && g_Map.area[vertexPos.x][vertexPos.y] == areaID)
				return true;
		}

		return false;
	};

	// Change height inside the area depending on the distance to the border
	breadthFirstSearchPaint({
		"area": area,
		"brushSize": brushSize,
		"gridSize": heightmapSize,
		"withinArea": withinArea,
		"paintTile": (point, distance) => {
			let a = 1;
			if (distance <= this.blendRadius)
				a = (distance - 1) / this.blendRadius;

			if (this.type == ELEVATION_SET)
				newHeight[point.x][point.y] = (1 - a) * g_Map.getHeight(point);

			newHeight[point.x][point.y] += a * this.elevation + randFloat(-0.5, 0.5) * this.randomElevation;
		}
	});

	// Smooth everything out
	let areaID = area.getID();
	for (let point of heightPoints)
	{
		if (!withinArea(areaID, point))
			continue;

		let count = 0;
		let sum = 0;

		for (let dx = -1; dx <= 1; ++dx)
		{
			let nx = point.x + dx;

			for (let dz = -1; dz <= 1; ++dz)
			{
				let nz = point.y + dz;

				if (g_Map.validHeight(new Vector2D(nx, nz)))
				{
					sum += newHeight[nx][nz];
					++count;
				}
			}
		}

		g_Map.setHeight(point, (newHeight[point.x][point.y] + sum / count) / 2);
	}
};

/**
 * Calls the given paintTile function on all points within the given Area,
 * providing the distance to the border of the area (1 for points on the border).
 * This function can traverse any grid, for instance the tile grid or the larger heightmap grid.
 *
 * @property area - An Area storing the set of points on the tile grid.
 * @property gridSize - The size of the grid to be traversed.
 * @property brushSize - Number of points per axis on the grid that are considered a point on the tilemap.
 * @property withinArea - Whether a point of the grid is considered part of the Area.
 * @property paintTile - Called for each point of the Area of the tile grid.
 */
function breadthFirstSearchPaint(args)
{
	// These variables save which points were visited already and the shortest distance to the area
	let saw = [];
	let dist = [];
	for (let i = 0; i < args.gridSize; ++i)
	{
		saw[i] = new Uint8Array(args.gridSize);
		dist[i] = new Uint16Array(args.gridSize);
	}

	let withinGrid = (x, z) => Math.min(x, z) >= 0 && Math.max(x, z) < args.gridSize;

	// Find all points outside of the area, mark them as seen and set zero distance
	let pointQueue = [];
	let areaID = args.area.getID();
	for (let point of args.area.points)
		// The brushSize is added because the entire brushSize is by definition part of the area
		for (let dx = -1; dx < 1 + args.brushSize; ++dx)
		{
			let nx = point.x + dx;
			for (let dz = -1; dz < 1 + args.brushSize; ++dz)
			{
				let nz = point.y + dz;
				let position = new Vector2D(nx, nz);

				if (!withinGrid(nx, nz) || args.withinArea(areaID, position) || saw[nx][nz])
					continue;

				saw[nx][nz] = 1;
				dist[nx][nz] = 0;
				pointQueue.push(position);
			}
		}

	// Visit these points, then direct neighbors of them, then their neighbors recursively.
	// Call the paintTile method for each point within the area, with distance == 1 for the border.
	while (pointQueue.length)
	{
		let point = pointQueue.shift();
		let distance = dist[point.x][point.y];

		if (args.withinArea(areaID, point))
			args.paintTile(point, distance);

		// Enqueue neighboring points
		for (let dx = -1; dx <= 1; ++dx)
		{
			let nx = point.x + dx;
			for (let dz = -1; dz <= 1; ++dz)
			{
				let nz = point.y + dz;
				let position = new Vector2D(nx, nz);

				if (!withinGrid(nx, nz) || !args.withinArea(areaID, position) || saw[nx][nz])
					continue;

				saw[nx][nz] = 1;
				dist[nx][nz] = distance + 1;
				pointQueue.push(position);
			}
		}
	}
}

/**
 * Paints the given texture-mapping to the given tiles.
 *
 * @param {String[]} textureIDs - Names of the terrain textures
 * @param {Number[]} textureNames - One-dimensional array of indices of texturenames, one for each tile of the entire map.
 * @returns
 */
function TerrainTextureArrayPainter(textureIDs, textureNames)
{
	this.textureIDs = textureIDs;
	this.textureNames = textureNames;
}

TerrainTextureArrayPainter.prototype.paint = function(area)
{
	let sourceSize = Math.sqrt(this.textureIDs.length);
	let scale = sourceSize / g_Map.getSize();

	for (let point of area.points)
	{
		let sourcePos = Vector2D.mult(point, scale).floor();
		g_Map.setTexture(point, this.textureNames[this.textureIDs[sourcePos.x * sourceSize + sourcePos.y]]);
	}
};

/**
 * Copies the given heightmap to the given area.
 * Scales the horizontal plane proportionally and applies bicubic interpolation.
 * The heightrange is either scaled proportionally or mapped to the given heightrange.
 *
 * @param {Uint16Array} heightmap - One dimensional array of vertex heights.
 * @param {Number} [normalMinHeight] - The minimum height the elevation grid of 320 tiles would have.
 * @param {Number} [normalMaxHeight] - The maximum height the elevation grid of 320 tiles would have.
 */
function HeightmapPainter(heightmap, normalMinHeight = undefined, normalMaxHeight = undefined)
{
	this.heightmap = heightmap;
	this.bicubicInterpolation = bicubicInterpolation;
	this.verticesPerSide = Math.sqrt(heightmap.length);
	this.normalMinHeight = normalMinHeight;
	this.normalMaxHeight = normalMaxHeight;
}

HeightmapPainter.prototype.getScale = function()
{
	return this.verticesPerSide / (g_Map.getSize() + 1);
};

HeightmapPainter.prototype.scaleHeight = function(height)
{
	if (this.normalMinHeight === undefined || this.normalMaxHeight === undefined)
		return height / this.getScale() / HEIGHT_UNITS_PER_METRE;

	let minHeight = this.normalMinHeight * (g_Map.getSize() + 1) / 321;
	let maxHeight = this.normalMaxHeight * (g_Map.getSize() + 1) / 321;

	return minHeight + (maxHeight - minHeight) * height / 0xFFFF;
};

HeightmapPainter.prototype.paint = function(area)
{
	let scale = this.getScale();
	let leftBottom = new Vector2D(0, 0);
	let rightTop = new Vector2D(this.verticesPerSide, this.verticesPerSide);
	let brushSize = new Vector2D(3, 3);
	let brushCenter = new Vector2D(1, 1);

	// Additional complexity to process all 4 vertices of each tile, i.e the last row too
	let seen = new Array(g_Map.height.length).fill(0).map(zero => new Uint8Array(g_Map.height.length).fill(0));

	for (let point of area.points)
		for (let vertex of g_TileVertices)
		{
			let vertexPos = Vector2D.add(point, vertex);

			if (!g_Map.validHeight(vertexPos) || seen[vertexPos.x][vertexPos.y])
				continue;

			seen[vertexPos.x][vertexPos.y] = 1;

			let sourcePos = Vector2D.mult(vertexPos, scale);
			let sourceTilePos = sourcePos.clone().floor();

			let brushPosition = Vector2D.max(
				leftBottom,
				Vector2D.min(
					Vector2D.sub(sourceTilePos, brushCenter),
					Vector2D.sub(rightTop, brushSize).sub(brushCenter)));

			g_Map.setHeight(vertexPos, bicubicInterpolation(
				Vector2D.sub(sourcePos, brushPosition).sub(brushCenter),
				...getPointsInBoundingBox(getBoundingBox([brushPosition, Vector2D.add(brushPosition, brushSize)])).map(pos =>
					this.scaleHeight(this.heightmap[pos.y * this.verticesPerSide + pos.x]))));
		}
};
