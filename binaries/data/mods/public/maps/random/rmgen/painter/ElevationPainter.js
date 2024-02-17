/**
 * Sets the given height in the given Area.
 */
function ElevationPainter(elevation)
{
	this.elevation = elevation;
}

ElevationPainter.prototype.paint = function(area)
{
	for (const point of area.getPoints())
		for (const vertex of g_TileVertices)
		{
			const position = Vector2D.add(point, vertex);
			if (g_Map.validHeight(position))
				g_Map.setHeight(position, this.elevation);
		}
};
