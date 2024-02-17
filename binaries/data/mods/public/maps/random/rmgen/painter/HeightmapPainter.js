/**
 * Copies the given heightmap to the given area.
 * Scales the horizontal plane proportionally and applies bicubic interpolation.
 * The heightrange is either scaled proportionally or mapped to the given heightrange.
 *
 * @param {Uint16Array} heightmap - One dimensional array of vertex heights.
 * @param {number} [normalMinHeight] - The minimum height the elevation grid of 320 tiles would have.
 * @param {number} [normalMaxHeight] - The maximum height the elevation grid of 320 tiles would have.
 */
function HeightmapPainter(heightmap, normalMinHeight = undefined, normalMaxHeight = undefined)
{
	this.heightmap = heightmap;
	this.bicubicInterpolation = bicubicInterpolation;
	this.verticesPerSide = heightmap.length;
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

	const minHeight = this.normalMinHeight * (g_Map.getSize() + 1) / 321;
	const maxHeight = this.normalMaxHeight * (g_Map.getSize() + 1) / 321;

	return minHeight + (maxHeight - minHeight) * height / 0xFFFF;
};

HeightmapPainter.prototype.paint = function(area)
{
	const scale = this.getScale();
	const leftBottom = new Vector2D(0, 0);
	const rightTop = new Vector2D(this.verticesPerSide, this.verticesPerSide);
	const brushSize = new Vector2D(3, 3);
	const brushCenter = new Vector2D(1, 1);

	// Additional complexity to process all 4 vertices of each tile, i.e the last row too
	const seen = new Array(g_Map.height.length).fill(0).map(zero => new Uint8Array(g_Map.height.length).fill(0));

	for (const point of area.getPoints())
		for (const vertex of g_TileVertices)
		{
			const vertexPos = Vector2D.add(point, vertex);

			if (!g_Map.validHeight(vertexPos) || seen[vertexPos.x][vertexPos.y])
				continue;

			seen[vertexPos.x][vertexPos.y] = 1;

			const sourcePos = Vector2D.mult(vertexPos, scale);
			const sourceTilePos = sourcePos.clone().floor();

			const brushPosition = Vector2D.max(
				leftBottom,
				Vector2D.min(
					Vector2D.sub(sourceTilePos, brushCenter),
					Vector2D.sub(rightTop, brushSize).sub(brushCenter)));

			g_Map.setHeight(vertexPos, bicubicInterpolation(
				Vector2D.sub(sourcePos, brushPosition).sub(brushCenter),
				...getPointsInBoundingBox(getBoundingBox([brushPosition, Vector2D.add(brushPosition, brushSize)])).map(pos =>
					this.scaleHeight(this.heightmap[pos.x][pos.y]))));
		}
};
