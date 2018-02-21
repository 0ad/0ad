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
	for (let point of area.getPoints())
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
	let withinArea = (area, position) => {
		for (let vertex of g_TileVertices)
		{
			let vertexPos = Vector2D.sub(position, vertex);
			if (g_Map.inMapBounds(vertexPos) && area.contains(vertexPos))
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
	for (let point of heightPoints)
	{
		if (!withinArea(area, point))
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
	for (let point of args.area.getPoints())
		// The brushSize is added because the entire brushSize is by definition part of the area
		for (let dx = -1; dx < 1 + args.brushSize; ++dx)
		{
			let nx = point.x + dx;
			for (let dz = -1; dz < 1 + args.brushSize; ++dz)
			{
				let nz = point.y + dz;
				let position = new Vector2D(nx, nz);

				if (!withinGrid(nx, nz) || args.withinArea(args.area, position) || saw[nx][nz])
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

		if (args.withinArea(args.area, point))
			args.paintTile(point, distance);

		// Enqueue neighboring points
		for (let dx = -1; dx <= 1; ++dx)
		{
			let nx = point.x + dx;
			for (let dz = -1; dz <= 1; ++dz)
			{
				let nz = point.y + dz;
				let position = new Vector2D(nx, nz);

				if (!withinGrid(nx, nz) || !args.withinArea(args.area, position) || saw[nx][nz])
					continue;

				saw[nx][nz] = 1;
				dist[nx][nz] = distance + 1;
				pointQueue.push(position);
			}
		}
	}
}
