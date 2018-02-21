/**
 * The MapBoundsPlacer returns all points on the tilemap that meet the constraint.
 */
function MapBoundsPlacer(failFraction = Infinity)
{
	let mapBounds = g_Map.getBounds();
	this.rectPlacer = new RectPlacer(new Vector2D(mapBounds.left, mapBounds.top), new Vector2D(mapBounds.right, mapBounds.bottom), failFraction);
}

MapBoundsPlacer.prototype.place = function(constraint)
{
	return this.rectPlacer.place(constraint);
};
