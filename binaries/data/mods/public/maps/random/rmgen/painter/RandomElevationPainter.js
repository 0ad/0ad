/**
 * Sets a random elevation of the given heightrange in the given Area.
 */
function RandomElevationPainter(minHeight, maxHeight)
{
	this.minHeight = minHeight;
	this.maxHeight = maxHeight;
}

RandomElevationPainter.prototype.paint = function(area)
{
	for (const point of area.getPoints())
		for (const vertex of g_TileVertices)
		{
			const position = Vector2D.add(point, vertex);
			if (g_Map.validHeight(position))
				g_Map.setHeight(position, randFloat(this.minHeight, this.maxHeight));
		}
};
