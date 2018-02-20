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
