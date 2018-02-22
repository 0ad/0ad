/**
 * Applies smoothing to the given area using Inverse-Distance-Weighting / Shepard's method.
 *
 * @param {Number} size - Determines the number of neighboring heights to interpolate. The area is a square with the length twice this size.
 * @param {Number} strength - Between 0 (no effect) and 1 (only neighbor heights count). This parameter has the lowest performance impact.
 * @param {Number} iterations - How often the process should be repeated. Typically 1. Can be used to gain even more smoothing.
 */
function SmoothingPainter(size, strength, iterations)
{
	if (size < 1)
		throw new Error("Invalid size: " + size);

	if (strength <= 0 || strength > 1)
		throw new Error("Invalid strength: " + strength);

	if (iterations <= 0)
		throw new Error("Invalid iterations: " + iterations);

	this.size = Math.floor(size);
	this.strength = strength;
	this.iterations = iterations;
}

SmoothingPainter.prototype.paint = function(area)
{
	let brushPoints = getPointsInBoundingBox(getBoundingBox(
		new Array(2).fill(0).map((zero, i) => new Vector2D(1, 1).mult(this.size).mult(i ? 1 : -1))));

	for (let i = 0; i < this.iterations; ++i)
	{
		let heightmap = clone(g_Map.height);

		// Additional complexity to process all 4 vertices of each tile, i.e the last row too
		let seen = new Array(heightmap.length).fill(0).map(zero => new Uint8Array(heightmap.length).fill(0));

		for (let point of area.getPoints())
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
