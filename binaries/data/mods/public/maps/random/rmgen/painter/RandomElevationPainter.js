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
	for (let point of area.getPoints())
		for (let vertex of g_TileVertices)
		{
			let position = Vector2D.add(point, vertex);
			if (g_Map.validHeight(position))
				g_Map.setHeight(position, randFloat(this.minHeight, this.maxHeight));
		}
};
